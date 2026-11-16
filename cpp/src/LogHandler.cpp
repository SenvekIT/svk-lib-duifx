#include "duifx/LogHandler.h"

#include "Logging.h"

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QRegularExpression>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <vector>

namespace duifx {

// --- levels -----------------------------------------------------------------------

QString logLevelName(int levelNo)
{
    switch (levelNo) {
    case loglevel::Critical:
        return QStringLiteral("CRITICAL");
    case loglevel::Error:
        return QStringLiteral("ERROR");
    case loglevel::Warning:
        return QStringLiteral("WARNING");
    case loglevel::Info:
        return QStringLiteral("INFO");
    case loglevel::Debug:
        return QStringLiteral("DEBUG");
    case loglevel::NotSet:
        return QStringLiteral("NOTSET");
    default:
        return QStringLiteral("Level %1").arg(levelNo);
    }
}

std::optional<int> logLevelNumber(const QString &name)
{
    if (name == QLatin1StringView("CRITICAL") || name == QLatin1StringView("FATAL"))
        return loglevel::Critical;
    if (name == QLatin1StringView("ERROR"))
        return loglevel::Error;
    if (name == QLatin1StringView("WARNING") || name == QLatin1StringView("WARN"))
        return loglevel::Warning;
    if (name == QLatin1StringView("INFO"))
        return loglevel::Info;
    if (name == QLatin1StringView("DEBUG"))
        return loglevel::Debug;
    if (name == QLatin1StringView("NOTSET"))
        return loglevel::NotSet;
    return std::nullopt;
}

int logLevelOf(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:
        return loglevel::Debug;
    case QtInfoMsg:
        return loglevel::Info;
    case QtWarningMsg:
        return loglevel::Warning;
    case QtCriticalMsg:
        return loglevel::Error;
    case QtFatalMsg:
        return loglevel::Critical;
    }
    return loglevel::Info;
}

// --- entries ----------------------------------------------------------------------

LogEntry LogEntry::make(int levelNo, const QString &loggerName, const QString &message)
{
    // Microseconds, like Python's time.time(); QDateTime only has milliseconds.
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto us = std::chrono::duration_cast<std::chrono::microseconds>(now).count();
    return LogEntry{static_cast<double>(us) / 1e6, loggerName, logLevelName(levelNo), levelNo, message};
}

LogEntry LogEntry::fromQtMessage(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    QString name = context.category ? QString::fromUtf8(context.category) : QString();
    // qDebug() without a category is the counterpart of logging.debug(): the root logger.
    if (name.isEmpty() || name == QLatin1StringView("default"))
        name = QStringLiteral("root");
    return make(logLevelOf(type), name, message);
}

namespace {

/// repr() of a Python float: shortest round-trip digits, ".0" on whole numbers,
/// scientific notation outside 1e-4 <= |d| < 1e16.
QString pyFloatRepr(double d)
{
    if (std::isnan(d))
        return QStringLiteral("NaN");
    if (std::isinf(d))
        return d > 0 ? QStringLiteral("Infinity") : QStringLiteral("-Infinity");
    const double magnitude = std::abs(d);
    if (magnitude >= 1e16 || (magnitude != 0.0 && magnitude < 1e-4))
        return QString::number(d, 'e', QLocale::FloatingPointShortest);
    QString text = QString::number(d, 'f', QLocale::FloatingPointShortest);
    if (!text.contains(QLatin1Char('.')))
        text += QStringLiteral(".0");
    return text;
}

/// A JSON string as json.dumps(ensure_ascii=False) writes it: only quotes,
/// backslashes and control characters are escaped.
void appendJsonString(QString &out, const QString &text)
{
    out += QLatin1Char('"');
    for (const QChar c : text) {
        switch (c.unicode()) {
        case '"':
            out += QStringLiteral("\\\"");
            break;
        case '\\':
            out += QStringLiteral("\\\\");
            break;
        case '\n':
            out += QStringLiteral("\\n");
            break;
        case '\r':
            out += QStringLiteral("\\r");
            break;
        case '\t':
            out += QStringLiteral("\\t");
            break;
        case '\b':
            out += QStringLiteral("\\b");
            break;
        case '\f':
            out += QStringLiteral("\\f");
            break;
        default:
            if (c.unicode() < 0x20)
                out += QStringLiteral("\\u%1").arg(uint(c.unicode()), 4, 16, QLatin1Char('0'));
            else
                out += c;
        }
    }
    out += QLatin1Char('"');
}

// The conversions Python applies to a parsed record: float(), int(), str().

std::optional<double> pyFloat(const QJsonValue &value)
{
    if (value.isDouble())
        return value.toDouble();
    if (value.isBool())
        return value.toBool() ? 1.0 : 0.0;
    if (value.isString()) {
        bool ok = false;
        const double d = value.toString().trimmed().toDouble(&ok);
        if (ok)
            return d;
    }
    return std::nullopt;
}

std::optional<int> pyInt(const QJsonValue &value)
{
    if (value.isDouble()) {
        const double d = std::trunc(value.toDouble());
        if (!std::isfinite(d) || d < std::numeric_limits<int>::min() || d > std::numeric_limits<int>::max())
            return std::nullopt;
        return static_cast<int>(d);
    }
    if (value.isBool())
        return value.toBool() ? 1 : 0;
    if (value.isString()) {
        bool ok = false;
        const int i = value.toString().trimmed().toInt(&ok);
        if (ok)
            return i;
    }
    return std::nullopt;
}

QString pyStr(const QJsonValue &value)
{
    switch (value.type()) {
    case QJsonValue::String:
        return value.toString();
    case QJsonValue::Bool:
        return value.toBool() ? QStringLiteral("True") : QStringLiteral("False");
    case QJsonValue::Null:
        return QStringLiteral("None");
    case QJsonValue::Double:
        // Integers stay integers ("20"), like int and float in Python.
        if (value.toVariant().typeId() == QMetaType::LongLong)
            return QString::number(value.toInteger());
        return pyFloatRepr(value.toDouble());
    case QJsonValue::Array:
        return QString::fromUtf8(QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact));
    case QJsonValue::Object:
        return QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact));
    case QJsonValue::Undefined:
        break;
    }
    return {};
}

std::optional<LogEntry> parseJsonLine(const QString &raw)
{
    QJsonParseError error{};
    const QJsonDocument document = QJsonDocument::fromJson(raw.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        return std::nullopt;
    const QJsonObject object = document.object();
    // A foreign JSON file would parse but lack the fields we index on.
    for (const char *key : {"ts", "name", "lvl", "lvl_no", "msg"}) {
        if (!object.contains(QLatin1StringView(key)))
            return std::nullopt;
    }
    const std::optional<double> ts = pyFloat(object.value(QLatin1StringView("ts")));
    const std::optional<int> lvlNo = pyInt(object.value(QLatin1StringView("lvl_no")));
    if (!ts || !lvlNo)
        return std::nullopt;
    return LogEntry{*ts, pyStr(object.value(QLatin1StringView("name"))), pyStr(object.value(QLatin1StringView("lvl"))),
                    *lvlNo, pyStr(object.value(QLatin1StringView("msg")))};
}

} // namespace

std::optional<LogEntry> parseLogLine(const QString &rawLine)
{
    const QString raw = rawLine.trimmed();
    if (raw.isEmpty())
        return std::nullopt;
    if (raw.startsWith(QLatin1Char('{')))
        return parseJsonLine(raw);

    // Lines written by formatLogEntry(): "2026-08-03 11:50:13,123 - app.core.x - DEBUG   - message".
    static const QRegularExpression textLine(
        QStringLiteral(R"(^(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2},\d{3}) - (\S+)\s+- (\w+)\s+- (.*)$)"),
        QRegularExpression::DotMatchesEverythingOption | QRegularExpression::UseUnicodePropertiesOption);
    const QRegularExpressionMatch match = textLine.match(raw);
    if (!match.hasMatch())
        return std::nullopt;

    const QDateTime time = QDateTime::fromString(match.captured(1), QStringLiteral("yyyy-MM-dd HH:mm:ss,zzz"));
    if (!time.isValid())
        return std::nullopt;
    const QString level = match.captured(3);
    // Unknown level names are treated as INFO so the entry stays visible.
    const int lvlNo = logLevelNumber(level).value_or(loglevel::Info);
    return LogEntry{static_cast<double>(time.toMSecsSinceEpoch()) / 1000.0, match.captured(2), level, lvlNo,
                    match.captured(4)};
}

QString logEntryToJson(const LogEntry &entry)
{
    QString out;
    out.reserve(64 + entry.name.size() + entry.msg.size());
    out += QStringLiteral("{\"ts\": ");
    out += pyFloatRepr(entry.ts);
    out += QStringLiteral(", \"name\": ");
    appendJsonString(out, entry.name);
    out += QStringLiteral(", \"lvl\": ");
    appendJsonString(out, entry.lvl);
    out += QStringLiteral(", \"lvl_no\": ");
    out += QString::number(entry.lvlNo);
    out += QStringLiteral(", \"msg\": ");
    appendJsonString(out, entry.msg);
    out += QLatin1Char('}');
    return out;
}

QString formatLogTimestamp(double ts, bool withMillis)
{
    // datetime.fromtimestamp() rounds the fraction to whole microseconds (half
    // to even); strftime then truncates. Doing the same keeps saved logs and
    // the table identical to the Python viewer's.
    double whole = 0.0;
    const double fraction = std::modf(ts, &whole);
    const qint64 us = static_cast<qint64>(whole) * 1'000'000 + static_cast<qint64>(std::nearbyint(fraction * 1e6));
    const qint64 ms = us >= 0 ? us / 1000 : -((-us + 999) / 1000);
    const QDateTime time = QDateTime::fromMSecsSinceEpoch(ms);
    return time.toString(withMillis ? QStringLiteral("yyyy-MM-dd HH:mm:ss,zzz") : QStringLiteral("yyyy-MM-dd HH:mm:ss"));
}

QString formatLogEntry(const LogEntry &entry)
{
    return QStringLiteral("%1 - %2 - %3 - %4")
        .arg(formatLogTimestamp(entry.ts, true), entry.name.leftJustified(11), entry.lvl.leftJustified(7), entry.msg);
}

// --- session files ----------------------------------------------------------------

QString sessionLogFileName(const QDateTime &when)
{
    return QLatin1StringView(kLogSessionPrefix) + when.toString(QStringLiteral("yyyyMMdd_HHmmss"))
           + QLatin1StringView(kLogSessionSuffix);
}

int pruneSessionLogs(const QString &logDir, int keep)
{
    if (keep <= 0)
        return 0;

    const QDir dir(logDir);
    if (logDir.isEmpty() || !dir.exists()) {
        qCWarning(lcDuifx, "Cannot list log directory %s: it does not exist", qPrintable(logDir));
        return 0;
    }
    // Directories with a matching name count too, as with os.listdir(); they
    // simply fail to delete.
    QStringList entries = dir.entryList(
        {QLatin1StringView(kLogSessionPrefix) + QLatin1Char('*') + QLatin1StringView(kLogSessionSuffix)},
        QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System, QDir::NoSort);
    // The name filter is case-insensitive on Windows; Python's match is not.
    entries.removeIf([](const QString &name) {
        return !name.startsWith(QLatin1StringView(kLogSessionPrefix))
               || !name.endsWith(QLatin1StringView(kLogSessionSuffix));
    });
    std::sort(entries.begin(), entries.end());

    // The file of the session about to start does not exist yet, so keep one
    // slot free for it.
    const qsizetype excess = entries.size() - std::max(keep - 1, 0);
    if (excess <= 0)
        return 0;

    int removed = 0;
    for (qsizetype i = 0; i < excess; ++i) {
        if (QFile::remove(dir.filePath(entries.at(i))))
            ++removed;
        else
            qCWarning(lcDuifx, "Cannot delete old log %s", qPrintable(entries.at(i)));
    }
    return removed;
}

// --- handler registry -------------------------------------------------------------

namespace {

struct Registration
{
    LogHandler *handler = nullptr;
    QStringList loggers;
};

struct Registry
{
    // Recursive: a handler whose receiver runs in the logging thread may detach
    // handlers (or delete one) while an entry is being delivered.
    std::recursive_mutex mutex;
    std::vector<Registration> registrations;
    QtMessageHandler previous = nullptr;
    bool installed = false;
};

Registry &registry()
{
    // Never destroyed: handlers may still detach during static destruction.
    static Registry *instance = new Registry;
    return *instance;
}

/// Set while this thread delivers an entry: a message logged by a handler (or
/// a receiver connected directly) is not delivered again, which would recurse
/// without end. It still reaches the previous Qt message handler.
thread_local bool t_dispatching = false;

bool isRoot(const QString &loggerName)
{
    return loggerName.isEmpty() || loggerName == QLatin1StringView("root");
}

bool loggerCovers(const QString &attached, const QString &name)
{
    if (isRoot(attached))
        return true;
    return name == attached || (name.startsWith(attached) && name.size() > attached.size()
                                && name.at(attached.size()) == QLatin1Char('.'));
}

bool isRegistered(const Registry &reg, const LogHandler *handler)
{
    return std::any_of(reg.registrations.begin(), reg.registrations.end(),
                       [handler](const Registration &r) { return r.handler == handler; });
}

void dispatch(const LogEntry &entry)
{
    if (t_dispatching)
        return;
    t_dispatching = true;
    Registry &reg = registry();
    {
        // Held while delivering, so a handler cannot be destroyed by another
        // thread in the middle of its handle().
        std::lock_guard lock(reg.mutex);
        std::vector<LogHandler *> targets;
        for (const Registration &r : reg.registrations) {
            if (std::any_of(r.loggers.begin(), r.loggers.end(),
                            [&entry](const QString &logger) { return loggerCovers(logger, entry.name); }))
                targets.push_back(r.handler);
        }
        for (LogHandler *handler : targets) {
            // A receiver in this thread may have detached the handler meanwhile.
            if (isRegistered(reg, handler))
                handler->handle(entry);
        }
    }
    t_dispatching = false;
}

void captureQtMessage(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    if (!t_dispatching)
        dispatch(LogEntry::fromQtMessage(type, context, message));
    QtMessageHandler previous = nullptr;
    {
        std::lock_guard lock(registry().mutex);
        previous = registry().previous;
    }
    if (previous)
        previous(type, context, message);
}

} // namespace

void addLogHandler(LogHandler *handler, const QString &loggerName)
{
    if (!handler)
        return;
    const QString name = isRoot(loggerName) ? QString() : loggerName;
    Registry &reg = registry();
    std::lock_guard lock(reg.mutex);
    auto it = std::find_if(reg.registrations.begin(), reg.registrations.end(),
                           [handler](const Registration &r) { return r.handler == handler; });
    if (it == reg.registrations.end())
        it = reg.registrations.insert(reg.registrations.end(), Registration{handler, {}});
    if (!it->loggers.contains(name))
        it->loggers.append(name);
    if (!reg.installed) {
        reg.previous = qInstallMessageHandler(captureQtMessage);
        reg.installed = true;
    }
}

void removeLogHandler(LogHandler *handler, const QString &loggerName)
{
    const QString name = isRoot(loggerName) ? QString() : loggerName;
    Registry &reg = registry();
    std::lock_guard lock(reg.mutex);
    auto it = std::find_if(reg.registrations.begin(), reg.registrations.end(),
                           [handler](const Registration &r) { return r.handler == handler; });
    if (it == reg.registrations.end())
        return;
    it->loggers.removeAll(name);
    if (it->loggers.isEmpty())
        reg.registrations.erase(it);
    if (reg.registrations.empty() && reg.installed) {
        const QtMessageHandler current = qInstallMessageHandler(reg.previous);
        if (current != captureQtMessage) {
            // Someone installed a handler on top of ours and chains to it:
            // leave theirs in place; ours keeps passing messages through.
            qInstallMessageHandler(current);
            return;
        }
        reg.installed = false;
        reg.previous = nullptr;
    }
}

void logMessage(int levelNo, const QString &loggerName, const QString &message)
{
    dispatch(LogEntry::make(levelNo, isRoot(loggerName) ? QStringLiteral("root") : loggerName, message));
}

// --- handlers ---------------------------------------------------------------------

LogHandler::~LogHandler()
{
    detachAll();
}

void LogHandler::handle(const LogEntry &entry)
{
    if (entry.lvlNo >= level())
        emitEntry(entry);
}

void LogHandler::detachAll()
{
    Registry &reg = registry();
    std::lock_guard lock(reg.mutex);
    auto it = std::find_if(reg.registrations.begin(), reg.registrations.end(),
                           [this](const Registration &r) { return r.handler == this; });
    if (it == reg.registrations.end())
        return;
    const QStringList loggers = it->loggers;
    for (const QString &logger : loggers)
        removeLogHandler(this, logger);
}

QtLogHandler::QtLogHandler(QObject *parent)
    : QObject(parent)
{
}

QtLogHandler::~QtLogHandler()
{
    detachAll();
}

void QtLogHandler::log(int levelNo, const QString &loggerName, const QString &message)
{
    handle(LogEntry::make(levelNo, loggerName, message));
}

void QtLogHandler::emitEntry(const LogEntry &entry)
{
    emit entryEmitted(entry);
}

JsonLinesFileHandler::JsonLinesFileHandler(const QString &path)
    : m_path(path)
    , m_file(path)
{
    // Text mode writes "\r\n" on Windows, as Python's text-mode file does.
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        qCWarning(lcDuifx, "Cannot open log file %s: %s", qPrintable(path), qPrintable(m_file.errorString()));
}

JsonLinesFileHandler::~JsonLinesFileHandler()
{
    detachAll();
    close();
}

bool JsonLinesFileHandler::isOpen() const
{
    std::lock_guard lock(m_mutex);
    return m_file.isOpen();
}

void JsonLinesFileHandler::flush()
{
    std::lock_guard lock(m_mutex);
    if (m_file.isOpen())
        m_file.flush();
}

void JsonLinesFileHandler::close()
{
    std::lock_guard lock(m_mutex);
    if (m_file.isOpen())
        m_file.close();
}

void JsonLinesFileHandler::emitEntry(const LogEntry &entry)
{
    QByteArray line = logEntryToJson(entry).toUtf8();
    line += '\n';
    std::lock_guard lock(m_mutex);
    if (!m_file.isOpen())
        return;
    m_file.write(line);
    m_file.flush();
}

} // namespace duifx

#pragma once

// Log entries, log handlers and the per-session log files. Port of the logging
// half of duifx/gui/widgets.py (_LogEntry, _parse_log_line, prune_session_logs,
// _JsonLinesFileHandler, QtLogHandler).
//
// Python attaches logging.Handler objects to loggers. C++ code logs through
// Qt (qDebug/qInfo/qWarning/qCritical, usually with a QLoggingCategory), so the
// handlers here capture Qt's message stream instead:
//
//     auto *handler = new duifx::QtLogHandler(this);
//     duifx::addLogHandler(handler);               // like logging.getLogger().addHandler()
//     duifx::addLogHandler(handler, "app.core");   // only "app.core" and "app.core.*"
//     connect(handler, &duifx::QtLogHandler::entryEmitted, ...);
//
// The first registered handler installs a Qt message handler (qInstallMessageHandler)
// that hands every message to the registered handlers and then chains to the
// handler installed before, so console output and other sinks keep working.
// Logger name = category name; Qt's "default" category is reported as "root",
// Python's name for the root logger. Levels follow Python's numbers:
//
//     qDebug -> DEBUG 10, qInfo -> INFO 20, qWarning -> WARNING 30,
//     qCritical -> ERROR 40, qFatal -> CRITICAL 50
//
// Code that does not use Qt logging posts entries directly with logMessage().
//
// Session files are JSON Lines, one object per line with the fields ts, name,
// lvl, lvl_no and msg, written exactly as Python's json.dumps(ensure_ascii=False)
// does, and named app_YYYYMMDD_HHMMSS.jsonl: the C++ and the Python viewers read
// each other's files.

#include <QDateTime>
#include <QFile>
#include <QMetaType>
#include <QObject>
#include <QString>
#include <QStringList>

#include <atomic>
#include <mutex>
#include <optional>

namespace duifx {

/// Python's logging level numbers.
namespace loglevel {
inline constexpr int NotSet = 0;
inline constexpr int Debug = 10;
inline constexpr int Info = 20;
inline constexpr int Warning = 30;
inline constexpr int Error = 40;
inline constexpr int Critical = 50;
} // namespace loglevel

/// Level name of a number, as logging.getLevelName(): "WARNING", or "Level 25"
/// for numbers without a name.
QString logLevelName(int levelNo);

/// Number of a level name ("WARNING", also "WARN" and "FATAL"), as
/// logging.getLevelName(name); nullopt for unknown names. Case-sensitive like Python.
std::optional<int> logLevelNumber(const QString &name);

/// One log entry: the fields of a JSON Lines record (_LogEntry).
struct LogEntry
{
    double ts = 0.0; ///< seconds since the epoch, as Python's record.created
    QString name;    ///< logger name (the Qt logging category)
    QString lvl;     ///< level name, e.g. "WARNING"
    int lvlNo = 0;   ///< level number, e.g. 30
    QString msg;     ///< the formatted message

    /// An entry stamped with the current time; lvl is logLevelName(levelNo).
    static LogEntry make(int levelNo, const QString &loggerName, const QString &message);

    /// The entry for a Qt message, mapped as described at the top.
    static LogEntry fromQtMessage(QtMsgType type, const QMessageLogContext &context, const QString &message);

    bool operator==(const LogEntry &other) const = default;
};

/// Python's level number of a Qt message type.
int logLevelOf(QtMsgType type);

/// One line of a log file as an entry, or nullopt. Accepts both formats the
/// applications produce: JSON Lines session files and the plain-text files
/// written by "Save Log" (formatLogEntry()).
std::optional<LogEntry> parseLogLine(const QString &raw);

/// The entry as one JSON Lines record (no line break), byte for byte what
/// Python's json.dumps(entry, ensure_ascii=False) writes.
QString logEntryToJson(const LogEntry &entry);

/// The entry as a line of a saved log: "2026-08-03 11:50:13,123 - app.core.x  - DEBUG   - message".
QString formatLogEntry(const LogEntry &entry);

/// Local time of ts as "yyyy-MM-dd HH:mm:ss", with ",zzz" if withMillis.
/// Rounds to whole microseconds first, as datetime.fromtimestamp() does.
QString formatLogTimestamp(double ts, bool withMillis = false);

// --- session files ----------------------------------------------------------------

/// Number of session files kept when the setting is absent. 0 disables pruning.
inline constexpr int kLogRetentionSessionsDefault = 30;

/// Above this many entries, loading a file asks for confirmation first: they
/// parse in about a second but occupy a few hundred MB.
inline constexpr int kLogLoadWarnEntries = 300'000;

/// Name pattern of the per-session files, used both for writing and for pruning.
inline constexpr const char *kLogSessionPrefix = "app_";
inline constexpr const char *kLogSessionSuffix = ".jsonl";

/// Session file name for when: "app_20260803_115013.jsonl".
QString sessionLogFileName(const QDateTime &when = QDateTime::currentDateTime());

/// Deletes all but the keep newest session logs in logDir, leaving one slot for
/// the session about to start (keep = 1 removes the previous session too).
/// Only files named like session logs are considered; sorting is by name, which
/// is chronological and, unlike mtime, survives a copy. Returns the number of
/// files removed. Failures are logged and swallowed: housekeeping must never
/// keep the application from starting.
int pruneSessionLogs(const QString &logDir, int keep);

// --- handlers ---------------------------------------------------------------------

/// Base of the handlers (logging.Handler): a level threshold and emitEntry().
/// handle() may be called from any thread.
///
/// A subclass must call detachAll() in its own destructor: once the subclass
/// part is destroyed, a message arriving from another thread would otherwise
/// reach the pure virtual emitEntry().
class LogHandler
{
public:
    LogHandler() = default;
    virtual ~LogHandler();
    LogHandler(const LogHandler &) = delete;
    LogHandler &operator=(const LogHandler &) = delete;

    /// Entries below levelNo are ignored (default NotSet: all).
    void setLevel(int levelNo) { m_level.store(levelNo); }
    int level() const { return m_level.load(); }

    /// Emits entry if it reaches level(). Thread-safe.
    void handle(const LogEntry &entry);

    /// Removes the handler from every logger, see addLogHandler().
    void detachAll();

protected:
    /// Delivers one entry; called by handle(), possibly from any thread.
    virtual void emitEntry(const LogEntry &entry) = 0;

private:
    std::atomic<int> m_level{loglevel::NotSet};
};

/// Forwards entries into the Qt event loop through entryEmitted (QtLogHandler).
/// The signal is emitted in the logging thread; receivers in another thread
/// get it queued, so a widget can connect to it directly.
class QtLogHandler : public QObject, public LogHandler
{
    Q_OBJECT

public:
    explicit QtLogHandler(QObject *parent = nullptr);
    ~QtLogHandler() override;

    /// Posts an entry stamped now, without going through Qt logging.
    void log(int levelNo, const QString &loggerName, const QString &message);

signals:
    void entryEmitted(const duifx::LogEntry &entry);

protected:
    void emitEntry(const LogEntry &entry) override;
};

/// Writes entries as JSON Lines to a file opened for appending (_JsonLinesFileHandler).
/// Each line is flushed, so a crash loses nothing that was logged. Thread-safe.
class JsonLinesFileHandler : public LogHandler
{
public:
    /// Opens (and creates) path; see isOpen().
    explicit JsonLinesFileHandler(const QString &path);
    ~JsonLinesFileHandler() override;

    QString path() const { return m_path; }
    bool isOpen() const;
    void flush();
    void close();

protected:
    void emitEntry(const LogEntry &entry) override;

private:
    QString m_path;
    mutable std::mutex m_mutex;
    QFile m_file;
};

/// Attaches handler to the logger loggerName and its children ("app" also gets
/// "app.core"); an empty name or "root" is the root logger, which gets every
/// message. The first attached handler starts capturing Qt messages. A handler
/// attached to several matching loggers still gets each entry once. Thread-safe.
void addLogHandler(LogHandler *handler, const QString &loggerName = {});

/// Detaches handler from loggerName; the last detached handler hands Qt's
/// messages back to the previous message handler.
void removeLogHandler(LogHandler *handler, const QString &loggerName = {});

/// Posts an entry to the handlers attached to loggerName (or a parent), as
/// logging.getLogger(loggerName).log(levelNo, message). Thread-safe. Unlike a Qt
/// message it does not reach the console.
void logMessage(int levelNo, const QString &loggerName, const QString &message);

} // namespace duifx

Q_DECLARE_METATYPE(duifx::LogEntry)

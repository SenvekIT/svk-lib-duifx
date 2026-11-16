// Log entries and files, the Qt message capture, the log table model and the
// log viewer widget.

#include "TestSupport.h"

#include <duifx/LogHandler.h>
#include <duifx/LogViewerWidget.h>
#include <duifx/Settings.h>

#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QLineEdit>
#include <QLoggingCategory>
#include <QScrollBar>
#include <QSignalSpy>
#include <QTableView>
#include <QTemporaryDir>
#include <QThread>
#include <QtTest>

#include <cmath>
#include <cstddef>
#include <utility>

using namespace duifx;

Q_LOGGING_CATEGORY(lcTest, "app.test")
Q_LOGGING_CATEGORY(lcOther, "other")

namespace {

// Written by Python: json.dumps({"ts": ..., ...}, ensure_ascii=False).
const QString kPythonLine = QStringLiteral(
    "{\"ts\": 1754214613.123456, \"name\": \"app.core\", \"lvl\": \"WARNING\", \"lvl_no\": 30, "
    "\"msg\": \"Temp \\\"high\\\" \\u00e4 \\\\ path\\nnext\\ttab \\u0001\"}");

LogEntry entry(double ts, int levelNo, const QString &msg, const QString &name = QStringLiteral("app"))
{
    return LogEntry{ts, name, logLevelName(levelNo), levelNo, msg};
}

void writeLines(const QString &path, const QStringList &lines)
{
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text));
    for (const QString &line : lines)
        file.write(line.toUtf8() + '\n');
}

QStringList messages(const LogTableModel &model)
{
    QStringList out;
    for (int row = 0; row < model.rowCount(); ++row)
        out << model.index(row, LogTableModel::MessageColumn).data().toString();
    return out;
}

QStringList g_previousHandlerMessages;

void previousHandler(QtMsgType, const QMessageLogContext &, const QString &message)
{
    g_previousHandlerMessages << message;
}

} // namespace

class TstLogViewer : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;

private slots:
    void initTestCase()
    {
        duifx_test::isolateSettings();
        QVERIFY(m_dir.isValid());
        // The widget's session files go to a folder of the test.
        getSettings()->setValue(QStringLiteral("log_directory"), m_dir.filePath(QStringLiteral("sessions")));
    }

    // --- entries ------------------------------------------------------------------

    void parsesPythonJsonLine()
    {
        const std::optional<LogEntry> e = parseLogLine(kPythonLine);
        QVERIFY(e);
        QCOMPARE(e->ts, 1754214613.123456);
        QCOMPARE(e->name, QStringLiteral("app.core"));
        QCOMPARE(e->lvl, QStringLiteral("WARNING"));
        QCOMPARE(e->lvlNo, 30);
        QCOMPARE(e->msg, QStringLiteral("Temp \"high\" ä \\ path\nnext\ttab \u0001"));
    }

    void serialisesLikePython()
    {
        // ensure_ascii=False: "ä" stays as it is, control characters are escaped.
        const LogEntry e = *parseLogLine(kPythonLine);
        QCOMPARE(logEntryToJson(e),
                 QStringLiteral("{\"ts\": 1754214613.123456, \"name\": \"app.core\", \"lvl\": \"WARNING\", \"lvl_no\": 30, "
                                "\"msg\": \"Temp \\\"high\\\" \u00e4 \\\\ path\\nnext\\ttab \\u0001\"}"));
        QCOMPARE(*parseLogLine(logEntryToJson(e)), e);
        // Whole seconds keep Python's ".0".
        QVERIFY(logEntryToJson(entry(1754214613.0, 20, QStringLiteral("x"))).startsWith(QStringLiteral("{\"ts\": 1754214613.0, ")));
        // A fresh entry survives the round trip exactly.
        const LogEntry now = LogEntry::make(loglevel::Error, QStringLiteral("app"), QStringLiteral("boom"));
        QCOMPARE(*parseLogLine(logEntryToJson(now)), now);
    }

    void parsesTextLine()
    {
        const std::optional<LogEntry> e
            = parseLogLine(QStringLiteral("2026-08-03 11:50:13,123 - app.core.x  - DEBUG   - a - b - c\r\n"));
        QVERIFY(e);
        QCOMPARE(e->name, QStringLiteral("app.core.x"));
        QCOMPARE(e->lvl, QStringLiteral("DEBUG"));
        QCOMPARE(e->lvlNo, 10);
        QCOMPARE(e->msg, QStringLiteral("a - b - c"));
        const QDateTime expected(QDate(2026, 8, 3), QTime(11, 50, 13, 123));
        QCOMPARE(qint64(std::llround(e->ts * 1000)), expected.toMSecsSinceEpoch());

        // Unknown level names stay visible as INFO.
        const std::optional<LogEntry> custom
            = parseLogLine(QStringLiteral("2026-08-03 11:50:13,123 - app - TRACE - x"));
        QVERIFY(custom);
        QCOMPARE(custom->lvl, QStringLiteral("TRACE"));
        QCOMPARE(custom->lvlNo, loglevel::Info);

        // formatLogEntry() writes what parseLogLine() reads (milliseconds kept).
        const LogEntry written = entry(1754214613.5678, 40, QStringLiteral("msg"));
        QCOMPARE(formatLogEntry(written).mid(19, 4), QStringLiteral(",567"));
        const LogEntry back = *parseLogLine(formatLogEntry(written));
        QCOMPARE(back.lvlNo, 40);
        QCOMPARE(back.ts, 1754214613.567);
        QVERIFY(formatLogEntry(written).contains(QStringLiteral(" - app         - ERROR   - msg")));
    }

    void rejectsForeignLines()
    {
        QVERIFY(!parseLogLine(QString()));
        QVERIFY(!parseLogLine(QStringLiteral("   ")));
        QVERIFY(!parseLogLine(QStringLiteral("hello world")));
        QVERIFY(!parseLogLine(QStringLiteral("{\"a\": 1}")));
        QVERIFY(!parseLogLine(QStringLiteral("{\"ts\": 1, \"name\": \"x\"")));
        QVERIFY(!parseLogLine(QStringLiteral("{\"ts\": \"abc\", \"name\": \"x\", \"lvl\": \"INFO\", \"lvl_no\": 20, \"msg\": \"\"}")));
        // Python's float()/int()/str() conversions.
        const auto converted = parseLogLine(QStringLiteral("{\"ts\": \"12.5\", \"name\": 7, \"lvl\": \"INFO\", \"lvl_no\": \"20\", \"msg\": null}"));
        QVERIFY(converted);
        QCOMPARE(converted->ts, 12.5);
        QCOMPARE(converted->name, QStringLiteral("7"));
        QCOMPARE(converted->lvlNo, 20);
        QCOMPARE(converted->msg, QStringLiteral("None"));
    }

    void levels()
    {
        QCOMPARE(logLevelName(30), QStringLiteral("WARNING"));
        QCOMPARE(logLevelName(25), QStringLiteral("Level 25"));
        QCOMPARE(*logLevelNumber(QStringLiteral("WARN")), 30);
        QCOMPARE(*logLevelNumber(QStringLiteral("FATAL")), 50);
        QVERIFY(!logLevelNumber(QStringLiteral("warning")));
        QCOMPARE(logLevelOf(QtDebugMsg), 10);
        QCOMPARE(logLevelOf(QtInfoMsg), 20);
        QCOMPARE(logLevelOf(QtWarningMsg), 30);
        QCOMPARE(logLevelOf(QtCriticalMsg), 40);
        QCOMPARE(logLevelOf(QtFatalMsg), 50);
    }

    // --- files --------------------------------------------------------------------

    void sessionFileName()
    {
        const QDateTime when(QDate(2026, 8, 3), QTime(9, 5, 7));
        QCOMPARE(sessionLogFileName(when), QStringLiteral("app_20260803_090507.jsonl"));
    }

    void prunesOldSessions()
    {
        QTemporaryDir dir;
        const QStringList sessions{QStringLiteral("app_20260101_000000.jsonl"), QStringLiteral("app_20260102_000000.jsonl"),
                                   QStringLiteral("app_20260103_000000.jsonl"), QStringLiteral("app_20260104_000000.jsonl"),
                                   QStringLiteral("app_20260105_000000.jsonl")};
        for (const QString &name : sessions + QStringList{QStringLiteral("other.txt"), QStringLiteral("app_notes.txt")})
            writeLines(dir.filePath(name), {QStringLiteral("x")});

        QCOMPARE(pruneSessionLogs(dir.path(), 0), 0); // disabled
        QCOMPARE(pruneSessionLogs(dir.path(), 10), 0);
        // keep 3: two old ones stay, one slot for the session about to start.
        QCOMPARE(pruneSessionLogs(dir.path(), 3), 3);
        QStringList left = QDir(dir.path()).entryList(QDir::Files, QDir::Name);
        QCOMPARE(left, (QStringList{QStringLiteral("app_20260104_000000.jsonl"), QStringLiteral("app_20260105_000000.jsonl"),
                                    QStringLiteral("app_notes.txt"), QStringLiteral("other.txt")}));
        QCOMPARE(pruneSessionLogs(dir.path(), 1), 2);
        QCOMPARE(QDir(dir.path()).entryList(QDir::Files).size(), 2);

        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("Cannot list log directory")));
        QCOMPARE(pruneSessionLogs(dir.filePath(QStringLiteral("missing")), 3), 0);
    }

    void fileHandlerWritesJsonLines()
    {
        const QString path = m_dir.filePath(QStringLiteral("handler.jsonl"));
        {
            JsonLinesFileHandler handler(path);
            QVERIFY(handler.isOpen());
            handler.setLevel(loglevel::Info);
            handler.handle(entry(1.5, 10, QStringLiteral("dropped by level")));
            handler.handle(entry(2.0, 20, QStringLiteral("first")));
            handler.handle(entry(3.25, 40, QStringLiteral("second ü")));
        }
        {
            // Appends, as Python's mode "a".
            JsonLinesFileHandler handler(path);
            handler.handle(entry(4.0, 30, QStringLiteral("third")));
        }
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray content = file.readAll();
#ifdef Q_OS_WIN
        // Text mode, as Python's text-mode file.
        QVERIFY(content.contains("\r\n"));
#endif
        const QList<QByteArray> lines = content.trimmed().split('\n');
        QCOMPARE(lines.size(), 3);
        QCOMPARE(lines.at(0).trimmed(),
                 QByteArray("{\"ts\": 2.0, \"name\": \"app\", \"lvl\": \"INFO\", \"lvl_no\": 20, \"msg\": \"first\"}"));
        QCOMPARE(parseLogLine(QString::fromUtf8(lines.at(1)))->msg, QStringLiteral("second ü"));
        QCOMPARE(parseLogLine(QString::fromUtf8(lines.at(2)))->ts, 4.0);
    }

    // --- Qt message capture -------------------------------------------------------

    void capturesQtMessagesAndChains()
    {
        g_previousHandlerMessages.clear();
        const QtMessageHandler original = qInstallMessageHandler(previousHandler);
        {
            QtLogHandler handler;
            addLogHandler(&handler);
            QSignalSpy spy(&handler, &QtLogHandler::entryEmitted);

            qCWarning(lcTest, "disk %d%% full", 93);
            qCCritical(lcOther) << "broken";
            qDebug("plain");

            QCOMPARE(spy.size(), 3);
            const LogEntry warning = spy.at(0).at(0).value<LogEntry>();
            QCOMPARE(warning.name, QStringLiteral("app.test"));
            QCOMPARE(warning.lvl, QStringLiteral("WARNING"));
            QCOMPARE(warning.lvlNo, 30);
            QCOMPARE(warning.msg, QStringLiteral("disk 93% full"));
            QVERIFY(qAbs(warning.ts - QDateTime::currentMSecsSinceEpoch() / 1000.0) < 5);
            const LogEntry critical = spy.at(1).at(0).value<LogEntry>();
            QCOMPARE(critical.lvl, QStringLiteral("ERROR"));
            QCOMPARE(critical.lvlNo, 40);
            QCOMPARE(spy.at(2).at(0).value<LogEntry>().name, QStringLiteral("root"));

            // The previously installed handler still gets everything.
            QCOMPARE(g_previousHandlerMessages.size(), 3);
            QCOMPARE(g_previousHandlerMessages.first(), QStringLiteral("disk 93% full"));

            // Level threshold.
            handler.setLevel(loglevel::Error);
            qCWarning(lcTest, "below");
            QCOMPARE(spy.size(), 3);
        }
        // The handler is gone: the previous handler is back in place.
        qWarning("after");
        QCOMPARE(g_previousHandlerMessages.size(), 5);
        QCOMPARE(g_previousHandlerMessages.last(), QStringLiteral("after"));
        QVERIFY(qInstallMessageHandler(original) == &previousHandler);
    }

    void loggerSubtree()
    {
        g_previousHandlerMessages.clear();
        const QtMessageHandler original = qInstallMessageHandler(previousHandler);
        QtLogHandler handler;
        addLogHandler(&handler, QStringLiteral("app"));
        addLogHandler(&handler, QStringLiteral("app.test")); // still once per entry
        QSignalSpy spy(&handler, &QtLogHandler::entryEmitted);
        qCWarning(lcTest, "in");
        qCWarning(lcOther, "out");
        logMessage(loglevel::Info, QStringLiteral("app.direct"), QStringLiteral("posted"));
        logMessage(loglevel::Info, QStringLiteral("application"), QStringLiteral("not a child"));
        QCOMPARE(spy.size(), 2);
        QCOMPARE(spy.at(0).at(0).value<LogEntry>().msg, QStringLiteral("in"));
        QCOMPARE(spy.at(1).at(0).value<LogEntry>().msg, QStringLiteral("posted"));
        // logMessage() does not reach the console handler.
        QCOMPARE(g_previousHandlerMessages, (QStringList{QStringLiteral("in"), QStringLiteral("out")}));
        removeLogHandler(&handler, QStringLiteral("app"));
        removeLogHandler(&handler, QStringLiteral("app.test"));
        qCWarning(lcTest, "detached");
        QCOMPARE(spy.size(), 2);
        qInstallMessageHandler(original);
    }

    void deliversFromWorkerThreadQueued()
    {
        QtLogHandler handler;
        addLogHandler(&handler, QStringLiteral("app"));
        QObject receiver;
        QList<LogEntry> received;
        QThread *receivedIn = nullptr;
        connect(&handler, &QtLogHandler::entryEmitted, &receiver, [&](const LogEntry &e) {
            received << e;
            receivedIn = QThread::currentThread();
        });
        QThread *worker = QThread::create([] {
            for (int i = 0; i < 20; ++i)
                qCInfo(lcTest, "worker %d", i);
        });
        worker->start();
        QVERIFY(worker->wait(5000));
        delete worker;
        QTRY_COMPARE(received.size(), 20);
        QCOMPARE(receivedIn, QThread::currentThread());
        QCOMPARE(received.last().msg, QStringLiteral("worker 19"));
        QCOMPARE(received.last().lvl, QStringLiteral("INFO"));
    }

    // --- model --------------------------------------------------------------------

    void modelFilters()
    {
        LogTableModel model;
        model.queueEntry(entry(1, 10, QStringLiteral("debug one")));
        model.queueEntry(entry(2, 20, QStringLiteral("info two")));
        model.queueEntry(entry(3, 30, QStringLiteral("warning three"), QStringLiteral("net")));
        model.queueEntry(entry(4, 40, QStringLiteral("error four")));
        model.queueEntry(entry(5, 50, QStringLiteral("critical five")));
        model.flushPending();
        QCOMPARE(model.rowCount(), 5);

        LogFilter filter;
        filter.showDebug = false;
        filter.showError = false; // ERROR and CRITICAL
        model.setFilter(filter);
        model.rebuildFilter();
        QCOMPARE(messages(model), (QStringList{QStringLiteral("info two"), QStringLiteral("warning three")}));

        filter = {};
        filter.searchText = QStringLiteral("net");
        model.setFilter(filter);
        model.rebuildFilter();
        QCOMPARE(messages(model), QStringList{QStringLiteral("warning three")});
        // The formatted time is searched too.
        filter.searchText = formatLogTimestamp(4).left(10);
        model.setFilter(filter);
        model.rebuildFilter();
        QCOMPARE(model.rowCount(), 5);

        // Unseen.
        filter = {};
        filter.showUnseenOnly = true;
        model.setFilter(filter);
        model.setSeenCount(3);
        model.rebuildFilter();
        QCOMPARE(messages(model), (QStringList{QStringLiteral("error four"), QStringLiteral("critical five")}));

        // Roles.
        const QModelIndex time = model.index(0, LogTableModel::TimeColumn);
        QCOMPARE(time.data(Qt::UserRole).toDouble(), 4.0);
        QCOMPARE(time.data().toString(), formatLogTimestamp(4));
        QCOMPARE(model.index(0, LogTableModel::LevelColumn).data().toString(), QStringLiteral("ERROR"));
        QCOMPARE(model.index(0, LogTableModel::LevelColumn).data(Qt::UserRole).toInt(), 40);
        QVERIFY(time.data(Qt::BackgroundRole).isValid());
        QVERIFY(time.data(Qt::ForegroundRole).isValid());
        QCOMPARE(model.headerData(3, Qt::Horizontal).toString(), QStringLiteral("Message"));
    }

    void modelMarkers()
    {
        LogTableModel model;
        for (int i = 0; i < 10; ++i)
            model.queueEntry(entry(i, i % 3 == 0 ? 30 + (i % 2) * 20 : 20, QString::number(i)));
        model.flushPending();
        const auto marks = model.markerRows();
        QCOMPARE(marks.size(), std::size_t(4)); // 0, 3, 6, 9
        QCOMPARE(marks.at(0), std::make_pair(0, 30));
        QCOMPARE(marks.at(1), std::make_pair(3, 50));
        QCOMPARE(model.index(3, 0).data(Qt::BackgroundRole).value<QColor>()
                     != model.index(0, 0).data(Qt::BackgroundRole).value<QColor>(),
                 true);
        QVERIFY(!model.index(1, 0).data(Qt::BackgroundRole).isValid());
    }

    void liveBufferFull()
    {
        LogTableModel model;
        model.setLiveMax(10);
        for (int i = 0; i < 25; ++i) {
            model.queueEntry(entry(i, i % 2 ? 20 : 10, QString::number(i)));
            if (i % 4 == 3)
                model.flushPending();
        }
        model.flushPending();
        QCOMPARE(model.liveRecords().size(), std::size_t(10));
        QCOMPARE(model.rowCount(), 10);
        QStringList expected;
        for (int i = 15; i < 25; ++i)
            expected << QString::number(i);
        QCOMPARE(messages(model), expected);

        // Filtered rows follow the dropped records too (no duplicates, no gaps).
        LogFilter filter;
        filter.showDebug = false;
        model.setFilter(filter);
        model.rebuildFilter();
        for (int i = 25; i < 31; ++i)
            model.queueEntry(entry(i, i % 2 ? 20 : 10, QString::number(i)));
        model.flushPending();
        QCOMPARE(messages(model), (QStringList{QStringLiteral("21"), QStringLiteral("23"), QStringLiteral("25"),
                                               QStringLiteral("27"), QStringLiteral("29")}));

        // The unseen boundary moves down with the records it covers.
        model.setFilter({});
        model.setSeenCount(model.recordCount()); // all 10 seen
        LogFilter unseen;
        unseen.showUnseenOnly = true;
        model.setFilter(unseen);
        model.rebuildFilter();
        QCOMPARE(model.rowCount(), 0);
        model.queueEntry(entry(31, 20, QStringLiteral("31")));
        model.queueEntry(entry(32, 20, QStringLiteral("32")));
        model.flushPending();
        QCOMPARE(model.seenCount(), 8);
        QCOMPARE(messages(model), (QStringList{QStringLiteral("31"), QStringLiteral("32")}));
        model.rebuildFilter();
        QCOMPARE(messages(model), (QStringList{QStringLiteral("31"), QStringLiteral("32")}));
    }

    void loadsOlderEntriesFromFile()
    {
        const QString path = m_dir.filePath(QStringLiteral("older.jsonl"));
        QStringList lines;
        for (int i = 0; i < 1200; ++i)
            lines << logEntryToJson(entry(1000 + i, i % 5 == 0 ? 30 : 20, QStringLiteral("line %1").arg(i)));
        lines.insert(600, QStringLiteral("not a log line"));
        writeLines(path, lines);

        LogTableModel model;
        model.setLogPath(path);
        QVERIFY(!model.fileExhausted());
        // A live entry newer than the file.
        model.queueEntry(entry(5000, 20, QStringLiteral("live")));
        model.flushPending();

        int total = 0;
        int rounds = 0;
        while (!model.fileExhausted() && rounds++ < 10) {
            const int n = model.loadOlderEntries(500);
            QVERIFY(n >= 0);
            total += n;
        }
        QVERIFY(model.fileExhausted());
        QCOMPARE(total, 1200);
        QCOMPARE(model.rowCount(), 1201);
        QCOMPARE(model.fileRecordCount(), 1200);
        for (int i = 0; i < 1200; ++i)
            QCOMPARE(model.record(i).msg, QStringLiteral("line %1").arg(i));
        QCOMPARE(model.record(1200).msg, QStringLiteral("live"));
        QCOMPARE(model.loadOlderEntries(500), 0);
    }

    void olderEntriesHiddenByFilter()
    {
        const QString path = m_dir.filePath(QStringLiteral("older_debug.jsonl"));
        QStringList lines;
        for (int i = 0; i < 20; ++i)
            lines << logEntryToJson(entry(100 + i, 10, QStringLiteral("debug %1").arg(i)));
        writeLines(path, lines);
        LogTableModel model;
        model.setLogPath(path);
        LogFilter filter;
        filter.showDebug = false;
        model.setFilter(filter);
        model.queueEntry(entry(500, 30, QStringLiteral("live warning")));
        model.flushPending();
        QSignalSpy inserted(&model, &QAbstractItemModel::rowsInserted);
        QCOMPARE(model.loadOlderEntries(500), 20);
        QCOMPARE(inserted.size(), 0); // nothing visible, no rows
        QCOMPARE(messages(model), QStringList{QStringLiteral("live warning")});
        model.setFilter({});
        model.rebuildFilter();
        QCOMPARE(model.rowCount(), 21);
    }

    void reviewMode()
    {
        const QString path = m_dir.filePath(QStringLiteral("review.log"));
        // A plain-text "Save Log" file and a session line, mixed.
        writeLines(path, {formatLogEntry(entry(100, 20, QStringLiteral("old a"))), QStringLiteral("junk"),
                          logEntryToJson(entry(101, 40, QStringLiteral("old b"))),
                          formatLogEntry(entry(102, 30, QStringLiteral("old c")))});
        QCOMPARE(LogTableModel::countLogEntries(path), 4);

        LogTableModel model;
        model.queueEntry(entry(1000, 20, QStringLiteral("live 1")));
        model.flushPending();

        QCOMPARE(model.enterReview(m_dir.filePath(QStringLiteral("missing.log"))), 0);
        QVERIFY(!model.reviewActive());
        QCOMPARE(model.enterReview(path, 2), 2);
        QVERIFY(model.reviewActive());
        model.rebuildFilter();
        QCOMPARE(messages(model), (QStringList{QStringLiteral("old b"), QStringLiteral("old c")}));
        QCOMPARE(model.activeLogPath(), path);

        // Live entries keep arriving but are not shown.
        model.queueEntry(entry(1001, 20, QStringLiteral("live 2")));
        model.flushPending();
        QCOMPARE(model.liveMissed(), 1);
        QCOMPARE(model.rowCount(), 2);

        model.exitReview();
        QVERIFY(!model.reviewActive());
        QCOMPARE(messages(model), (QStringList{QStringLiteral("live 1"), QStringLiteral("live 2")}));
    }

    // --- widget -------------------------------------------------------------------

    void widgetSessionFileAndLiveEntries()
    {
        LogViewerWidget viewer;
        const QString sessionPath = viewer.model()->logPath();
        QVERIFY(QFile::exists(sessionPath));
        QCOMPARE(QFileInfo(sessionPath).absolutePath(), QFileInfo(m_dir.filePath(QStringLiteral("sessions"))).absoluteFilePath());
        QVERIFY(QFileInfo(sessionPath).fileName().startsWith(QStringLiteral("app_")));
        QVERIFY(sessionPath.endsWith(QStringLiteral(".jsonl")));
        QVERIFY(viewer.fileHandler() && viewer.fileHandler()->isOpen());

        viewer.attachLogger(QStringLiteral("app"));
        qCWarning(lcTest, "captured warning");
        qCWarning(lcOther, "not attached");
        viewer.log(loglevel::Info, QStringLiteral("app.direct"), QStringLiteral("posted"));
        QTRY_COMPARE(viewer.model()->rowCount(), 2);
        QCOMPARE(messages(*viewer.model()), (QStringList{QStringLiteral("captured warning"), QStringLiteral("posted")}));

        // Both are on disk, as JSON Lines.
        QFile file(sessionPath);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QList<QByteArray> lines = file.readAll().trimmed().split('\n');
        QCOMPARE(lines.size(), 2);
        QCOMPARE(parseLogLine(QString::fromUtf8(lines.at(0)))->name, QStringLiteral("app.test"));

        // Save Log: plain text, readable again.
        const QString saved = m_dir.filePath(QStringLiteral("saved.log"));
        QVERIFY(viewer.saveLogsTo(saved));
        QFile savedFile(saved);
        QVERIFY(savedFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QStringList savedLines = QString::fromUtf8(savedFile.readAll()).split(QLatin1Char('\n'));
        QCOMPARE(savedLines.size(), 2);
        QVERIFY(savedLines.at(0).endsWith(QStringLiteral(" - app.test    - WARNING - captured warning")));
        QCOMPARE(parseLogLine(savedLines.at(1))->msg, QStringLiteral("posted"));

        // Review the saved file, then back to live.
        viewer.log(loglevel::Error, QStringLiteral("app"), QStringLiteral("while reviewing"));
        QCOMPARE(viewer.reviewFile(saved), 2);
        QVERIFY(viewer.isReviewing());
        QVERIFY(!viewer.findChild<QCheckBox *>(QStringLiteral("checkboxNewOnly"))->isEnabled());
        QTRY_COMPARE(viewer.model()->liveMissed(), 1);
        QCOMPARE(viewer.model()->rowCount(), 2);
        viewer.exitReview();
        QVERIFY(!viewer.isReviewing());
        QCOMPARE(viewer.model()->rowCount(), 3);
        viewer.detachLogger(QStringLiteral("app"));
    }

    void widgetUnseenAndFilters()
    {
        LogViewerWidget viewer;
        for (int i = 0; i < 3; ++i)
            viewer.log(loglevel::Info, QStringLiteral("app"), QStringLiteral("before %1").arg(i));
        QTRY_COMPARE(viewer.model()->rowCount(), 3);
        viewer.markSeen();
        viewer.findChild<QCheckBox *>(QStringLiteral("checkboxNewOnly"))->setChecked(true);
        QCOMPARE(viewer.model()->rowCount(), 0);
        viewer.log(loglevel::Debug, QStringLiteral("app"), QStringLiteral("after"));
        QTRY_COMPARE(viewer.model()->rowCount(), 1);

        viewer.findChild<QCheckBox *>(QStringLiteral("checkboxDebug"))->setChecked(false);
        QCOMPARE(viewer.model()->rowCount(), 0);
        viewer.findChild<QCheckBox *>(QStringLiteral("checkboxNewOnly"))->setChecked(false);
        QCOMPARE(viewer.model()->rowCount(), 3);
        // The search is applied once typing pauses.
        viewer.findChild<QLineEdit *>(QStringLiteral("searchInput"))->setText(QStringLiteral("  BEFORE 1 "));
        QCOMPARE(viewer.model()->rowCount(), 3);
        QTRY_COMPARE(viewer.model()->rowCount(), 1);
    }

    void widgetPrunesSessions()
    {
        const QString dir = m_dir.filePath(QStringLiteral("prune"));
        QDir().mkpath(dir);
        for (int day = 1; day <= 6; ++day)
            writeLines(QDir(dir).filePath(QStringLiteral("app_2026010%1_000000.jsonl").arg(day)), {});
        auto settings = getSettings();
        const QVariant previousDir = settings->value(QStringLiteral("log_directory"));
        settings->setValue(QStringLiteral("log_directory"), dir);
        settings->setValue(QStringLiteral("log_retention_sessions"), 3);
        settings->sync();
        {
            LogViewerWidget viewer;
            // Two old sessions and the new one.
            QCOMPARE(QDir(dir).entryList(QDir::Files).size(), 3);
            QVERIFY(QDir(dir).entryList(QDir::Files).contains(QFileInfo(viewer.model()->logPath()).fileName()));
        }
        settings->setValue(QStringLiteral("log_directory"), previousDir);
        settings->remove(QStringLiteral("log_retention_sessions"));
    }
};

QTEST_MAIN(TstLogViewer)
#include "tst_logviewer.moc"

#pragma once

// Performant log viewer. Port of LogViewerWidget (and its _LogTableModel) in
// duifx/gui/widgets.py.
//
// Level filters, a search box, an "Unseen" filter with "Mark seen", automatic
// persistence into a per-session JSON Lines file (see LogHandler.h), a ring
// buffer of live entries with batched (50 ms) table updates, scroll-back that
// loads older entries from the file when the table is scrolled to the top, a
// review mode that shows a saved log file instead of the running session, and
// an overview scroll bar that marks where the warnings and errors are.
//
//     auto *viewer = new duifx::LogViewerWidget;
//     viewer->attachLogger();            // every Qt message (or attachLogger("app"))
//     viewer->log(duifx::loglevel::Info, "app", "Started");   // without Qt logging
//
// Settings read at construction: "log_directory" (default ~/logs) and
// "log_retention_sessions" (default kLogRetentionSessionsDefault).

#include "duifx/LogHandler.h"

#include <QAbstractTableModel>
#include <QWidget>

#include <deque>
#include <memory>
#include <utility>
#include <vector>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableView;
class QTimer;

namespace duifx {

/// Which entries LogTableModel shows.
struct LogFilter
{
    bool showDebug = true;
    bool showInfo = true;
    bool showWarning = true;
    bool showError = true;
    bool showUnseenOnly = false;
    /// Lower-case search text; matched against module, level, message and the
    /// formatted time.
    QString searchText;
};

/// Table model of LogViewerWidget: records loaded from the file (scroll-back)
/// followed by the live ring buffer, or the records of a reviewed file. Rows
/// are the visible subset; filtering lives here, not in a proxy model, which
/// would cost far too much at six-figure row counts.
class LogTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column { TimeColumn, ModuleColumn, LevelColumn, MessageColumn, ColumnCount };

    explicit LogTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    /// DisplayRole: formatted text; UserRole: raw value (ts, name, lvl_no, msg);
    /// BackgroundRole / ForegroundRole: the severity tint of WARNING and above.
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // --- filter -----------------------------------------------------------------
    /// Takes effect with the next rebuildFilter() (and for new entries at once).
    void setFilter(const LogFilter &filter) { m_filter = filter; }
    const LogFilter &filter() const { return m_filter; }
    /// Records below this index count as seen (the "Unseen" filter).
    int seenCount() const { return m_seenCount; }
    void setSeenCount(int count) { m_seenCount = count; }
    /// Recomputes the visible rows (a model reset).
    void rebuildFilter();

    // --- records ----------------------------------------------------------------
    /// Records currently backing the view: file + live, or the reviewed file.
    int recordCount() const;
    const LogEntry &record(int index) const;
    /// Index (into record()) shown in row.
    int recordIndex(int row) const { return m_filtered.at(static_cast<std::size_t>(row)); }
    int fileRecordCount() const { return static_cast<int>(m_fileRecords.size()); }
    const std::deque<LogEntry> &liveRecords() const { return m_liveRecords; }
    bool hasRecords() const;

    /// (row, lvl_no) of every visible row at WARNING or above, for the overview
    /// scroll bar. Reads the records directly; data() would cost far more.
    std::vector<std::pair<int, int>> markerRows() const;

    // --- live buffer ------------------------------------------------------------
    int liveMax() const { return m_liveMax; }
    void setLiveMax(int max) { m_liveMax = max; }
    /// Queues an entry for the next flushPending().
    void queueEntry(const LogEntry &entry) { m_pending.push_back(entry); }
    bool hasPending() const { return !m_pending.empty(); }
    /// Moves the queued entries into the live buffer in one batch, dropping the
    /// oldest live records beyond liveMax().
    void flushPending();

    // --- scroll-back ------------------------------------------------------------
    /// The session file; scroll-back starts at its current end.
    void setLogPath(const QString &path);
    QString logPath() const { return m_logPath; }
    /// Nothing older left to load from the active file.
    bool fileExhausted() const { return m_reviewActive ? m_reviewExhausted : m_fileExhausted; }
    /// Reads up to count older entries (backwards, overshooting by at most one
    /// 8 KiB chunk) from the active file and puts them in front. Returns the
    /// number of entries loaded.
    int loadOlderEntries(int count = 500);

    // --- review -----------------------------------------------------------------
    bool reviewActive() const { return m_reviewActive; }
    QString reviewPath() const { return m_reviewPath; }
    /// Live entries that arrived while a file was under review.
    int liveMissed() const { return m_liveMissed; }
    /// Path of the file currently backing the view.
    QString activeLogPath() const { return m_reviewActive ? m_reviewPath : m_logPath; }
    /// Shows path instead of the running session, read in one forward pass.
    /// count > 0 keeps only the last count entries. Returns the number of entries
    /// loaded; 0 means nothing readable, and review mode is not entered.
    int enterReview(const QString &path, int count = 0);
    /// Drops the reviewed file and shows the running session again.
    void exitReview();
    /// Line count of path without parsing it (the warning before a large load).
    static int countLogEntries(const QString &path);

private:
    bool isVisible(const LogEntry &entry, int globalIndex) const;
    void dropFiltered(int first, int count);
    void setFileExhausted(bool exhausted);

    std::deque<LogEntry> m_fileRecords;
    std::deque<LogEntry> m_liveRecords;
    int m_liveMax = 5'000;
    std::vector<int> m_filtered;
    std::vector<LogEntry> m_pending;

    LogFilter m_filter;
    int m_seenCount = 0;

    QString m_logPath;
    qint64 m_fileReadPos = 0;
    bool m_fileExhausted = true;

    // Review: a log file loaded for inspection. Incoming entries keep filling
    // the live buffer, so returning to live is instant.
    bool m_reviewActive = false;
    std::deque<LogEntry> m_reviewRecords;
    QString m_reviewPath;
    qint64 m_reviewReadPos = 0;
    bool m_reviewExhausted = true;
    int m_liveMissed = 0;
};

class LogViewerWidget : public QWidget
{
    Q_OBJECT

public:
    /// Live entries kept in memory (BUFFER_MAX).
    static constexpr int kBufferMax = 5'000;
    /// Pending entries are flushed into the table this often.
    static constexpr int kBatchIntervalMs = 50;

    explicit LogViewerWidget(QWidget *parent = nullptr);
    ~LogViewerWidget() override;

    /// The handler that forwards entries to this widget.
    QtLogHandler *handler() const { return m_handler; }
    /// The handler writing the session file (nullptr if none).
    JsonLinesFileHandler *fileHandler() const { return m_fileHandler.get(); }

    /// Attaches both handlers to the logger (Qt category) loggerName and its
    /// children; empty is every message. See addLogHandler().
    void attachLogger(const QString &loggerName = {});
    /// Detaches both handlers from loggerName.
    void detachLogger(const QString &loggerName = {});

    /// Posts an entry to this widget and its session file, without Qt logging.
    /// Thread-safe.
    void log(int levelNo, const QString &loggerName, const QString &message);

    LogTableModel *model() const { return m_model; }
    QTableView *tableView() const { return m_table; }
    bool isReviewing() const;

    /// "Load Log": asks, picks a file, loads it into review mode.
    void loadLogs();
    /// Review path without any dialog; limit > 0 keeps only the last entries.
    /// Returns the number of entries loaded (0: nothing changed).
    int reviewFile(const QString &path, int limit = 0);
    /// "Back to Live".
    void exitReview();
    /// "Mark seen": everything present now counts as seen.
    void markSeen();
    /// "Save Log": picks a file and calls saveLogsTo().
    void saveLogs();
    /// Writes what is on screen -- the reviewed file, or the session file plus
    /// live entries not yet on disk -- as plain text (formatLogEntry()).
    bool saveLogsTo(const QString &path);

private:
    void setupFileHandler();
    void appendEntry(const LogEntry &entry);
    void flushAndScroll();
    void updateReviewState();
    void updateReviewLabel();
    void syncFilterState();
    void onFilterChanged();
    void onScroll(int value);

    LogTableModel *m_model = nullptr;
    QtLogHandler *m_handler = nullptr;
    std::unique_ptr<JsonLinesFileHandler> m_fileHandler;
    QTimer *m_flushTimer = nullptr;
    QTimer *m_searchTimer = nullptr;
    QTimer *m_reviewTimer = nullptr;

    QCheckBox *m_checkboxError = nullptr;
    QCheckBox *m_checkboxWarning = nullptr;
    QCheckBox *m_checkboxInfo = nullptr;
    QCheckBox *m_checkboxDebug = nullptr;
    QCheckBox *m_checkboxNewOnly = nullptr;
    QPushButton *m_btnMarkSeen = nullptr;
    QPushButton *m_btnSave = nullptr;
    QPushButton *m_btnLoad = nullptr;
    QLineEdit *m_searchInput = nullptr;
    QWidget *m_reviewBar = nullptr;
    QLabel *m_reviewLabel = nullptr;
    QPushButton *m_btnBackLive = nullptr;
    QTableView *m_table = nullptr;
    bool m_loadingOlder = false;
};

} // namespace duifx

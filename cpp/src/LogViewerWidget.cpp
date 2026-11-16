#include "duifx/LogViewerWidget.h"

#include "duifx/Colors.h"
#include "duifx/Settings.h"
#include "duifx/Tables.h"
#include "duifx/Utils.h"
#include "Logging.h"

#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QScrollBar>
#include <QStyle>
#include <QStyleOptionSlider>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <limits>
#include <map>
#include <optional>

namespace duifx {

namespace {

const char *const kColumns[LogTableModel::ColumnCount] = {"Time", "Module", "Level", "Message"};

// Severity colours: the same values as widgets.py, so a log reads the same
// in the Python and the C++ applications.

/// Row background per severity, matched by threshold (highest first) so custom
/// levels in between are tinted too.
std::optional<QColor> levelBackground(int lvlNo)
{
    if (lvlNo >= loglevel::Critical)
        return QColor(0xf5, 0xc2, 0xc7); // red, stronger than ERROR
    if (lvlNo >= loglevel::Error)
        return QColor(0xf8, 0xd7, 0xda); // red
    if (lvlNo >= loglevel::Warning)
        return QColor(0xfa, 0xff, 0xb2); // orange
    return std::nullopt;
}

/// Marks on the overview scroll bar: saturated on purpose, they are a few
/// pixels tall and have to read against the groove.
std::optional<QColor> markerColor(int lvlNo)
{
    if (lvlNo >= loglevel::Critical)
        return QColor(0xb7, 0x1c, 0x1c);
    if (lvlNo >= loglevel::Error)
        return QColor(0xe5, 0x39, 0x35);
    if (lvlNo >= loglevel::Warning)
        return QColor(0xfb, 0x8c, 0x00);
    return std::nullopt;
}

std::optional<LogEntry> decodeLine(const QByteArray &raw)
{
    // Invalid UTF-8 becomes U+FFFD, as errors="replace".
    return parseLogLine(QString::fromUtf8(raw));
}

/// Paints the severity background behind each cell. The view's
/// "QTableView::item" stylesheet rule routes painting through QStyleSheetStyle,
/// which ignores Qt::BackgroundRole. Selected rows are left to the style so the
/// selection stays visible.
class LogItemDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        if (!(option.state & QStyle::State_Selected)) {
            const QVariant background = index.data(Qt::BackgroundRole);
            if (background.isValid())
                painter->fillRect(option.rect, background.value<QColor>());
        }
        QStyledItemDelegate::paint(painter, option, index);
    }
};

/// Vertical scroll bar that marks where the warnings and errors are, like the
/// overview ruler of an editor: every visible row at WARNING or above leaves a
/// coloured tick at its relative position.
class OverviewScrollBar : public QScrollBar
{
public:
    /// Rebuilds are coalesced: during live logging the model changes on every
    /// 50 ms flush, and rescanning a six-figure row count that often costs more
    /// than the ruler is worth.
    static constexpr int kRebuildDelayMs = 250;
    static constexpr int kMarkHeight = 2;

    OverviewScrollBar(LogTableModel *model, QWidget *parent)
        : QScrollBar(Qt::Vertical, parent)
        , m_model(model)
    {
        m_rebuildTimer = new QTimer(this);
        m_rebuildTimer->setSingleShot(true);
        m_rebuildTimer->setInterval(kRebuildDelayMs);
        connect(m_rebuildTimer, &QTimer::timeout, this, [this] {
            m_marks = m_model->markerRows();
            update();
        });
        const auto schedule = [this] {
            if (!m_rebuildTimer->isActive())
                m_rebuildTimer->start();
        };
        connect(model, &QAbstractItemModel::rowsInserted, this, schedule);
        connect(model, &QAbstractItemModel::rowsRemoved, this, schedule);
        connect(model, &QAbstractItemModel::modelReset, this, schedule);
        connect(model, &QAbstractItemModel::layoutChanged, this, schedule);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        QScrollBar::paintEvent(event);
        if (m_marks.empty())
            return;
        const qint64 total = m_model->rowCount();
        if (total <= 0)
            return;
        QStyleOptionSlider option;
        initStyleOption(&option);
        const QRect groove = style()->subControlRect(QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarGroove, this);
        if (groove.height() <= kMarkHeight)
            return;

        // One mark per pixel row, strongest severity winning: a 100 000-row log
        // would otherwise paint the same few hundred pixels thousands of times.
        const qint64 span = groove.height() - kMarkHeight;
        std::map<int, int> buckets;
        for (const auto &[row, lvlNo] : m_marks) {
            if (row >= total)
                continue;
            const int y = groove.top() + static_cast<int>(span * row / total);
            int &strongest = buckets[y];
            strongest = std::max(strongest, lvlNo);
        }
        if (buckets.empty())
            return;

        // Drawn over the handle, not beside it: a mark hidden under the handle
        // is a mark the user cannot find.
        const int width = std::max(3, groove.width() / 2);
        const int x = groove.right() - width + 1;
        QPainter painter(this);
        for (const auto &[y, lvlNo] : buckets) {
            if (const std::optional<QColor> c = markerColor(lvlNo))
                painter.fillRect(x, y, width, kMarkHeight, *c);
        }
    }

private:
    LogTableModel *m_model;
    QTimer *m_rebuildTimer = nullptr;
    std::vector<std::pair<int, int>> m_marks;
};

} // namespace

// =====================================================================================
// LogTableModel
// =====================================================================================

LogTableModel::LogTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int LogTableModel::recordCount() const
{
    if (m_reviewActive)
        return static_cast<int>(m_reviewRecords.size());
    return static_cast<int>(m_fileRecords.size() + m_liveRecords.size());
}

const LogEntry &LogTableModel::record(int index) const
{
    const auto i = static_cast<std::size_t>(index);
    if (m_reviewActive)
        return m_reviewRecords.at(i);
    if (i < m_fileRecords.size())
        return m_fileRecords[i];
    return m_liveRecords.at(i - m_fileRecords.size());
}

bool LogTableModel::hasRecords() const
{
    return !m_fileRecords.empty() || !m_liveRecords.empty() || !m_pending.empty();
}

bool LogTableModel::isVisible(const LogEntry &entry, int globalIndex) const
{
    const int lvl = entry.lvlNo;
    if (lvl >= loglevel::Error && !m_filter.showError)
        return false;
    if (lvl >= loglevel::Warning && lvl < loglevel::Error && !m_filter.showWarning)
        return false;
    if (lvl >= loglevel::Info && lvl < loglevel::Warning && !m_filter.showInfo)
        return false;
    if (lvl < loglevel::Info && !m_filter.showDebug)
        return false;
    if (m_filter.showUnseenOnly && globalIndex < m_seenCount)
        return false;
    if (!m_filter.searchText.isEmpty()) {
        const QString &needle = m_filter.searchText;
        // Cheap fields first; the timestamp is formatted only if needed.
        if (entry.name.contains(needle, Qt::CaseInsensitive) || entry.lvl.contains(needle, Qt::CaseInsensitive)
            || entry.msg.contains(needle, Qt::CaseInsensitive))
            return true;
        return formatLogTimestamp(entry.ts).contains(needle);
    }
    return true;
}

int LogTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_filtered.size());
}

int LogTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant LogTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal && section >= 0 && section < ColumnCount)
        return QString::fromLatin1(kColumns[section]);
    return {};
}

QVariant LogTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount())
        return {};
    if (role != Qt::DisplayRole && role != Qt::UserRole && role != Qt::BackgroundRole && role != Qt::ForegroundRole)
        return {};
    const int recordIdx = m_filtered[static_cast<std::size_t>(index.row())];
    if (recordIdx >= recordCount())
        return {};
    const LogEntry &entry = record(recordIdx);
    const int column = index.column();

    if (role == Qt::BackgroundRole) {
        if (const auto background = levelBackground(entry.lvlNo))
            return *background;
        return {};
    }
    if (role == Qt::ForegroundRole) {
        // Only where a tinted background was applied.
        // The tints are light in every theme: pin the light theme's dark ink.
        if (levelBackground(entry.lvlNo))
            return color(Token::TextPrimary, std::nullopt, Theme::Light);
        return {};
    }
    if (role == Qt::UserRole) {
        // Raw sortable values.
        switch (column) {
        case TimeColumn:
            return entry.ts;
        case ModuleColumn:
            return entry.name;
        case LevelColumn:
            return entry.lvlNo;
        case MessageColumn:
            return entry.msg;
        default:
            return {};
        }
    }
    switch (column) {
    case TimeColumn:
        return formatLogTimestamp(entry.ts);
    case ModuleColumn:
        return entry.name;
    case LevelColumn:
        return entry.lvl;
    case MessageColumn:
        return entry.msg;
    default:
        return {};
    }
}

std::vector<std::pair<int, int>> LogTableModel::markerRows() const
{
    std::vector<std::pair<int, int>> out;
    const int total = recordCount();
    for (std::size_t row = 0; row < m_filtered.size(); ++row) {
        const int idx = m_filtered[row];
        if (idx < total) {
            const int lvlNo = record(idx).lvlNo;
            if (lvlNo >= loglevel::Warning)
                out.emplace_back(static_cast<int>(row), lvlNo);
        }
    }
    return out;
}

void LogTableModel::flushPending()
{
    if (m_pending.empty())
        return;
    std::vector<LogEntry> batch;
    batch.swap(m_pending);
    const int batchSize = static_cast<int>(batch.size());

    // Trim the live buffer if over the limit (drop the oldest).
    const int firstLive = static_cast<int>(m_fileRecords.size());
    const int liveCount = static_cast<int>(m_liveRecords.size());
    const int dropped = std::min(std::max(liveCount + batchSize - m_liveMax, 0), liveCount);
    if (dropped) {
        m_liveRecords.erase(m_liveRecords.begin(), m_liveRecords.begin() + dropped);
        // Every later record moved down by dropped: so does the unseen
        // boundary, or new entries would land below it and stay hidden.
        if (m_seenCount > firstLive)
            m_seenCount = std::max(firstLive, m_seenCount - dropped);
    }
    m_liveRecords.insert(m_liveRecords.end(), std::make_move_iterator(batch.begin()),
                         std::make_move_iterator(batch.end()));

    // While a file is under review the live entries are collected but not
    // shown: mixed into a log from another day they would read as one
    // continuous run. They appear as soon as the user returns to live.
    if (m_reviewActive) {
        m_liveMissed += batchSize;
        return;
    }

    if (dropped)
        dropFiltered(firstLive, dropped);

    const int total = recordCount();
    std::vector<int> newVisible;
    for (int i = total - batchSize; i < total; ++i) {
        if (isVisible(record(i), i))
            newVisible.push_back(i);
    }
    if (!newVisible.empty()) {
        const int pos = static_cast<int>(m_filtered.size());
        beginInsertRows({}, pos, pos + static_cast<int>(newVisible.size()) - 1);
        m_filtered.insert(m_filtered.end(), newVisible.begin(), newVisible.end());
        endInsertRows();
    }
}

void LogTableModel::dropFiltered(int first, int count)
{
    // Records first .. first + count - 1 were dropped: remove their rows and move
    // the later indices down by count. Otherwise the rows kept pointing at the
    // old positions, and with the buffer full entries showed up twice.
    const auto lo = std::lower_bound(m_filtered.begin(), m_filtered.end(), first);
    const auto hi = std::lower_bound(lo, m_filtered.end(), first + count);
    const auto shift = [count](int &i) { i -= count; };
    if (hi > lo) {
        const int loRow = static_cast<int>(lo - m_filtered.begin());
        const int hiRow = static_cast<int>(hi - m_filtered.begin());
        beginRemoveRows({}, loRow, hiRow - 1);
        m_filtered.erase(m_filtered.begin() + loRow, m_filtered.begin() + hiRow);
        std::for_each(m_filtered.begin() + loRow, m_filtered.end(), shift);
        endRemoveRows();
    } else {
        std::for_each(hi, m_filtered.end(), shift);
    }
}

void LogTableModel::rebuildFilter()
{
    beginResetModel();
    m_filtered.clear();
    const int total = recordCount();
    for (int i = 0; i < total; ++i) {
        if (isVisible(record(i), i))
            m_filtered.push_back(i);
    }
    endResetModel();
}

void LogTableModel::setLogPath(const QString &path)
{
    m_logPath = path;
    const QFileInfo info(path);
    if (!path.isEmpty() && info.isFile()) {
        m_fileReadPos = info.size();
        m_fileExhausted = m_fileReadPos == 0;
    } else {
        m_fileReadPos = 0;
        m_fileExhausted = true;
    }
}

void LogTableModel::setFileExhausted(bool exhausted)
{
    (m_reviewActive ? m_reviewExhausted : m_fileExhausted) = exhausted;
}

int LogTableModel::loadOlderEntries(int count)
{
    // Scroll-back reads the session file normally and the reviewed file while
    // one is loaded; the same backward reader serves both.
    const QString source = activeLogPath();
    if (fileExhausted() || source.isEmpty())
        return 0;
    qint64 &readPos = m_reviewActive ? m_reviewReadPos : m_fileReadPos;

    std::vector<LogEntry> loaded; // newest first
    constexpr qint64 kChunkSize = 8192;
    QByteArray remainder;
    QFile file(source);
    if (!file.open(QIODevice::ReadOnly)) {
        setFileExhausted(true);
        return 0;
    }
    qint64 pos = readPos;
    while (static_cast<int>(loaded.size()) < count && pos > 0) {
        const qint64 readSize = std::min(kChunkSize, pos);
        pos -= readSize;
        if (!file.seek(pos)) {
            setFileExhausted(true);
            return 0;
        }
        const QByteArray chunk = file.read(readSize) + remainder;
        const QList<QByteArray> lines = chunk.split('\n');
        remainder = lines.first();
        // Every line of the chunk is consumed, even once count is reached: the
        // read position below is the start of the chunk, so stopping mid-chunk
        // would skip entries that were never returned.
        for (qsizetype i = lines.size() - 1; i >= 1; --i) {
            if (std::optional<LogEntry> entry = decodeLine(lines.at(i)))
                loaded.push_back(std::move(*entry));
        }
    }
    if (pos == 0 && !remainder.trimmed().isEmpty()) {
        if (std::optional<LogEntry> entry = decodeLine(remainder))
            loaded.push_back(std::move(*entry));
        remainder.clear();
    }
    readPos = pos + remainder.size();
    if (readPos <= 0)
        setFileExhausted(true);

    if (loaded.empty()) {
        setFileExhausted(true);
        return 0;
    }
    std::reverse(loaded.begin(), loaded.end());

    // Only what is older than everything already present.
    const std::deque<LogEntry> *existing = m_reviewActive           ? &m_reviewRecords
                                           : !m_fileRecords.empty() ? &m_fileRecords
                                                                    : &m_liveRecords;
    if (!existing->empty()) {
        const double oldestTs = existing->front().ts;
        std::erase_if(loaded, [oldestTs](const LogEntry &e) { return e.ts >= oldestTs; });
    }
    if (loaded.empty())
        return 0;

    const int n = static_cast<int>(loaded.size());
    // The older entries go in front, below the unseen boundary: once something
    // was marked seen they count as seen, and the boundary moves with the
    // records it covers.
    if (!m_reviewActive && m_seenCount > 0)
        m_seenCount += n;
    std::vector<int> newRows;
    for (int i = 0; i < n; ++i) {
        if (isVisible(loaded[static_cast<std::size_t>(i)], i))
            newRows.push_back(i);
    }

    // Python announces n rows here even when fewer are visible; only the
    // visible ones are rows.
    const bool insertsRows = !newRows.empty();
    if (insertsRows)
        beginInsertRows({}, 0, static_cast<int>(newRows.size()) - 1);
    std::deque<LogEntry> &target = m_reviewActive ? m_reviewRecords : m_fileRecords;
    target.insert(target.begin(), std::make_move_iterator(loaded.begin()), std::make_move_iterator(loaded.end()));
    for (int &i : m_filtered)
        i += n;
    m_filtered.insert(m_filtered.begin(), newRows.begin(), newRows.end());
    if (insertsRows)
        endInsertRows();
    return n;
}

int LogTableModel::enterReview(const QString &path, int count)
{
    if (path.isEmpty() || !QFileInfo(path).isFile())
        return 0;

    // One forward pass over the whole file: far cheaper than it sounds, and
    // much faster than walking it backwards in chunks.
    std::deque<LogEntry> records;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(lcDuifx, "Cannot read log file %s: %s", qPrintable(path), qPrintable(file.errorString()));
        return 0;
    }
    while (!file.atEnd()) {
        if (std::optional<LogEntry> entry = decodeLine(file.readLine()))
            records.push_back(std::move(*entry));
    }
    if (records.empty())
        return 0;
    if (count > 0 && static_cast<int>(records.size()) > count)
        records.erase(records.begin(), records.end() - count);

    beginResetModel();
    m_reviewActive = true;
    m_reviewRecords = std::move(records);
    m_reviewPath = path;
    // Everything is in memory, so there is nothing left to scroll back into.
    m_reviewReadPos = 0;
    m_reviewExhausted = true;
    m_liveMissed = 0;
    m_filtered.clear();
    endResetModel();
    return static_cast<int>(m_reviewRecords.size());
}

int LogTableModel::countLogEntries(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return 0;
    qint64 total = 0;
    while (true) {
        const QByteArray block = file.read(1 << 20);
        if (block.isEmpty())
            break;
        total += block.count('\n');
    }
    return static_cast<int>(std::min<qint64>(total, std::numeric_limits<int>::max()));
}

void LogTableModel::exitReview()
{
    if (!m_reviewActive)
        return;
    beginResetModel();
    m_reviewActive = false;
    m_reviewRecords.clear();
    m_reviewPath.clear();
    m_reviewReadPos = 0;
    m_reviewExhausted = true;
    m_liveMissed = 0;
    m_filtered.clear();
    endResetModel();
    rebuildFilter();
}

// =====================================================================================
// LogViewerWidget
// =====================================================================================

LogViewerWidget::LogViewerWidget(QWidget *parent)
    : QWidget(parent)
{
    initResources();

    m_handler = new QtLogHandler(this);
    connect(m_handler, &QtLogHandler::entryEmitted, this, &LogViewerWidget::appendEntry);

    m_model = new LogTableModel(this);
    m_model->setLiveMax(kBufferMax);

    m_flushTimer = new QTimer(this);
    m_flushTimer->setInterval(kBatchIntervalMs);
    connect(m_flushTimer, &QTimer::timeout, this, &LogViewerWidget::flushAndScroll);

    setupFileHandler();

    // --- filter widgets ---
    m_checkboxError = new QCheckBox(QStringLiteral("Error"));
    m_checkboxWarning = new QCheckBox(QStringLiteral("Warning"));
    m_checkboxInfo = new QCheckBox(QStringLiteral("Info"));
    m_checkboxDebug = new QCheckBox(QStringLiteral("Debug"));
    m_checkboxNewOnly = new QCheckBox(QStringLiteral("Unseen"));
    m_checkboxError->setObjectName(QStringLiteral("checkboxError"));
    m_checkboxWarning->setObjectName(QStringLiteral("checkboxWarning"));
    m_checkboxInfo->setObjectName(QStringLiteral("checkboxInfo"));
    m_checkboxDebug->setObjectName(QStringLiteral("checkboxDebug"));
    m_checkboxNewOnly->setObjectName(QStringLiteral("checkboxNewOnly"));

    for (QCheckBox *cb : {m_checkboxDebug, m_checkboxInfo, m_checkboxWarning, m_checkboxError, m_checkboxNewOnly})
        cb->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    for (QCheckBox *cb : {m_checkboxDebug, m_checkboxInfo, m_checkboxWarning, m_checkboxError}) {
        cb->setChecked(true);
        connect(cb, &QCheckBox::toggled, this, &LogViewerWidget::onFilterChanged);
    }
    m_checkboxNewOnly->setChecked(false);
    connect(m_checkboxNewOnly, &QCheckBox::toggled, this, &LogViewerWidget::onFilterChanged);

    m_btnMarkSeen = new QPushButton(QStringLiteral("✓ Mark seen"));
    m_btnMarkSeen->setToolTip(QStringLiteral("Mark all current logs as seen"));
    connect(m_btnMarkSeen, &QPushButton::clicked, this, &LogViewerWidget::markSeen);

    m_btnSave = new QPushButton(QStringLiteral("Save Log"));
    connect(m_btnSave, &QPushButton::clicked, this, &LogViewerWidget::saveLogs);

    m_btnLoad = new QPushButton(QStringLiteral("Load Log"));
    m_btnLoad->setToolTip(QStringLiteral("Open a saved log file for inspection"));
    connect(m_btnLoad, &QPushButton::clicked, this, &LogViewerWidget::loadLogs);

    m_searchInput = new QLineEdit;
    m_searchInput->setObjectName(QStringLiteral("searchInput"));
    m_searchInput->setPlaceholderText(QStringLiteral("Search..."));
    m_searchInput->setClearButtonEnabled(true);
    // Rebuilding the filter scans every record: fine once, wasteful on every
    // keystroke, so typing is applied when it pauses.
    m_searchTimer = new QTimer(this);
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(200);
    connect(m_searchTimer, &QTimer::timeout, this, &LogViewerWidget::onFilterChanged);
    connect(m_searchInput, &QLineEdit::textChanged, m_searchTimer, [this] { m_searchTimer->start(); });

    auto *filtersLayout = new QHBoxLayout;
    filtersLayout->addWidget(m_checkboxDebug);
    filtersLayout->addWidget(m_checkboxInfo);
    filtersLayout->addWidget(m_checkboxWarning);
    filtersLayout->addWidget(m_checkboxError);
    filtersLayout->addWidget(m_checkboxNewOnly);
    filtersLayout->addWidget(m_searchInput, 1);
    filtersLayout->addWidget(m_btnMarkSeen);
    filtersLayout->addWidget(m_btnLoad);
    filtersLayout->addWidget(m_btnSave);

    // --- review banner ---
    // Hidden while live. It is the only thing telling the user that what they
    // see is a file and not the running application, so it carries the way back.
    m_reviewLabel = new QLabel;
    m_reviewLabel->setWordWrap(false);
    m_btnBackLive = new QPushButton(QStringLiteral("← Back to Live"));
    connect(m_btnBackLive, &QPushButton::clicked, this, &LogViewerWidget::exitReview);

    m_reviewBar = new QWidget;
    m_reviewBar->setObjectName(QStringLiteral("reviewBar"));
    auto *reviewLayout = new QHBoxLayout(m_reviewBar);
    reviewLayout->setContentsMargins(6, 3, 6, 3);
    reviewLayout->addWidget(m_reviewLabel, 1);
    reviewLayout->addWidget(m_btnBackLive);
    m_reviewBar->setStyleSheet(QStringLiteral("QWidget { background: palette(highlight); }"
                                              "QLabel { color: palette(highlighted-text); font-weight: bold; }"));
    m_reviewBar->setVisible(false);

    // Refreshes the "N new entries" counter while reviewing.
    m_reviewTimer = new QTimer(this);
    m_reviewTimer->setInterval(1000);
    connect(m_reviewTimer, &QTimer::timeout, this, &LogViewerWidget::updateReviewLabel);

    // --- virtual table view ---
    // The model is used directly: no proxy and no view-side sorting. Records are
    // appended in chronological order and column 0 is the timestamp, so sorting
    // would only reconfirm the order, at a cost that grows with every reset. If
    // sorting is ever wanted, reorder the rows in the model.
    m_table = new QTableView;
    m_table->setModel(m_model);
    m_table->setSortingEnabled(false);
    m_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QHeaderView *header = m_table->horizontalHeader();
    header->setMinimumSectionSize(30);
    // Draggable columns, sized to their content until dragged; the message
    // takes the rest of the width.
    makeColumnsInteractive(m_table);
    header->setStyleSheet(QStringLiteral("QHeaderView::section { padding: 2px 4px; font-size: 14px; }"));
    m_table->verticalHeader()->setVisible(false);
    m_table->setShowGrid(false);
    m_table->verticalHeader()->setDefaultSectionSize(18);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setStyleSheet(
        QStringLiteral("QTableView { font-family: Consolas, 'Courier New', monospace; font-size: 12px; }"
                       "QTableView::item { padding: 0px 4px; border: none; }"));
    m_table->setItemDelegate(new LogItemDelegate(m_table));

    // Installed before the valueChanged connection, which would otherwise bind
    // to the scroll bar this one replaces.
    m_table->setVerticalScrollBar(new OverviewScrollBar(m_model, m_table));
    connect(m_table->verticalScrollBar(), &QScrollBar::valueChanged, this, &LogViewerWidget::onScroll);

    auto *layout = new QVBoxLayout;
    layout->addLayout(filtersLayout);
    layout->addWidget(m_reviewBar);
    layout->addWidget(m_table);
    setLayout(layout);
}

LogViewerWidget::~LogViewerWidget()
{
    // Before any member goes: no entry may reach a half-destroyed widget.
    m_handler->detachAll();
    if (m_fileHandler)
        m_fileHandler->detachAll();
}

void LogViewerWidget::setupFileHandler()
{
    QString logDir = QDir::homePath() + QStringLiteral("/logs");
    int keep = kLogRetentionSessionsDefault;
    {
        const auto settings = getSettings();
        const QString stored = settings->value(QStringLiteral("log_directory"), logDir).toString();
        if (!stored.isEmpty())
            logDir = stored;
        bool ok = false;
        const int storedKeep = settings->value(QStringLiteral("log_retention_sessions"), keep).toInt(&ok);
        if (ok)
            keep = storedKeep;
    }
    qCDebug(lcDuifx, "Setting up log file handler with directory: %s", qPrintable(logDir));

    const QString logPath = QDir(logDir).filePath(sessionLogFileName());
    QDir().mkpath(logDir);

    // Prune before opening the new file, so the current session is never a
    // candidate for deletion.
    const int removed = pruneSessionLogs(logDir, keep);
    if (removed)
        qCDebug(lcDuifx, "Removed %d old session log(s), keeping the last %d", removed, keep);

    m_fileHandler = std::make_unique<JsonLinesFileHandler>(logPath);
    m_model->setLogPath(logPath);
}

void LogViewerWidget::attachLogger(const QString &loggerName)
{
    addLogHandler(m_handler, loggerName);
    if (m_fileHandler)
        addLogHandler(m_fileHandler.get(), loggerName);
}

void LogViewerWidget::detachLogger(const QString &loggerName)
{
    removeLogHandler(m_handler, loggerName);
    if (m_fileHandler)
        removeLogHandler(m_fileHandler.get(), loggerName);
}

void LogViewerWidget::log(int levelNo, const QString &loggerName, const QString &message)
{
    const LogEntry entry = LogEntry::make(levelNo, loggerName, message);
    m_handler->handle(entry);
    if (m_fileHandler)
        m_fileHandler->handle(entry);
}

bool LogViewerWidget::isReviewing() const
{
    return m_model->reviewActive();
}

// --- entry handling ---------------------------------------------------------------

void LogViewerWidget::appendEntry(const LogEntry &entry)
{
    m_model->queueEntry(entry);
    if (!m_flushTimer->isActive())
        m_flushTimer->start();
}

void LogViewerWidget::flushAndScroll()
{
    if (!m_model->hasPending()) {
        m_flushTimer->stop();
        return;
    }
    // While reviewing, flushPending() only fills the ring buffer and inserts no
    // rows, so the scroll position stays exactly where the user put it.
    if (m_model->reviewActive()) {
        m_model->flushPending();
        return;
    }
    QScrollBar *bar = m_table->verticalScrollBar();
    const bool wasAtBottom = bar->value() >= bar->maximum() - 5;
    m_model->flushPending();
    if (wasAtBottom)
        m_table->scrollToBottom();
}

// --- load / review ----------------------------------------------------------------

void LogViewerWidget::loadLogs()
{
    // Already reviewing: swapping one file for another loses nothing, so go
    // straight to the file dialog.
    if (!m_model->reviewActive() && m_model->hasRecords()) {
        const QString sessionPath = m_model->logPath().isEmpty() ? QStringLiteral("(no session file)")
                                                                 : QDir::toNativeSeparators(m_model->logPath());
        QMessageBox box(this);
        box.setIcon(QMessageBox::Question);
        box.setWindowTitle(QStringLiteral("Replace displayed logs?"));
        box.setText(QStringLiteral("Replace the logs currently shown with a file?"));
        box.setInformativeText(QStringLiteral("The current logs are not lost. Every entry of this session is "
                                              "already written to disk as it happens:\n\n%1\n\n"
                                              "New entries keep being recorded while you look at the loaded "
                                              "file, and \"Back to Live\" brings this session back.")
                                   .arg(sessionPath));
        box.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
        box.setDefaultButton(QMessageBox::Cancel);
        box.button(QMessageBox::Yes)->setText(QStringLiteral("Load File"));
        if (box.exec() != QMessageBox::Yes)
            return;
    }

    QString startDir;
    if (!m_model->logPath().isEmpty())
        startDir = QFileInfo(m_model->logPath()).absolutePath();
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Load log file"), startDir,
                                                      QStringLiteral("Log Files (*.jsonl *.log);;Session Logs (*.jsonl);;"
                                                                     "Saved Logs (*.log);;All Files (*)"));
    if (path.isEmpty())
        return;

    // A very large file is loaded whole only on request.
    int limit = 0;
    const int total = LogTableModel::countLogEntries(path);
    if (total > kLogLoadWarnEntries) {
        const QLocale english(QLocale::English);
        QMessageBox box(this);
        box.setIcon(QMessageBox::Question);
        box.setWindowTitle(QStringLiteral("Large log file"));
        box.setText(QStringLiteral("This file holds about %1 entries.").arg(english.toString(total)));
        box.setInformativeText(QStringLiteral("Loading all of it takes a moment and a few hundred megabytes "
                                              "of memory.\n\nYou can also load just the most recent %1 entries.")
                                   .arg(english.toString(kLogLoadWarnEntries)));
        QPushButton *loadAll = box.addButton(QStringLiteral("Load All"), QMessageBox::AcceptRole);
        QPushButton *loadTail = box.addButton(QStringLiteral("Load Recent"), QMessageBox::AcceptRole);
        box.addButton(QMessageBox::Cancel);
        box.setDefaultButton(loadTail);
        box.exec();
        QAbstractButton *clicked = box.clickedButton();
        if (clicked == loadTail)
            limit = kLogLoadWarnEntries;
        else if (clicked != loadAll)
            return;
    }

    if (reviewFile(path, limit) == 0) {
        QMessageBox::warning(this, QStringLiteral("Nothing to show"),
                             QStringLiteral("No log entries could be read from:\n%1\n\n"
                                            "Expected a session file (.jsonl) or a log saved with \"Save Log\".")
                                 .arg(QDir::toNativeSeparators(path)));
    }
}

int LogViewerWidget::reviewFile(const QString &path, int limit)
{
    const int loaded = m_model->enterReview(path, limit);
    if (loaded > 0) {
        syncFilterState();
        m_model->rebuildFilter();
        m_table->scrollToBottom();
    }
    updateReviewState();
    return loaded;
}

void LogViewerWidget::exitReview()
{
    m_model->exitReview();
    syncFilterState();
    m_model->rebuildFilter();
    m_table->scrollToBottom();
    updateReviewState();
}

void LogViewerWidget::updateReviewState()
{
    const bool active = m_model->reviewActive();
    m_reviewBar->setVisible(active);
    // Marking entries of a foreign file as seen would move the unseen boundary
    // of the live session, so these rest while reviewing.
    m_btnMarkSeen->setEnabled(!active);
    m_checkboxNewOnly->setEnabled(!active);
    if (active) {
        updateReviewLabel();
        m_reviewTimer->start();
    } else {
        m_reviewTimer->stop();
    }
}

void LogViewerWidget::updateReviewLabel()
{
    if (!m_model->reviewActive())
        return;
    const QString name = QFileInfo(m_model->reviewPath()).fileName();
    const int missed = m_model->liveMissed();
    QString text = QStringLiteral("Reviewing file: %1").arg(name);
    if (missed)
        text += QStringLiteral(" — %1 new live entr%2 waiting")
                    .arg(missed)
                    .arg(missed == 1 ? QStringLiteral("y") : QStringLiteral("ies"));
    m_reviewLabel->setText(text);
}

// --- filters ----------------------------------------------------------------------

void LogViewerWidget::syncFilterState()
{
    LogFilter filter;
    filter.showDebug = m_checkboxDebug->isChecked();
    filter.showInfo = m_checkboxInfo->isChecked();
    filter.showWarning = m_checkboxWarning->isChecked();
    filter.showError = m_checkboxError->isChecked();
    filter.showUnseenOnly = m_checkboxNewOnly->isChecked();
    filter.searchText = m_searchInput->text().trimmed().toLower();
    m_model->setFilter(filter);
}

void LogViewerWidget::onFilterChanged()
{
    syncFilterState();
    m_model->rebuildFilter();
}

void LogViewerWidget::markSeen()
{
    if (m_model->reviewActive())
        return;
    m_model->setSeenCount(m_model->recordCount());
    if (m_checkboxNewOnly->isChecked())
        m_model->rebuildFilter();
}

// --- scroll-back ------------------------------------------------------------------

void LogViewerWidget::onScroll(int value)
{
    if (value != 0 || m_loadingOlder || m_model->fileExhausted())
        return;
    m_loadingOlder = true;
    QScrollBar *bar = m_table->verticalScrollBar();
    const int oldMax = bar->maximum();
    if (m_model->loadOlderEntries(500) > 0)
        bar->setValue(bar->value() + (bar->maximum() - oldMax));
    m_loadingOlder = false;
}

// --- save -------------------------------------------------------------------------

void LogViewerWidget::saveLogs()
{
    const QString defaultName = QDateTime::currentDateTime().toString(QStringLiteral("'logs_'yyyyMMdd_HHmmss'.log'"));
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Save logs"), defaultName,
                                                      QStringLiteral("Log Files (*.log);;All Files (*)"));
    if (!path.isEmpty())
        saveLogsTo(path);
}

bool LogViewerWidget::saveLogsTo(const QString &path)
{
    // Pending entries into the model and everything onto the disk first.
    m_model->flushPending();
    if (m_fileHandler)
        m_fileHandler->flush();

    // What is on screen: the reviewed file while one is loaded, the running
    // session otherwise.
    const bool reviewing = m_model->reviewActive();
    const QString logPath = m_model->activeLogPath();
    QStringList lines;
    double lastFileTs = 0.0;

    // 1) Every entry of the backing file (the authoritative copy).
    if (!logPath.isEmpty() && QFileInfo(logPath).isFile()) {
        QFile file(logPath);
        if (file.open(QIODevice::ReadOnly)) {
            while (!file.atEnd()) {
                if (std::optional<LogEntry> entry = decodeLine(file.readLine())) {
                    lastFileTs = entry->ts;
                    lines.append(formatLogEntry(*entry));
                }
            }
        }
    }

    // 2) Live entries not yet in the file. Not while reviewing: those belong to
    //    the session, not to the file being saved.
    if (!reviewing) {
        for (const LogEntry &entry : m_model->liveRecords()) {
            if (entry.ts > lastFileTs)
                lines.append(formatLogEntry(entry));
        }
    }

    QFile out(path);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)
        || out.write(lines.join(QLatin1Char('\n')).toUtf8()) < 0) {
        qCCritical(lcDuifx, "Failed to save logs: %s", qPrintable(out.errorString()));
        return false;
    }
    return true;
}

} // namespace duifx

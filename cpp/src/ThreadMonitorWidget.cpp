#include "duifx/ThreadMonitorWidget.h"

#include "duifx/Tables.h"
#include "duifx/Utils.h"

#include <QAbstractTableModel>
#include <QCheckBox>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QTableView>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>

#include <cstddef>
#include <iterator>
#include <utility>
#include <vector>

#if defined(Q_OS_WIN)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#  include <tlhelp32.h>
#elif defined(Q_OS_LINUX)
#  include <QDir>
#  include <QFile>
#  include <sys/syscall.h>
#  include <unistd.h>
#endif

namespace duifx {

namespace {

const char *const kThreadColumns[] = {"Name", "ID", "Type", "Status", "Daemon", "Priority", "Target"};
constexpr int kThreadColumnCount = static_cast<int>(std::size(kThreadColumns));

/// A thread as the operating system reports it.
struct OsThread
{
    quint64 tid = 0;
    QString name;
    QString status;
    QString priority;
};

QString qtPriorityName(QThread::Priority priority)
{
    switch (priority) {
    case QThread::IdlePriority:
        return QStringLiteral("Idle");
    case QThread::LowestPriority:
        return QStringLiteral("Lowest");
    case QThread::LowPriority:
        return QStringLiteral("Low");
    case QThread::NormalPriority:
        return QStringLiteral("Normal");
    case QThread::HighPriority:
        return QStringLiteral("High");
    case QThread::HighestPriority:
        return QStringLiteral("Highest");
    case QThread::TimeCriticalPriority:
        return QStringLiteral("TimeCritical");
    case QThread::InheritPriority:
        return QStringLiteral("Inherit");
    }
    return QStringLiteral("Unknown");
}

#if defined(Q_OS_WIN)

using GetThreadDescriptionFn = HRESULT(WINAPI *)(HANDLE, PWSTR *);

/// GetThreadDescription exists from Windows 10 1607 on, so it is looked up at
/// run time instead of being linked.
GetThreadDescriptionFn getThreadDescriptionFn()
{
    static const GetThreadDescriptionFn fn = [] {
        for (const wchar_t *module : {L"kernel32.dll", L"kernelbase.dll"}) {
            if (HMODULE handle = GetModuleHandleW(module)) {
                if (FARPROC proc = GetProcAddress(handle, "GetThreadDescription"))
                    return reinterpret_cast<GetThreadDescriptionFn>(reinterpret_cast<void (*)()>(proc));
            }
        }
        return GetThreadDescriptionFn{};
    }();
    return fn;
}

QString windowsPriorityName(int priority)
{
    switch (priority) {
    case THREAD_PRIORITY_IDLE:
        return QStringLiteral("Idle");
    case THREAD_PRIORITY_LOWEST:
        return QStringLiteral("Lowest");
    case THREAD_PRIORITY_BELOW_NORMAL:
        return QStringLiteral("Low");
    case THREAD_PRIORITY_NORMAL:
        return QStringLiteral("Normal");
    case THREAD_PRIORITY_ABOVE_NORMAL:
        return QStringLiteral("High");
    case THREAD_PRIORITY_HIGHEST:
        return QStringLiteral("Highest");
    case THREAD_PRIORITY_TIME_CRITICAL:
        return QStringLiteral("TimeCritical");
    case THREAD_PRIORITY_ERROR_RETURN:
        return QStringLiteral("-");
    default:
        return QString::number(priority);
    }
}

std::vector<OsThread> osThreads()
{
    std::vector<OsThread> threads;
    const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return threads;
    const DWORD pid = GetCurrentProcessId();
    THREADENTRY32 entry{};
    entry.dwSize = sizeof(entry);
    // The snapshot holds the threads of every process; ours are filtered here.
    for (BOOL ok = Thread32First(snapshot, &entry); ok; ok = Thread32Next(snapshot, &entry)) {
        if (entry.dwSize >= offsetof(THREADENTRY32, th32OwnerProcessID) + sizeof(entry.th32OwnerProcessID)
            && entry.th32OwnerProcessID == pid) {
            OsThread thread;
            thread.tid = entry.th32ThreadID;
            thread.status = QStringLiteral("Running");
            thread.priority = QStringLiteral("-");
            if (HANDLE handle = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, entry.th32ThreadID)) {
                if (const GetThreadDescriptionFn describe = getThreadDescriptionFn()) {
                    PWSTR description = nullptr;
                    if (SUCCEEDED(describe(handle, &description)) && description) {
                        thread.name = QString::fromWCharArray(description);
                        LocalFree(description);
                    }
                }
                thread.priority = windowsPriorityName(GetThreadPriority(handle));
                DWORD exitCode = 0;
                if (GetExitCodeThread(handle, &exitCode) && exitCode != STILL_ACTIVE)
                    thread.status = QStringLiteral("Finished");
                CloseHandle(handle);
            }
            threads.push_back(thread);
        }
        entry.dwSize = sizeof(entry);
    }
    CloseHandle(snapshot);
    return threads;
}

#elif defined(Q_OS_LINUX)

QString linuxStateName(QChar state)
{
    switch (state.toLatin1()) {
    case 'R':
        return QStringLiteral("Running");
    case 'S':
        return QStringLiteral("Sleeping");
    case 'D':
        return QStringLiteral("Disk wait");
    case 'Z':
        return QStringLiteral("Zombie");
    case 'T':
        return QStringLiteral("Stopped");
    case 't':
        return QStringLiteral("Tracing stop");
    case 'X':
    case 'x':
        return QStringLiteral("Dead");
    case 'I':
        return QStringLiteral("Idle");
    case 'P':
        return QStringLiteral("Parked");
    case 'W':
        return QStringLiteral("Waking");
    case 'K':
        return QStringLiteral("Wakekill");
    default:
        return QString(state);
    }
}

QByteArray readProcFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return file.readAll();
}

std::vector<OsThread> osThreads()
{
    std::vector<OsThread> threads;
    const QDir taskDir(QStringLiteral("/proc/self/task"));
    const QStringList tids = taskDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &tidText : tids) {
        bool ok = false;
        const quint64 tid = tidText.toULongLong(&ok);
        if (!ok)
            continue;
        OsThread thread;
        thread.tid = tid;
        thread.name = QString::fromUtf8(readProcFile(taskDir.filePath(tidText + QStringLiteral("/comm")))).trimmed();
        thread.status = QStringLiteral("Running");
        thread.priority = QStringLiteral("-");
        // "tid (comm) S ppid ...": the name may hold spaces and parentheses, so
        // the fields are counted from the last ')'. State is field 3, nice field 19.
        const QByteArray stat = readProcFile(taskDir.filePath(tidText + QStringLiteral("/stat")));
        const qsizetype close = stat.lastIndexOf(')');
        if (close >= 0) {
            const QList<QByteArray> fields = stat.mid(close + 2).simplified().split(' ');
            if (!fields.isEmpty() && !fields.first().isEmpty())
                thread.status = linuxStateName(QLatin1Char(fields.first().at(0)));
            if (fields.size() > 16) {
                const int nice = fields.at(16).toInt(&ok);
                if (ok)
                    thread.priority = nice == 0 ? QStringLiteral("Normal") : QStringLiteral("nice %1").arg(nice);
            }
        }
        threads.push_back(thread);
    }
    return threads;
}

#else

std::vector<OsThread> osThreads()
{
    // No portable way to list the threads: only the calling one is known.
    return {OsThread{currentNativeThreadId(), {}, QStringLiteral("Running"), QStringLiteral("-")}};
}

#endif

/// Does the OS thread name belong to thread? Qt names its threads after
/// objectName() or, without one, the class name; Linux truncates to 15 bytes.
bool carriesName(const QString &osName, const QThread *thread)
{
    if (osName.isEmpty())
        return false;
    const QString qtName = thread->objectName().isEmpty() ? QString::fromUtf8(thread->metaObject()->className())
                                                          : thread->objectName();
#if defined(Q_OS_LINUX)
    return qtName.toUtf8().left(15) == osName.toUtf8();
#else
    return qtName == osName;
#endif
}

} // namespace

quint64 currentNativeThreadId()
{
#if defined(Q_OS_WIN)
    return GetCurrentThreadId();
#elif defined(Q_OS_LINUX)
    return static_cast<quint64>(syscall(SYS_gettid));
#else
    return reinterpret_cast<quintptr>(QThread::currentThreadId());
#endif
}

QList<ThreadEntry> snapshotThreads()
{
    QList<ThreadEntry> entries;
    QCoreApplication *app = QCoreApplication::instance();
    const quint64 self = currentNativeThreadId();
    const bool onGuiThread = app && QThread::currentThread() == app->thread();

    QList<QThread *> qtThreads = app ? app->findChildren<QThread *>() : QList<QThread *>();
    for (const OsThread &os : osThreads()) {
        ThreadEntry entry{os.name.isEmpty() ? QStringLiteral("Thread-%1").arg(os.tid) : os.name,
                          QString::number(os.tid),
                          QStringLiteral("Native"),
                          os.status,
                          QStringLiteral("-"),
                          os.priority,
                          QStringLiteral("-")};
        if (onGuiThread && os.tid == self) {
            entry.type = QStringLiteral("Qt");
            entry.target = QString::fromUtf8(app->metaObject()->className());
            if (os.name.isEmpty())
                entry.name = QStringLiteral("MainThread");
        } else {
            for (qsizetype i = 0; i < qtThreads.size(); ++i) {
                QThread *thread = qtThreads.at(i);
                if (thread->isRunning() && carriesName(os.name, thread)) {
                    entry.type = QStringLiteral("Qt");
                    entry.target = QString::fromUtf8(thread->metaObject()->className());
                    qtThreads.removeAt(i);
                    break;
                }
            }
        }
        entries.append(entry);
    }

    // QThreads without an OS thread of their own (not started, finished, or
    // not identifiable by name).
    for (QThread *thread : std::as_const(qtThreads)) {
        entries.append(ThreadEntry{
            thread->objectName().isEmpty()
                ? QStringLiteral("QThread-0x%1").arg(reinterpret_cast<quintptr>(thread), 0, 16)
                : thread->objectName(),
            QStringLiteral("-"), QStringLiteral("Qt"),
            thread->isRunning() ? QStringLiteral("Running") : QStringLiteral("Finished"), QStringLiteral("-"),
            qtPriorityName(thread->priority()), QString::fromUtf8(thread->metaObject()->className())});
    }
    return entries;
}

// --- model ------------------------------------------------------------------------

/// Snapshot of the threads, filtered by the search text (_ThreadTableModel).
class ThreadMonitorWidget::Model : public QAbstractTableModel
{
public:
    using QAbstractTableModel::QAbstractTableModel;

    void refresh()
    {
        m_entries = snapshotThreads();
        rebuildFilter();
    }

    void setSearchText(const QString &text)
    {
        m_searchText = text;
        rebuildFilter();
    }

    int rowCount(const QModelIndex &parent = {}) const override
    {
        return parent.isValid() ? 0 : static_cast<int>(m_filtered.size());
    }

    int columnCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : kThreadColumnCount; }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    {
        if (role == Qt::DisplayRole && orientation == Qt::Horizontal && section >= 0 && section < kThreadColumnCount)
            return QString::fromLatin1(kThreadColumns[section]);
        return {};
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || role != Qt::DisplayRole || index.row() >= rowCount())
            return {};
        const ThreadEntry &entry = m_entries.at(m_filtered.at(static_cast<std::size_t>(index.row())));
        switch (index.column()) {
        case 0:
            return entry.name;
        case 1:
            return entry.ident;
        case 2:
            return entry.type;
        case 3:
            return entry.status;
        case 4:
            return entry.daemon;
        case 5:
            return entry.priority;
        case 6:
            return entry.target;
        default:
            return {};
        }
    }

private:
    void rebuildFilter()
    {
        beginResetModel();
        m_filtered.clear();
        for (qsizetype i = 0; i < m_entries.size(); ++i) {
            const ThreadEntry &e = m_entries.at(i);
            if (m_searchText.isEmpty() || e.name.contains(m_searchText, Qt::CaseInsensitive)
                || e.type.contains(m_searchText, Qt::CaseInsensitive)
                || e.status.contains(m_searchText, Qt::CaseInsensitive)
                || e.target.contains(m_searchText, Qt::CaseInsensitive))
                m_filtered.push_back(i);
        }
        endResetModel();
    }

    QList<ThreadEntry> m_entries;
    std::vector<qsizetype> m_filtered;
    QString m_searchText;
};

// --- widget -----------------------------------------------------------------------

ThreadMonitorWidget::ThreadMonitorWidget(QWidget *parent)
    : QWidget(parent)
{
    initResources();

    m_model = new Model(this);

    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(kRefreshIntervalMs);
    connect(m_refreshTimer, &QTimer::timeout, this, &ThreadMonitorWidget::refresh);

    m_btnRefresh = new QPushButton(QStringLiteral("Refresh"));
    m_btnRefresh->setToolTip(QStringLiteral("Refresh thread list now"));
    connect(m_btnRefresh, &QPushButton::clicked, this, &ThreadMonitorWidget::refresh);

    m_cbAuto = new QCheckBox(QStringLiteral("Auto"));
    m_cbAuto->setToolTip(QStringLiteral("Auto-refresh every second"));
    m_cbAuto->setChecked(true);
    connect(m_cbAuto, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked)
            m_refreshTimer->start();
        else
            m_refreshTimer->stop();
    });

    m_searchInput = new QLineEdit;
    m_searchInput->setPlaceholderText(QStringLiteral("Filter threads..."));
    m_searchInput->setClearButtonEnabled(true);
    connect(m_searchInput, &QLineEdit::textChanged, this,
            [this](const QString &text) { m_model->setSearchText(text.trimmed().toLower()); });

    auto *toolbar = new QHBoxLayout;
    toolbar->addWidget(m_btnRefresh);
    toolbar->addWidget(m_cbAuto);
    toolbar->addWidget(m_searchInput, 1);

    m_table = new QTableView;
    m_table->setModel(m_model);
    m_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_table->horizontalHeader()->setMinimumSectionSize(40);
    // Draggable columns, sized to their content until dragged; Target (the
    // last) takes the rest of the width.
    makeColumnsInteractive(m_table);
    m_table->verticalHeader()->setVisible(false);
    m_table->setShowGrid(false);
    m_table->verticalHeader()->setDefaultSectionSize(22);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setStyleSheet(
        QStringLiteral("QTableView { font-family: Consolas, 'Courier New', monospace; font-size: 12px; }"
                       "QTableView::item { padding: 0px 4px; border: none; }"));

    auto *layout = new QVBoxLayout;
    layout->addLayout(toolbar);
    layout->addWidget(m_table);
    setLayout(layout);

    refresh();
    m_refreshTimer->start();
}

void ThreadMonitorWidget::refresh()
{
    m_model->refresh();
}

void ThreadMonitorWidget::setAutoRefresh(bool enabled)
{
    m_cbAuto->setChecked(enabled);
}

bool ThreadMonitorWidget::autoRefresh() const
{
    return m_cbAuto->isChecked();
}

QAbstractItemModel *ThreadMonitorWidget::model() const
{
    return m_model;
}

} // namespace duifx

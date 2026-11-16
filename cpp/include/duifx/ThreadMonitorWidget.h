#pragma once

// Debug widget listing the threads of the process. Port of ThreadMonitorWidget
// (and its _ThreadTableModel) in duifx/gui/widgets.py.
//
// Same columns, filter box, "Refresh" button and once-a-second "Auto" refresh
// as in Python. Python lists the interpreter's threads (threading.enumerate()
// plus the QThreads that are children of the application); C++ has no such
// registry, so the rows come from the operating system:
//
//  - Windows: Toolhelp32 (CreateToolhelp32Snapshot / Thread32First / Thread32Next),
//    names from GetThreadDescription (resolved at run time; empty before
//    Windows 10 1607), priority from GetThreadPriority.
//  - Linux: /proc/self/task/<tid>/comm for the name and .../stat for state and
//    nice value.
//
// Deviations from Python:
//  - Type is "Qt" for the GUI thread and for threads that carry the name of a
//    running QThread child of the application (Qt names its threads after
//    objectName() or the class name; Linux keeps 15 characters), else "Native".
//    QThread children without a running OS thread are listed with ID "-".
//  - ID is the OS thread id; the name falls back to "Thread-<id>".
//  - Status: Linux reports the scheduler state ("Running", "Sleeping",
//    "Disk wait", "Stopped", "Zombie", ...); Windows only "Running"/"Finished".
//  - Daemon is always "-" (no such notion outside Python).
//  - Priority: Windows maps GetThreadPriority() to the QThread::Priority names
//    ("Normal", "High", ...); Linux shows "Normal" for nice 0, else "nice <n>".
//  - Target is the class name of the QThread (for the GUI thread: the
//    application's class), "-" for other threads.
//  - Other operating systems list only the QThread children and the GUI thread.

#include <QList>
#include <QString>
#include <QWidget>

class QAbstractItemModel;
class QCheckBox;
class QLineEdit;
class QPushButton;
class QTableView;
class QTimer;

namespace duifx {

/// One row of the thread monitor (_ThreadEntry).
struct ThreadEntry
{
    QString name;
    QString ident;
    QString type;
    QString status;
    QString daemon;
    QString priority;
    QString target;
};

/// OS id of the calling thread (GetCurrentThreadId / gettid), as in the ID column.
quint64 currentNativeThreadId();

/// The rows ThreadMonitorWidget shows: the threads of this process right now.
QList<ThreadEntry> snapshotThreads();

class ThreadMonitorWidget : public QWidget
{
    Q_OBJECT

public:
    static constexpr int kRefreshIntervalMs = 1000;

    explicit ThreadMonitorWidget(QWidget *parent = nullptr);

    /// Re-scans the threads now.
    void refresh();
    /// The "Auto" checkbox: refresh every kRefreshIntervalMs.
    void setAutoRefresh(bool enabled);
    bool autoRefresh() const;

    QAbstractItemModel *model() const;
    QTableView *tableView() const { return m_table; }

private:
    class Model;
    Model *m_model = nullptr;
    QTimer *m_refreshTimer = nullptr;
    QPushButton *m_btnRefresh = nullptr;
    QCheckBox *m_cbAuto = nullptr;
    QLineEdit *m_searchInput = nullptr;
    QTableView *m_table = nullptr;
};

} // namespace duifx

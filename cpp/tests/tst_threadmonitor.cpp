// The thread monitor: OS thread snapshot, model and filter.

#include "TestSupport.h"

#include <duifx/ThreadMonitorWidget.h>

#include <QAbstractItemModel>
#include <QApplication>
#include <QCheckBox>
#include <QLineEdit>
#include <QSemaphore>
#include <QTableView>
#include <QThread>
#include <QtTest>

using namespace duifx;

namespace {

/// A QThread child of the application, blocked until released.
class ParkedThread : public QThread
{
public:
    explicit ParkedThread(const QString &name)
        : QThread(QCoreApplication::instance())
    {
        setObjectName(name);
    }
    QSemaphore started;
    QSemaphore release;
    quint64 tid = 0;

protected:
    void run() override
    {
        tid = currentNativeThreadId();
        started.release();
        release.acquire();
    }
};

const ThreadEntry *findById(const QList<ThreadEntry> &entries, quint64 tid)
{
    for (const ThreadEntry &e : entries) {
        if (e.ident == QString::number(tid))
            return &e;
    }
    return nullptr;
}

} // namespace

class TstThreadMonitor : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() { duifx_test::isolateSettings(); }

    void snapshotContainsCurrentThread()
    {
        const QList<ThreadEntry> entries = snapshotThreads();
        QVERIFY(!entries.isEmpty());
        const ThreadEntry *self = findById(entries, currentNativeThreadId());
        QVERIFY(self);
        QCOMPARE(self->type, QStringLiteral("Qt"));
        QCOMPARE(self->target, QString::fromUtf8(qApp->metaObject()->className()));
        QCOMPARE(self->daemon, QStringLiteral("-"));
        QVERIFY(!self->name.isEmpty());
        QVERIFY(!self->status.isEmpty());
    }

    void snapshotIdentifiesQThreads()
    {
        ParkedThread worker(QStringLiteral("duifxWorker"));
        ParkedThread idle(QStringLiteral("duifxIdle")); // never started
        worker.start();
        QVERIFY(worker.started.tryAcquire(1, 5000));

        const QList<ThreadEntry> entries = snapshotThreads();
        const ThreadEntry *row = findById(entries, worker.tid);
        QVERIFY(row);
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
        // Qt names the OS thread after objectName().
        QCOMPARE(row->name, QStringLiteral("duifxWorker"));
        QCOMPARE(row->type, QStringLiteral("Qt"));
        QCOMPARE(row->target, QStringLiteral("QThread"));
#endif
        // The thread that never ran is listed without an id.
        bool idleListed = false;
        for (const ThreadEntry &e : entries) {
            if (e.name == QStringLiteral("duifxIdle")) {
                idleListed = true;
                QCOMPARE(e.ident, QStringLiteral("-"));
                QCOMPARE(e.status, QStringLiteral("Finished"));
            }
        }
        QVERIFY(idleListed);

        worker.release.release();
        QVERIFY(worker.wait(5000));
    }

    void widgetListsAndFilters()
    {
        ThreadMonitorWidget monitor;
        QVERIFY(monitor.autoRefresh());
        QAbstractItemModel *model = monitor.model();
        QCOMPARE(model->columnCount(), 7);
        QCOMPARE(model->headerData(0, Qt::Horizontal).toString(), QStringLiteral("Name"));
        QCOMPARE(model->headerData(6, Qt::Horizontal).toString(), QStringLiteral("Target"));
        QVERIFY(model->rowCount() >= 1);

        bool found = false;
        const QString self = QString::number(currentNativeThreadId());
        for (int row = 0; row < model->rowCount(); ++row)
            found = found || model->index(row, 1).data().toString() == self;
        QVERIFY(found);

        // Filter on name, type, status and target, case-insensitive.
        auto *search = monitor.findChild<QLineEdit *>();
        search->setText(QStringLiteral("  QT "));
        QVERIFY(model->rowCount() >= 1);
        for (int row = 0; row < model->rowCount(); ++row) {
            QStringList cells;
            for (int column : {0, 2, 3, 6})
                cells << model->index(row, column).data().toString().toLower();
            QVERIFY(cells.join(QLatin1Char('|')).contains(QStringLiteral("qt")));
        }
        search->setText(QStringLiteral("no-such-thread-xyz"));
        QCOMPARE(model->rowCount(), 0);
        search->clear();
        QVERIFY(model->rowCount() >= 1);

        // Auto refresh on and off.
        monitor.setAutoRefresh(false);
        QVERIFY(!monitor.findChild<QCheckBox *>()->isChecked());
        QVERIFY(!monitor.autoRefresh());
        monitor.setAutoRefresh(true);
        QSignalSpy resets(model, &QAbstractItemModel::modelReset);
        QTRY_VERIFY_WITH_TIMEOUT(resets.size() >= 1, 3000);
    }
};

QTEST_MAIN(TstThreadMonitor)
#include "tst_threadmonitor.moc"

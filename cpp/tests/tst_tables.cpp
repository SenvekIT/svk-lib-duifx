// ColumnSizer: interactive columns sized to content, one column fills.

#include "TestSupport.h"

#include <duifx/Tables.h>

#include <QHeaderView>
#include <QStandardItemModel>
#include <QTableView>
#include <QTreeWidget>
#include <QtTest>

using namespace duifx;

class TstTables : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() { duifx_test::isolateSettings(); }

    void columnsAreInteractiveAndLastFills()
    {
        QTableView view;
        QStandardItemModel model(2, 3);
        model.setItem(0, 0, new QStandardItem(QStringLiteral("a")));
        model.setItem(0, 1, new QStandardItem(QString(60, QLatin1Char('x'))));
        view.setModel(&model);
        view.resize(900, 300);
        view.show();
        ColumnSizer *sizer = makeColumnsInteractive(&view);
        QCOMPARE(sizer->parent(), &view);
        QHeaderView *header = view.horizontalHeader();
        for (int c = 0; c < 3; ++c)
            QCOMPARE(header->sectionResizeMode(c), QHeaderView::Interactive);
        QCOMPARE(*sizer->fillColumn(), 2);
        // The long column is wider than the short one, and the last one takes the rest.
        QVERIFY(header->sectionSize(1) > header->sectionSize(0));
        QCOMPARE(header->sectionSize(0) + header->sectionSize(1) + header->sectionSize(2), view.viewport()->width());
    }

    void maxWidthAndAutoFit()
    {
        QTableView view;
        QStandardItemModel model(1, 2);
        view.setModel(&model);
        view.resize(400, 200);
        view.show();
        ColumnSizer::Options options;
        options.fill = std::nullopt;
        options.maxWidth = 120;
        options.intervalMs = 0;
        ColumnSizer *sizer = makeColumnsInteractive(&view, options);
        QVERIFY(!sizer->fillColumn());
        model.setItem(0, 0, new QStandardItem(QString(200, QLatin1Char('x'))));
        // Fitted again after the change, capped at maxWidth.
        QTRY_COMPARE(view.horizontalHeader()->sectionSize(0), 120);
    }

    void treeFillsTheGivenColumn()
    {
        QTreeWidget tree;
        tree.setHeaderLabels({QStringLiteral("Setting"), QStringLiteral("Value"), QString()});
        tree.resize(600, 300);
        tree.show();
        ColumnSizer::Options options;
        options.fill = 1;
        options.autoFit = false;
        ColumnSizer *sizer = makeColumnsInteractive(&tree, options);
        QCOMPARE(*sizer->fillColumn(), 1);
        QHeaderView *header = tree.header();
        QCOMPARE(header->sectionSize(0) + header->sectionSize(1) + header->sectionSize(2), tree.viewport()->width());
        sizer->reset();
        QVERIFY(sizer->userSized().isEmpty());
    }
};

QTEST_MAIN(TstTables)
#include "tst_tables.moc"

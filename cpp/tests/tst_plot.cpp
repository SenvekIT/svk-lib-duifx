// Plot widgets: style, helpers, ticks, time series, scatter, linked axes,
// mouse interaction, rendering, compass dial and threshold bar.

#include "TestSupport.h"

#include <duifx/Colors.h>
#include <duifx/CompassDial.h>
#include <duifx/Plot.h>
#include <duifx/ScatterPlot.h>
#include <duifx/ThresholdBar.h>
#include <duifx/TimeSeriesPlot.h>

#include <QAction>
#include <QApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QMenu>
#include <QMouseEvent>
#include <QTemporaryDir>
#include <QWheelEvent>
#include <QtTest>

#include <cmath>
#include <memory>
#include <numbers>

using namespace duifx;

namespace duifx {

// Readable QCOMPARE failures.
char *toString(const PlotRange &range)
{
    return QTest::toString(QStringLiteral("[%1, %2]").arg(range.min, 0, 'g', 17).arg(range.max, 0, 'g', 17));
}

} // namespace duifx

namespace {

bool containsColor(const QImage &image, const QColor &wanted, int tolerance = 0)
{
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor c = image.pixelColor(x, y);
            if (std::abs(c.red() - wanted.red()) <= tolerance && std::abs(c.green() - wanted.green()) <= tolerance
                && std::abs(c.blue() - wanted.blue()) <= tolerance)
                return true;
        }
    }
    return false;
}

QImage render(QWidget &widget)
{
    return widget.grab().toImage().convertToFormat(QImage::Format_RGB32);
}

QList<double> range(int count, double step = 1.0, double start = 0.0)
{
    QList<double> values;
    for (int i = 0; i < count; ++i)
        values.append(start + i * step);
    return values;
}

void sendMouse(QWidget *widget, QEvent::Type type, QPointF pos, Qt::MouseButton button, Qt::MouseButtons buttons)
{
    QMouseEvent event(type, pos, widget->mapToGlobal(pos), button, buttons, Qt::NoModifier);
    QApplication::sendEvent(widget, &event);
}

void sendWheel(QWidget *widget, QPointF pos, int delta)
{
    QWheelEvent event(pos, widget->mapToGlobal(pos), QPoint(), QPoint(0, delta), Qt::NoButton, Qt::NoModifier,
                      Qt::NoScrollPhase, false);
    QApplication::sendEvent(widget, &event);
}

QAction *findAction(QMenu *menu, const QString &text)
{
    for (QAction *action : menu->actions()) {
        if (action->text() == text)
            return action;
    }
    return nullptr;
}

} // namespace

class TstPlot : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        duifx_test::isolateSettings();
        setActiveTheme(Theme::Light);
    }

    // --- style and helpers ---

    void styleDefaults()
    {
        const PlotStyle style;
        QCOMPARE(style.background, color(Token::PlotBackground));
        QCOMPARE(style.axis, color(Token::PlotAxis));
        QCOMPARE(style.reference, color(Token::PlotSeries4));
        QCOMPARE(style.ok, color(Token::StateOk));
        QCOMPARE(style.series.size(), 6);
        QCOMPARE(style.seriesColor(0), color(Token::PlotSeries1));
        QCOMPARE(style.seriesColor(6), style.seriesColor(0));
        QCOMPARE(style.seriesColor(-1), style.seriesColor(5));
        // The grid colour equals the plot_grid token: axis colour at 0.3 -> 77.
        QCOMPARE(qRound(style.gridAlpha * 255), color(Token::PlotGrid).alpha());
        QCOMPARE(style.axis.rgb(), color(Token::PlotGrid).rgb());
        QVERIFY(style == defaultPlotStyle());
    }

    void defaultStyleAppliesToNewWidgets()
    {
        TimeSeriesPlot before;
        PlotStyle custom = defaultPlotStyle();
        custom.lineWidth = 3.0;
        setDefaultPlotStyle(custom);
        TimeSeriesPlot after;
        ThresholdBar bar;
        CompassDial dial;
        QCOMPARE(after.plotStyle().lineWidth, 3.0);
        QCOMPARE(bar.plotStyle().lineWidth, 3.0);
        QCOMPARE(dial.plotStyle().lineWidth, 3.0);
        QCOMPARE(before.plotStyle().lineWidth, 1.0);
        before.setPlotStyle(custom);
        QCOMPARE(before.plotStyle().lineWidth, 3.0);
        setDefaultPlotStyle(PlotStyle{});
    }

    void axisLabels()
    {
        QCOMPARE(axisLabel(QStringLiteral("time"), QStringLiteral("s")), QStringLiteral("time [s]"));
        QCOMPARE(axisLabel(QStringLiteral("time")), QStringLiteral("time"));
        QCOMPARE(axisLabel(QString(), QStringLiteral("µT")), QStringLiteral("[µT]"));
        QCOMPARE(axisLabel(QString()), QString());
    }

    void compassPoints()
    {
        QCOMPARE(compassPoint(0), QStringLiteral("N"));
        QCOMPARE(compassPoint(22.4), QStringLiteral("N"));
        QCOMPARE(compassPoint(22.5), QStringLiteral("NE"));
        QCOMPARE(compassPoint(359), QStringLiteral("N"));
        QCOMPARE(compassPoint(-90), QStringLiteral("W"));
        QCOMPARE(compassPoint(720 + 180), QStringLiteral("S"));
        QCOMPARE(compassPoint(22.5, 16), QStringLiteral("NNE"));
        QCOMPARE(compassPoint(292.5, 16), QStringLiteral("WNW"));
        QCOMPARE(compassPoint(100, 4), QStringLiteral("E"));
        QCOMPARE(compassPoint(135, 4), QStringLiteral("S"));
        QCOMPARE(compassPoint(315, 4), QStringLiteral("N"));
    }

    void wrapBreaking()
    {
        const QList<QPointF> wrapped{{0, 350}, {1, 355}, {2, 2}, {3, 5}, {4, 340}};
        const QList<QPointF> broken = breakWraps(wrapped, 360.0);
        QCOMPARE(broken.size(), 7);
        QVERIFY(std::isnan(broken.at(2).x()) && std::isnan(broken.at(2).y()));
        QCOMPARE(broken.at(3), QPointF(2, 2));
        QVERIFY(std::isnan(broken.at(5).y()));
        const QList<QPointF> smooth{{0, 10}, {1, 20}, {2, 30}};
        QCOMPARE(breakWraps(smooth, 360.0), smooth);
        QCOMPARE(breakWraps({{0, 1}}, 360.0).size(), 1);
    }

    void tickLevels()
    {
        // 0..10 over 500 px: majors every 10, then 2, then 1 (pyqtgraph's
        // AxisItem picks the same), without repeating higher levels.
        const QList<PlotTickLevel> levels = plotTickLevels(0.0, 10.0, 500.0);
        QCOMPARE(levels.size(), 3);
        QCOMPARE(levels.at(0).spacing, 10.0);
        QCOMPARE(levels.at(0).values, (QList<double>{0.0, 10.0}));
        QCOMPARE(levels.at(1).spacing, 2.0);
        QCOMPARE(levels.at(1).values, (QList<double>{2.0, 4.0, 6.0, 8.0}));
        QCOMPARE(levels.at(2).spacing, 1.0);
        QCOMPARE(levels.at(2).values, (QList<double>{1.0, 3.0, 5.0, 7.0, 9.0}));

        // Few pixels: 200 / 100 / 20, each level only inside the range.
        const QList<PlotTickLevel> coarse = plotTickLevels(0.0, 100.0, 60.0);
        QCOMPARE(coarse.size(), 3);
        QCOMPARE(coarse.at(0).spacing, 200.0);
        QCOMPARE(coarse.at(0).values, QList<double>{0.0});
        QCOMPARE(coarse.at(1).values, QList<double>{100.0});
        QCOMPARE(coarse.at(2).values, (QList<double>{20.0, 40.0, 60.0, 80.0}));

        // Reversed bounds, negative ranges, nothing for empty ones.
        const QList<PlotTickLevel> negative = plotTickLevels(-0.35, -0.05, 300.0);
        QVERIFY(!negative.isEmpty());
        for (const PlotTickLevel &level : negative) {
            for (double v : level.values)
                QVERIFY(v >= -0.35 - 1e-12 && v <= -0.05 + 1e-12);
        }
        QCOMPARE(plotTickLevels(10.0, 0.0, 500.0).at(0).values, levels.at(0).values);
        QVERIFY(plotTickLevels(1.0, 1.0, 500.0).isEmpty());
        QVERIFY(plotTickLevels(0.0, 1.0, 0.0).isEmpty());
    }

    void tickStrings()
    {
        QCOMPARE(plotTickStrings({0.0, 0.5, 1.0}, 0.5), (QStringList{"0", "0.5", "1.0"}));
        QCOMPARE(plotTickStrings({-20.0, 0.0, 20.0}, 20.0), (QStringList{"-20", "0", "20"}));
        QCOMPARE(plotTickStrings({0.0002, 0.0004}, 0.0002), (QStringList{"0.0002", "0.0004"}));
        QCOMPARE(plotTickStrings({20000.0, 1e6}, 10000.0), (QStringList{"20000", "1e+06"}));
        QCOMPARE(plotTickStrings({0.25}, 0.05), QStringList{"0.25"});
    }

    // --- time series ---

    void rollingWindow()
    {
        TimeSeriesPlot plot({.window = 5.0});
        plot.resize(400, 300);
        for (int t = 0; t <= 20; ++t)
            plot.append(t, double(t));
        QCOMPARE(plot.seriesKeys(), QStringList{"data"});
        QCOMPARE(plot.latestTime().value_or(-1.0), 20.0);
        const QList<QPointF> drawn = plot.drawnData(QStringLiteral("data"));
        // Relative time: the newest sample at 0, the window back to -5.
        QCOMPARE(drawn.size(), 6);
        QCOMPARE(drawn.first(), QPointF(-5, 15));
        QCOMPARE(drawn.last(), QPointF(0, 20));
        QCOMPARE(plot.viewXRange(), (PlotRange{-5.0, 0.0}));
        QVERIFY(!plot.isAutoRange(Qt::Horizontal));

        plot.setWindow(2.0);
        QCOMPARE(plot.drawnData(QStringLiteral("data")).size(), 3);
        QCOMPARE(plot.viewXRange(), (PlotRange{-2.0, 0.0}));

        // Absolute time.
        TimeSeriesPlot absolute({.window = 5.0, .relativeTime = false});
        absolute.resize(400, 300);
        absolute.setData(QStringLiteral("v"), range(21), range(21));
        QCOMPARE(absolute.drawnData(QStringLiteral("v")).first(), QPointF(15, 15));
        QCOMPARE(absolute.viewXRange(), (PlotRange{15.0, 20.0}));

        // No window: everything, x auto-ranged.
        plot.setWindow(std::nullopt);
        QCOMPARE(plot.drawnData(QStringLiteral("data")).size(), 21);
        QVERIFY(plot.isAutoRange(Qt::Horizontal));
        QVERIFY(plot.viewXRange().min < -20.0 && plot.viewXRange().max > 0.0);
    }

    void bufferTrimmingAndNow()
    {
        TimeSeriesPlot plot({.window = std::nullopt, .maxSamples = 10});
        plot.extend(range(25), {{QStringLiteral("a"), range(25)}});
        const QList<QPointF> drawn = plot.drawnData(QStringLiteral("a"));
        QCOMPARE(drawn.size(), 10);
        QCOMPARE(drawn.first().y(), 15.0);
        QCOMPARE(drawn.last(), QPointF(0, 24));

        // Pinned "now": the window ends there; later samples stay (as Python).
        TimeSeriesPlot pinned({.window = 4.0});
        pinned.setData(QStringLiteral("a"), range(10), range(10));
        pinned.setNow(5.0);
        QCOMPARE(pinned.now().value_or(-1.0), 5.0);
        const QList<QPointF> cut = pinned.drawnData(QStringLiteral("a"));
        QCOMPARE(cut.first(), QPointF(-4, 1));
        QCOMPARE(cut.last(), QPointF(4, 9));
        pinned.clear();
        QVERIFY(pinned.drawnData(QStringLiteral("a")).isEmpty());
        QVERIFY(pinned.hasSeries(QStringLiteral("a")));
    }

    void seriesConfiguration()
    {
        TimeSeriesPlot plot;
        plot.addSeries(QStringLiteral("x"), {.label = QStringLiteral("X")});
        plot.addSeries(QStringLiteral("hidden"), {.label = QString()});
        plot.append(0.0, {{QStringLiteral("y"), 1.0}, {QStringLiteral("z"), 2.0}});
        QCOMPARE(plot.seriesKeys(), (QStringList{"x", "hidden", "y", "z"}));
        QCOMPARE(plot.legendLabels(), (QStringList{"X", "y", "z"}));
        const PlotStyle style = plot.plotStyle();
        QCOMPARE(plot.seriesColor(QStringLiteral("x")), style.seriesColor(0));
        QCOMPARE(plot.seriesColor(QStringLiteral("z")), style.seriesColor(3));
        // Colours continue after a removal; replacing moves a series to the end.
        plot.removeSeries(QStringLiteral("z"));
        plot.addSeries(QStringLiteral("w"), {.color = color(Token::PlotMagnitude)});
        QCOMPARE(plot.seriesColor(QStringLiteral("w")), color(Token::PlotMagnitude));
        plot.addSeries(QStringLiteral("x"));
        QCOMPARE(plot.seriesKeys(), (QStringList{"hidden", "y", "w", "x"}));
        QCOMPARE(plot.seriesColor(QStringLiteral("x")), style.seriesColor(4));
        QVERIFY(!plot.seriesColor(QStringLiteral("nope")).isValid());
        QVERIFY(plot.drawnData(QStringLiteral("nope")).isEmpty());
        // A plain number goes to the first series.
        plot.append(1.0, 7.0);
        QCOMPARE(plot.drawnData(QStringLiteral("hidden")).size(), 1);

        QVERIFY_THROWS_EXCEPTION(std::invalid_argument,
                                 plot.extend(range(3), {{QStringLiteral("y"), range(3)}, {QStringLiteral("w"), range(2)}}));
        QCOMPARE(plot.drawnData(QStringLiteral("y")).size(), 1); // nothing added
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, plot.setData(QStringLiteral("y"), range(3), range(4)));
    }

    void wrappingSeries()
    {
        TimeSeriesPlot plot({.window = std::nullopt});
        plot.addSeries(QStringLiteral("heading"), {.wrap = 360.0});
        plot.addSeries(QStringLiteral("plain"));
        const QList<double> headings{350, 355, 2, 5};
        plot.setData(QStringLiteral("heading"), range(4), headings);
        plot.setData(QStringLiteral("plain"), range(4), headings);
        const QList<QPointF> drawn = plot.drawnData(QStringLiteral("heading"));
        QCOMPARE(drawn.size(), 5);
        QVERIFY(std::isnan(drawn.at(2).y()));
        QCOMPARE(plot.drawnData(QStringLiteral("plain")).size(), 4);
    }

    void autoRangeAndFixedY()
    {
        TimeSeriesPlot plot({.window = 100.0});
        plot.resize(400, 300);
        plot.setData(QStringLiteral("v"), range(11), range(11));
        plot.redraw();
        const PlotRange y = plot.viewYRange();
        QVERIFY(plot.isAutoRange(Qt::Vertical));
        // Padded as pyqtgraph: 1/sqrt(height) of the span per side, 2..10 %.
        const double padding = std::clamp(1.0 / std::sqrt(plot.viewRect().height()), 0.02, 0.1);
        QVERIFY(std::abs(y.min - (0.0 - 10.0 * padding)) < 0.2);
        QVERIFY(std::abs(y.max - (10.0 + 10.0 * padding)) < 0.2);
        QVERIFY(y.min < 0.0 && y.max > 10.0);

        // Reference lines count for the y range while shown.
        plot.addReferenceLine(QStringLiteral("expected"), 50.0);
        QCOMPARE(plot.reference(QStringLiteral("expected")).value_or(-1.0), 50.0);
        QVERIFY(plot.viewYRange().max > 50.0);
        plot.setReference(QStringLiteral("expected"), std::nullopt);
        QVERIFY(!plot.reference(QStringLiteral("expected")));
        QVERIFY(plot.viewYRange().max < 15.0);
        plot.setReference(QStringLiteral("unknown"), 3.0); // ignored
        QVERIFY(!plot.reference(QStringLiteral("unknown")));
        plot.removeReferenceLine(QStringLiteral("expected"));
        plot.setReference(QStringLiteral("expected"), 1.0);
        QVERIFY(!plot.reference(QStringLiteral("expected")));

        // Fixed y range, then back to auto.
        plot.setYRange(-1.0, 1.0);
        QCOMPARE(plot.viewYRange(), (PlotRange{-1.0, 1.0}));
        QVERIFY(!plot.isAutoRange(Qt::Vertical));
        plot.append(20.0, {{QStringLiteral("v"), 99.0}});
        plot.redraw();
        QCOMPARE(plot.viewYRange(), (PlotRange{-1.0, 1.0}));
        // Zoomed away, resetView() returns to the fixed range.
        plot.setViewYRange(5.0, 6.0);
        plot.resetView();
        QCOMPARE(plot.viewYRange(), (PlotRange{-1.0, 1.0}));
        plot.setYRange();
        QVERIFY(plot.isAutoRange(Qt::Vertical));
        QVERIFY(plot.viewYRange().max > 99.0);
        QVERIFY(!plot.yRange());
    }

    void hiddenSeriesLeaveAutoRange()
    {
        TimeSeriesPlot plot({.window = std::nullopt});
        plot.resize(400, 300);
        plot.setData(QStringLiteral("small"), range(5), range(5));
        plot.setData(QStringLiteral("big"), range(5), range(5, 100.0));
        plot.redraw();
        QVERIFY(plot.viewYRange().max > 400.0);
        plot.setSeriesVisible(QStringLiteral("big"), false);
        QVERIFY(!plot.isSeriesVisible(QStringLiteral("big")));
        QVERIFY(plot.viewYRange().max < 10.0);
        // No readout line for the hidden series.
        const QList<PlotReadoutLine> lines = plot.crosshairReadout(QPointF(1.0, 1.0));
        QCOMPARE(lines.size(), 2);
    }

    void readout()
    {
        TimeSeriesPlot plot({.yLabel = QStringLiteral("a"), .yUnit = QStringLiteral("g"), .window = std::nullopt});
        plot.addSeries(QStringLiteral("x"), {.label = QStringLiteral("X")});
        plot.addSeries(QStringLiteral("quiet"), {.label = QString()});
        plot.setData(QStringLiteral("x"), {0.0, 1.0, 2.0}, {10.0, 20.0, std::numeric_limits<double>::quiet_NaN()});
        plot.setData(QStringLiteral("quiet"), {0.0}, {1.0});
        plot.redraw();
        // Relative time: x = -2, -1, 0.
        QList<PlotReadoutLine> lines = plot.crosshairReadout(QPointF(-1.1, 0.5));
        QCOMPARE(lines.size(), 2);
        QCOMPARE(lines.at(0).text, QStringLiteral("time: -1.1 s  ·  a: 0.5 g"));
        QCOMPARE(lines.at(0).color, plot.plotStyle().text);
        QCOMPARE(lines.at(1).text, QStringLiteral("X: 20 g"));
        QCOMPARE(lines.at(1).color, plot.seriesColor(QStringLiteral("x")));
        lines = plot.crosshairReadout(QPointF(0.2, 0.0));
        QCOMPARE(lines.at(1).text, QStringLiteral("X: --"));

        ScatterPlot scatter({.xLabel = QStringLiteral("Bx"), .unit = QStringLiteral("µT")});
        lines = scatter.crosshairReadout(QPointF(1.23456, -2.0));
        QCOMPARE(lines.size(), 2);
        QCOMPARE(lines.at(0).text, QStringLiteral("Bx: 1.235 µT"));
        QCOMPARE(lines.at(1).text, QStringLiteral("y: -2 µT"));
    }

    void axesAndLabels()
    {
        TimeSeriesPlot plot({.title = QStringLiteral("Accel"), .yLabel = QStringLiteral("a"), .yUnit = QStringLiteral("g")});
        QCOMPARE(plot.title(), QStringLiteral("Accel"));
        QCOMPARE(plot.axisLabelText(AxisSide::Left), QStringLiteral("a [g]"));
        QCOMPARE(plot.axisLabelText(AxisSide::Bottom), QStringLiteral("time [s]"));
        QVERIFY(!plot.isAxisShown(AxisSide::Right));
        plot.setAxisLabel(AxisSide::Right, QStringLiteral("other"));
        QVERIFY(plot.isAxisShown(AxisSide::Right));
        plot.setYLabel(QStringLiteral("b"), QStringLiteral("m/s²"));
        QCOMPARE(plot.axisLabelText(AxisSide::Left), QStringLiteral("b [m/s²]"));

        ScatterPlot scatter({.xLabel = QStringLiteral("x"), .yLabel = QStringLiteral("y"), .unit = QStringLiteral("µT")});
        QCOMPARE(scatter.axisLabelText(AxisSide::Bottom), QStringLiteral("x [µT]"));
        scatter.setAxisLabels(std::nullopt, QStringLiteral("z"));
        QCOMPARE(scatter.axisLabelText(AxisSide::Bottom), QStringLiteral("x [µT]"));
        QCOMPARE(scatter.axisLabelText(AxisSide::Left), QStringLiteral("z [µT]"));
        scatter.setAxisLabels(std::nullopt, std::nullopt, QStringLiteral("G"));
        QCOMPARE(scatter.axisLabelText(AxisSide::Left), QStringLiteral("z [G]"));
    }

    // --- scatter ---

    void scatterAspectAndCircle()
    {
        ScatterPlot plot;
        plot.resize(500, 250);
        QVERIFY(plot.isAspectLocked());
        QList<double> xs, ys;
        for (int i = 0; i < 100; ++i) {
            const double a = 2.0 * std::numbers::pi * i / 100.0;
            xs.append(std::cos(a));
            ys.append(std::sin(a));
        }
        plot.setData(xs, ys);
        QCOMPARE(plot.seriesKeys(), QStringList{"data"});
        QVERIFY(plot.legendLabels().isEmpty()); // unlabelled, and no legend by default
        plot.redraw();
        plot.grab(); // final layout
        const QRectF view = plot.viewRect();
        const double unitsX = plot.viewXRange().span() / view.width();
        const double unitsY = plot.viewYRange().span() / view.height();
        QVERIFY2(std::abs(unitsX - unitsY) / unitsY < 1e-6, qPrintable(QStringLiteral("%1 vs %2").arg(unitsX).arg(unitsY)));
        // y is the tight axis: the data plus padding; x got wider.
        QVERIFY(plot.viewYRange().max > 1.0 && plot.viewYRange().max < 1.3);
        QVERIFY(plot.viewXRange().span() > plot.viewYRange().span());

        plot.setReferenceCircle(2.0, QPointF(0.5, 0.0));
        const auto circle = plot.referenceCircle();
        QVERIFY(circle);
        QCOMPARE(circle->radius, 2.0);
        QCOMPARE(circle->center, QPointF(0.5, 0.0));
        plot.redraw();
        QVERIFY(plot.viewYRange().max > 2.0);
        plot.setReferenceCircle(0.0);
        QVERIFY(!plot.referenceCircle());
        plot.redraw();
        QVERIFY(plot.viewYRange().max < 1.3);

        ScatterPlot free({.aspectLocked = false});
        QVERIFY(!free.isAspectLocked());
    }

    void scatterBuffers()
    {
        ScatterPlot plot({.maxPoints = 5, .legend = true});
        plot.addSeries(QStringLiteral("cloud"), {.label = QStringLiteral("Cloud"), .size = 6.0, .alpha = 255});
        plot.addSeries(QStringLiteral("path"), {.line = true});
        QCOMPARE(plot.legendLabels(), (QStringList{"Cloud", "path"}));
        for (int i = 0; i < 8; ++i)
            plot.append(i, -i); // the first series
        QCOMPARE(plot.drawnData(QStringLiteral("cloud")).size(), 5);
        QCOMPARE(plot.drawnData(QStringLiteral("cloud")).first(), QPointF(3, -3));
        plot.extend({1, 2}, {3, 4}, QStringLiteral("path"));
        QCOMPARE(plot.drawnData(QStringLiteral("path")).size(), 2);
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, plot.extend({1, 2}, {3}, QStringLiteral("path")));
        plot.setData(range(10), range(10), QStringLiteral("new"));
        QCOMPARE(plot.drawnData(QStringLiteral("new")).size(), 5);
        QCOMPARE(plot.maxPoints(), 5);
    }

    // --- linked axes ---

    void linkedXAxes()
    {
        TimeSeriesPlot a({.window = std::nullopt});
        TimeSeriesPlot b({.window = std::nullopt});
        auto c = std::make_unique<ScatterPlot>(ScatterPlot::Options{.aspectLocked = false});
        for (PlotBase *plot : std::initializer_list<PlotBase *>{&a, &b, c.get()})
            plot->resize(400, 300);
        a.setViewXRange(2.0, 4.0);
        linkXAxes({&a, &b, c.get()});
        // Linking makes the others follow the first at once.
        QCOMPARE(b.viewXRange(), (PlotRange{2.0, 4.0}));
        QCOMPARE(c->viewXRange(), (PlotRange{2.0, 4.0}));
        QVERIFY(!b.isAutoRange(Qt::Horizontal));

        // Any of them leads.
        b.setViewXRange(5.0, 7.0);
        QCOMPARE(a.viewXRange(), (PlotRange{5.0, 7.0}));
        QCOMPARE(c->viewXRange(), (PlotRange{5.0, 7.0}));
        // y stays independent.
        a.setViewYRange(0.0, 1.0);
        b.setViewYRange(10.0, 20.0);
        QCOMPARE(a.viewYRange(), (PlotRange{0.0, 1.0}));

        // A destroyed member drops out quietly.
        c.reset();
        a.setViewXRange(-1.0, 1.0);
        QCOMPARE(b.viewXRange(), (PlotRange{-1.0, 1.0}));

        // A rolling window drives the linked plot too.
        TimeSeriesPlot d({.window = 3.0});
        d.resize(400, 300);
        d.linkX(&a);
        d.setData(QStringLiteral("v"), range(10), range(10));
        d.redraw();
        QCOMPARE(a.viewXRange(), (PlotRange{-3.0, 0.0}));
        QCOMPARE(b.viewXRange(), (PlotRange{-3.0, 0.0}));
    }

    // --- mouse interaction ---

    void mouseZoomPanAndReset()
    {
        TimeSeriesPlot plot({.window = std::nullopt, .refreshMs = 0});
        plot.resize(500, 350);
        plot.show();
        QVERIFY(QTest::qWaitForWindowExposed(&plot));
        plot.setData(QStringLiteral("v"), range(101), range(101));
        plot.redraw();
        const PlotRange x0 = plot.viewXRange();
        const PlotRange y0 = plot.viewYRange();
        const QPointF centre = plot.viewRect().center();

        // Wheel up zooms in around the mouse and stops auto-ranging.
        sendWheel(&plot, centre, 120);
        QVERIFY(plot.viewXRange().span() < x0.span());
        QVERIFY(plot.viewYRange().span() < y0.span());
        QVERIFY(!plot.isAutoRange(Qt::Horizontal) && !plot.isAutoRange(Qt::Vertical));

        // Double-click returns to the plot's own view.
        QTest::mouseDClick(&plot, Qt::LeftButton, Qt::NoModifier, centre.toPoint());
        QVERIFY(plot.isAutoRange(Qt::Horizontal) && plot.isAutoRange(Qt::Vertical));
        QCOMPARE(plot.viewXRange(), x0);

        // Left drag pans: dragging right moves the view to smaller x.
        sendMouse(&plot, QEvent::MouseButtonPress, centre, Qt::LeftButton, Qt::LeftButton);
        sendMouse(&plot, QEvent::MouseMove, centre + QPointF(30, 0), Qt::NoButton, Qt::LeftButton);
        sendMouse(&plot, QEvent::MouseMove, centre + QPointF(60, 0), Qt::NoButton, Qt::LeftButton);
        sendMouse(&plot, QEvent::MouseButtonRelease, centre + QPointF(60, 0), Qt::LeftButton, Qt::NoButton);
        QVERIFY(plot.viewXRange().min < x0.min);
        QVERIFY(std::abs(plot.viewXRange().span() - x0.span()) < 1e-9);
        QVERIFY(!plot.isAutoRange(Qt::Horizontal));

        // The wheel over the left axis zooms y only.
        plot.resetView();
        const QPointF onAxis(plot.viewRect().left() - 5.0, centre.y());
        sendWheel(&plot, onAxis, -120);
        QVERIFY(plot.viewYRange().span() > y0.span());
        QCOMPARE(plot.viewXRange(), x0);
        QVERIFY(plot.isAutoRange(Qt::Horizontal));

        // Mouse disabled on x: wheel leaves x alone.
        plot.resetView();
        plot.setMouseEnabled(Qt::Vertical);
        sendWheel(&plot, centre, 120);
        QCOMPARE(plot.viewXRange(), x0);
        plot.setMouseEnabled(Qt::Horizontal | Qt::Vertical);

        // Rectangle zoom ("1 button" mode).
        plot.resetView();
        plot.setMouseMode(PlotBase::MouseMode::Rect);
        const QPointF a = centre - QPointF(50, 40);
        const QPointF b = centre + QPointF(50, 40);
        sendMouse(&plot, QEvent::MouseButtonPress, a, Qt::LeftButton, Qt::LeftButton);
        sendMouse(&plot, QEvent::MouseMove, b, Qt::NoButton, Qt::LeftButton);
        render(plot); // rubber band
        sendMouse(&plot, QEvent::MouseButtonRelease, b, Qt::LeftButton, Qt::NoButton);
        QVERIFY(plot.viewXRange().span() < x0.span() / 3.0);
        QVERIFY(plot.viewXRange().min > x0.min);
    }

    void legendClickHidesSeries()
    {
        TimeSeriesPlot plot({.window = std::nullopt});
        plot.resize(400, 300);
        plot.show();
        QVERIFY(QTest::qWaitForWindowExposed(&plot));
        plot.addSeries(QStringLiteral("x"), {.label = QStringLiteral("X axis")});
        plot.setData(QStringLiteral("x"), range(5), range(5));
        plot.redraw();
        render(plot); // lays out the legend
        const QPointF entry = plot.viewRect().topLeft() + QPointF(20, 20);
        QTest::mouseClick(&plot, Qt::LeftButton, Qt::NoModifier, entry.toPoint());
        QVERIFY(!plot.isSeriesVisible(QStringLiteral("x")));
        render(plot);
        QTest::mouseClick(&plot, Qt::LeftButton, Qt::NoModifier, entry.toPoint());
        QVERIFY(plot.isSeriesVisible(QStringLiteral("x")));
    }

    void contextMenuAndCrosshair()
    {
        TimeSeriesPlot plot({.crosshair = true});
        QVERIFY(plot.crosshair());
        QMenu *menu = plot.contextMenu();
        QVERIFY(menu);
        QAction *crosshair = findAction(menu, QStringLiteral("Crosshair"));
        QAction *reset = findAction(menu, QStringLiteral("Reset view"));
        QVERIFY(crosshair && reset && findAction(menu, QStringLiteral("View All")));
        QVERIFY(crosshair->isChecked());
        crosshair->setChecked(false);
        QVERIFY(!plot.crosshair());
        plot.setCrosshair(true);
        QVERIFY(crosshair->isChecked());

        plot.setViewXRange(0.0, 1.0);
        plot.setWindow(std::nullopt);
        plot.setViewXRange(0.0, 1.0);
        reset->trigger();
        QVERIFY(plot.isAutoRange(Qt::Horizontal));
    }

    void aspectAndInversion()
    {
        ScatterPlot plot({.aspectLocked = false});
        plot.resize(400, 300);
        plot.setViewXRange(0.0, 10.0);
        plot.setViewYRange(0.0, 10.0);
        plot.grab();
        const QRectF v = plot.viewRect();
        QPointF p = plot.mapFromData(QPointF(0.0, 0.0));
        QVERIFY(std::abs(p.x() - v.left()) < 1e-6 && std::abs(p.y() - v.bottom()) < 1e-6);
        plot.setAxisInverted(Qt::Vertical, true);
        QVERIFY(plot.isAxisInverted(Qt::Vertical));
        p = plot.mapFromData(QPointF(0.0, 0.0));
        QVERIFY(std::abs(p.y() - v.top()) < 1e-6);
        const QPointF back = plot.mapToData(plot.mapFromData(QPointF(3.0, 7.0)));
        QVERIFY(std::abs(back.x() - 3.0) < 1e-9 && std::abs(back.y() - 7.0) < 1e-9);
        plot.viewAll(); // no data: unchanged
        QCOMPARE(plot.viewXRange(), (PlotRange{0.0, 10.0}));
    }

    // --- redraw throttling ---

    void throttledRedraw()
    {
        TimeSeriesPlot plot({.refreshMs = 0});
        plot.append(0.0, 1.0);
        QVERIFY(plot.isRedrawPending());
        // Hidden: the timer fires but the plot stays dirty.
        QTest::qWait(20);
        QVERIFY(plot.isRedrawPending());
        plot.show();
        QTRY_VERIFY(!plot.isRedrawPending());
    }

    // --- rendering ---

    void renderTimeSeries()
    {
        TimeSeriesPlot plot({.title = QStringLiteral("Live"), .yLabel = QStringLiteral("v"), .window = std::nullopt});
        plot.resize(480, 320);
        plot.addSeries(QStringLiteral("sine"), {.label = QStringLiteral("Sine")});
        plot.addSeries(QStringLiteral("dash"), {.label = QStringLiteral("Dashed"), .dashed = true});
        plot.addSeries(QStringLiteral("dots"), {.points = true, .size = 5.0, .alpha = 255});
        plot.addReferenceLine(QStringLiteral("ref"), 0.5);
        QList<double> t, s, c;
        for (int i = 0; i < 20000; ++i) {
            t.append(i * 0.001);
            s.append(std::sin(i * 0.01));
            c.append(std::cos(i * 0.003) * 0.5);
        }
        plot.setData(QStringLiteral("sine"), t, s);
        plot.setData(QStringLiteral("dash"), t, c);
        plot.setData(QStringLiteral("dots"), range(20, 1.0), range(20, 0.05, -0.5));
        plot.redraw();
        QElapsedTimer timer;
        timer.start();
        const QImage image = render(plot);
        qDebug("20000-sample render: %lld ms", static_cast<long long>(timer.elapsed()));
        const PlotStyle style = plot.plotStyle();
        QCOMPARE(image.pixelColor(1, 1), style.background);
        QVERIFY(containsColor(image, plot.seriesColor(QStringLiteral("sine"))));
        QVERIFY(containsColor(image, plot.seriesColor(QStringLiteral("dash"))));
        QVERIFY(containsColor(image, plot.seriesColor(QStringLiteral("dots"))));
        QVERIFY(containsColor(image, style.reference));
        QVERIFY(containsColor(image, style.axis));

        // Crosshair and readout over the data, then leaving.
        plot.show();
        QVERIFY(QTest::qWaitForWindowExposed(&plot));
        sendMouse(&plot, QEvent::MouseMove, plot.viewRect().center() + QPointF(60, -40), Qt::NoButton, Qt::NoButton);
        render(plot);
        QEvent leave(QEvent::Leave);
        QApplication::sendEvent(&plot, &leave);
        render(plot);

        // Restyled: a new series colour shows up.
        PlotStyle restyled = style;
        restyled.series[0] = color(Token::PlotMagnitude);
        plot.setPlotStyle(restyled);
        QVERIFY(containsColor(render(plot), color(Token::PlotMagnitude)));
    }

    void renderScatter()
    {
        ScatterPlot plot({.legend = true});
        plot.resize(360, 360);
        plot.addSeries(QStringLiteral("cloud"), {.label = QStringLiteral("Cloud"), .size = 6.0, .alpha = 255});
        for (int i = 0; i < 3000; ++i) {
            const double a = i * 0.01;
            plot.append(std::cos(a) * (1.0 + 0.001 * i), std::sin(a));
        }
        plot.setReferenceCircle(1.5);
        plot.redraw();
        const QImage image = render(plot);
        QVERIFY(containsColor(image, plot.seriesColor(QStringLiteral("cloud"))));
        QVERIFY(containsColor(image, plot.plotStyle().reference));

        // Degenerate sizes and an empty plot paint without trouble.
        ScatterPlot empty;
        empty.resize(10, 10);
        render(empty);
        empty.resize(300, 200);
        empty.append(1.0, 1.0); // a single point: zero span keeps a scale
        empty.redraw();
        QVERIFY(empty.viewXRange().span() > 0.0);
        render(empty);
    }

    void exportCsv()
    {
        QTemporaryDir dir;
        TimeSeriesPlot plot({.window = std::nullopt, .relativeTime = false});
        plot.addSeries(QStringLiteral("x"), {.label = QStringLiteral("X")});
        plot.setData(QStringLiteral("x"), {0.0, 1.0}, {2.0, 3.5});
        plot.setData(QStringLiteral("y"), {0.0}, {1.0});
        const QString path = dir.filePath(QStringLiteral("plot.csv"));
        QVERIFY(plot.exportCsv(path));
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        const QStringList lines = QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        QCOMPARE(lines, (QStringList{"X_x,X_y,y_x,y_y", "0,2,0,1", "1,3.5,,"}));
        plot.resize(200, 150);
        QVERIFY(plot.exportImage(dir.filePath(QStringLiteral("plot.png"))));
    }

    // --- compass dial ---

    void compassDial()
    {
        CompassDial dial;
        QCOMPARE(dial.needleKeys(), QStringList{"heading"});
        QVERIFY(!dial.heading());
        QCOMPARE(dial.readoutText(), QString());
        dial.setHeading(370.0);
        QCOMPARE(*dial.heading(), 10.0);
        QCOMPARE(dial.readoutText(), QStringLiteral("10.0°  N"));
        dial.setHeading(-45.0);
        QCOMPARE(*dial.heading(), 315.0);
        QCOMPARE(dial.readoutText(), QStringLiteral("315.0°  NW"));
        dial.setReadout(QStringLiteral("custom"));
        QCOMPARE(dial.readoutText(), QStringLiteral("custom"));
        dial.setReadout(std::nullopt);
        QCOMPARE(dial.readoutText(), QStringLiteral("315.0°  NW"));

        dial.setUncertainty(-5.0);
        QCOMPARE(*dial.uncertainty(), 5.0);
        const PlotStyle style = dial.plotStyle();
        QCOMPARE(dial.needleColor(QStringLiteral("heading")), style.accent);

        // Labelled needles grow a legend under the rose.
        const int bare = dial.heightForWidth(200);
        QCOMPARE(bare, 200);
        dial.addNeedle(QStringLiteral("gps"), {}, QStringLiteral("GPS"));
        QCOMPARE(dial.needleColor(QStringLiteral("gps")), style.seriesColor(0));
        QVERIFY(dial.heightForWidth(200) > bare);
        dial.setHeading(90.0, QStringLiteral("gps"));
        dial.setHeading(180.0, QStringLiteral("new")); // created on demand
        QCOMPARE(dial.needleKeys(), (QStringList{"heading", "gps", "new"}));
        // Re-adding keeps the heading.
        dial.addNeedle(QStringLiteral("gps"), color(Token::PlotMagnitude), QStringLiteral("GPS"));
        QCOMPARE(*dial.heading(QStringLiteral("gps")), 90.0);
        QCOMPARE(dial.needleColor(QStringLiteral("gps")), color(Token::PlotMagnitude));

        dial.resize(240, dial.heightForWidth(240));
        const QImage image = render(dial);
        QVERIFY(containsColor(image, style.accent));
        QVERIFY(containsColor(image, color(Token::PlotMagnitude)));
        QVERIFY(containsColor(image, style.axis));

        dial.clear();
        QVERIFY(!dial.heading() && !dial.uncertainty());
        dial.removeNeedle(QStringLiteral("gps"));
        QCOMPARE(dial.needleKeys(), (QStringList{"heading", "new"}));

        CompassDial four({.points = 4, .showReadout = false, .showLegend = false});
        four.addNeedle(QStringLiteral("x"), {}, QStringLiteral("label"));
        QCOMPARE(four.heightForWidth(150), 150);
        four.resize(150, 150);
        render(four);
        four.resize(15, 15); // too small for a rose
        render(four);
    }

    // --- threshold bar ---

    void thresholdBar()
    {
        const PlotStyle style;
        ThresholdBar bar({.maximum = 360.0, .gate = 180.0, .unit = QStringLiteral("°")});
        QCOMPARE(bar.maximum(), 360 * ThresholdBar::kScale);
        QCOMPARE(bar.maximumF(), 360.0);
        QVERIFY(!bar.valueF());
        QVERIFY(!bar.gateMet());

        bar.setValueF(143.0);
        QCOMPARE(bar.valueF().value_or(-1.0), 143.0);
        QCOMPARE(bar.value(), 143000);
        QCOMPARE(*bar.gateMet(), false);
        QCOMPARE(bar.format(), QStringLiteral("143.0°  (gate ≥ 180°)"));
        QVERIFY(bar.styleSheet().contains(style.warn.name()));

        bar.setValueF(180.0);
        QCOMPARE(*bar.gateMet(), true);
        QVERIFY(bar.styleSheet().contains(style.ok.name()));

        // Out of scale: the bar is clamped, the text is not.
        bar.setValueF(1000.0);
        QCOMPARE(bar.value(), bar.maximum());
        QVERIFY(bar.format().startsWith(QStringLiteral("1000.0")));
        bar.setValueF(-5.0);
        QCOMPARE(bar.value(), 0);

        // No gate: no judgement.
        bar.setGate(std::nullopt);
        QVERIFY(!bar.gateMet());
        QCOMPARE(bar.format(), QStringLiteral("-5.0°"));
        QVERIFY(bar.styleSheet().isEmpty());

        // Cleared: a dash.
        bar.setValueF(std::nullopt);
        QCOMPARE(bar.format(), QStringLiteral("—"));
        QCOMPARE(bar.value(), 0);

        // A maximum gate.
        ThresholdBar error({.maximum = 10.0, .gate = 2.5, .decimals = 2, .higherIsBetter = false});
        error.setValueF(1.234);
        QCOMPARE(*error.gateMet(), true);
        QCOMPARE(error.format(), QStringLiteral("1.23  (gate ≤ 2.5)"));
        error.setValueF(3.0);
        QCOMPARE(*error.gateMet(), false);
        error.setMaximumF(0.0001);
        QCOMPARE(error.maximum(), 1);

        PlotStyle restyled = style;
        restyled.warn = color(Token::StateError);
        error.setPlotStyle(restyled);
        QVERIFY(error.styleSheet().contains(color(Token::StateError).name()));
        error.resize(200, 24);
        render(error);
    }
};

QTEST_MAIN(TstPlot)
#include "tst_plot.moc"

#include "duifx/ScatterPlot.h"

#include "PlotPrivate.h"

#include <numbers>
#include <stdexcept>

namespace duifx {

namespace {

/// Points on a reference circle, as numpy.linspace(0, 2 pi, 181) in Python.
constexpr int kCircleSegments = 180;

} // namespace

ScatterPlot::ScatterPlot(QWidget *parent)
    : ScatterPlot(Options{}, parent)
{
}

ScatterPlot::ScatterPlot(const Options &options, QWidget *parent)
    : PlotBase(options.title, options.style, options.legend, options.crosshair, options.refreshMs, parent)
    , m_maxPoints(std::max(0, options.maxPoints))
{
    setAspectLocked(options.aspectLocked);
    setAxisLabels(options.xLabel, options.yLabel, options.unit);
}

void ScatterPlot::setAxisLabels(const std::optional<QString> &xLabel, const std::optional<QString> &yLabel,
                                const std::optional<QString> &unit)
{
    const int bottom = static_cast<int>(AxisSide::Bottom);
    const int left = static_cast<int>(AxisSide::Left);
    const QString xText = d->labelText[bottom];
    const QString xUnit = d->labelUnit[bottom];
    const QString yText = d->labelText[left];
    const QString yUnit = d->labelUnit[left];
    setAxisLabel(AxisSide::Bottom, xLabel ? *xLabel : xText, unit ? *unit : xUnit);
    setAxisLabel(AxisSide::Left, yLabel ? *yLabel : yText, unit ? *unit : yUnit);
}

void ScatterPlot::addSeries(const QString &key)
{
    addSeries(key, SeriesOptions{});
}

void ScatterPlot::addSeries(const QString &key, const SeriesOptions &options)
{
    PlotSeries &s = insertSeries(key, options.label, m_maxPoints);
    s.color = options.color;
    s.points = !options.line;
    s.size = options.size;
    s.alpha = options.alpha;
    itemsChanged();
}

void ScatterPlot::addDefaultSeries(const QString &key)
{
    // Unlabelled: a single default cloud needs no legend entry.
    SeriesOptions options;
    options.label = QString();
    addSeries(key, options);
}

int ScatterPlot::maxPoints() const
{
    return m_maxPoints;
}

void ScatterPlot::append(double x, double y, const QString &key)
{
    seriesFor(key).data.push(QPointF(x, y));
    scheduleRedraw();
}

void ScatterPlot::extend(const QList<double> &xs, const QList<double> &ys, const QString &key)
{
    if (xs.size() != ys.size())
        throw std::invalid_argument("x and y differ in length");
    PlotSeries &s = seriesFor(key);
    for (qsizetype i = 0; i < xs.size(); ++i)
        s.data.push(QPointF(xs.at(i), ys.at(i)));
    scheduleRedraw();
}

void ScatterPlot::setData(const QList<double> &xs, const QList<double> &ys, const QString &key)
{
    setSeriesData(key, xs, ys);
}

void ScatterPlot::setReferenceCircle(std::optional<double> radius, QPointF center, const QString &key,
                                     const QColor &color, bool dashed)
{
    if (!radius || !(*radius > 0.0)) {
        m_circles.remove(key);
        d->removeReference(key);
        itemsChanged();
        return;
    }
    m_circles.insert(key, Circle{*radius, center});
    PlotReference *reference = d->findReference(key);
    if (!reference) {
        PlotReference added;
        added.key = key;
        added.kind = PlotReference::Kind::Curve;
        added.order = d->nextOrder++;
        d->references.push_back(added);
        reference = &d->references.back();
    }
    reference->color = color;
    reference->dashed = dashed;
    scheduleRedraw();
}

std::optional<ScatterPlot::Circle> ScatterPlot::referenceCircle(const QString &key) const
{
    const auto it = m_circles.constFind(key);
    if (it == m_circles.cend())
        return std::nullopt;
    return *it;
}

void ScatterPlot::renderData()
{
    for (const auto &s : d->series) {
        QList<QPointF> &drawn = s->drawn;
        drawn.clear();
        drawn.reserve(s->data.size());
        s->data.forEach([&](const QPointF &p) { drawn.append(p); });
    }
    for (auto it = m_circles.cbegin(); it != m_circles.cend(); ++it) {
        PlotReference *reference = d->findReference(it.key());
        if (!reference)
            continue;
        const Circle &circle = it.value();
        reference->points.resize(kCircleSegments + 1);
        for (int i = 0; i <= kCircleSegments; ++i) {
            const double angle = 2.0 * std::numbers::pi * i / kCircleSegments;
            reference->points[i] = QPointF(circle.center.x() + circle.radius * std::cos(angle),
                                           circle.center.y() + circle.radius * std::sin(angle));
        }
    }
}

} // namespace duifx

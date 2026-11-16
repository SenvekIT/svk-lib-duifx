#include "duifx/TimeSeriesPlot.h"

#include "PlotPrivate.h"

#include <limits>
#include <stdexcept>
#include <string>

namespace duifx {

TimeSeriesPlot::TimeSeriesPlot(QWidget *parent)
    : TimeSeriesPlot(Options{}, parent)
{
}

TimeSeriesPlot::TimeSeriesPlot(const Options &options, QWidget *parent)
    : PlotBase(options.title, options.style, options.legend, options.crosshair, options.refreshMs, parent)
    , m_window(options.window)
    , m_maxSamples(std::max(0, options.maxSamples))
    , m_relative(options.relativeTime)
{
    setAxisLabel(AxisSide::Left, options.yLabel, options.yUnit);
    setAxisLabel(AxisSide::Bottom, options.xLabel, options.xUnit);
}

void TimeSeriesPlot::addSeries(const QString &key)
{
    addSeries(key, SeriesOptions{});
}

void TimeSeriesPlot::addSeries(const QString &key, const SeriesOptions &options)
{
    PlotSeries &s = insertSeries(key, options.label, m_maxSamples);
    s.color = options.color;
    s.dashed = options.dashed;
    s.points = options.points;
    s.width = options.width;
    s.size = options.size;
    s.alpha = options.alpha;
    s.wrap = options.wrap;
    itemsChanged();
}

void TimeSeriesPlot::addDefaultSeries(const QString &key)
{
    addSeries(key);
}

void TimeSeriesPlot::setYLabel(const QString &text, const QString &unit)
{
    setAxisLabel(AxisSide::Left, text, unit);
}

void TimeSeriesPlot::setWindow(std::optional<double> seconds)
{
    m_window = seconds;
    if (!seconds)
        setAutoRange(Qt::Horizontal, true);
    scheduleRedraw();
}

std::optional<double> TimeSeriesPlot::window() const
{
    return m_window;
}

void TimeSeriesPlot::setYRange(std::optional<double> low, std::optional<double> high)
{
    if (!low || !high) {
        m_yRange.reset();
        setAutoRange(Qt::Vertical, true);
    } else {
        m_yRange = PlotRange{*low, *high};
        setViewYRange(*low, *high);
    }
}

std::optional<PlotRange> TimeSeriesPlot::yRange() const
{
    return m_yRange;
}

void TimeSeriesPlot::setNow(std::optional<double> t)
{
    m_now = t;
    scheduleRedraw();
}

std::optional<double> TimeSeriesPlot::now() const
{
    return m_now;
}

bool TimeSeriesPlot::relativeTime() const
{
    return m_relative;
}

int TimeSeriesPlot::maxSamples() const
{
    return m_maxSamples;
}

void TimeSeriesPlot::linkX(TimeSeriesPlot *other)
{
    linkXAxes({other, this});
}

// --- reference lines ---------------------------------------------------------------

void TimeSeriesPlot::addReferenceLine(const QString &key, std::optional<double> value, const QColor &color,
                                      bool dashed)
{
    d->removeReference(key);
    PlotReference reference;
    reference.key = key;
    reference.kind = PlotReference::Kind::HLine;
    reference.color = color;
    reference.dashed = dashed;
    reference.order = d->nextOrder++;
    reference.value = value;
    d->references.push_back(reference);
    itemsChanged();
}

void TimeSeriesPlot::setReference(const QString &key, std::optional<double> value)
{
    PlotReference *reference = d->findReference(key);
    if (!reference)
        return;
    reference->value = value;
    itemsChanged();
}

std::optional<double> TimeSeriesPlot::reference(const QString &key) const
{
    const PlotReference *reference = d->findReference(key);
    return reference ? reference->value : std::nullopt;
}

void TimeSeriesPlot::removeReferenceLine(const QString &key)
{
    d->removeReference(key);
    itemsChanged();
}

// --- data ----------------------------------------------------------------------------

void TimeSeriesPlot::append(double t, const Values &values)
{
    for (const auto &[key, value] : values)
        seriesFor(key).data.push(QPointF(t, value));
    scheduleRedraw();
}

void TimeSeriesPlot::append(double t, double value)
{
    seriesFor(QString()).data.push(QPointF(t, value));
    scheduleRedraw();
}

void TimeSeriesPlot::extend(const QList<double> &t, const Blocks &values)
{
    for (const auto &[key, ys] : values) {
        if (ys.size() != t.size()) {
            throw std::invalid_argument("series " + key.toStdString() + ": " + std::to_string(ys.size())
                                        + " values for " + std::to_string(t.size()) + " times");
        }
    }
    for (const auto &[key, ys] : values) {
        PlotSeries &s = seriesFor(key);
        for (qsizetype i = 0; i < t.size(); ++i)
            s.data.push(QPointF(t.at(i), ys.at(i)));
    }
    scheduleRedraw();
}

void TimeSeriesPlot::setData(const QString &key, const QList<double> &t, const QList<double> &y)
{
    setSeriesData(key, t, y);
}

std::optional<double> TimeSeriesPlot::latestTime() const
{
    std::optional<double> latest;
    for (const auto &s : d->series) {
        if (s->data.isEmpty())
            continue;
        const double end = s->data.back().x();
        if (!latest || end > *latest)
            latest = end;
    }
    return latest;
}

// --- view ------------------------------------------------------------------------------

void TimeSeriesPlot::resetView()
{
    if (m_yRange)
        d->setTarget(1, *m_yRange, true);
    else
        d->autoRange[1] = true;
    if (!m_window)
        d->autoRange[0] = true;
    redraw();
}

QList<PlotReadoutLine> TimeSeriesPlot::readoutLines(double x, double y) const
{
    const QString &unit = d->labelUnit[static_cast<int>(AxisSide::Left)];
    QList<PlotReadoutLine> lines{
        {d->style.text, coordinate(AxisSide::Bottom, x, QStringLiteral("time")) + QStringLiteral("  ·  ")
                            + coordinate(AxisSide::Left, y, QStringLiteral("y"))}};
    for (const auto &s : d->series) {
        if (s->label.isEmpty() || !s->visible || s->drawn.isEmpty())
            continue;
        // The sample nearest in time; NaN gaps do not count.
        qsizetype best = -1;
        double bestDistance = std::numeric_limits<double>::infinity();
        for (qsizetype i = 0; i < s->drawn.size(); ++i) {
            const double distance = std::abs(s->drawn.at(i).x() - x);
            if (distance < bestDistance) {
                bestDistance = distance;
                best = i;
            }
        }
        if (best < 0)
            continue;
        const double value = s->drawn.at(best).y();
        QString text = std::isnan(value) ? QStringLiteral("--") : formatG(value) + QLatin1Char(' ') + unit;
        while (text.endsWith(QLatin1Char(' ')))
            text.chop(1);
        lines.append({d->seriesColor(*s), s->label + QStringLiteral(": ") + text});
    }
    return lines;
}

void TimeSeriesPlot::renderData()
{
    const std::optional<double> now = m_now ? m_now : latestTime();
    const double threshold = m_window && now ? *now - *m_window : -std::numeric_limits<double>::infinity();
    const double shift = m_relative && now ? *now : 0.0;
    for (const auto &s : d->series) {
        QList<QPointF> &drawn = s->drawn;
        drawn.clear(); // keeps the capacity: no reallocation per frame
        if (!now || s->data.isEmpty())
            continue;
        drawn.reserve(s->data.size());
        // Cut to the window (every sample at or after its start, as numpy's
        // mask), shift to relative time and break the line at wraps -- in one
        // pass over the ring buffer.
        const double half = s->wrap && *s->wrap != 0.0 ? *s->wrap / 2.0 : 0.0;
        bool havePrevious = false;
        double previous = 0.0;
        s->data.forEach([&](const QPointF &p) {
            if (m_window && !(p.x() >= threshold))
                return;
            if (half > 0.0) {
                if (havePrevious && std::abs(p.y() - previous) > half)
                    drawn.append(QPointF(kNaN, kNaN));
                previous = p.y();
                havePrevious = true;
            }
            drawn.append(QPointF(p.x() - shift, p.y()));
        });
    }
    if (m_window && now) {
        const double end = m_relative ? 0.0 : *now;
        d->setTarget(0, PlotRange{end - *m_window, end}, true);
    }
}

} // namespace duifx

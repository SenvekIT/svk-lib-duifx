// Style and helpers of the plot widgets (duifx/gui/plot.py). PlotBase is in
// PlotBase.cpp.

#include "duifx/Plot.h"

#include "PlotPrivate.h"

#include <array>
#include <cmath>
#include <mutex>

namespace duifx {

// --- style -------------------------------------------------------------------

QList<QColor> PlotStyle::defaultSeries()
{
    QList<QColor> result;
    result.reserve(static_cast<qsizetype>(kPlotSeries.size()));
    for (Token token : kPlotSeries)
        result.append(color(token));
    return result;
}

QColor PlotStyle::seriesColor(int index) const
{
    const auto count = static_cast<int>(series.size());
    if (count == 0)
        return plotSeriesColor(index);
    return series.at(((index % count) + count) % count);
}

namespace {

std::mutex g_styleMutex;
// Created on first use, so the active theme decides, not static init order.
std::optional<PlotStyle> g_defaultStyle;

} // namespace

PlotStyle defaultPlotStyle()
{
    std::lock_guard lock(g_styleMutex);
    if (!g_defaultStyle)
        g_defaultStyle = PlotStyle{};
    return *g_defaultStyle;
}

void setDefaultPlotStyle(const PlotStyle &style)
{
    std::lock_guard lock(g_styleMutex);
    g_defaultStyle = style;
}

// --- helpers -------------------------------------------------------------------

QString axisLabel(const QString &text, const QString &unit)
{
    if (unit.isEmpty())
        return text;
    if (text.isEmpty())
        return QStringLiteral("[%1]").arg(unit);
    return QStringLiteral("%1 [%2]").arg(text, unit);
}

QString compassPoint(double heading, int points)
{
    static const std::array<const char *, 8> points8{"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    static const std::array<const char *, 16> points16{"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
                                                       "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};
    // Python's % is never negative.
    double h = std::fmod(heading, 360.0);
    if (h < 0)
        h += 360.0;
    if (!std::isfinite(h))
        h = 0.0;
    const int count = points == 16 ? 16 : points == 4 ? 4 : 8;
    const double step = 360.0 / count;
    const int index = static_cast<int>(h / step + 0.5) % count;
    if (count == 16)
        return QLatin1StringView(points16[static_cast<std::size_t>(index)]).toString();
    // The four main points are every other of the eight.
    const int index8 = count == 4 ? index * 2 : index;
    return QLatin1StringView(points8[static_cast<std::size_t>(index8)]).toString();
}

QList<QPointF> breakWraps(const QList<QPointF> &points, double period)
{
    if (points.size() < 2)
        return points;
    const double half = period / 2.0;
    QList<QPointF> result;
    result.reserve(points.size() + 8);
    result.append(points.first());
    for (qsizetype i = 1; i < points.size(); ++i) {
        // NaN compares false: no break at an existing gap, as numpy.
        if (std::abs(points.at(i).y() - points.at(i - 1).y()) > half)
            result.append(QPointF(kNaN, kNaN));
        result.append(points.at(i));
    }
    return result;
}

QList<PlotTickLevel> plotTickLevels(double min, double max, double lengthPx)
{
    // pyqtgraph's AxisItem.tickSpacing() and tickValues().
    if (min > max)
        std::swap(min, max);
    const double dif = max - min;
    if (!(dif > 0.0) || !std::isfinite(dif) || !(lengthPx > 0.0))
        return {};
    const double optimalTickCount = std::max(2.0, std::log(lengthPx));
    const double optimalSpacing = dif / optimalTickCount;
    const double p10unit = std::pow(10.0, std::floor(std::log10(optimalSpacing)));
    const std::array<double, 5> intervals{1.0 * p10unit, 2.0 * p10unit, 10.0 * p10unit, 20.0 * p10unit,
                                          100.0 * p10unit};
    std::size_t minorIndex = 0;
    while (minorIndex < 2 && intervals[minorIndex + 1] <= optimalSpacing)
        ++minorIndex;
    QList<double> spacings{intervals[minorIndex + 2], intervals[minorIndex + 1]};
    // A third level when it is not too dense.
    const double minSpacing = std::min(lengthPx / 20.0, 30.0);
    const double maxTickCount = lengthPx / minSpacing;
    if (dif / intervals[minorIndex] <= maxTickCount)
        spacings.append(intervals[minorIndex]);

    QList<PlotTickLevel> levels;
    QList<double> all;
    for (double spacing : spacings) {
        PlotTickLevel level;
        level.spacing = spacing;
        const double start = std::ceil(min / spacing) * spacing;
        const auto count = static_cast<qsizetype>(std::min((max - start) / spacing + 1.0, 10000.0));
        for (qsizetype i = 0; i < count; ++i) {
            const double value = static_cast<double>(i) * spacing + start;
            if (value < min - spacing * 1e-9 || value > max + spacing * 1e-9)
                continue;
            // Leave out ticks a higher level already has.
            const bool duplicate = std::any_of(all.cbegin(), all.cend(), [&](double other) {
                return std::abs(other - value) <= spacing * 0.01;
            });
            if (!duplicate)
                level.values.append(value);
        }
        all.append(level.values);
        levels.append(level);
    }
    return levels;
}

QStringList plotTickStrings(const QList<double> &values, double spacing)
{
    // pyqtgraph's AxisItem.tickStrings() without SI scaling.
    const int places = std::max(0, static_cast<int>(std::ceil(-std::log10(spacing))));
    QStringList strings;
    strings.reserve(values.size());
    for (double value : values) {
        if (std::abs(value) < 0.001 || std::abs(value) >= 10000.0)
            strings.append(QString::number(value, 'g', 6));
        else
            strings.append(QString::number(value, 'f', places));
    }
    return strings;
}

} // namespace duifx

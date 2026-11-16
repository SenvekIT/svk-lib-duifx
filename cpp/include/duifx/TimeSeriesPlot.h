#pragma once

// Named series over time with a rolling window, for live sensor traces.
// Port of TimeSeriesPlot in duifx/gui/plot.py.
//
// Feed samples with append() / extend(), or hand over whole buffers with
// setData(). Samples outside the window are kept but not drawn; each series
// keeps at most maxSamples.
//
// With relativeTime the newest sample sits at x = 0 and the window runs to
// -window, so the axis reads "seconds ago" whatever the device clock says.
// "Newest" is the latest timestamp of this plot's series unless setNow() pins
// it -- which keeps several plots fed from one source aligned.
//
//     auto *plot = new duifx::TimeSeriesPlot({.title = "Accelerometer", .yLabel = "a", .yUnit = "g"});
//     plot->addSeries("x", {.label = "X"});
//     plot->addSeries("norm", {.label = "|a|", .color = duifx::color(duifx::Token::PlotMagnitude), .dashed = true});
//     plot->append(t, {{"x", ax}, {"norm", n}});
//
// A series can be a line, a dashed line or points, and can break at a
// wrap-around (a heading jumping from 359 to 0 degrees). The crosshair reads
// out every visible series at the cursor's time.

#include "duifx/Plot.h"

#include <optional>
#include <utility>

namespace duifx {

class TimeSeriesPlot : public PlotBase
{
    Q_OBJECT

public:
    struct Options
    {
        QString title{};
        /// Left axis, shown as "label [unit]".
        QString yLabel{};
        QString yUnit{};
        /// Bottom axis.
        QString xLabel = QStringLiteral("time");
        QString xUnit = QStringLiteral("s");
        /// Visible time span; nullopt shows everything and auto-ranges x.
        std::optional<double> window = 10.0;
        /// Buffer length per series.
        int maxSamples = kDefaultMaxSamples;
        /// Legend of the labelled series.
        bool legend = true;
        /// Time relative to the newest sample (see above).
        bool relativeTime = true;
        /// Crosshair reading out every series at the cursor's time.
        bool crosshair = true;
        /// Colours; defaultPlotStyle() when unset.
        std::optional<PlotStyle> style{};
        /// Minimum interval between redraws.
        int refreshMs = kDefaultPlotRefreshMs;
    };

    struct SeriesOptions
    {
        /// Legend entry; unset: the key, "" keeps it out of the legend.
        std::optional<QString> label{};
        /// Fixed colour; invalid: the next one of the style's cycle.
        QColor color{};
        /// Dashed line, e.g. for a derived quantity such as a magnitude.
        bool dashed = false;
        /// Points instead of a line.
        bool points = false;
        /// Line width, point size and opacity (0..255) overriding the style.
        std::optional<double> width{};
        std::optional<double> size{};
        std::optional<int> alpha{};
        /// Period of a wrapping quantity, e.g. 360 for a heading: the line
        /// breaks at a wrap instead of crossing the plot.
        std::optional<double> wrap{};
    };

    /// One value per series key, for append().
    using Values = QList<std::pair<QString, double>>;
    /// A block of values per series key, for extend().
    using Blocks = QList<std::pair<QString, QList<double>>>;

    explicit TimeSeriesPlot(QWidget *parent = nullptr);
    explicit TimeSeriesPlot(const Options &options, QWidget *parent = nullptr);

    // -- configuration -------------------------------------------------------

    /// Adds (or replaces) the series key.
    void addSeries(const QString &key);
    void addSeries(const QString &key, const SeriesOptions &options);

    void setYLabel(const QString &text, const QString &unit = {});

    /// Visible time span; nullopt shows everything.
    void setWindow(std::optional<double> seconds);
    std::optional<double> window() const;

    /// Fixes the y range, or auto-ranges it again without arguments.
    void setYRange(std::optional<double> low = std::nullopt, std::optional<double> high = std::nullopt);
    /// The fixed y range, nullopt while auto-ranged.
    std::optional<PlotRange> yRange() const;

    /// Pins the time the window ends at; nullopt follows the newest sample.
    void setNow(std::optional<double> t);
    std::optional<double> now() const;

    bool relativeTime() const;
    int maxSamples() const;

    /// Zooms and pans the x axis together with other.
    void linkX(TimeSeriesPlot *other);

    // -- reference lines -------------------------------------------------------

    /// A horizontal line at value, e.g. an expected magnitude. Hidden while
    /// value is unset; move it with setReference(). color invalid: the style's
    /// reference colour.
    void addReferenceLine(const QString &key, std::optional<double> value = std::nullopt, const QColor &color = {},
                          bool dashed = true);
    void setReference(const QString &key, std::optional<double> value);
    /// Value of the reference line key; nullopt while hidden or unknown.
    std::optional<double> reference(const QString &key) const;
    void removeReferenceLine(const QString &key);

    // -- data ------------------------------------------------------------------

    /// Adds one sample at time t; unknown keys get a new series with default
    /// looks.
    void append(double t, const Values &values);
    /// Adds one sample to the first series (a new "data" series if none).
    void append(double t, double value);
    /// Adds a block of samples: t and, per key, as many values. Throws
    /// std::invalid_argument (adding nothing) when a block differs in length.
    void extend(const QList<double> &t, const Blocks &values);
    /// Replaces the whole series key (created if new). Throws
    /// std::invalid_argument when t and y differ in length.
    void setData(const QString &key, const QList<double> &t, const QList<double> &y);
    /// The newest timestamp of any series; nullopt when empty.
    std::optional<double> latestTime() const;

    /// Returns to the configured view: the time window, and the y range set
    /// with setYRange() or else auto-ranged on the data.
    void resetView() override;

protected:
    void renderData() override;
    void addDefaultSeries(const QString &key) override;
    /// The cursor position, then each visible series at the nearest sample.
    QList<PlotReadoutLine> readoutLines(double x, double y) const override;

private:
    std::optional<double> m_window;
    int m_maxSamples;
    bool m_relative;
    std::optional<double> m_now;
    std::optional<PlotRange> m_yRange;
};

} // namespace duifx

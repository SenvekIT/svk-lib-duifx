#pragma once

// Aspect-locked XY point clouds with a reference circle, for magnetometer and
// similar calibration checks. Port of ScatterPlot in duifx/gui/plot.py.
//
// A magnetometer turned in every direction traces a circle; how round and how
// centred it is shows the remaining hard- and soft-iron error -- visible only
// when a circle looks like a circle, hence the locked aspect ratio.
// setReferenceCircle() draws the radius the data should have.
//
//     auto *plot = new duifx::ScatterPlot({.title = "XY", .xLabel = "x", .yLabel = "y", .unit = "µT"});
//     plot->append(mx, my);                          // the first (default) series
//     plot->setReferenceCircle(48.0);
//
// Each series keeps the newest maxPoints points.

#include "duifx/Plot.h"

#include <QHash>

#include <optional>

namespace duifx {

class ScatterPlot : public PlotBase
{
    Q_OBJECT

public:
    struct Options
    {
        QString title{};
        QString xLabel{};
        QString yLabel{};
        /// Unit of both axes.
        QString unit{};
        /// Buffer length per series.
        int maxPoints = kDefaultMaxPoints;
        /// Keep one unit on x as long as one unit on y.
        bool aspectLocked = true;
        /// Legend of the labelled series.
        bool legend = false;
        /// Crosshair reading out the cursor's x/y position.
        bool crosshair = true;
        std::optional<PlotStyle> style{};
        int refreshMs = kDefaultPlotRefreshMs;
    };

    struct SeriesOptions
    {
        /// Legend entry; unset: the key, "" keeps it out of the legend.
        std::optional<QString> label{};
        /// Fixed colour; invalid: the next one of the style's cycle.
        QColor color{};
        /// Point size and opacity (0..255) overriding the style.
        std::optional<double> size{};
        std::optional<int> alpha{};
        /// Connect the points instead: a trajectory rather than a cloud.
        bool line = false;
    };

    struct Circle
    {
        double radius = 0.0;
        QPointF center{};
    };

    explicit ScatterPlot(QWidget *parent = nullptr);
    explicit ScatterPlot(const Options &options, QWidget *parent = nullptr);

    /// Changes any of the axis labels or the shared unit; unset ones stay.
    void setAxisLabels(const std::optional<QString> &xLabel, const std::optional<QString> &yLabel = std::nullopt,
                       const std::optional<QString> &unit = std::nullopt);

    /// Adds (or replaces) the point series key.
    void addSeries(const QString &key);
    void addSeries(const QString &key, const SeriesOptions &options);

    int maxPoints() const;

    // -- data ------------------------------------------------------------------
    // A null key means the first series (a new unlabelled "data" one if none).

    void append(double x, double y, const QString &key = QString());
    /// Throws std::invalid_argument when xs and ys differ in length.
    void extend(const QList<double> &xs, const QList<double> &ys, const QString &key = QString());
    /// Replaces the points of key; only the newest maxPoints are kept.
    void setData(const QList<double> &xs, const QList<double> &ys, const QString &key = QString());

    // -- reference circle --------------------------------------------------------

    /// Draws a circle of radius around center; unset or <= 0 removes it.
    /// color invalid: the style's reference colour.
    void setReferenceCircle(std::optional<double> radius, QPointF center = {},
                            const QString &key = QStringLiteral("reference"), const QColor &color = {},
                            bool dashed = true);
    /// The reference circle key, or nullopt.
    std::optional<Circle> referenceCircle(const QString &key = QStringLiteral("reference")) const;

protected:
    void renderData() override;
    void addDefaultSeries(const QString &key) override;

private:
    int m_maxPoints;
    QHash<QString, Circle> m_circles;
};

} // namespace duifx

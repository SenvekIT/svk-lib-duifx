#pragma once

// Plot widgets for instrument views: style, helpers and the shared plot base.
// Port of duifx/gui/plot.py (PlotStyle, default_plot_style, axis_label,
// compass_point, link_x_axes, _break_wraps, _PlotBase). The widgets themselves
// are in TimeSeriesPlot.h, ScatterPlot.h, CompassDial.h and ThresholdBar.h.
//
// The widgets are data-agnostic: they know samples, series, ranges, labels and
// units, never where the numbers come from.
//
// Python draws the two plots with pyqtgraph. Qt Charts / Qt Graphs are GPL or
// commercial only, so here PlotBase draws itself with QPainter and reproduces
// exactly what plot.py uses of pyqtgraph: axes with pyqtgraph's tick spacing
// and labels, the grid, lines (solid, dashed, broken at NaN gaps) and point
// clouds, the legend (click an entry to hide its series), auto range with
// pyqtgraph's padding, a locked aspect ratio, linked x axes, wheel and drag
// zoom/pan (also per axis, over an axis), the "A" button, the rectangle zoom
// mode and the right-click menu.
//
// Feeding data only fills ring buffers and marks the plot dirty; a timer
// redraws at most every refreshMs, and not while the widget is hidden. Long
// lines are decimated to a min/max per pixel column, which looks the same.

#include "duifx/Colors.h"

#include <QColor>
#include <QList>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QWidget>

#include <memory>
#include <optional>

class QMenu;

namespace duifx {

struct PlotBasePrivate;
struct PlotSeries;

/// Redraw interval of the plots: a 10 Hz display refresh is plenty for the
/// eye and leaves the GUI thread free for everything else.
inline constexpr int kDefaultPlotRefreshMs = 100;
/// Default buffer length per time series.
inline constexpr int kDefaultMaxSamples = 20000;
/// Default buffer length per scatter series: a few thousand points already
/// show the shape of a cloud; more only cost redraw time.
inline constexpr int kDefaultMaxPoints = 3000;

// ---------------------------------------------------------------------------
// Style
// ---------------------------------------------------------------------------

/// All colours and stroke sizes of the plot widgets, in one place.
///
/// The defaults come from the DUIFx palette (Colors.h), resolved when a style
/// is created, so the active theme decides. Derive variants by copying:
///
///     duifx::PlotStyle style = duifx::defaultPlotStyle();
///     style.lineWidth = 2.0;
///     plot->setPlotStyle(style);
struct PlotStyle
{
    /// Plot canvas and dial face.
    QColor background = color(Token::PlotBackground);
    /// Axis lines and tick labels; the grid too, at gridAlpha.
    QColor axis = color(Token::PlotAxis);
    /// Axis labels, legend entries, dial labels and readout.
    QColor text = color(Token::PlotAxis);
    /// Plot titles.
    QColor title = color(Token::PlotAxis);
    /// Opacity of the grid lines, 0..1.
    double gridAlpha = 0.3;
    /// Colour cycle for series without an explicit colour (kPlotSeries order:
    /// x / y / z axis colours, then amber, pink and mint).
    QList<QColor> series = defaultSeries();
    /// Derived traces such as a magnitude |v|.
    QColor magnitude = color(Token::PlotMagnitude);
    /// Reference lines and circles (expected values, calibration radius).
    QColor reference = color(Token::PlotSeries4);
    /// Main needle of CompassDial.
    QColor accent = color(Token::PlotSeries4);
    /// Opacity of scatter points, 0..255: overlapping points build up density
    /// instead of a solid blob.
    int markerAlpha = 150;
    /// Opacity of the dial's uncertainty wedge, 0..255.
    int wedgeAlpha = 70;
    /// Line width in pixels.
    double lineWidth = 1.0;
    /// Diameter of scatter points in pixels.
    double markerSize = 3.0;
    /// ThresholdBar chunk when the gate is met.
    QColor ok = color(Token::StateOk);
    /// ThresholdBar chunk when the gate is missed.
    QColor warn = color(Token::StateWarning);
    /// Muted hint text (kept for parity with Python, which shows a hint when
    /// pyqtgraph is missing; the C++ plots always draw).
    QColor muted = color(Token::TextMuted);

    /// Colour of the index-th series, cycling through series.
    QColor seriesColor(int index) const;

    /// The palette's series cycle.
    static QList<QColor> defaultSeries();

    bool operator==(const PlotStyle &other) const = default;
};

/// The style widgets use when none is passed (created on first use).
PlotStyle defaultPlotStyle();

/// Replaces the style used by widgets created from now on; existing widgets
/// keep theirs (restyle them with setPlotStyle()).
void setDefaultPlotStyle(const PlotStyle &style);

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// "text [unit]", or just text without a unit. Units go in brackets and are
/// never SI-rescaled: a field in µT shown as "mT" helps nobody.
QString axisLabel(const QString &text, const QString &unit = {});

/// Name of the compass point nearest to heading (degrees, 0 = north).
/// points is 4, 8 or 16.
QString compassPoint(double heading, int points = 8);

/// Inserts NaN gaps where y wraps, so a line does not cross the plot: a jump
/// of more than half a period between neighbours is taken as a wrap
/// (359 -> 1 degrees), not as motion.
QList<QPointF> breakWraps(const QList<QPointF> &points, double period);

/// One level of axis ticks: major first, then minor.
struct PlotTickLevel
{
    double spacing = 0.0;
    QList<double> values{};
};

/// The tick levels pyqtgraph's AxisItem picks for [min, max] drawn over
/// lengthPx pixels: 1-2-10 steps around log(lengthPx) ticks, at most three
/// levels, without duplicates between levels.
QList<PlotTickLevel> plotTickLevels(double min, double max, double lengthPx);

/// Tick labels as pyqtgraph writes them: as many decimals as spacing needs,
/// "%g" for very small or large values.
QStringList plotTickStrings(const QList<double> &values, double spacing);

// ---------------------------------------------------------------------------
// Plot base
// ---------------------------------------------------------------------------

/// The four sides of a plot. Left and bottom are always shown; right and top
/// appear once they get a label.
enum class AxisSide { Left, Bottom, Right, Top };

/// A closed value range.
struct PlotRange
{
    double min = 0.0;
    double max = 1.0;

    double span() const { return max - min; }
    bool operator==(const PlotRange &other) const = default;
};

/// One line of the crosshair readout.
struct PlotReadoutLine
{
    QColor color{};
    QString text{};
};

/// Shared base of TimeSeriesPlot and ScatterPlot: series, references, view
/// range, axes, legend, crosshair, mouse interaction and throttled redraws.
class PlotBase : public QWidget
{
    Q_OBJECT

public:
    /// What a left drag does in the plot, as pyqtgraph's "Mouse Mode".
    enum class MouseMode {
        Pan,  ///< "3 button": left drag pans, right drag zooms
        Rect, ///< "1 button": left drag zooms into a rectangle
    };

    ~PlotBase() override;

    // -- style and labels -------------------------------------------------

    /// The PlotStyle in use.
    PlotStyle plotStyle() const;
    /// Restyles the plot and every series without an explicit colour.
    void setPlotStyle(const PlotStyle &style);

    QString title() const;
    void setTitle(const QString &title);

    /// Labels an axis "text [unit]". Labelling the right or top axis shows it.
    void setAxisLabel(AxisSide side, const QString &text, const QString &unit = {});
    /// The label as drawn, "text [unit]".
    QString axisLabelText(AxisSide side) const;
    bool isAxisShown(AxisSide side) const;

    // -- series -----------------------------------------------------------

    void removeSeries(const QString &key);
    /// Keys of the series, in the order they were added.
    QStringList seriesKeys() const;
    bool hasSeries(const QString &key) const;
    /// The colour series key is drawn in (explicit or from the style's cycle);
    /// invalid for an unknown key.
    QColor seriesColor(const QString &key) const;
    /// Legend labels in legend order (series with an empty label are left out).
    QStringList legendLabels() const;

    /// The points series key draws: the buffer after the plot is done with it
    /// (cut to the window, in relative time, NaN gaps at wraps). A pending
    /// redraw is brought forward, visible or not. Empty for an unknown key.
    QList<QPointF> drawnData(const QString &key);

    /// Empties every series, keeping the series themselves.
    void clear();

    /// Shows or hides a series, as clicking its legend entry does. A hidden
    /// series takes no part in auto range or readout.
    void setSeriesVisible(const QString &key, bool visible);
    bool isSeriesVisible(const QString &key) const;

    // -- view ----------------------------------------------------------------

    /// The visible ranges.
    PlotRange viewXRange() const;
    PlotRange viewYRange() const;
    /// Shows [min, max] plus padding (fraction of the span on each side);
    /// switches auto range off for that axis.
    void setViewXRange(double min, double max, double padding = 0.0);
    void setViewYRange(double min, double max, double padding = 0.0);

    /// Switches auto range on (and ranges at once) or off for axes.
    void setAutoRange(Qt::Orientations axes, bool enabled);
    void enableAutoRange(Qt::Orientations axes = Qt::Horizontal | Qt::Vertical) { setAutoRange(axes, true); }
    bool isAutoRange(Qt::Orientation axis) const;

    /// Fits the view to the data once, leaving auto range off ("View All").
    void viewAll();

    /// Keeps one unit on x as long as one unit on y.
    void setAspectLocked(bool locked);
    bool isAspectLocked() const;

    /// Which axes wheel and drag may change.
    void setMouseEnabled(Qt::Orientations axes);
    Qt::Orientations mouseEnabled() const;

    void setAxisInverted(Qt::Orientation axis, bool inverted);
    bool isAxisInverted(Qt::Orientation axis) const;

    void setMouseMode(MouseMode mode);
    MouseMode mouseMode() const;

    /// Returns to the plot's own view: auto-ranged on the data. Bound to a
    /// double-click into the plot and to "Reset view" in the right-click menu.
    virtual void resetView();

    // -- crosshair -----------------------------------------------------------

    /// Shows or hides the crosshair and its value readout under the mouse.
    /// The right-click menu has the same switch.
    void setCrosshair(bool enabled);
    bool crosshair() const;

    /// The readout the crosshair shows at a point in data coordinates.
    QList<PlotReadoutLine> crosshairReadout(QPointF dataPos) const;

    // -- geometry ------------------------------------------------------------

    /// The data area in widget pixels.
    QRectF viewRect() const;
    QPointF mapToData(QPointF widgetPos) const;
    QPointF mapFromData(QPointF dataPos) const;

    /// The right-click menu; applications may add their own actions.
    QMenu *contextMenu() const;

    // -- export ----------------------------------------------------------------

    /// Saves the plot as an image (format from the suffix).
    bool exportImage(const QString &path);
    /// Saves the drawn data as CSV: "<label>_x,<label>_y" columns per series.
    bool exportCsv(const QString &path);

    // -- redraw ----------------------------------------------------------------

    /// Marks the plot dirty; it redraws within refreshMs.
    void scheduleRedraw();
    /// Redraws now, visible or not.
    void redraw();
    /// Whether data waits for a redraw.
    bool isRedrawPending() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    /// The visible x or y range changed (data, zoom, pan, linked axis, resize).
    void viewRangeChanged();

protected:
    PlotBase(const QString &title, const std::optional<PlotStyle> &style, bool legend, bool crosshair, int refreshMs,
             QWidget *parent);

    /// Fills each series' drawn points from its buffer (and may set the view).
    virtual void renderData() = 0;
    /// Creates series key with default looks when data arrives for a new key.
    virtual void addDefaultSeries(const QString &key) = 0;
    /// Readout next to the crosshair; default: the cursor position.
    virtual QList<PlotReadoutLine> readoutLines(double x, double y) const;
    /// "label: value unit" for a cursor coordinate on axis side.
    QString coordinate(AxisSide side, double value, const QString &name) const;

    /// Adds (or replaces) series key at the end of the colour cycle and legend;
    /// the caller sets its looks. label nullopt: the key.
    PlotSeries &insertSeries(const QString &key, const std::optional<QString> &label, qsizetype capacity);
    /// Series key, created with default looks if new; a null key means the
    /// first series, or a new one named "data".
    PlotSeries &seriesFor(const QString &key);
    /// Replaces the points of series key; throws std::invalid_argument when
    /// xs and ys differ in length.
    void setSeriesData(const QString &key, const QList<double> &xs, const QList<double> &ys);
    /// Series looks or reference items changed: restyle and repaint.
    void itemsChanged();

    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void changeEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

    std::unique_ptr<PlotBasePrivate> d;

private:
    friend struct PlotBasePrivate;
    friend void linkXAxes(const QList<PlotBase *> &plots);
};

/// Links the x axes of plots to the first one: zoom and pan move all.
void linkXAxes(const QList<PlotBase *> &plots);

} // namespace duifx

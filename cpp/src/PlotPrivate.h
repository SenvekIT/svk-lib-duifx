#pragma once

// Internals of PlotBase shared by Plot*.cpp, TimeSeriesPlot.cpp and
// ScatterPlot.cpp: ring buffers, series, reference items and view state.

#include "duifx/Plot.h"

#include <QPixmap>
#include <QPointer>
#include <QTimer>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <vector>

class QAction;
class QMenu;
class QPainter;

namespace duifx {

inline constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();

/// Fixed-capacity ring of points, oldest first: the deque(maxlen=...) of the
/// Python series. Grows on demand up to its capacity, then overwrites the
/// oldest point, so a running plot never reallocates.
class PointRing
{
public:
    explicit PointRing(qsizetype capacity = 0)
        : m_capacity(std::max<qsizetype>(0, capacity))
    {
    }

    qsizetype capacity() const { return m_capacity; }
    qsizetype size() const { return static_cast<qsizetype>(m_buffer.size()); }
    bool isEmpty() const { return m_buffer.empty(); }

    void clear()
    {
        m_buffer.clear();
        m_head = 0;
    }

    void push(QPointF point)
    {
        if (m_capacity == 0)
            return;
        if (size() < m_capacity) {
            m_buffer.push_back(point);
            return;
        }
        m_buffer[static_cast<std::size_t>(m_head)] = point;
        m_head = (m_head + 1) % m_capacity;
    }

    /// The i-th point, oldest first.
    const QPointF &at(qsizetype i) const
    {
        const qsizetype n = size();
        qsizetype j = m_head + i;
        if (j >= n)
            j -= n;
        return m_buffer[static_cast<std::size_t>(j)];
    }

    const QPointF &back() const { return at(size() - 1); }

    /// Calls f(point) for every point, oldest first.
    template<typename F>
    void forEach(F &&f) const
    {
        const auto head = static_cast<std::size_t>(m_head);
        for (std::size_t i = head; i < m_buffer.size(); ++i)
            f(m_buffer[i]);
        for (std::size_t i = 0; i < head; ++i)
            f(m_buffer[i]);
    }

private:
    std::vector<QPointF> m_buffer;
    qsizetype m_capacity = 0;
    qsizetype m_head = 0; ///< index of the oldest point once full
};

/// One named series: its look, its buffer and what it draws.
struct PlotSeries
{
    QString key;
    QString label;          ///< legend entry; empty keeps it out of the legend
    QColor color;           ///< invalid: the style's cycle at index
    int index = 0;          ///< position in the colour cycle
    int order = 0;          ///< drawing order among series and references
    bool dashed = false;
    bool points = false;    ///< points instead of a line
    std::optional<double> width;
    std::optional<double> size;
    std::optional<int> alpha;
    std::optional<double> wrap;
    bool visible = true;
    PointRing data;
    QList<QPointF> drawn;   ///< what the plot draws, see drawnData()

    // Cached point sprite (one antialiased disc, blitted per point).
    QPixmap sprite;
    QColor spriteColor;
    double spriteSize = 0.0;
    qreal spriteDpr = 0.0;
};

/// A reference item: a horizontal line or a curve (circle) in a fixed style.
struct PlotReference
{
    enum class Kind { HLine, Curve };

    QString key;
    Kind kind = Kind::HLine;
    QColor color; ///< invalid: style.reference
    bool dashed = true;
    int order = 0;
    std::optional<double> value; ///< HLine: y; hidden while unset
    QList<QPointF> points;       ///< Curve
};

/// Plots whose x axes move together.
struct XLinkGroup
{
    QList<QPointer<PlotBase>> members;
    bool propagating = false;
};

struct PlotLayout
{
    QRectF view;
    QRectF title;
    QRectF axis[4]; ///< by AxisSide
    QRectF autoButton;
};

struct PlotBasePrivate
{
    explicit PlotBasePrivate(PlotBase *owner)
        : q(owner)
    {
    }

    PlotBase *q;
    PlotStyle style;
    QString title;
    QString labelText[4];
    QString labelUnit[4];
    bool axisShown[4] = {true, true, false, false};

    std::vector<std::unique_ptr<PlotSeries>> series;
    std::vector<PlotReference> references;
    int nextOrder = 0;

    bool legend = true;
    bool crosshair = true;
    bool dirty = false;
    QTimer *timer = nullptr;

    // View state as pyqtgraph's ViewBox: the requested (target) ranges and
    // the shown ones, which differ only by the aspect lock.
    PlotRange target[2];
    PlotRange view[2];
    QSizeF viewPx; ///< pixel size view[] was computed for
    bool autoRange[2] = {true, true};
    bool mouseOn[2] = {true, true};
    bool inverted[2] = {false, false};
    bool aspectLocked = false;
    PlotBase::MouseMode mouseMode = PlotBase::MouseMode::Pan;
    std::shared_ptr<XLinkGroup> xLink;

    // Layout: tick label widths of the vertical axes, with pyqtgraph's
    // hysteresis (grow at once, shrink only by more than 10 px).
    mutable double textWidth[4] = {0, 0, 0, 0};
    mutable bool layoutValid = false;
    mutable PlotLayout layout;

    // Painting.
    QPixmap cache;
    bool cacheValid = false;
    struct LegendRow
    {
        QRectF rect;
        QString key;
    };
    QList<LegendRow> legendRows;

    // Mouse.
    std::optional<QPointF> mousePos;
    Qt::MouseButton dragButton = Qt::NoButton;
    QPointF pressPos;
    QPointF lastPos;
    bool dragging = false;
    int dragAxis = -1; ///< -1 both, 0 x only, 1 y only
    QString legendPressKey;
    bool pressOnLegend = false;
    bool pressOnAutoButton = false;

    // Menu.
    QMenu *menu = nullptr;
    QAction *autoAction[2] = {nullptr, nullptr};
    QAction *mouseAction[2] = {nullptr, nullptr};
    QAction *invertAction[2] = {nullptr, nullptr};
    QAction *panModeAction = nullptr;
    QAction *rectModeAction = nullptr;
    QAction *crosshairAction = nullptr;

    PlotSeries *find(const QString &key) const
    {
        for (const auto &s : series) {
            if (s->key == key)
                return s.get();
        }
        return nullptr;
    }

    PlotReference *findReference(const QString &key)
    {
        for (PlotReference &r : references) {
            if (r.key == key)
                return &r;
        }
        return nullptr;
    }

    void removeReference(const QString &key)
    {
        std::erase_if(references, [&](const PlotReference &r) { return r.key == key; });
    }

    QColor seriesColor(const PlotSeries &s) const { return s.color.isValid() ? s.color : style.seriesColor(s.index); }

    // Implemented in PlotBase.cpp.
    const PlotLayout &ensureLayout() const;
    /// Labels, title, font or size changed.
    void invalidateLayout();
    /// The cached picture is stale; repaint.
    void invalidate();
    /// Auto-range padding as a fraction of the span: pyqtgraph's
    /// ViewBox.suggestPadding(), 1/sqrt(pixels) within 0.02..0.1.
    double suggestPadding(int axis) const;
    /// Data bounds of the visible items, per axis (nullopt without data), and
    /// the largest stroke in pixels.
    struct Bounds
    {
        std::array<std::optional<PlotRange>, 2> range;
        double pixelPad = 0.0;
    };
    Bounds childrenBounds() const;
    /// bounds of one axis plus auto-range padding and the stroke.
    PlotRange padded(int axis, const PlotRange &bounds, double pixelPad) const;
    /// Sets the target range of one axis (keeping the span for an empty
    /// range); optionally switches its auto range off.
    void setTarget(int axis, PlotRange range, bool disableAuto);
    /// Ranges every auto-ranged axis on the data, then updateViewRange().
    void updateAutoRange();
    /// Derives the shown ranges from the targets (aspect lock), signals and
    /// passes a changed x range on to linked plots.
    void updateViewRange();
    void scaleBy(std::optional<double> sx, std::optional<double> sy, QPointF center);
    void translateBy(std::optional<double> dx, std::optional<double> dy);
    bool autoButtonVisible() const;
    void buildMenu();
    void syncMenu();
    void paintPlot(QPainter &painter);
    void paintOverlay(QPainter &painter);
};

/// Python's "value or default": unset or zero falls back.
inline double orStyle(const std::optional<double> &value, double fallback)
{
    return value && *value != 0.0 ? *value : fallback;
}

/// Python's "{value:.4g}".
inline QString formatG(double value, int precision = 4)
{
    return QString::number(value, 'g', precision);
}

} // namespace duifx

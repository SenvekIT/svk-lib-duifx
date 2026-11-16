// PlotBase: the shared plot widget of duifx/gui/plot.py (_PlotBase), drawn
// with QPainter instead of pyqtgraph. Only what plot.py uses of pyqtgraph is
// reproduced; numbers quoted from pyqtgraph (tick spacing, padding, wheel
// factor, legend offset, ...) keep the two versions looking alike.

#include "duifx/Plot.h"

#include "PlotPrivate.h"

#include <QActionGroup>
#include <QApplication>
#include <QContextMenuEvent>
#include <QFile>
#include <QFileDialog>
#include <QFontMetricsF>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QTextStream>
#include <QWheelEvent>

#include <stdexcept>
#include <utility>

namespace duifx {

namespace {

constexpr int kX = 0;
constexpr int kY = 1;
/// The plot layout's outer margin.
constexpr double kOuterMargin = 2.0;
/// pyqtgraph's tickTextOffset: vertical axes 5 px, horizontal ones 2 px.
constexpr double kTickTextOffsetV = 5.0;
constexpr double kTickTextOffsetH = 2.0;
/// addLegend(offset=(10, 10)) in plot.py.
constexpr double kLegendOffset = 10.0;
/// pyqtgraph's legend: 20 px samples, 25 px column spacing plus the label's
/// 4 px text margin.
constexpr double kSampleSize = 20.0;
constexpr double kLegendGap = 29.0;
/// The "A" (auto range) button in the corner between the axes.
constexpr double kAutoButtonSize = 14.0;

constexpr int sideIndex(AxisSide side)
{
    return static_cast<int>(side);
}

bool finite(QPointF p)
{
    return std::isfinite(p.x()) && std::isfinite(p.y());
}

QFont titleFont(const QFont &base)
{
    QFont font(base);
    font.setPointSizeF(11.0); // pyqtgraph's PlotItem title
    return font;
}

QFont legendFont(const QFont &base)
{
    QFont font(base);
    font.setPointSizeF(9.0); // pyqtgraph's LegendItem labelTextSize
    return font;
}

/// Height an axis label takes: a QGraphicsTextItem (4 px margins) at 80 %, as
/// pyqtgraph's AxisItem sizes it.
double labelBand(const QFontMetricsF &fm)
{
    return (fm.height() + 8.0) * 0.8;
}

QColor withAlpha(QColor color, int alpha)
{
    color.setAlpha(std::clamp(alpha, 0, 255));
    return color;
}

QString rstrip(QString text)
{
    while (text.endsWith(QLatin1Char(' ')))
        text.chop(1);
    return text;
}

/// Data <-> widget pixels for the current view.
struct Mapper
{
    double ax = 1, bx = 0, ay = 1, by = 0;

    Mapper(const QRectF &view, const PlotRange &xr, const PlotRange &yr, bool invertX, bool invertY)
    {
        const double sx = xr.span() != 0.0 ? view.width() / xr.span() : 0.0;
        const double sy = yr.span() != 0.0 ? view.height() / yr.span() : 0.0;
        ax = invertX ? -sx : sx;
        bx = invertX ? view.right() + xr.min * sx : view.left() - xr.min * sx;
        ay = invertY ? sy : -sy;
        by = invertY ? view.top() - yr.min * sy : view.bottom() + yr.min * sy;
    }

    double x(double v) const { return ax * v + bx; }
    double y(double v) const { return ay * v + by; }
    QPointF map(QPointF p) const { return {x(p.x()), y(p.y())}; }
    QPointF unmap(QPointF p) const
    {
        return {ax != 0.0 ? (p.x() - bx) / ax : 0.0, ay != 0.0 ? (p.y() - by) / ay : 0.0};
    }
};

Mapper mapperOf(const PlotBasePrivate &d)
{
    return Mapper(d.ensureLayout().view, d.view[kX], d.view[kY], d.inverted[kX], d.inverted[kY]);
}

/// A tick label to draw: value, text and size (height trimmed to 80 %, as
/// pyqtgraph measures it).
struct TickLabel
{
    double value;
    QString text;
    QSizeF size;
};

/// The tick labels pyqtgraph draws: the major level always, finer levels
/// while the text fills little enough of the axis (its textFillLimits).
QList<TickLabel> tickLabels(const QList<PlotTickLevel> &levels, double lengthPx, bool vertical,
                            const QFontMetricsF &fm)
{
    static constexpr std::array<std::pair<int, double>, 4> fillLimits{
        {{0, 0.8}, {2, 0.6}, {4, 0.4}, {6, 0.2}}};
    QList<TickLabel> result;
    double textSize = 0.0;
    const qsizetype levelCount = std::min<qsizetype>(levels.size(), 3);
    for (qsizetype i = 0; i < levelCount; ++i) {
        const PlotTickLevel &level = levels.at(i);
        if (level.values.isEmpty())
            continue;
        const QStringList strings = plotTickStrings(level.values, level.spacing);
        QList<TickLabel> current;
        double size = 0.0;
        for (qsizetype j = 0; j < strings.size(); ++j) {
            const QSizeF extent(fm.horizontalAdvance(strings.at(j)), fm.height() * 0.8);
            current.append({level.values.at(j), strings.at(j), extent});
            size += vertical ? extent.height() : extent.width();
        }
        const double total = textSize + size;
        if (i > 0 && lengthPx > 0) {
            const double ratio = total / lengthPx;
            const bool crowded = std::any_of(fillLimits.cbegin(), fillLimits.cend(), [&](const auto &limit) {
                return result.size() >= limit.first && ratio >= limit.second;
            });
            if (crowded)
                break;
        }
        textSize = total;
        result.append(current);
    }
    return result;
}

/// Liang-Barsky: clips the segment a-b to rect; false when it misses.
bool clipSegment(QPointF &a, QPointF &b, bool &aClipped, bool &bClipped, const QRectF &rect)
{
    const double dx = b.x() - a.x();
    const double dy = b.y() - a.y();
    double t0 = 0.0;
    double t1 = 1.0;
    const auto clip = [&](double p, double q) {
        if (p == 0.0)
            return q >= 0.0;
        const double t = q / p;
        if (p < 0.0) {
            if (t > t1)
                return false;
            t0 = std::max(t0, t);
        } else {
            if (t < t0)
                return false;
            t1 = std::min(t1, t);
        }
        return true;
    };
    if (!clip(-dx, a.x() - rect.left()) || !clip(dx, rect.right() - a.x()) || !clip(-dy, a.y() - rect.top())
        || !clip(dy, rect.bottom() - a.y()))
        return false;
    const QPointF origin = a;
    aClipped = t0 > 0.0;
    bClipped = t1 < 1.0;
    if (aClipped)
        a = origin + QPointF(dx * t0, dy * t0);
    if (bClipped)
        b = origin + QPointF(dx * t1, dy * t1);
    return true;
}

/// Draws a polyline in widget pixels, broken at gaps, clipped to the view and
/// decimated: the points falling into one pixel column are reduced to the
/// first, the lowest, the highest and the last, which draws the same pixels.
/// That keeps tens of thousands of samples cheap at any zoom.
class PolylineDrawer
{
public:
    PolylineDrawer(QPainter &painter, const QRectF &view, std::vector<QPointF> &buffer)
        : m_painter(painter)
        , m_clip(view.adjusted(-2.0, -2.0, 2.0, 2.0))
        , m_poly(buffer)
    {
        m_poly.clear();
    }

    void add(QPointF p)
    {
        const double column = std::floor(p.x());
        if (m_count > 0 && column == m_column) {
            ++m_seq;
            if (p.y() < m_low.y()) {
                m_low = p;
                m_lowSeq = m_seq;
            }
            if (p.y() > m_high.y()) {
                m_high = p;
                m_highSeq = m_seq;
            }
            m_last = p;
            m_lastSeq = m_seq;
            ++m_count;
            return;
        }
        flushColumn();
        m_column = column;
        m_first = m_low = m_high = m_last = p;
        m_seq = m_lowSeq = m_highSeq = m_lastSeq = 0;
        m_count = 1;
    }

    /// A gap (NaN): the line stops here.
    void gap()
    {
        flushColumn();
        finishPolyline();
        m_hasPrev = false;
    }

    void finish() { gap(); }

private:
    void flushColumn()
    {
        if (m_count == 0)
            return;
        emitPoint(m_first);
        if (m_count > 1) {
            // Lowest and highest in the order they came, then the last.
            const bool lowFirst = m_lowSeq <= m_highSeq;
            const QPointF &a = lowFirst ? m_low : m_high;
            const QPointF &b = lowFirst ? m_high : m_low;
            const int aSeq = lowFirst ? m_lowSeq : m_highSeq;
            const int bSeq = lowFirst ? m_highSeq : m_lowSeq;
            if (aSeq != 0)
                emitPoint(a);
            if (bSeq != 0 && bSeq != aSeq && bSeq != m_lastSeq)
                emitPoint(b);
            if (m_lastSeq != aSeq || aSeq == 0)
                emitPoint(m_last);
        }
        m_count = 0;
    }

    void emitPoint(QPointF p)
    {
        if (!m_hasPrev) {
            m_prev = p;
            m_hasPrev = true;
            m_open = false;
            return;
        }
        QPointF a = m_prev;
        QPointF b = p;
        bool aClipped = false;
        bool bClipped = false;
        if (!clipSegment(a, b, aClipped, bClipped, m_clip)) {
            finishPolyline();
        } else {
            if (!m_open || aClipped) {
                finishPolyline();
                m_poly.push_back(a);
            }
            m_poly.push_back(b);
            m_open = !bClipped;
            if (bClipped)
                finishPolyline();
        }
        m_prev = p;
    }

    void finishPolyline()
    {
        if (m_poly.size() >= 2)
            m_painter.drawPolyline(m_poly.data(), static_cast<int>(m_poly.size()));
        m_poly.clear();
        m_open = false;
    }

    QPainter &m_painter;
    QRectF m_clip;
    std::vector<QPointF> &m_poly;
    QPointF m_prev;
    bool m_hasPrev = false;
    bool m_open = false;

    double m_column = 0.0;
    int m_count = 0;
    int m_seq = 0;
    QPointF m_first, m_low, m_high, m_last;
    int m_lowSeq = 0, m_highSeq = 0, m_lastSeq = 0;
};

/// Shared by all plots: one scratch buffer for the polylines.
std::vector<QPointF> &polylineBuffer()
{
    static std::vector<QPointF> buffer;
    return buffer;
}

void drawCurve(QPainter &painter, const QList<QPointF> &points, const Mapper &mapper, const QRectF &view)
{
    PolylineDrawer drawer(painter, view, polylineBuffer());
    for (const QPointF &p : points) {
        if (finite(p))
            drawer.add(mapper.map(p));
        else
            drawer.gap();
    }
    drawer.finish();
}

/// One antialiased disc, blitted per point: pyqtgraph's symbol atlas.
const QPixmap &pointSprite(PlotSeries &series, const QColor &color, double size, qreal dpr)
{
    if (series.sprite.isNull() || series.spriteColor != color || series.spriteSize != size || series.spriteDpr != dpr) {
        const int side = static_cast<int>(std::ceil(size)) + 2;
        QPixmap sprite(QSize(side, side) * dpr);
        sprite.setDevicePixelRatio(dpr);
        sprite.fill(Qt::transparent);
        QPainter p(&sprite);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawEllipse(QPointF(side / 2.0, side / 2.0), size / 2.0, size / 2.0);
        p.end();
        series.sprite = sprite;
        series.spriteColor = color;
        series.spriteSize = size;
        series.spriteDpr = dpr;
    }
    return series.sprite;
}

QPen cosmeticPen(const QColor &color, double width, bool dashed = false)
{
    QPen pen(color, width, dashed ? Qt::DashLine : Qt::SolidLine);
    pen.setCosmetic(true);
    return pen;
}

} // namespace

// --- PlotBasePrivate: layout, view state ----------------------------------------

const PlotLayout &PlotBasePrivate::ensureLayout() const
{
    if (layoutValid)
        return layout;
    const QFont font = q->font();
    const QFontMetricsF fm(font);
    const double width = q->width();
    const double height = q->height();
    const double m = kOuterMargin;
    const double textHeight = fm.height() * 0.8;
    const double band = labelBand(fm);
    const auto hasLabel = [&](AxisSide side) {
        return !labelText[sideIndex(side)].isEmpty() || !labelUnit[sideIndex(side)].isEmpty();
    };
    const auto shown = [&](AxisSide side) { return axisShown[sideIndex(side)]; };

    const double titleHeight = title.isEmpty() ? 0.0 : QFontMetricsF(titleFont(font)).height() + 8.0;
    const double topHeight =
        shown(AxisSide::Top) ? textHeight + kTickTextOffsetH + (hasLabel(AxisSide::Top) ? band : 0.0) : 0.0;
    const double bottomHeight = textHeight + kTickTextOffsetH + (hasLabel(AxisSide::Bottom) ? band : 0.0);
    const double viewTop = m + titleHeight + topHeight;
    const double viewHeight = std::max(0.0, height - m - bottomHeight - viewTop);

    // Vertical axes are as wide as their tick labels, with pyqtgraph's
    // hysteresis so the plot does not twitch while the numbers change.
    double needed = 0.0;
    for (const TickLabel &label : tickLabels(plotTickLevels(view[kY].min, view[kY].max, viewHeight), viewHeight,
                                             true, fm))
        needed = std::max(needed, label.size.width());
    for (AxisSide side : {AxisSide::Left, AxisSide::Right}) {
        double &current = textWidth[sideIndex(side)];
        if (needed > current || needed < current - 10.0)
            current = std::ceil(needed);
    }
    const double leftWidth =
        textWidth[sideIndex(AxisSide::Left)] + kTickTextOffsetV + (hasLabel(AxisSide::Left) ? band : 0.0);
    const double rightWidth = shown(AxisSide::Right) ? textWidth[sideIndex(AxisSide::Right)] + kTickTextOffsetV
                                                           + (hasLabel(AxisSide::Right) ? band : 0.0)
                                                     : 0.0;

    PlotLayout result;
    result.title = QRectF(0.0, m, width, titleHeight);
    result.view = QRectF(m + leftWidth, viewTop, std::max(0.0, width - 2.0 * m - leftWidth - rightWidth), viewHeight);
    const QRectF &v = result.view;
    result.axis[sideIndex(AxisSide::Left)] = QRectF(m, v.top(), leftWidth, v.height());
    result.axis[sideIndex(AxisSide::Right)] = QRectF(v.right(), v.top(), rightWidth, v.height());
    result.axis[sideIndex(AxisSide::Bottom)] = QRectF(v.left(), v.bottom(), v.width(), bottomHeight);
    result.axis[sideIndex(AxisSide::Top)] = QRectF(v.left(), v.top() - topHeight, v.width(), topHeight);
    result.autoButton = QRectF(m, height - m - kAutoButtonSize, kAutoButtonSize, kAutoButtonSize);
    layout = result;
    layoutValid = true;
    return layout;
}

void PlotBasePrivate::invalidateLayout()
{
    layoutValid = false;
    invalidate();
}

void PlotBasePrivate::invalidate()
{
    cacheValid = false;
    q->update();
}

double PlotBasePrivate::suggestPadding(int axis) const
{
    const QRectF &v = ensureLayout().view;
    const double length = axis == kX ? v.width() : v.height();
    if (length <= 0.0)
        return 0.02;
    return std::clamp(1.0 / std::sqrt(length), 0.02, 0.1);
}

PlotBasePrivate::Bounds PlotBasePrivate::childrenBounds() const
{
    Bounds bounds;
    const auto extend = [&](int axis, double value) {
        if (!std::isfinite(value))
            return;
        auto &b = bounds.range[static_cast<std::size_t>(axis)];
        if (!b)
            b = PlotRange{value, value};
        else
            b = PlotRange{std::min(b->min, value), std::max(b->max, value)};
    };
    for (const auto &s : series) {
        if (!s->visible || s->drawn.isEmpty())
            continue;
        for (const QPointF &p : s->drawn) {
            extend(kX, p.x());
            extend(kY, p.y());
        }
        // pyqtgraph pads items by their stroke: 0.7072 x the symbol or pen width.
        const double extent = s->points ? orStyle(s->size, style.markerSize) : orStyle(s->width, style.lineWidth);
        bounds.pixelPad = std::max(bounds.pixelPad, extent * 0.7072);
    }
    for (const PlotReference &r : references) {
        if (r.kind == PlotReference::Kind::HLine) {
            // A horizontal line counts for y only (InfiniteLine.dataBounds).
            if (r.value)
                extend(kY, *r.value);
        } else {
            for (const QPointF &p : r.points) {
                extend(kX, p.x());
                extend(kY, p.y());
            }
        }
    }
    return bounds;
}

PlotRange PlotBasePrivate::padded(int axis, const PlotRange &bounds, double pixelPad) const
{
    const QRectF &v = ensureLayout().view;
    const double px = axis == kX ? v.width() : v.height();
    const double fraction = suggestPadding(axis);
    double pad = bounds.span() * fraction;
    // The stroke in data units of the range being set (pyqtgraph takes the
    // current view's, which would make the result depend on the zoom).
    if (px > 0.0)
        pad += pixelPad * bounds.span() * (1.0 + 2.0 * fraction) / px;
    return PlotRange{bounds.min - pad, bounds.max + pad};
}

void PlotBasePrivate::setTarget(int axis, PlotRange range, bool disableAuto)
{
    if (!std::isfinite(range.min) || !std::isfinite(range.max))
        return;
    if (range.min > range.max)
        std::swap(range.min, range.max);
    if (range.min == range.max) {
        // An empty range keeps the previous scale (pyqtgraph's setRange).
        double span = view[axis].span();
        if (span == 0.0)
            span = 1.0;
        range = PlotRange{range.min - span * 0.5, range.max + span * 0.5};
    }
    target[axis] = range;
    if (disableAuto)
        autoRange[axis] = false;
}

void PlotBasePrivate::updateAutoRange()
{
    if (!autoRange[kX] && !autoRange[kY]) {
        updateViewRange();
        return;
    }
    const Bounds bounds = childrenBounds();
    // The new y range may change the tick label width and with it the view
    // width the padding depends on: range again until the layout holds.
    for (int pass = 0; pass < 3; ++pass) {
        const QSizeF used = ensureLayout().view.size();
        for (int axis : {kX, kY}) {
            const auto &b = bounds.range[static_cast<std::size_t>(axis)];
            if (autoRange[axis] && b)
                setTarget(axis, padded(axis, *b, bounds.pixelPad), false);
        }
        updateViewRange();
        if (ensureLayout().view.size() == used)
            break;
    }
}

void PlotBasePrivate::updateViewRange()
{
    const QRectF &v = ensureLayout().view;
    PlotRange shown[2] = {target[kX], target[kY]};
    if (aspectLocked && v.width() > 0.0 && v.height() > 0.0) {
        // Expand (never shrink) one axis so a unit is as long on both.
        const double unitsPerPx = std::max(shown[kX].span() / v.width(), shown[kY].span() / v.height());
        for (int axis : {kX, kY}) {
            const double centre = (shown[axis].min + shown[axis].max) / 2.0;
            const double half = unitsPerPx * (axis == kX ? v.width() : v.height()) / 2.0;
            shown[axis] = PlotRange{centre - half, centre + half};
        }
    }
    viewPx = v.size();
    const bool xChanged = !(shown[kX] == view[kX]);
    const bool yChanged = !(shown[kY] == view[kY]);
    if (!xChanged && !yChanged)
        return;
    view[kX] = shown[kX];
    view[kY] = shown[kY];
    // The y tick labels may need another width.
    layoutValid = false;
    invalidate();
    emit q->viewRangeChanged();
    if (xChanged && xLink && !xLink->propagating) {
        xLink->propagating = true;
        const auto members = xLink->members;
        for (const QPointer<PlotBase> &member : members) {
            if (!member || member == q)
                continue;
            PlotBasePrivate &other = *member->d;
            other.target[kX] = view[kX];
            other.autoRange[kX] = false;
            other.updateViewRange();
        }
        xLink->propagating = false;
    }
}

void PlotBasePrivate::scaleBy(std::optional<double> sx, std::optional<double> sy, QPointF center)
{
    if (aspectLocked && (sx || sy)) {
        const double s = sy ? *sy : *sx;
        sx = s;
        sy = s;
    }
    const std::optional<double> scale[2] = {sx, sy};
    const double centre[2] = {center.x(), center.y()};
    for (int axis : {kX, kY}) {
        if (!scale[axis] || !std::isfinite(*scale[axis]))
            continue;
        const double s = *scale[axis];
        const double c = centre[axis];
        setTarget(axis, PlotRange{c + (target[axis].min - c) * s, c + (target[axis].max - c) * s}, true);
    }
    updateViewRange();
}

void PlotBasePrivate::translateBy(std::optional<double> dx, std::optional<double> dy)
{
    const std::optional<double> shift[2] = {dx, dy};
    for (int axis : {kX, kY}) {
        if (!shift[axis])
            continue;
        setTarget(axis, PlotRange{target[axis].min + *shift[axis], target[axis].max + *shift[axis]}, true);
    }
    updateViewRange();
}

bool PlotBasePrivate::autoButtonVisible() const
{
    return mousePos.has_value() && !(autoRange[kX] && autoRange[kY]);
}

// --- PlotBasePrivate: menu --------------------------------------------------------

void PlotBasePrivate::buildMenu()
{
    // pyqtgraph's view menu (the parts that apply), then plot.py's own entries.
    menu = new QMenu(q);
    QObject::connect(menu->addAction(PlotBase::tr("View All")), &QAction::triggered, q, [this] { q->viewAll(); });
    const char *axisTitles[2] = {QT_TR_NOOP("X axis"), QT_TR_NOOP("Y axis")};
    for (int axis : {kX, kY}) {
        QMenu *sub = menu->addMenu(PlotBase::tr(axisTitles[axis]));
        const Qt::Orientation orientation = axis == kX ? Qt::Horizontal : Qt::Vertical;
        autoAction[axis] = sub->addAction(PlotBase::tr("Auto Range"));
        autoAction[axis]->setCheckable(true);
        QObject::connect(autoAction[axis], &QAction::triggered, q,
                         [this, orientation](bool on) { q->setAutoRange(orientation, on); });
        invertAction[axis] = sub->addAction(PlotBase::tr("Invert Axis"));
        invertAction[axis]->setCheckable(true);
        QObject::connect(invertAction[axis], &QAction::triggered, q,
                         [this, orientation](bool on) { q->setAxisInverted(orientation, on); });
        mouseAction[axis] = sub->addAction(PlotBase::tr("Mouse Enabled"));
        mouseAction[axis]->setCheckable(true);
        QObject::connect(mouseAction[axis], &QAction::triggered, q, [this, axis](bool on) {
            mouseOn[axis] = on;
        });
    }
    QMenu *modes = menu->addMenu(PlotBase::tr("Mouse Mode"));
    auto *group = new QActionGroup(modes);
    panModeAction = modes->addAction(PlotBase::tr("3 button"));
    rectModeAction = modes->addAction(PlotBase::tr("1 button"));
    for (QAction *action : {panModeAction, rectModeAction}) {
        action->setCheckable(true);
        group->addAction(action);
    }
    QObject::connect(panModeAction, &QAction::triggered, q, [this] { q->setMouseMode(PlotBase::MouseMode::Pan); });
    QObject::connect(rectModeAction, &QAction::triggered, q, [this] { q->setMouseMode(PlotBase::MouseMode::Rect); });
    QMenu *exportMenu = menu->addMenu(PlotBase::tr("Export"));
    QObject::connect(exportMenu->addAction(PlotBase::tr("Image...")), &QAction::triggered, q, [this] {
        const QString path = QFileDialog::getSaveFileName(q, PlotBase::tr("Export image"), QString(),
                                                          PlotBase::tr("Images (*.png *.jpg *.bmp)"));
        if (!path.isEmpty())
            q->exportImage(path);
    });
    QObject::connect(exportMenu->addAction(PlotBase::tr("CSV...")), &QAction::triggered, q, [this] {
        const QString path = QFileDialog::getSaveFileName(q, PlotBase::tr("Export data"), QString(),
                                                          PlotBase::tr("CSV files (*.csv)"));
        if (!path.isEmpty())
            q->exportCsv(path);
    });

    menu->addSeparator();
    QObject::connect(menu->addAction(PlotBase::tr("Reset view")), &QAction::triggered, q, [this] { q->resetView(); });
    crosshairAction = menu->addAction(PlotBase::tr("Crosshair"));
    crosshairAction->setCheckable(true);
    crosshairAction->setChecked(crosshair);
    QObject::connect(crosshairAction, &QAction::toggled, q, [this](bool on) { q->setCrosshair(on); });
    QObject::connect(menu, &QMenu::aboutToShow, q, [this] { syncMenu(); });
}

void PlotBasePrivate::syncMenu()
{
    for (int axis : {kX, kY}) {
        autoAction[axis]->setChecked(autoRange[axis]);
        invertAction[axis]->setChecked(inverted[axis]);
        mouseAction[axis]->setChecked(mouseOn[axis]);
    }
    panModeAction->setChecked(mouseMode == PlotBase::MouseMode::Pan);
    rectModeAction->setChecked(mouseMode == PlotBase::MouseMode::Rect);
    crosshairAction->setChecked(crosshair);
}

// --- PlotBasePrivate: painting ------------------------------------------------------

void PlotBasePrivate::paintPlot(QPainter &painter)
{
    const PlotLayout &l = ensureLayout();
    const QRectF &v = l.view;
    const QFont font = q->font();
    painter.fillRect(q->rect(), style.background);
    painter.setRenderHint(QPainter::TextAntialiasing);

    if (!title.isEmpty()) {
        painter.setFont(titleFont(font));
        painter.setPen(style.title);
        painter.drawText(QRectF(v.left(), l.title.top(), v.width(), l.title.height()), Qt::AlignCenter, title);
    }
    if (v.width() < 1.0 || v.height() < 1.0)
        return;

    const Mapper mapper = mapperOf(*this);
    const QFontMetricsF fm(font);
    const QList<PlotTickLevel> xLevels = plotTickLevels(view[kX].min, view[kX].max, v.width());
    const QList<PlotTickLevel> yLevels = plotTickLevels(view[kY].min, view[kY].max, v.height());

    // Grid: pyqtgraph draws the ticks across the view, level i at
    // 255/(i+1) x gridAlpha, thinned out where they get dense.
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.save();
    painter.setClipRect(v);
    std::vector<QLineF> lines;
    for (bool horizontal : {true, false}) {
        const QList<PlotTickLevel> &levels = horizontal ? xLevels : yLevels;
        const double length = horizontal ? v.width() : v.height();
        for (qsizetype i = 0; i < levels.size(); ++i) {
            const QList<double> &values = levels.at(i).values;
            const double density = std::clamp(0.05 * length / static_cast<double>(values.size() + 1), 0.0, 1.0);
            const int alpha = qRound(255.0 / static_cast<double>(i + 1) * style.gridAlpha * density);
            if (alpha <= 0 || values.isEmpty())
                continue;
            lines.clear();
            for (double value : values) {
                if (horizontal) {
                    const double x = mapper.x(value);
                    lines.emplace_back(x, v.top(), x, v.bottom());
                } else {
                    const double y = mapper.y(value);
                    lines.emplace_back(v.left(), y, v.right(), y);
                }
            }
            painter.setPen(cosmeticPen(withAlpha(style.axis, alpha), 1.0));
            painter.drawLines(lines.data(), static_cast<int>(lines.size()));
        }
    }
    painter.restore();

    // Axis lines.
    painter.setPen(cosmeticPen(style.axis, 1.0));
    painter.drawLine(QLineF(v.left(), v.top(), v.left(), v.bottom()));
    painter.drawLine(QLineF(v.left(), v.bottom(), v.right(), v.bottom()));
    if (axisShown[sideIndex(AxisSide::Right)])
        painter.drawLine(QLineF(v.right(), v.top(), v.right(), v.bottom()));
    if (axisShown[sideIndex(AxisSide::Top)])
        painter.drawLine(QLineF(v.left(), v.top(), v.right(), v.top()));

    // Tick labels; a label that would leave the axis is left out.
    painter.setFont(font);
    painter.setPen(style.axis);
    for (const TickLabel &label : tickLabels(yLevels, v.height(), true, fm)) {
        const double y = mapper.y(label.value);
        const double top = y - label.size.height() / 2.0;
        if (top < v.top() || top + label.size.height() > v.bottom())
            continue;
        painter.drawText(QRectF(v.left() - kTickTextOffsetV - label.size.width(), top, label.size.width(),
                                label.size.height()),
                         Qt::AlignRight | Qt::AlignVCenter | Qt::TextDontClip, label.text);
        if (axisShown[sideIndex(AxisSide::Right)])
            painter.drawText(QRectF(v.right() + kTickTextOffsetV, top, label.size.width(), label.size.height()),
                             Qt::AlignLeft | Qt::AlignVCenter | Qt::TextDontClip, label.text);
    }
    for (const TickLabel &label : tickLabels(xLevels, v.width(), false, fm)) {
        const double x = mapper.x(label.value);
        const double left = x - label.size.width() / 2.0;
        if (left < v.left() || left + label.size.width() > v.right())
            continue;
        painter.drawText(QRectF(left, v.bottom() + kTickTextOffsetH, label.size.width(), label.size.height()),
                         Qt::AlignHCenter | Qt::AlignTop | Qt::TextDontClip, label.text);
        if (axisShown[sideIndex(AxisSide::Top)])
            painter.drawText(QRectF(left, v.top() - kTickTextOffsetH - label.size.height(), label.size.width(),
                                    label.size.height()),
                             Qt::AlignHCenter | Qt::AlignBottom | Qt::TextDontClip, label.text);
    }

    // Axis labels.
    const double band = labelBand(fm);
    painter.setPen(style.text);
    for (AxisSide side : {AxisSide::Left, AxisSide::Bottom, AxisSide::Right, AxisSide::Top}) {
        const int i = sideIndex(side);
        const QString text = axisLabel(labelText[i], labelUnit[i]);
        if (text.isEmpty() || !axisShown[i])
            continue;
        const QRectF &a = l.axis[i];
        if (side == AxisSide::Left || side == AxisSide::Right) {
            painter.save();
            painter.translate(side == AxisSide::Left ? a.left() + band / 2.0 : a.right() - band / 2.0, v.center().y());
            painter.rotate(-90.0);
            painter.drawText(QRectF(-v.height() / 2.0, -band / 2.0, v.height(), band), Qt::AlignCenter, text);
            painter.restore();
        } else {
            const double top = side == AxisSide::Bottom ? a.bottom() - band : a.top();
            painter.drawText(QRectF(v.left(), top, v.width(), band), Qt::AlignCenter, text);
        }
    }

    // Items in the order they were added, clipped to the view.
    painter.save();
    painter.setClipRect(v);
    struct Item
    {
        int order;
        PlotSeries *series;
        const PlotReference *reference;
    };
    std::vector<Item> items;
    items.reserve(series.size() + references.size());
    for (const auto &s : series)
        items.push_back({s->order, s.get(), nullptr});
    for (const PlotReference &r : references)
        items.push_back({r.order, nullptr, &r});
    std::sort(items.begin(), items.end(), [](const Item &a, const Item &b) { return a.order < b.order; });
    const qreal dpr = painter.device() ? painter.device()->devicePixelRatioF() : 1.0;
    for (const Item &item : items) {
        if (item.reference) {
            const PlotReference &r = *item.reference;
            painter.setPen(cosmeticPen(r.color.isValid() ? r.color : style.reference, style.lineWidth, r.dashed));
            painter.setBrush(Qt::NoBrush);
            if (r.kind == PlotReference::Kind::HLine) {
                if (r.value && std::isfinite(*r.value)) {
                    const double y = mapper.y(*r.value);
                    painter.drawLine(QLineF(v.left(), y, v.right(), y));
                }
            } else {
                drawCurve(painter, r.points, mapper, v);
            }
            continue;
        }
        PlotSeries &s = *item.series;
        if (!s.visible || s.drawn.isEmpty())
            continue;
        QColor c = seriesColor(s);
        if (s.points) {
            c.setAlpha(std::clamp(s.alpha ? *s.alpha : style.markerAlpha, 0, 255));
            const double size = orStyle(s.size, style.markerSize);
            const QPixmap &sprite = pointSprite(s, c, size, dpr);
            const double half = sprite.width() / dpr / 2.0;
            const QRectF bounds = v.adjusted(-size, -size, size, size);
            for (const QPointF &p : s.drawn) {
                if (!finite(p))
                    continue;
                const QPointF sp = mapper.map(p);
                if (bounds.contains(sp))
                    painter.drawPixmap(QPointF(sp.x() - half, sp.y() - half), sprite);
            }
        } else {
            if (s.alpha)
                c.setAlpha(std::clamp(*s.alpha, 0, 255));
            painter.setPen(cosmeticPen(c, orStyle(s.width, style.lineWidth), s.dashed));
            painter.setBrush(Qt::NoBrush);
            drawCurve(painter, s.drawn, mapper, v);
        }
    }

    // Legend: top left in the view, a sample and the label per named series.
    legendRows.clear();
    if (legend) {
        const QFont lf = legendFont(font);
        const QFontMetricsF lfm(lf);
        const double rowHeight = std::max(kSampleSize, lfm.height() + 8.0);
        const double left = v.left() + kLegendOffset;
        double top = v.top() + kLegendOffset;
        painter.setFont(lf);
        for (const auto &s : series) {
            if (s->label.isEmpty())
                continue;
            // A hidden series stays listed, dimmed: clicking shows it again.
            painter.setOpacity(s->visible ? 1.0 : 0.3);
            const QRectF sample(left, top + (rowHeight - kSampleSize) / 2.0, kSampleSize, kSampleSize);
            QColor c = seriesColor(*s);
            if (s->points) {
                c.setAlpha(std::clamp(s->alpha ? *s->alpha : style.markerAlpha, 0, 255));
                const double size = orStyle(s->size, style.markerSize);
                painter.setRenderHint(QPainter::Antialiasing, true);
                painter.setPen(Qt::NoPen);
                painter.setBrush(c);
                painter.drawEllipse(sample.center(), size / 2.0, size / 2.0);
                painter.setRenderHint(QPainter::Antialiasing, false);
            } else {
                if (s->alpha)
                    c.setAlpha(std::clamp(*s->alpha, 0, 255));
                painter.setPen(cosmeticPen(c, orStyle(s->width, style.lineWidth), s->dashed));
                painter.drawLine(QLineF(sample.left(), sample.top() + 11.0, sample.right(), sample.top() + 11.0));
            }
            const double labelWidth = lfm.horizontalAdvance(s->label);
            painter.setPen(style.text);
            painter.drawText(QRectF(left + kSampleSize + kLegendGap, top, labelWidth + 2.0, rowHeight),
                             Qt::AlignLeft | Qt::AlignVCenter | Qt::TextDontClip, s->label);
            legendRows.append({QRectF(left, top, kSampleSize + kLegendGap + labelWidth, rowHeight), s->key});
            top += rowHeight;
        }
        painter.setOpacity(1.0);
    }
    painter.restore();
}

void PlotBasePrivate::paintOverlay(QPainter &painter)
{
    const PlotLayout &l = ensureLayout();
    const QRectF &v = l.view;
    const QFont font = q->font();

    if (crosshair && mousePos && v.contains(*mousePos)) {
        const QPointF pos = *mousePos;
        painter.save();
        painter.setClipRect(v);
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setPen(cosmeticPen(withAlpha(style.axis, 160), 1.0, true));
        painter.drawLine(QLineF(pos.x(), v.top(), pos.x(), v.bottom()));
        painter.drawLine(QLineF(v.left(), pos.y(), v.right(), pos.y()));

        const QPointF data = mapperOf(*this).unmap(pos);
        const QList<PlotReadoutLine> lines = q->readoutLines(data.x(), data.y());
        if (!lines.isEmpty()) {
            const QFontMetricsF fm(font);
            double width = 0.0;
            for (const PlotReadoutLine &line : lines)
                width = std::max(width, fm.horizontalAdvance(line.text));
            width += 8.0;
            const double height = fm.lineSpacing() * static_cast<double>(lines.size()) + 8.0;
            // Keep the readout inside the plot: in the right or upper half it
            // goes to the cursor's left or below it.
            const double ax = data.x() > (view[kX].min + view[kX].max) / 2.0 ? 1.0 : 0.0;
            const double ay = data.y() > (view[kY].min + view[kY].max) / 2.0 ? 0.0 : 1.0;
            const QRectF box(pos.x() - ax * width, pos.y() - ay * height, width, height);
            painter.setPen(cosmeticPen(withAlpha(style.axis, 160), 1.0));
            painter.setBrush(withAlpha(style.background, 210));
            painter.drawRect(box);
            painter.setFont(font);
            for (qsizetype i = 0; i < lines.size(); ++i) {
                painter.setPen(lines.at(i).color);
                painter.drawText(QPointF(box.left() + 4.0, box.top() + 4.0 + fm.ascent()
                                                               + fm.lineSpacing() * static_cast<double>(i)),
                                 lines.at(i).text);
            }
        }
        painter.restore();
    }

    if (dragging && dragButton == Qt::LeftButton && mouseMode == PlotBase::MouseMode::Rect && dragAxis < 0) {
        painter.save();
        painter.setPen(cosmeticPen(style.accent, 1.0));
        painter.setBrush(withAlpha(style.accent, 100));
        painter.drawRect(QRectF(pressPos, lastPos).normalized() & v);
        painter.restore();
    }

    if (autoButtonVisible()) {
        painter.save();
        QFont bold(font);
        bold.setBold(true);
        bold.setPixelSize(12);
        painter.setFont(bold);
        painter.setPen(style.axis);
        painter.drawText(l.autoButton, Qt::AlignCenter, QStringLiteral("A"));
        painter.restore();
    }
}

// --- PlotBase ----------------------------------------------------------------------

PlotBase::PlotBase(const QString &title, const std::optional<PlotStyle> &style, bool legend, bool crosshair,
                   int refreshMs, QWidget *parent)
    : QWidget(parent)
    , d(std::make_unique<PlotBasePrivate>(this))
{
    d->style = style ? *style : defaultPlotStyle();
    d->title = title;
    d->legend = legend;
    d->crosshair = crosshair;
    d->timer = new QTimer(this);
    d->timer->setSingleShot(true);
    d->timer->setInterval(std::max(0, refreshMs));
    connect(d->timer, &QTimer::timeout, this, [this] {
        // Hidden (another tab, a collapsed splitter): stay dirty and catch up
        // in showEvent instead of spending time on pixels nobody sees.
        if (d->dirty && isVisible())
            redraw();
    });
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    d->buildMenu();
}

PlotBase::~PlotBase()
{
    if (d->xLink)
        d->xLink->members.removeAll(this);
}

PlotStyle PlotBase::plotStyle() const
{
    return d->style;
}

void PlotBase::setPlotStyle(const PlotStyle &style)
{
    d->style = style;
    d->updateAutoRange(); // stroke widths pad the auto range
    d->invalidate();
}

QString PlotBase::title() const
{
    return d->title;
}

void PlotBase::setTitle(const QString &title)
{
    d->title = title;
    d->invalidateLayout();
}

void PlotBase::setAxisLabel(AxisSide side, const QString &text, const QString &unit)
{
    const int i = sideIndex(side);
    d->labelText[i] = text;
    d->labelUnit[i] = unit;
    d->axisShown[i] = true; // as pyqtgraph's setLabel(), which shows the axis
    d->invalidateLayout();
}

QString PlotBase::axisLabelText(AxisSide side) const
{
    const int i = sideIndex(side);
    return axisLabel(d->labelText[i], d->labelUnit[i]);
}

bool PlotBase::isAxisShown(AxisSide side) const
{
    return d->axisShown[sideIndex(side)];
}

void PlotBase::removeSeries(const QString &key)
{
    const auto before = d->series.size();
    std::erase_if(d->series, [&](const auto &s) { return s->key == key; });
    if (d->series.size() != before)
        itemsChanged();
}

QStringList PlotBase::seriesKeys() const
{
    QStringList keys;
    for (const auto &s : d->series)
        keys.append(s->key);
    return keys;
}

bool PlotBase::hasSeries(const QString &key) const
{
    return d->find(key) != nullptr;
}

QColor PlotBase::seriesColor(const QString &key) const
{
    const PlotSeries *s = d->find(key);
    return s ? d->seriesColor(*s) : QColor();
}

QStringList PlotBase::legendLabels() const
{
    QStringList labels;
    if (!d->legend)
        return labels;
    for (const auto &s : d->series) {
        if (!s->label.isEmpty())
            labels.append(s->label);
    }
    return labels;
}

QList<QPointF> PlotBase::drawnData(const QString &key)
{
    if (d->dirty)
        redraw();
    const PlotSeries *s = d->find(key);
    return s ? s->drawn : QList<QPointF>();
}

void PlotBase::clear()
{
    for (const auto &s : d->series)
        s->data.clear();
    scheduleRedraw();
}

void PlotBase::setSeriesVisible(const QString &key, bool visible)
{
    PlotSeries *s = d->find(key);
    if (!s || s->visible == visible)
        return;
    s->visible = visible;
    itemsChanged();
}

bool PlotBase::isSeriesVisible(const QString &key) const
{
    const PlotSeries *s = d->find(key);
    return s && s->visible;
}

PlotRange PlotBase::viewXRange() const
{
    return d->view[kX];
}

PlotRange PlotBase::viewYRange() const
{
    return d->view[kY];
}

void PlotBase::setViewXRange(double min, double max, double padding)
{
    const double pad = std::abs(max - min) * padding;
    d->setTarget(kX, PlotRange{std::min(min, max) - pad, std::max(min, max) + pad}, true);
    d->updateViewRange();
}

void PlotBase::setViewYRange(double min, double max, double padding)
{
    const double pad = std::abs(max - min) * padding;
    d->setTarget(kY, PlotRange{std::min(min, max) - pad, std::max(min, max) + pad}, true);
    d->updateViewRange();
}

void PlotBase::setAutoRange(Qt::Orientations axes, bool enabled)
{
    if (axes.testFlag(Qt::Horizontal))
        d->autoRange[kX] = enabled;
    if (axes.testFlag(Qt::Vertical))
        d->autoRange[kY] = enabled;
    d->updateAutoRange();
    d->invalidate();
}

bool PlotBase::isAutoRange(Qt::Orientation axis) const
{
    return d->autoRange[axis == Qt::Horizontal ? kX : kY];
}

void PlotBase::viewAll()
{
    const PlotBasePrivate::Bounds bounds = d->childrenBounds();
    for (int axis : {kX, kY}) {
        const auto &b = bounds.range[static_cast<std::size_t>(axis)];
        if (b)
            d->setTarget(axis, d->padded(axis, *b, bounds.pixelPad), true);
    }
    d->updateViewRange();
}

void PlotBase::setAspectLocked(bool locked)
{
    if (d->aspectLocked == locked)
        return;
    d->aspectLocked = locked;
    d->updateViewRange();
}

bool PlotBase::isAspectLocked() const
{
    return d->aspectLocked;
}

void PlotBase::setMouseEnabled(Qt::Orientations axes)
{
    d->mouseOn[kX] = axes.testFlag(Qt::Horizontal);
    d->mouseOn[kY] = axes.testFlag(Qt::Vertical);
}

Qt::Orientations PlotBase::mouseEnabled() const
{
    Qt::Orientations axes;
    axes.setFlag(Qt::Horizontal, d->mouseOn[kX]);
    axes.setFlag(Qt::Vertical, d->mouseOn[kY]);
    return axes;
}

void PlotBase::setAxisInverted(Qt::Orientation axis, bool inverted)
{
    d->inverted[axis == Qt::Horizontal ? kX : kY] = inverted;
    d->invalidate();
}

bool PlotBase::isAxisInverted(Qt::Orientation axis) const
{
    return d->inverted[axis == Qt::Horizontal ? kX : kY];
}

void PlotBase::setMouseMode(MouseMode mode)
{
    d->mouseMode = mode;
}

PlotBase::MouseMode PlotBase::mouseMode() const
{
    return d->mouseMode;
}

void PlotBase::resetView()
{
    enableAutoRange();
}

void PlotBase::setCrosshair(bool enabled)
{
    d->crosshair = enabled;
    if (d->crosshairAction && d->crosshairAction->isChecked() != enabled)
        d->crosshairAction->setChecked(enabled);
    update();
}

bool PlotBase::crosshair() const
{
    return d->crosshair;
}

QList<PlotReadoutLine> PlotBase::crosshairReadout(QPointF dataPos) const
{
    return readoutLines(dataPos.x(), dataPos.y());
}

QRectF PlotBase::viewRect() const
{
    return d->ensureLayout().view;
}

QPointF PlotBase::mapToData(QPointF widgetPos) const
{
    return mapperOf(*d).unmap(widgetPos);
}

QPointF PlotBase::mapFromData(QPointF dataPos) const
{
    return mapperOf(*d).map(dataPos);
}

QMenu *PlotBase::contextMenu() const
{
    return d->menu;
}

bool PlotBase::exportImage(const QString &path)
{
    if (d->dirty)
        redraw();
    return grab().save(path);
}

bool PlotBase::exportCsv(const QString &path)
{
    if (d->dirty)
        redraw();
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream out(&file);
    QStringList header;
    qsizetype rows = 0;
    for (const auto &s : d->series) {
        const QString name = s->label.isEmpty() ? s->key : s->label;
        header << name + QStringLiteral("_x") << name + QStringLiteral("_y");
        rows = std::max(rows, s->drawn.size());
    }
    out << header.join(QLatin1Char(',')) << '\n';
    for (qsizetype i = 0; i < rows; ++i) {
        QStringList cells;
        for (const auto &s : d->series) {
            if (i < s->drawn.size()) {
                cells << QString::number(s->drawn.at(i).x(), 'g', 10) << QString::number(s->drawn.at(i).y(), 'g', 10);
            } else {
                cells << QString() << QString();
            }
        }
        out << cells.join(QLatin1Char(',')) << '\n';
    }
    return out.status() == QTextStream::Ok;
}

void PlotBase::scheduleRedraw()
{
    d->dirty = true;
    if (!d->timer->isActive())
        d->timer->start();
}

void PlotBase::redraw()
{
    d->dirty = false;
    renderData();
    d->updateAutoRange();
    d->invalidate();
}

bool PlotBase::isRedrawPending() const
{
    return d->dirty;
}

QSize PlotBase::sizeHint() const
{
    return {400, 300};
}

QSize PlotBase::minimumSizeHint() const
{
    return {120, 80};
}

QList<PlotReadoutLine> PlotBase::readoutLines(double x, double y) const
{
    return {{d->style.text, coordinate(AxisSide::Bottom, x, QStringLiteral("x"))},
            {d->style.text, coordinate(AxisSide::Left, y, QStringLiteral("y"))}};
}

QString PlotBase::coordinate(AxisSide side, double value, const QString &name) const
{
    const int i = sideIndex(side);
    const QString &text = d->labelText[i];
    return rstrip(QStringLiteral("%1: %2 %3").arg(text.isEmpty() ? name : text, formatG(value), d->labelUnit[i]));
}

PlotSeries &PlotBase::insertSeries(const QString &key, const std::optional<QString> &label, qsizetype capacity)
{
    removeSeries(key);
    int index = -1;
    for (const auto &s : d->series)
        index = std::max(index, s->index);
    auto series = std::make_unique<PlotSeries>();
    series->key = key;
    series->label = label ? *label : key;
    series->index = index + 1;
    series->order = d->nextOrder++;
    series->data = PointRing(capacity);
    d->series.push_back(std::move(series));
    return *d->series.back();
}

PlotSeries &PlotBase::seriesFor(const QString &key)
{
    QString name = key;
    if (name.isNull()) {
        if (!d->series.empty())
            return *d->series.front();
        name = QStringLiteral("data");
    }
    if (!d->find(name))
        addDefaultSeries(name);
    PlotSeries *s = d->find(name);
    Q_ASSERT(s);
    return *s;
}

void PlotBase::setSeriesData(const QString &key, const QList<double> &xs, const QList<double> &ys)
{
    if (xs.size() != ys.size())
        throw std::invalid_argument("series " + key.toStdString() + ": x and y differ in length");
    PlotSeries &s = seriesFor(key);
    s.data.clear();
    for (qsizetype i = 0; i < xs.size(); ++i)
        s.data.push(QPointF(xs.at(i), ys.at(i)));
    scheduleRedraw();
}

void PlotBase::itemsChanged()
{
    d->updateAutoRange();
    d->invalidate();
}

// --- events ------------------------------------------------------------------------

void PlotBase::paintEvent(QPaintEvent *)
{
    // The layout may have moved since the ranges were derived (tick label
    // width): the aspect lock needs the final pixel size.
    for (int pass = 0; pass < 2 && d->ensureLayout().view.size() != d->viewPx; ++pass)
        d->updateViewRange();

    const qreal dpr = devicePixelRatioF();
    const QSize pixels = (QSizeF(size()) * dpr).toSize();
    if (!d->cacheValid || d->cache.size() != pixels || d->cache.devicePixelRatio() != dpr) {
        if (d->cache.size() != pixels)
            d->cache = QPixmap(pixels);
        d->cache.setDevicePixelRatio(dpr);
        QPainter cachePainter(&d->cache);
        d->paintPlot(cachePainter);
        cachePainter.end();
        d->cacheValid = true;
    }
    QPainter painter(this);
    painter.drawPixmap(0, 0, d->cache);
    d->paintOverlay(painter);
}

void PlotBase::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    d->layoutValid = false;
    d->updateAutoRange(); // padding and aspect depend on the pixel size
    d->invalidate();
}

void PlotBase::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (d->dirty)
        scheduleRedraw();
}

void PlotBase::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::FontChange)
        d->invalidateLayout();
}

void PlotBase::mousePressEvent(QMouseEvent *event)
{
    const QPointF pos = event->position();
    const PlotLayout &l = d->ensureLayout();
    d->dragging = false;
    d->pressOnLegend = false;
    d->pressOnAutoButton = false;
    d->dragButton = Qt::NoButton;
    if (event->button() == Qt::LeftButton && d->autoButtonVisible() && l.autoButton.contains(pos)) {
        d->pressOnAutoButton = true;
        event->accept();
        return;
    }
    // Where the press lands decides the axes: the view moves both, an axis
    // only itself (as pyqtgraph forwards axis drags to the view).
    int axis = -2;
    if (l.view.contains(pos))
        axis = -1;
    else if (l.axis[sideIndex(AxisSide::Left)].contains(pos) || l.axis[sideIndex(AxisSide::Right)].contains(pos))
        axis = kY;
    else if (l.axis[sideIndex(AxisSide::Bottom)].contains(pos) || l.axis[sideIndex(AxisSide::Top)].contains(pos))
        axis = kX;
    if (axis == -2) {
        QWidget::mousePressEvent(event);
        return;
    }
    if (event->button() == Qt::LeftButton && axis == -1) {
        for (const auto &row : std::as_const(d->legendRows)) {
            if (row.rect.contains(pos)) {
                d->pressOnLegend = true;
                d->legendPressKey = row.key;
                break;
            }
        }
    }
    d->dragButton = event->button();
    d->dragAxis = axis;
    d->pressPos = d->lastPos = pos;
    event->accept();
}

void PlotBase::mouseMoveEvent(QMouseEvent *event)
{
    const QPointF pos = event->position();
    d->mousePos = pos;
    if (d->dragButton != Qt::NoButton && (event->buttons() & d->dragButton)) {
        if (!d->dragging && (pos - d->pressPos).manhattanLength() >= QApplication::startDragDistance()) {
            d->dragging = true;
            d->pressOnLegend = false;
        }
        if (d->dragging) {
            const QPointF delta = pos - d->lastPos;
            d->lastPos = pos;
            const QRectF &v = d->ensureLayout().view;
            const bool useX = d->mouseOn[kX] && d->dragAxis != kY;
            const bool useY = d->mouseOn[kY] && d->dragAxis != kX;
            const bool rectZoom =
                d->dragButton == Qt::LeftButton && d->mouseMode == MouseMode::Rect && d->dragAxis < 0;
            if (!rectZoom && (d->dragButton == Qt::LeftButton || d->dragButton == Qt::MiddleButton)) {
                // Pan: the data follows the mouse.
                if (v.width() > 0 && v.height() > 0) {
                    const double dx = -delta.x() * d->view[kX].span() / v.width() * (d->inverted[kX] ? -1.0 : 1.0);
                    const double dy = delta.y() * d->view[kY].span() / v.height() * (d->inverted[kY] ? -1.0 : 1.0);
                    d->translateBy(useX ? std::optional(dx) : std::nullopt, useY ? std::optional(dy) : std::nullopt);
                }
            } else if (d->dragButton == Qt::RightButton) {
                // Zoom around the press point: right/up zooms in, as pyqtgraph.
                const QPointF centre = mapToData(d->pressPos);
                d->scaleBy(useX ? std::optional(std::pow(1.02, -delta.x())) : std::nullopt,
                           useY ? std::optional(std::pow(1.02, delta.y())) : std::nullopt, centre);
            }
        }
    }
    update(); // crosshair, readout, rubber band, "A" button
}

void PlotBase::mouseReleaseEvent(QMouseEvent *event)
{
    const QPointF pos = event->position();
    if (d->pressOnAutoButton) {
        d->pressOnAutoButton = false;
        if (d->ensureLayout().autoButton.contains(pos))
            enableAutoRange();
        event->accept();
        return;
    }
    if (event->button() != d->dragButton) {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    if (!d->dragging) {
        if (event->button() == Qt::LeftButton && d->pressOnLegend) {
            for (const auto &row : std::as_const(d->legendRows)) {
                if (row.rect.contains(pos) && row.key == d->legendPressKey) {
                    setSeriesVisible(row.key, !isSeriesVisible(row.key));
                    break;
                }
            }
        } else if (event->button() == Qt::RightButton) {
            d->menu->popup(event->globalPosition().toPoint());
        }
    } else if (event->button() == Qt::LeftButton && d->mouseMode == MouseMode::Rect && d->dragAxis < 0) {
        const QRectF &v = d->ensureLayout().view;
        const QRectF box = QRectF(d->pressPos, pos).normalized() & v;
        if (box.width() > 1.0 && box.height() > 1.0) {
            const QPointF a = mapToData(box.topLeft());
            const QPointF b = mapToData(box.bottomRight());
            const double xs[2] = {std::min(a.x(), b.x()), std::max(a.x(), b.x())};
            const double ys[2] = {std::min(a.y(), b.y()), std::max(a.y(), b.y())};
            const double padX = (xs[1] - xs[0]) * d->suggestPadding(kX);
            const double padY = (ys[1] - ys[0]) * d->suggestPadding(kY);
            d->setTarget(kX, PlotRange{xs[0] - padX, xs[1] + padX}, true);
            d->setTarget(kY, PlotRange{ys[0] - padY, ys[1] + padY}, true);
            d->updateViewRange();
        }
    }
    d->dragging = false;
    d->pressOnLegend = false;
    d->dragButton = Qt::NoButton;
    update();
    event->accept();
}

void PlotBase::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && d->ensureLayout().view.contains(event->position())) {
        event->accept();
        resetView();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void PlotBase::wheelEvent(QWheelEvent *event)
{
    const QPointF pos = event->position();
    const PlotLayout &l = d->ensureLayout();
    bool useX = d->mouseOn[kX];
    bool useY = d->mouseOn[kY];
    if (l.view.contains(pos)) {
        // both axes
    } else if (l.axis[sideIndex(AxisSide::Left)].contains(pos) || l.axis[sideIndex(AxisSide::Right)].contains(pos)) {
        useX = false;
    } else if (l.axis[sideIndex(AxisSide::Bottom)].contains(pos) || l.axis[sideIndex(AxisSide::Top)].contains(pos)) {
        useY = false;
    } else {
        QWidget::wheelEvent(event);
        return;
    }
    int delta = event->angleDelta().y();
    if (delta == 0)
        delta = event->angleDelta().x();
    // pyqtgraph: 1.02 ** (delta * -1/8), around the mouse.
    const double scale = std::pow(1.02, -static_cast<double>(delta) / 8.0);
    d->scaleBy(useX ? std::optional(scale) : std::nullopt, useY ? std::optional(scale) : std::nullopt,
               mapToData(pos));
    event->accept();
}

void PlotBase::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    d->mousePos.reset();
    update();
}

void PlotBase::contextMenuEvent(QContextMenuEvent *event)
{
    // Mouse right-clicks open the menu on release (a right drag zooms).
    if (event->reason() != QContextMenuEvent::Mouse)
        d->menu->popup(event->globalPos());
    event->accept();
}

// --- linked axes ----------------------------------------------------------------------

void linkXAxes(const QList<PlotBase *> &plots)
{
    if (plots.size() < 2 || !plots.first())
        return;
    PlotBase *first = plots.first();
    if (!first->d->xLink) {
        first->d->xLink = std::make_shared<XLinkGroup>();
        first->d->xLink->members.append(first);
    }
    const std::shared_ptr<XLinkGroup> group = first->d->xLink;
    for (qsizetype i = 1; i < plots.size(); ++i) {
        PlotBase *plot = plots.at(i);
        if (!plot || plot == first || plot->d->xLink == group)
            continue;
        if (plot->d->xLink)
            plot->d->xLink->members.removeAll(plot);
        plot->d->xLink = group;
        group->members.append(plot);
        // Follow the first plot at once, as pyqtgraph's setXLink() does.
        plot->d->target[kX] = first->d->view[kX];
        plot->d->autoRange[kX] = false;
        plot->d->updateViewRange();
    }
}

} // namespace duifx

#include "duifx/CompassDial.h"

#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace duifx {

namespace {

double positiveMod(double value, double period)
{
    const double result = std::fmod(value, period);
    return result < 0.0 ? result + period : result;
}

QColor withAlpha(QColor color, int alpha)
{
    color.setAlpha(std::clamp(alpha, 0, 255));
    return color;
}

} // namespace

CompassDial::CompassDial(QWidget *parent)
    : CompassDial(Options{}, parent)
{
}

CompassDial::CompassDial(const Options &options, QWidget *parent)
    : QWidget(parent)
    , m_style(options.style ? *options.style : defaultPlotStyle())
    , m_points(options.points >= 8 ? 8 : 4)
    , m_showReadout(options.showReadout)
    , m_showLegend(options.showLegend)
{
    addNeedle(QStringLiteral("heading"));
    QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
}

PlotStyle CompassDial::plotStyle() const
{
    return m_style;
}

void CompassDial::setPlotStyle(const PlotStyle &style)
{
    m_style = style;
    update();
}

CompassDial::Needle *CompassDial::find(const QString &key)
{
    for (Needle &needle : m_needles) {
        if (needle.key == key)
            return &needle;
    }
    return nullptr;
}

const CompassDial::Needle *CompassDial::find(const QString &key) const
{
    for (const Needle &needle : m_needles) {
        if (needle.key == key)
            return &needle;
    }
    return nullptr;
}

void CompassDial::addNeedle(const QString &key, const QColor &color, const QString &label)
{
    // An existing needle keeps its place, heading and uncertainty.
    if (Needle *needle = find(key)) {
        needle->color = color;
        needle->label = label;
    } else {
        m_needles.append(Needle{key, std::nullopt, std::nullopt, color, label});
    }
    updateGeometry(); // the legend may have grown
    update();
}

void CompassDial::removeNeedle(const QString &key)
{
    m_needles.removeIf([&](const Needle &needle) { return needle.key == key; });
    updateGeometry();
    update();
}

QStringList CompassDial::needleKeys() const
{
    QStringList keys;
    for (const Needle &needle : m_needles)
        keys.append(needle.key);
    return keys;
}

QColor CompassDial::needleColor(const QString &key) const
{
    for (qsizetype i = 0; i < m_needles.size(); ++i) {
        if (m_needles.at(i).key == key)
            return colorOf(i, m_needles.at(i));
    }
    return {};
}

void CompassDial::setHeading(std::optional<double> heading, const QString &key)
{
    if (!find(key))
        addNeedle(key);
    find(key)->heading = heading ? std::optional(positiveMod(*heading, 360.0)) : std::nullopt;
    update();
}

std::optional<double> CompassDial::heading(const QString &key) const
{
    const Needle *needle = find(key);
    return needle ? needle->heading : std::nullopt;
}

void CompassDial::setUncertainty(std::optional<double> degrees, const QString &key)
{
    if (!find(key))
        addNeedle(key);
    find(key)->uncertainty = degrees ? std::optional(std::abs(*degrees)) : std::nullopt;
    update();
}

std::optional<double> CompassDial::uncertainty(const QString &key) const
{
    const Needle *needle = find(key);
    return needle ? needle->uncertainty : std::nullopt;
}

void CompassDial::setReadout(const std::optional<QString> &text)
{
    m_readout = text;
    update();
}

QString CompassDial::readoutText() const
{
    if (m_readout)
        return *m_readout;
    if (m_needles.isEmpty() || !m_needles.first().heading)
        return {};
    const double heading = *m_needles.first().heading;
    return QStringLiteral("%1°  %2").arg(QString::number(heading, 'f', 1), compassPoint(heading, 16));
}

void CompassDial::clear()
{
    for (Needle &needle : m_needles) {
        needle.heading.reset();
        needle.uncertainty.reset();
    }
    update();
}

// --- geometry --------------------------------------------------------------------

QSize CompassDial::sizeHint() const
{
    return {200, 200 + legendHeight()};
}

QSize CompassDial::minimumSizeHint() const
{
    return {100, 100 + legendHeight()};
}

bool CompassDial::hasHeightForWidth() const
{
    return true;
}

int CompassDial::heightForWidth(int width) const
{
    return width + legendHeight();
}

int CompassDial::legendHeight() const
{
    if (!m_showLegend)
        return 0;
    const auto entries = std::count_if(m_needles.cbegin(), m_needles.cend(),
                                       [](const Needle &needle) { return !needle.label.isEmpty(); });
    if (entries == 0)
        return 0;
    return static_cast<int>(entries) * fontMetrics().height() + 4;
}

// --- painting --------------------------------------------------------------------

QColor CompassDial::colorOf(qsizetype index, const Needle &needle) const
{
    if (needle.color.isValid())
        return needle.color;
    return index == 0 ? m_style.accent : m_style.seriesColor(static_cast<int>(index) - 1);
}

void CompassDial::paintLegend(QPainter &painter, double top) const
{
    // One line per labelled needle: a short stroke and the label.
    const QFontMetrics metrics = fontMetrics();
    const double lineHeight = metrics.height();
    const double swatch = lineHeight * 1.2;
    int textWidth = 0;
    for (const Needle &needle : m_needles) {
        if (!needle.label.isEmpty())
            textWidth = std::max(textWidth, metrics.horizontalAdvance(needle.label));
    }
    const double total = swatch + 6.0 + textWidth;
    const double left = std::max(2.0, (width() - total) / 2.0);
    painter.setFont(font());
    int row = 0;
    for (qsizetype i = 0; i < m_needles.size(); ++i) {
        const Needle &needle = m_needles.at(i);
        if (needle.label.isEmpty())
            continue;
        const double y = top + row * lineHeight;
        painter.setPen(QPen(colorOf(i, needle), 2.0, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(left, y + lineHeight / 2.0), QPointF(left + swatch, y + lineHeight / 2.0));
        painter.setPen(m_style.text);
        painter.drawText(QRectF(left + swatch + 6.0, y, width() - left - swatch - 6.0, lineHeight),
                         Qt::AlignLeft | Qt::AlignVCenter, needle.label);
        ++row;
    }
}

void CompassDial::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), m_style.background);

    // The legend gets a strip at the bottom, the rose what is left.
    const int legendH = legendHeight();
    const double roseHeight = height() - legendH;
    if (legendH > 0)
        paintLegend(painter, roseHeight);
    const double side = std::min<double>(width(), roseHeight);
    const double radius = side / 2.0 - std::max(4.0, side * 0.03);
    if (radius <= 10.0)
        return;
    const QPointF center(width() / 2.0, roseHeight / 2.0);
    const auto at = [&](double angleDeg, double r) {
        const double a = angleDeg * std::numbers::pi / 180.0;
        return QPointF(center.x() + r * std::sin(a), center.y() - r * std::cos(a));
    };

    // Ring and ticks: long every 30 degrees, medium every 10, short every 5.
    const QColor axis = m_style.axis;
    painter.setPen(QPen(axis, 1.5));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(center, radius, radius);
    for (int deg = 0; deg < 360; deg += 5) {
        const double length = radius * (deg % 30 == 0 ? 0.10 : deg % 10 == 0 ? 0.06 : 0.03);
        painter.setPen(QPen(axis, deg % 30 == 0 ? 1.5 : 1.0));
        painter.drawLine(at(deg, radius), at(deg, radius - length));
    }

    // Labels: the compass points, and the degrees at the remaining 30s.
    const double labelRadius = radius * 0.76;
    QFont boldFont(font());
    boldFont.setPixelSize(std::max(8, static_cast<int>(radius * 0.13)));
    boldFont.setBold(true);
    QFont smallFont(boldFont);
    smallFont.setBold(false);
    smallFont.setPixelSize(std::max(7, static_cast<int>(radius * 0.085)));
    const int step = 360 / m_points;
    static const char *const names8[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    painter.setPen(m_style.text);
    for (int deg = 0; deg < 360; deg += step == 90 ? 30 : 15) {
        QString text;
        if (deg % step == 0) {
            painter.setFont(deg % 90 == 0 ? boldFont : smallFont);
            // Four points are every other of the eight.
            text = QLatin1StringView(names8[(deg / step) * (m_points == 8 ? 1 : 2)]).toString();
        } else if (deg % 30 == 0) {
            painter.setFont(smallFont);
            text = QString::number(deg);
        } else {
            continue;
        }
        const QPointF p = at(deg, labelRadius);
        const QRectF box(p.x() - radius * 0.2, p.y() - radius * 0.1, radius * 0.4, radius * 0.2);
        painter.drawText(box, Qt::AlignCenter, text);
    }

    // Uncertainty wedges first, so the needles stay on top.
    painter.setPen(Qt::NoPen);
    const QRectF ring(center.x() - radius, center.y() - radius, 2.0 * radius, 2.0 * radius);
    for (qsizetype i = 0; i < m_needles.size(); ++i) {
        const Needle &needle = m_needles.at(i);
        if (!needle.heading || !needle.uncertainty || *needle.uncertainty == 0.0)
            continue;
        const double spread = std::min(*needle.uncertainty, 180.0);
        painter.setBrush(withAlpha(colorOf(i, needle), m_style.wedgeAlpha));
        QPainterPath path(center);
        // Qt counts angles counter-clockwise from 3 o'clock.
        path.arcTo(ring, 90.0 - (*needle.heading - spread), -2.0 * spread);
        path.closeSubpath();
        painter.drawPath(path);
    }

    // Needles: a line with an arrow head, and a short tail.
    for (qsizetype i = 0; i < m_needles.size(); ++i) {
        const Needle &needle = m_needles.at(i);
        if (!needle.heading)
            continue;
        const double heading = *needle.heading;
        const QColor c = colorOf(i, needle);
        painter.setPen(QPen(c, std::max(2.0, radius * 0.025), Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(at(heading + 180.0, radius * 0.18), at(heading, radius * 0.70));
        painter.setPen(Qt::NoPen);
        painter.setBrush(c);
        const QPointF head[3] = {at(heading, radius * 0.88), at(heading + 7.0, radius * 0.66),
                                 at(heading - 7.0, radius * 0.66)};
        painter.drawPolygon(head, 3);
    }

    painter.setBrush(axis);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(center, radius * 0.04, radius * 0.04);

    if (m_showReadout) {
        const QString text = readoutText();
        if (!text.isEmpty()) {
            QFont readout(font());
            readout.setPixelSize(std::max(8, static_cast<int>(radius * 0.12)));
            painter.setFont(readout);
            painter.setPen(m_style.text);
            const QRectF box(center.x() - radius * 0.6, center.y() + radius * 0.22, radius * 1.2, radius * 0.24);
            painter.drawText(box, Qt::AlignCenter, text);
        }
    }
}

} // namespace duifx

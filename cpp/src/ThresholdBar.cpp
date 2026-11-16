#include "duifx/ThresholdBar.h"

#include <algorithm>
#include <climits>
#include <cmath>

namespace duifx {

namespace {

/// value x kScale as an int, truncated like Python's int() and kept in range.
int scaled(double value)
{
    const double v = std::trunc(value * ThresholdBar::kScale);
    if (std::isnan(v))
        return 0;
    return static_cast<int>(std::clamp(v, static_cast<double>(INT_MIN), static_cast<double>(INT_MAX)));
}

} // namespace

ThresholdBar::ThresholdBar(QWidget *parent)
    : ThresholdBar(Options{}, parent)
{
}

ThresholdBar::ThresholdBar(const Options &options, QWidget *parent)
    : QProgressBar(parent)
    , m_style(options.style ? *options.style : defaultPlotStyle())
    , m_gate(options.gate)
    , m_unit(options.unit)
    , m_decimals(options.decimals)
    , m_higherIsBetter(options.higherIsBetter)
{
    setTextVisible(true);
    setMaximumF(options.maximum);
}

PlotStyle ThresholdBar::plotStyle() const
{
    return m_style;
}

void ThresholdBar::setPlotStyle(const PlotStyle &style)
{
    m_style = style;
    setValueF(m_value);
}

void ThresholdBar::setMaximumF(double maximum)
{
    m_maximum = maximum;
    setRange(0, std::max(1, scaled(maximum)));
    setValueF(m_value);
}

double ThresholdBar::maximumF() const
{
    return m_maximum;
}

void ThresholdBar::setGate(std::optional<double> gate)
{
    m_gate = gate;
    setValueF(m_value);
}

std::optional<double> ThresholdBar::gate() const
{
    return m_gate;
}

std::optional<double> ThresholdBar::valueF() const
{
    return m_value;
}

std::optional<bool> ThresholdBar::gateMet() const
{
    if (!m_value || !m_gate)
        return std::nullopt;
    return m_higherIsBetter ? *m_value >= *m_gate : *m_value <= *m_gate;
}

void ThresholdBar::setValueF(std::optional<double> value)
{
    m_value = value;
    if (!value) {
        setValue(0);
        setFormat(QStringLiteral("—"));
        setStyleSheet(QString());
        return;
    }
    setValue(std::clamp(scaled(*value), 0, maximum()));
    const QString text = QString::number(*value, 'f', m_decimals) + m_unit;
    const std::optional<bool> met = gateMet();
    if (!met) {
        setFormat(text);
        setStyleSheet(QString());
        return;
    }
    const QString relation = m_higherIsBetter ? QStringLiteral("≥") : QStringLiteral("≤");
    setFormat(QStringLiteral("%1  (gate %2 %3%4)").arg(text, relation, QString::number(*m_gate, 'g', 6), m_unit));
    const QColor chunk = *met ? m_style.ok : m_style.warn;
    setStyleSheet(QStringLiteral("QProgressBar::chunk { background-color: %1; }").arg(chunk.name()));
}

} // namespace duifx

#pragma once

// A progress bar judged against a gate. Port of ThresholdBar in
// duifx/gui/plot.py.
//
// The bar runs to the physical full scale; the gate is what matters, so the
// chunk turns style.ok once the value meets it and style.warn while it does
// not, and the text names both: "143.0°  (gate ≥ 180°)". higherIsBetter
// false flips the test, for limits like a maximum error.
//
//     auto *bar = new duifx::ThresholdBar({.maximum = 360.0, .gate = 180.0, .unit = "°"});
//     bar->setValueF(143.0);
//
// QProgressBar is integer-only: the floating-point API carries an "F" suffix
// (setValueF, valueF, setMaximumF) next to QProgressBar's own int one, which
// it drives scaled by kScale.

#include "duifx/Plot.h"

#include <QProgressBar>

#include <optional>

namespace duifx {

class ThresholdBar : public QProgressBar
{
    Q_OBJECT

public:
    /// Values are scaled by this before they reach QProgressBar.
    static constexpr int kScale = 1000;

    struct Options
    {
        /// Full scale.
        double maximum = 100.0;
        /// Threshold; unset shows the value without judging it.
        std::optional<double> gate{};
        /// Appended to value and gate, e.g. "°".
        QString unit{};
        /// Decimals of the value in the text.
        int decimals = 1;
        /// The gate is a minimum (true) or a maximum (false).
        bool higherIsBetter = true;
        std::optional<PlotStyle> style{};
    };

    explicit ThresholdBar(QWidget *parent = nullptr);
    explicit ThresholdBar(const Options &options, QWidget *parent = nullptr);

    PlotStyle plotStyle() const;
    void setPlotStyle(const PlotStyle &style);

    void setMaximumF(double maximum);
    double maximumF() const;

    void setGate(std::optional<double> gate);
    std::optional<double> gate() const;

    /// Shows value; unset clears the bar to a dash.
    void setValueF(std::optional<double> value);
    /// The value as set, unscaled; nullopt when cleared.
    std::optional<double> valueF() const;

    /// Whether the value meets the gate; nullopt without value or gate.
    std::optional<bool> gateMet() const;

private:
    PlotStyle m_style;
    double m_maximum = 100.0;
    std::optional<double> m_gate;
    QString m_unit;
    int m_decimals;
    bool m_higherIsBetter;
    std::optional<double> m_value;
};

} // namespace duifx

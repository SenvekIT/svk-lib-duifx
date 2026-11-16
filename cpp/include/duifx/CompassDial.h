#pragma once

// A north-up compass rose with rotating needles. Port of CompassDial in
// duifx/gui/plot.py.
//
// Headings are in degrees, clockwise from north. The first needle uses the
// style's accent colour, further ones the series cycle. An uncertainty
// (+- degrees) draws a wedge around its needle; a readout under the centre
// shows the first needle's heading and compass point unless replaced with
// setReadout(). Needles with a label are listed in a legend under the rose,
// one per line in the needle's colour.
//
//     auto *dial = new duifx::CompassDial;
//     dial->setHeading(heading);                       // the default "heading" needle
//     dial->setUncertainty(3.5);
//     dial->addNeedle("gps", {}, "GPS course");
//     dial->setHeading(course, "gps");

#include "duifx/Plot.h"

#include <QWidget>

#include <optional>

namespace duifx {

class CompassDial : public QWidget
{
    Q_OBJECT

public:
    struct Options
    {
        /// Compass points labelled on the rose: 4 or 8.
        int points = 8;
        /// The numeric readout under the centre.
        bool showReadout = true;
        /// The legend of the labelled needles.
        bool showLegend = true;
        /// Colours; defaultPlotStyle() when unset.
        std::optional<PlotStyle> style{};
    };

    explicit CompassDial(QWidget *parent = nullptr);
    explicit CompassDial(const Options &options, QWidget *parent = nullptr);

    PlotStyle plotStyle() const;
    void setPlotStyle(const PlotStyle &style);

    /// Adds (or restyles) the needle key, hidden until it has a heading.
    /// color invalid: accent for the first needle, then the series cycle.
    void addNeedle(const QString &key, const QColor &color = {}, const QString &label = {});
    void removeNeedle(const QString &key);
    /// Needle keys in the order they were added.
    QStringList needleKeys() const;
    /// The colour needle key is drawn in; invalid for an unknown key.
    QColor needleColor(const QString &key) const;

    /// Points needle key at heading; unset hides it. A new key adds a needle.
    void setHeading(std::optional<double> heading, const QString &key = QStringLiteral("heading"));
    std::optional<double> heading(const QString &key = QStringLiteral("heading")) const;

    /// +- degrees wedge around needle key; unset removes it.
    void setUncertainty(std::optional<double> degrees, const QString &key = QStringLiteral("heading"));
    std::optional<double> uncertainty(const QString &key = QStringLiteral("heading")) const;

    /// Replaces the readout text; unset restores the automatic one.
    void setReadout(const std::optional<QString> &text);
    /// The readout as shown: the custom text, or "123.4°  SE" for the first
    /// needle (empty while it has no heading).
    QString readoutText() const;

    /// Hides every needle and wedge.
    void clear();

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int width) const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    struct Needle
    {
        QString key{};
        std::optional<double> heading{};
        std::optional<double> uncertainty{};
        QColor color{};
        QString label{};
    };

    Needle *find(const QString &key);
    const Needle *find(const QString &key) const;
    QColor colorOf(qsizetype index, const Needle &needle) const;
    /// Pixels the legend takes under the rose, 0 without one.
    int legendHeight() const;
    void paintLegend(QPainter &painter, double top) const;

    PlotStyle m_style;
    int m_points;
    bool m_showReadout;
    bool m_showLegend;
    std::optional<QString> m_readout;
    QList<Needle> m_needles;
};

} // namespace duifx

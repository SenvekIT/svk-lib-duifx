#pragma once

// Central colour palette: semantic colour tokens. Port of duifx/gui/colors.py.
//
// Widgets do not carry their own hex literals. They ask for a role -- "state
// ok", "muted text", "accent button" -- and get the colour the current theme
// assigns to it:
//
//     label->setStyleSheet(QStringLiteral("color: %1;").arg(duifx::css(duifx::Token::TextMuted)));
//     item->setForeground(duifx::stateColors().color(entry.state));
//
// Dark values are defined but provisional: activeTheme() returns Light until a
// dark theme exists (see Theming.h). Plot colours are the same in both themes.

#include "duifx/Theming.h"

#include <QColor>
#include <QHash>
#include <QString>

#include <array>
#include <optional>

namespace duifx {

/// Semantic colour roles. Resolve with color() or css().
enum class Token {
    // States: connection, device and value status.
    StateOk,       ///< connected, operational, value good
    StateBusy,     ///< scanning, connecting
    StateWarning,  ///< needs attention, not good enough yet
    StateError,    ///< error, fault, heartbeat lost
    StateInactive, ///< disconnected, offline, not valid
    StateInfo,     ///< neutral notice

    // Text.
    TextHeading,     ///< titles, item names
    TextPrimary,     ///< body text, values
    TextSecondary,   ///< labels, section headers
    TextMuted,       ///< hints, captions, descriptions
    TextPlaceholder, ///< "nothing selected" placeholders
    TextDisabled,    ///< disabled entries
    TextWarning,     ///< warning sentences (readable orange)
    TextOnAccent,    ///< text on Accent / Danger buttons

    // Surfaces and lines.
    Surface,              ///< input fields, plain content
    SurfaceAlt,           ///< tables, tooltips on chrome
    SurfaceMuted,         ///< toolbars, cards, table header rows
    Border,               ///< input borders
    BorderStrong,         ///< tooltip and emphasised borders
    HighlightChanged,     ///< background of a changed value
    SurfaceSelected,      ///< active navigation entry, selected item
    SurfaceSelectedHover, ///< the same under the mouse

    // Overlays for flat buttons (translucent).
    OverlayHover,
    OverlayPressed,

    // Buttons.
    Accent,
    AccentHover,
    AccentPressed,
    Danger,
    DangerHover,
    DangerPressed,
    ControlDisabled,

    // Indicator dots (bit LEDs) without ok/error meaning.
    IndicatorOn,
    IndicatorOff,

    // Plots. Series 1-3 double as the x/y/z axis colours.
    PlotSeries1,
    PlotSeries2,
    PlotSeries3,
    PlotSeries4,
    PlotSeries5,
    PlotSeries6,
    PlotMagnitude,  ///< |v|, sigma and other derived traces
    PlotBackground,
    PlotAxis,       ///< axis lines, ticks, labels
    PlotGrid,       ///< grid lines (translucent axis colour)

    PlotAxisX = PlotSeries1,
    PlotAxisY = PlotSeries2,
    PlotAxisZ = PlotSeries3,
};

/// Number of distinct tokens (aliases not counted).
inline constexpr int kTokenCount = static_cast<int>(Token::PlotGrid) + 1;

/// Plot series in cycle order, see plotSeriesColor().
inline constexpr std::array<Token, 6> kPlotSeries{Token::PlotSeries1, Token::PlotSeries2, Token::PlotSeries3,
                                                  Token::PlotSeries4, Token::PlotSeries5, Token::PlotSeries6};

/// The token's Python value, e.g. "state_ok", "plot_series_1".
QString tokenName(Token token);

/// The token for a Python value ("state_ok"); nullopt if unknown.
std::optional<Token> tokenFromName(const QString &name);

/// Theme the tokens resolve against (Light or Dark): the stored theme setting,
/// read on first use and kept for the rest of the process.
Theme activeTheme();

/// Pins the theme colours resolve against; nullopt re-reads the setting.
void setActiveTheme(std::optional<Theme> theme);

/// The token's colour for theme (default: activeTheme()). alpha (0-255)
/// overrides the token's own opacity.
QColor color(Token token, std::optional<int> alpha = std::nullopt, std::optional<Theme> theme = std::nullopt);

/// The token's colour as "#rrggbb" (opacity dropped).
QString colorHex(Token token, std::optional<Theme> theme = std::nullopt);

/// The token's colour for a Qt stylesheet: "#rrggbb" when opaque,
/// "rgba(r, g, b, a)" when translucent.
QString css(Token token, std::optional<int> alpha = std::nullopt, std::optional<Theme> theme = std::nullopt);

/// Colour of the index-th plot series, cycling through kPlotSeries.
QColor plotSeriesColor(int index, std::optional<int> alpha = std::nullopt);

/// Maps state names (case-insensitive) to colours. Covers CAN network, Modbus
/// master and CANopen node states ("connected", "operational", "scanning",
/// "error", "heartbeat lost", ...); unknown states get the default token.
class StateColors
{
public:
    /// mapping adds to (or overrides) the default states.
    explicit StateColors(const QHash<QString, Token> &mapping = {}, Token defaultToken = Token::TextSecondary);

    /// The default states, as DEFAULT_STATE_TOKENS in Python.
    static QHash<QString, Token> defaultStateTokens();

    Token token(const QString &state) const;
    QColor color(const QString &state) const;
    /// "#rrggbb".
    QString hex(const QString &state) const;
    /// The hex colour; defaultHex if the state is unknown and defaultHex is set.
    QString get(const QString &state, const QString &defaultHex = {}) const;
    bool contains(const QString &state) const;
    QString operator[](const QString &state) const { return hex(state); }

private:
    QHash<QString, Token> m_tokens;
    Token m_default;
};

/// Shared instance with the default states.
const StateColors &stateColors();

} // namespace duifx

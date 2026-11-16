#pragma once

// Colour palette (Tailwind-inspired scales) and the named base colours.
// Port of duifx/gui/resources/theme.py and the BasePalette of colors.py.
// Widgets do not use these directly but the semantic tokens of Colors.h.

#include <QMap>
#include <QString>

namespace duifx {

/// Ten-shade colour scale from light (50) to dark (900), as "#rrggbb".
struct ColorScale
{
    const char *s50;
    const char *s100;
    const char *s200;
    const char *s300;
    const char *s400;
    const char *s500;
    const char *s600;
    const char *s700;
    const char *s800;
    const char *s900;
};

namespace palette {

inline constexpr ColorScale gray{"#f9fafb", "#f3f4f6", "#e5e7eb", "#d1d5db", "#9ca3af",
                                 "#6b7280", "#4b5563", "#374151", "#1f2937", "#111827"};
inline constexpr ColorScale blue{"#eff6ff", "#dbeafe", "#bfdbfe", "#93c5fd", "#60a5fa",
                                 "#3b82f6", "#2563eb", "#1d4ed8", "#1e40af", "#1e3a8a"};
inline constexpr ColorScale green{"#f0fdf4", "#dcfce7", "#bbf7d0", "#86efac", "#4ade80",
                                  "#22c55e", "#16a34a", "#15803d", "#166534", "#14532d"};
inline constexpr ColorScale red{"#fef2f2", "#fee2e2", "#fecaca", "#fca5a5", "#f87171",
                                "#ef4444", "#dc2626", "#b91c1c", "#991b1b", "#7f1d1d"};
inline constexpr ColorScale yellow{"#fefce8", "#fef3c7", "#fde68a", "#fcd34d", "#fbbf24",
                                   "#f59e0b", "#d97706", "#b45309", "#92400e", "#78350f"};
inline constexpr ColorScale purple{"#faf5ff", "#f3e8ff", "#e9d5ff", "#d8b4fe", "#c084fc",
                                   "#a855f7", "#9333ea", "#7c3aed", "#6b21a8", "#581c87"};
inline constexpr ColorScale orange{"#fff7ed", "#ffedd5", "#fed7aa", "#fdba74", "#fb923c",
                                   "#f97316", "#ea580c", "#c2410c", "#9a3412", "#7c2d12"};
inline constexpr ColorScale teal{"#f0fdfa", "#ccfbf1", "#99f6e4", "#5eead4", "#2dd4bf",
                                 "#14b8a6", "#0d9488", "#0f766e", "#115e59", "#134e4a"};

inline constexpr const char *white = "#ffffff";
inline constexpr const char *black = "#000000";

// Semantic aliases of Palette.
inline constexpr const char *primary = blue.s500;
inline constexpr const char *secondary = gray.s500;
inline constexpr const char *success = green.s500;
inline constexpr const char *warning = yellow.s500;
inline constexpr const char *danger = red.s500;
inline constexpr const char *info = teal.s500;

/// All colours of Palette as a flat map ("gray_50", "blue_500", ..., "white",
/// "black", "primary", ...), like Palette.get_color_dict().
QMap<QString, QString> colorDict();

} // namespace palette

/// The named base colours: the scales above plus the colours that are not on
/// a scale (BasePalette in colors.py).
namespace base {

// Neutrals the stylesheet (modern_light.qss) uses and widgets must match.
inline constexpr const char *ink = "#212529";    // default text colour
inline constexpr const char *chrome = "#e9ecef"; // sidebar, footer and toolbar background

// Signal colours for states.
inline constexpr const char *signalGreen = "#27ae60";
inline constexpr const char *signalOrange = "#e67e22";
inline constexpr const char *signalRed = "#e74c3c";

// Plot colours, as in the firmware's monitor (drawn on black).
inline constexpr const char *plotRed = "#e65050";
inline constexpr const char *plotGreen = "#50c850";
inline constexpr const char *plotBlue = "#5a8cff";
inline constexpr const char *plotAmber = "#ffc83c";
inline constexpr const char *plotPink = "#ff78c8";
inline constexpr const char *plotMint = "#5ac88c";
inline constexpr const char *plotLight = "#dcdcdc";
inline constexpr const char *plotAxis = "#969696";
inline constexpr const char *plotBlack = "#000000";

/// palette::colorDict() plus the extras, under their Python names
/// ("ink", "chrome", "signal_green", ..., "plot_black").
QMap<QString, QString> colorDict();

} // namespace base

} // namespace duifx

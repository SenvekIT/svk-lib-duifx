#pragma once

// Application theme (colour scheme) selection. Port of duifx/gui/theming.py.
//
// Qt follows the operating system's light/dark setting by default. The DUIFx
// stylesheet is a light one, so on a dark desktop plain widgets would turn
// dark under light styled ones. applyTheme() pins the colour scheme instead.
// The theme is stored in the application settings (shared with the Python
// applications: same file, same key, same values) and applied at startup,
// before any window is built:
//
//     QApplication app(argc, argv);
//     duifx::applyTheme(&app);
//
// Theme::Dark is reserved: not offered by selectableThemes() and applied as
// Light until a dark stylesheet exists.

#include <QString>
#include <QStringList>

#include <optional>

class QApplication;

namespace duifx {

/// Available colour schemes.
enum class Theme {
    Light,
    Dark, ///< Reserved, falls back to Light.
    System,
};

/// Settings key holding the selected theme ("appearance/theme").
inline constexpr const char *kThemeSettingKey = "appearance/theme";

/// Theme used when nothing (valid) is stored.
inline constexpr Theme kDefaultTheme = Theme::Light;

/// Enum value as in Python and in the settings: "light", "dark", "system".
QString themeValue(Theme theme);

/// Human-readable label, also what is stored: "Light", "Dark", "Follow system setting".
QString themeLabel(Theme theme);

/// Themes that may be offered to the user (excludes unimplemented ones).
QList<Theme> selectableThemes();

/// Labels of selectableThemes(), in display order.
QStringList themeLabels();

/// The theme for value, tolerating labels and case ("light", "Follow system
/// setting"). Unknown or empty input yields kDefaultTheme.
Theme parseTheme(const QString &value);

/// The theme stored in the application settings.
Theme getTheme();

/// Persists theme. Takes effect on the next application start.
void setTheme(Theme theme);

/// Applies theme (or the stored one) to app (or the running application).
/// Call before creating windows. Returns the theme actually applied, which
/// differs from the request when an unimplemented one was asked for.
Theme applyTheme(QApplication *app = nullptr, std::optional<Theme> theme = std::nullopt);

} // namespace duifx

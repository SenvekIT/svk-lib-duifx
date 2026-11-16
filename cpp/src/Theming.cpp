#include "duifx/Theming.h"

#include "duifx/Settings.h"
#include "duifx/Utils.h"
#include "Logging.h"

#include <QApplication>
#include <QStyleHints>

namespace duifx {

QString themeValue(Theme theme)
{
    switch (theme) {
    case Theme::Light:
        return QStringLiteral("light");
    case Theme::Dark:
        return QStringLiteral("dark");
    case Theme::System:
        return QStringLiteral("system");
    }
    return QStringLiteral("light");
}

QString themeLabel(Theme theme)
{
    switch (theme) {
    case Theme::Light:
        return QStringLiteral("Light");
    case Theme::Dark:
        return QStringLiteral("Dark");
    case Theme::System:
        return QStringLiteral("Follow system setting");
    }
    return QStringLiteral("Light");
}

QList<Theme> selectableThemes()
{
    return {Theme::Light, Theme::System};
}

QStringList themeLabels()
{
    QStringList labels;
    for (Theme theme : selectableThemes())
        labels << themeLabel(theme);
    return labels;
}

Theme parseTheme(const QString &value)
{
    const QString text = value.trimmed().toLower();
    if (text.isEmpty())
        return kDefaultTheme;
    for (Theme theme : {Theme::Light, Theme::Dark, Theme::System}) {
        if (text == themeValue(theme) || text == themeLabel(theme).toLower())
            return theme;
    }
    qCWarning(lcDuifx, "Unknown theme '%s', falling back to %s", qPrintable(value), qPrintable(themeValue(kDefaultTheme)));
    return kDefaultTheme;
}

Theme getTheme()
{
    return parseTheme(getSettings()->value(QLatin1StringView(kThemeSettingKey), themeValue(kDefaultTheme)).toString());
}

void setTheme(Theme theme)
{
    const auto settings = getSettings();
    settings->setValue(QLatin1StringView(kThemeSettingKey), themeLabel(theme));
    settings->sync();
}

Theme applyTheme(QApplication *app, std::optional<Theme> theme)
{
    if (!app)
        app = qobject_cast<QApplication *>(QCoreApplication::instance());
    if (!app) {
        qCWarning(lcDuifx, "applyTheme() needs a QApplication instance");
        return kDefaultTheme;
    }
    initResources();

    const Theme requested = theme ? *theme : getTheme();
    Theme effective = requested;
    if (requested != Theme::Light && requested != Theme::System) {
        qCWarning(lcDuifx, "Theme '%s' is not implemented yet, using '%s' instead.", qPrintable(themeValue(requested)),
                  qPrintable(themeValue(kDefaultTheme)));
        effective = kDefaultTheme;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    app->styleHints()->setColorScheme(effective == Theme::Light ? Qt::ColorScheme::Light : Qt::ColorScheme::Unknown);
    qCDebug(lcDuifx, "Theme applied: %s", qPrintable(themeValue(effective)));
#else
    qCWarning(lcDuifx, "Qt build has no QStyleHints::setColorScheme(); the theme setting cannot be enforced and the OS "
                       "setting stays in charge.");
#endif
    return effective;
}

} // namespace duifx

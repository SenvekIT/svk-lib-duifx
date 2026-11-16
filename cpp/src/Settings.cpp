#include "duifx/Settings.h"

#include <QCoreApplication>
#include <QDir>

namespace duifx {

QString settingsFilePath()
{
    QString application = QStringLiteral("DUIFx");
    QString organisation = QStringLiteral("DUIFx");
    if (QCoreApplication::instance()) {
        if (!QCoreApplication::applicationName().isEmpty())
            application = QCoreApplication::applicationName();
        if (!QCoreApplication::organizationName().isEmpty())
            organisation = QCoreApplication::organizationName();
    }

#ifdef Q_OS_WIN
    const QString folder = QDir::homePath() + QStringLiteral("/AppData/Roaming/") + organisation + QLatin1Char('/') + application;
#else
    const QString folder = QDir::homePath() + QStringLiteral("/.config/") + organisation + QLatin1Char('/') + application;
#endif
    QDir().mkpath(folder);
    return folder + QStringLiteral("/settings.ini");
}

std::unique_ptr<QSettings> getSettings()
{
    return std::make_unique<QSettings>(settingsFilePath(), QSettings::IniFormat);
}

} // namespace duifx

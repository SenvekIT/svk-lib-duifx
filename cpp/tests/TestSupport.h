#pragma once

// Shared by the tests: keeps them away from the user's real settings.

#include <QCoreApplication>
#include <QDir>
#include <QTemporaryDir>

#include <memory>

namespace duifx_test {

/// Points the home folder (and with it duifx::settingsFilePath()) to a
/// temporary folder for the rest of the test run. Call in initTestCase().
inline void isolateSettings()
{
    static std::unique_ptr<QTemporaryDir> home;
    if (home)
        return;
    home = std::make_unique<QTemporaryDir>();
    qputenv("HOME", home->path().toLocal8Bit());
    qputenv("USERPROFILE", QDir::toNativeSeparators(home->path()).toLocal8Bit());
    QCoreApplication::setOrganizationName(QStringLiteral("DUIFxTests"));
    QCoreApplication::setApplicationName(QStringLiteral("tests"));
}

} // namespace duifx_test

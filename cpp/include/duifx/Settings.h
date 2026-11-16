#pragma once

// Application settings, as get_settings() in duifx/gui/settings.py: an INI
// file per organisation and application, the same file the Python
// applications use.
//
//   Windows:      %USERPROFILE%/AppData/Roaming/<organisation>/<application>/settings.ini
//   Linux, macOS: ~/.config/<organisation>/<application>/settings.ini
//
// Organisation and application come from QCoreApplication (organizationName,
// applicationName), "DUIFx" for what is not set.

#include <QSettings>
#include <QString>

#include <memory>

namespace duifx {

/// Path of the settings file; creates its folder.
QString settingsFilePath();

/// The application settings (INI format). A new object per call, like Python's
/// get_settings(); every instance reads and writes the same file.
std::unique_ptr<QSettings> getSettings();

} // namespace duifx

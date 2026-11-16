#pragma once

// The settings dialog: a tree of categories and settings, each with an editor
// and a restore-to-default button. Port of SettingsDialog in
// duifx/gui/settings.py.
//
// Values live in the application settings (getSettings(), Settings.h) under the
// same keys and in the same formats as in Python -- bool "true"/"false",
// integers, strings, colours as "#rrggbb" -- so Python and C++ applications
// read each other's settings. Edits are collected and written on accept();
// a setting restored to its default is removed from the file then, so a later
// change of the default takes effect.
//
//     class MySettings : public duifx::SettingsDialog {
//     public:
//         explicit MySettings(QWidget *parent) : SettingsDialog(parent) {
//             QTreeWidgetItem *general = addCategory(QStringLiteral("General"));
//             addBooleanSetting(general, QStringLiteral("debug_mode"), QStringLiteral("Debug Mode"), false);
//             addThemeSetting(general);
//             loadSettings();
//         }
//     };

#include "duifx/Dialogs.h"

#include <QColor>
#include <QHash>
#include <QMetaType>
#include <QStringList>
#include <QVariant>

#include <memory>
#include <type_traits>
#include <utility>

class QLabel;
class QPoint;
class QPushButton;
class QSettings;
class QTreeWidget;
class QTreeWidgetItem;

namespace duifx {

class SettingsDialog : public HeaderDialog
{
    Q_OBJECT

public:
    /// createDefaults adds the demo categories of the Python dialog (General:
    /// language; Advanced: debug_mode, max_recent_files, log_directory).
    /// Loads the stored values and creates the log folder (the setting
    /// "log_directory", default ~/logs) if it is missing.
    explicit SettingsDialog(QWidget *parent = nullptr, const QString &windowTitle = QStringLiteral("Settings"),
                            bool createDefaults = false);
    ~SettingsDialog() override;

    QTreeWidget *treeWidget() const { return m_tree; }

    /// The application settings the dialog reads and writes.
    QSettings *settings() const { return m_settings.get(); }

    // --- building the tree ---
    // A null parent adds the setting as a top-level item. Adding a key again
    // replaces its earlier entry (keeping its place in settingKeys()).

    QTreeWidgetItem *addCategory(const QString &name);
    void addBooleanSetting(QTreeWidgetItem *parent, const QString &key, const QString &label, bool defaultValue);
    void addIntegerSetting(QTreeWidgetItem *parent, const QString &key, const QString &label, int defaultValue,
                           int minValue = 0, int maxValue = 100);
    void addTextSetting(QTreeWidgetItem *parent, const QString &key, const QString &label, const QString &defaultValue);
    void addComboSetting(QTreeWidgetItem *parent, const QString &key, const QString &label, const QStringList &options,
                         const QString &defaultValue);
    /// Stored as "#rrggbb".
    void addColorSetting(QTreeWidgetItem *parent, const QString &key, const QString &label, const QColor &defaultValue);
    void addFileSetting(QTreeWidgetItem *parent, const QString &key, const QString &label, const QString &defaultValue);
    void addFolderSetting(QTreeWidgetItem *parent, const QString &key, const QString &label, const QString &defaultValue);

    /// The application theme selector (key kThemeSettingKey, the labels of
    /// themeLabels(), default the stored theme). Applied on the next start --
    /// a palette swap does not fully reach widgets that already exist.
    void addThemeSetting(QTreeWidgetItem *parent, const QString &label = QStringLiteral("Appearance (restart required)"));

    // --- the settings shown ---

    /// Keys of the settings in the tree, in the order they were added.
    QStringList settingKeys() const { return m_keys; }
    QTreeWidgetItem *settingItem(const QString &key) const;
    /// The value editor: QCheckBox, QSpinBox, QLineEdit (text, file, folder),
    /// QComboBox, or for a colour the widget holding button and label.
    QWidget *settingEditor(const QString &key) const;
    /// The default the setting was added with (QVariant() for unknown keys).
    QVariant settingDefault(const QString &key) const;
    /// The restore-to-default button of the setting.
    QPushButton *restoreButton(const QString &key) const;
    /// Edits not yet written (accept() writes them).
    QVariantMap changedSettings() const;
    /// The setting's label is bold and italic, its restore button enabled.
    bool isSettingModified(const QString &key) const;

    // --- values ---

    /// The stored value of key, or defaultValue. With a valid type the value
    /// is converted; a value that does not convert yields defaultValue.
    /// sync re-reads the file first.
    QVariant getSetting(const QString &key, const QVariant &defaultValue = {}, QMetaType type = {}, bool sync = false) const;

    /// Typed shortcut: getSetting<int>(key, 10).
    template <typename T>
        requires(!std::is_same_v<T, QVariant>)
    T getSetting(const QString &key, const T &defaultValue, bool sync = false) const
    {
        return getSetting(key, QVariant::fromValue(defaultValue), QMetaType::fromType<T>(), sync).template value<T>();
    }

    /// Shows value in the setting's editor (if the key is in the tree) and
    /// stores it at once. A colour may be a QColor or a colour name.
    void setSetting(const QString &key, const QVariant &value);

    /// Shows the stored values (or the defaults); a stored value marks its
    /// setting as modified. Discards pending edits.
    void loadSettings();

    /// setSetting() with the default of every setting in the tree.
    void resetToDefaults();

    /// Copies every stored setting to an INI file.
    void exportSettings(const QString &filePath) const;
    /// setSetting() for every key of an INI file.
    void importSettings(const QString &filePath);

    /// Shows the default of key and marks it for removal from the settings on
    /// accept() (what the restore button does).
    void restoreSetting(const QString &key);

public slots:
    void expandAll();
    void collapseAll();

    /// Removes restored settings, writes the edits, emits settingChanged for
    /// each, then closes the dialog.
    void accept() override;

signals:
    /// A setting was written on accept(): key and new value (the default for a
    /// restored one).
    void settingChanged(const QString &key, const QVariant &value);

protected:
    /// Records an edit; called by the editors.
    void onSettingChanged(const QString &key, const QVariant &value);

private:
    struct Entry
    {
        QTreeWidgetItem *item = nullptr;
        QWidget *editor = nullptr;      // the value widget (for colours: the container)
        QPushButton *colorButton = nullptr;
        QLabel *colorLabel = nullptr;
        QPushButton *restore = nullptr;
        QVariant defaultValue;
    };

    void initTreeWidget();
    void showTreeContextMenu(const QPoint &position);
    void createDefaultSettings();
    void createLogFolder();
    QTreeWidgetItem *addSettingItem(QTreeWidgetItem *parent, const QString &label);
    void registerSetting(const QString &key, const Entry &entry);
    void addPathSetting(QTreeWidgetItem *parent, const QString &key, const QString &label, const QString &defaultValue,
                        bool folder);
    void addRestoreButton(const QString &key);
    void updateItemStyle(const QString &key, bool modified);
    void showValue(const Entry &entry, const QVariant &value);
    void takeChanged(const QString &key);

    std::unique_ptr<QSettings> m_settings;
    QTreeWidget *m_tree = nullptr;
    QStringList m_keys;
    QHash<QString, Entry> m_entries;
    // Pending edits in the order they were made (Python's dict order).
    QList<std::pair<QString, QVariant>> m_changed;
    QStringList m_keysToRestore;
};

} // namespace duifx

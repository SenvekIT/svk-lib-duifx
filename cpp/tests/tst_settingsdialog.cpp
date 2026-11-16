// SettingsDialog: editors, changed-item highlighting, restore, accept()
// semantics, Python-compatible INI values, import/export, expand/collapse.

#include "TestSupport.h"

#include <duifx/Settings.h>
#include <duifx/SettingsDialog.h>
#include <duifx/Theming.h>

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QSignalSpy>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <QtTest>

using namespace duifx;

namespace {

QString iniText()
{
    QFile file(settingsFilePath());
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QString::fromUtf8(file.readAll());
}

template <typename T>
T *editor(const SettingsDialog &dialog, const QString &key)
{
    return qobject_cast<T *>(dialog.settingEditor(key));
}

bool labelEmphasised(const SettingsDialog &dialog, const QString &key)
{
    const QFont font = dialog.settingItem(key)->font(0);
    return font.bold() && font.italic();
}

bool allExpanded(const QTreeWidget *tree, bool expanded)
{
    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        if (tree->topLevelItem(i)->isExpanded() != expanded)
            return false;
    }
    return true;
}

QPushButton *buttonWithText(const QWidget &parent, const QString &text)
{
    for (QPushButton *button : parent.findChildren<QPushButton *>()) {
        if (button->text() == text)
            return button;
    }
    return nullptr;
}

} // namespace

class TstSettingsDialog : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() { duifx_test::isolateSettings(); }

    void init()
    {
        const auto settings = getSettings();
        settings->clear();
        settings->sync();
    }

    void defaultSettingsAndLogFolder()
    {
        const QString logs = QDir::homePath() + QStringLiteral("/logs");

        SettingsDialog dialog(nullptr, QStringLiteral("Preferences"), true);
        QCOMPARE(dialog.windowTitle(), QStringLiteral("Preferences"));
        QCOMPARE(dialog.settingKeys(), (QStringList{QStringLiteral("language"), QStringLiteral("debug_mode"),
                                                    QStringLiteral("max_recent_files"), QStringLiteral("log_directory")}));
        QVERIFY(QDir(logs).exists());
        QCOMPARE(dialog.settingDefault(QStringLiteral("log_directory")).toString(), QDir::toNativeSeparators(logs));
        QCOMPARE(dialog.settingDefault(QStringLiteral("max_recent_files")), QVariant(10));

        QTreeWidget *tree = dialog.treeWidget();
        QCOMPARE(tree->headerItem()->text(0), QStringLiteral("Setting"));
        QCOMPARE(tree->headerItem()->text(1), QStringLiteral("Value"));
        QCOMPARE(tree->topLevelItemCount(), 2);
        QCOMPARE(tree->topLevelItem(0)->text(0), QStringLiteral("General"));
        QCOMPARE(tree->topLevelItem(1)->flags(), Qt::ItemFlags(Qt::ItemIsEnabled));
        QVERIFY(allExpanded(tree, true));
        QCOMPARE(tree->header()->sectionSize(0), 250);
        QCOMPARE(tree->header()->sectionSize(2), 40);
        QCOMPARE(tree->header()->sectionResizeMode(1), QHeaderView::Interactive);

        auto *spin = editor<QSpinBox>(dialog, QStringLiteral("max_recent_files"));
        QVERIFY(spin);
        QCOMPARE(spin->value(), 10);
        QCOMPARE(spin->minimum(), 1);
        QCOMPARE(spin->maximum(), 50);
        QCOMPARE(spin->toolTip(), QStringLiteral("Default: 10, Range: 1 - 50"));
        QCOMPARE(editor<QCheckBox>(dialog, QStringLiteral("debug_mode"))->toolTip(), QStringLiteral("Default: Disabled"));
        QCOMPARE(editor<QComboBox>(dialog, QStringLiteral("language"))->count(), 3);
        // The folder editor is the line edit beside a "Browse..." button.
        auto *folder = editor<QLineEdit>(dialog, QStringLiteral("log_directory"));
        QVERIFY(folder);
        QCOMPARE(folder->text(), QDir::toNativeSeparators(logs));
        QVERIFY(buttonWithText(dialog, QStringLiteral("Browse...")));

        QPushButton *restore = dialog.restoreButton(QStringLiteral("debug_mode"));
        QCOMPARE(restore->objectName(), QStringLiteral("RestoreButton"));
        QCOMPARE(restore->text(), QString(QChar(0x21BA)));
        QCOMPARE(restore->toolTip(), QStringLiteral("Restore"));
        QCOMPARE(tree->itemWidget(dialog.settingItem(QStringLiteral("debug_mode")), 2), restore);
        for (const QString &key : dialog.settingKeys()) {
            QVERIFY2(!dialog.isSettingModified(key), qPrintable(key));
            QVERIFY(!labelEmphasised(dialog, key));
        }
        QVERIFY(dialog.changedSettings().isEmpty());
    }

    void createsTheConfiguredLogFolder()
    {
        QTemporaryDir dir;
        const QString folder = dir.filePath(QStringLiteral("nested/logs"));
        {
            const auto settings = getSettings();
            settings->setValue(QStringLiteral("log_directory"), folder);
            settings->sync();
        }
        QVERIFY(!QDir(folder).exists());
        SettingsDialog dialog;
        QVERIFY(QDir(folder).exists());
    }

    void editsAreWrittenOnAccept()
    {
        SettingsDialog dialog(nullptr, QStringLiteral("Settings"), true);
        QSignalSpy changed(&dialog, &SettingsDialog::settingChanged);

        editor<QCheckBox>(dialog, QStringLiteral("debug_mode"))->setChecked(true);
        editor<QSpinBox>(dialog, QStringLiteral("max_recent_files"))->setValue(20);
        editor<QComboBox>(dialog, QStringLiteral("language"))->setCurrentText(QStringLiteral("Deutsch"));

        // Highlighted and restorable, but not written before accept().
        QVERIFY(dialog.isSettingModified(QStringLiteral("debug_mode")));
        QVERIFY(labelEmphasised(dialog, QStringLiteral("debug_mode")));
        QVERIFY(dialog.restoreButton(QStringLiteral("debug_mode"))->isEnabled());
        QVERIFY(!dialog.isSettingModified(QStringLiteral("log_directory")));
        QCOMPARE(dialog.changedSettings().value(QStringLiteral("max_recent_files")), QVariant(20));
        QVERIFY(!getSettings()->contains(QStringLiteral("debug_mode")));
        QCOMPARE(changed.count(), 0);

        dialog.accept();
        QCOMPARE(dialog.result(), int(QDialog::Accepted));
        QCOMPARE(changed.count(), 3);
        // In the order of the edits.
        QCOMPARE(changed.at(0).at(0).toString(), QStringLiteral("debug_mode"));
        QCOMPARE(changed.at(0).at(1), QVariant(true));
        QCOMPARE(changed.at(1).at(1), QVariant(20));
        QCOMPARE(changed.at(2).at(1), QVariant(QStringLiteral("Deutsch")));
        QVERIFY(dialog.changedSettings().isEmpty());

        // The values as Python writes them.
        const QString ini = iniText();
        QVERIFY2(ini.contains(QStringLiteral("debug_mode=true")), qPrintable(ini));
        QVERIFY(ini.contains(QStringLiteral("max_recent_files=20")));
        QVERIFY(ini.contains(QStringLiteral("language=Deutsch")));

        // Cancel discards.
        SettingsDialog second(nullptr, QStringLiteral("Settings"), true);
        editor<QSpinBox>(second, QStringLiteral("max_recent_files"))->setValue(33);
        second.reject();
        QCOMPARE(getSettings()->value(QStringLiteral("max_recent_files")).toInt(), 20);
    }

    void loadsValuesWrittenByPython()
    {
        QFile file(settingsFilePath());
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write("[General]\ndebug_mode=true\nmax_recent_files=7\nlanguage=Italiano\naccent=#00ff00\n");
        file.close();

        SettingsDialog dialog(nullptr, QStringLiteral("Settings"), true);
        QVERIFY(editor<QCheckBox>(dialog, QStringLiteral("debug_mode"))->isChecked());
        QCOMPARE(editor<QSpinBox>(dialog, QStringLiteral("max_recent_files"))->value(), 7);
        QCOMPARE(editor<QComboBox>(dialog, QStringLiteral("language"))->currentText(), QStringLiteral("Italiano"));
        // A stored value marks the setting as changed from its default.
        QVERIFY(dialog.isSettingModified(QStringLiteral("debug_mode")));
        QVERIFY(labelEmphasised(dialog, QStringLiteral("max_recent_files")));
        QVERIFY(!dialog.isSettingModified(QStringLiteral("log_directory")));
        QVERIFY(dialog.changedSettings().isEmpty());

        dialog.addColorSetting(nullptr, QStringLiteral("accent"), QStringLiteral("Accent"), QColor(Qt::blue));
        dialog.loadSettings();
        const auto *label = dialog.settingEditor(QStringLiteral("accent"))->findChild<QLabel *>();
        QCOMPARE(label->text(), QStringLiteral("#00ff00"));
        QCOMPARE(dialog.settingDefault(QStringLiteral("accent")).toString(), QStringLiteral("#0000ff"));
    }

    void restoreRemovesTheStoredValue()
    {
        {
            const auto settings = getSettings();
            settings->setValue(QStringLiteral("max_recent_files"), 30);
            settings->setValue(QStringLiteral("debug_mode"), true);
            settings->sync();
        }
        SettingsDialog dialog(nullptr, QStringLiteral("Settings"), true);
        QSignalSpy changed(&dialog, &SettingsDialog::settingChanged);
        auto *spin = editor<QSpinBox>(dialog, QStringLiteral("max_recent_files"));
        auto *check = editor<QCheckBox>(dialog, QStringLiteral("debug_mode"));
        QCOMPARE(spin->value(), 30);
        QVERIFY(dialog.isSettingModified(QStringLiteral("max_recent_files")));

        spin->setValue(31);
        dialog.restoreButton(QStringLiteral("max_recent_files"))->click();
        QCOMPARE(spin->value(), 10);
        QVERIFY(!dialog.isSettingModified(QStringLiteral("max_recent_files")));
        QVERIFY(!labelEmphasised(dialog, QStringLiteral("max_recent_files")));
        QVERIFY(!dialog.changedSettings().contains(QStringLiteral("max_recent_files")));

        // An edit after the restore wins over it.
        dialog.restoreSetting(QStringLiteral("debug_mode"));
        QVERIFY(!check->isChecked());
        check->setChecked(true);
        QVERIFY(dialog.isSettingModified(QStringLiteral("debug_mode")));

        QVERIFY(getSettings()->contains(QStringLiteral("max_recent_files")));
        dialog.accept();
        QVERIFY(!getSettings()->contains(QStringLiteral("max_recent_files")));
        QCOMPARE(getSettings()->value(QStringLiteral("debug_mode")).toBool(), true);
        QCOMPARE(changed.count(), 2);
        QCOMPARE(changed.at(0).at(0).toString(), QStringLiteral("max_recent_files"));
        QCOMPARE(changed.at(0).at(1), QVariant(10)); // the default
        QCOMPARE(changed.at(1).at(0).toString(), QStringLiteral("debug_mode"));
    }

    void everySettingType()
    {
        SettingsDialog dialog;
        QVERIFY(dialog.settingKeys().isEmpty());
        QTreeWidgetItem *category = dialog.addCategory(QStringLiteral("All"));
        dialog.addTextSetting(category, QStringLiteral("name"), QStringLiteral("Name"), QStringLiteral("Bob"));
        dialog.addFileSetting(category, QStringLiteral("firmware"), QStringLiteral("Firmware"), QStringLiteral("fw.bin"));
        dialog.addColorSetting(category, QStringLiteral("accent"), QStringLiteral("Accent"), QColor(Qt::blue));
        dialog.addFolderSetting(nullptr, QStringLiteral("export_dir"), QStringLiteral("Export"), QStringLiteral("out"));
        dialog.addThemeSetting(category);
        dialog.loadSettings();

        QCOMPARE(category->childCount(), 4);
        QCOMPARE(dialog.settingItem(QStringLiteral("export_dir"))->parent(), nullptr); // top level
        QCOMPARE(editor<QLineEdit>(dialog, QStringLiteral("name"))->toolTip(), QStringLiteral("Default: Bob"));
        QCOMPARE(editor<QLineEdit>(dialog, QStringLiteral("firmware"))->text(), QStringLiteral("fw.bin"));

        // Theme: the selectable labels, stored under the shared key.
        const QString themeKey = QString::fromLatin1(kThemeSettingKey);
        auto *theme = editor<QComboBox>(dialog, themeKey);
        QVERIFY(theme);
        QStringList items;
        for (int i = 0; i < theme->count(); ++i)
            items << theme->itemText(i);
        QCOMPARE(items, themeLabels());
        QCOMPARE(theme->currentText(), themeLabel(getTheme()));
        QCOMPARE(dialog.settingItem(themeKey)->text(0), QStringLiteral("Appearance (restart required)"));
        theme->setCurrentText(themeLabel(Theme::System));
        dialog.accept();
        QCOMPARE(getTheme(), Theme::System);
        setTheme(Theme::Light);

        // Colours are stored as "#rrggbb", whether given as QColor or name.
        auto *container = dialog.settingEditor(QStringLiteral("accent"));
        auto *label = container->findChild<QLabel *>();
        auto *button = container->findChild<QPushButton *>();
        QCOMPARE(label->text(), QStringLiteral("#0000ff"));
        QCOMPARE(button->toolTip(), QStringLiteral("Default: #0000ff"));
        dialog.setSetting(QStringLiteral("accent"), QColor(Qt::red));
        QCOMPARE(label->text(), QStringLiteral("#ff0000"));
        QCOMPARE(button->styleSheet(), QStringLiteral("background-color: #ff0000"));
        QVERIFY(iniText().contains(QStringLiteral("accent=#ff0000")));
        dialog.setSetting(QStringLiteral("accent"), QStringLiteral("#123456"));
        QCOMPARE(getSettings()->value(QStringLiteral("accent")).toString(), QStringLiteral("#123456"));
        dialog.restoreSetting(QStringLiteral("accent"));
        QCOMPARE(label->text(), QStringLiteral("#0000ff"));

        // setSetting shows and stores at once.
        dialog.setSetting(QStringLiteral("name"), QStringLiteral("Alice"));
        QCOMPARE(editor<QLineEdit>(dialog, QStringLiteral("name"))->text(), QStringLiteral("Alice"));
        QCOMPARE(getSettings()->value(QStringLiteral("name")).toString(), QStringLiteral("Alice"));
        // Keys without an editor are stored too.
        dialog.setSetting(QStringLiteral("hidden/key"), 5);
        QCOMPARE(getSettings()->value(QStringLiteral("hidden/key")).toInt(), 5);
    }

    void getSettingConverts()
    {
        SettingsDialog dialog;
        dialog.setSetting(QStringLiteral("count"), QStringLiteral("12"));
        dialog.setSetting(QStringLiteral("bad"), QStringLiteral("abc"));
        dialog.setSetting(QStringLiteral("flag"), true);

        const QVariant count = dialog.getSetting(QStringLiteral("count"), 0, QMetaType::fromType<int>());
        QCOMPARE(count.metaType(), QMetaType::fromType<int>());
        QCOMPARE(count.toInt(), 12);
        QCOMPARE(dialog.getSetting<int>(QStringLiteral("count"), 0), 12);
        QCOMPARE(dialog.getSetting<int>(QStringLiteral("bad"), 5), 5);
        QCOMPARE(dialog.getSetting<bool>(QStringLiteral("flag"), false, true), true);
        QCOMPARE(dialog.getSetting<QString>(QStringLiteral("missing"), QStringLiteral("x")), QStringLiteral("x"));
        QCOMPARE(dialog.getSetting(QStringLiteral("missing")), QVariant());
        QCOMPARE(dialog.getSetting(QStringLiteral("count")).toString(), QStringLiteral("12"));
    }

    void resetImportExport()
    {
        QTemporaryDir dir;
        const QString exported = dir.filePath(QStringLiteral("export.ini"));

        SettingsDialog dialog(nullptr, QStringLiteral("Settings"), true);
        auto *check = editor<QCheckBox>(dialog, QStringLiteral("debug_mode"));
        dialog.setSetting(QStringLiteral("debug_mode"), true);
        dialog.setSetting(QStringLiteral("max_recent_files"), 42);
        QVERIFY(check->isChecked());
        dialog.exportSettings(exported);
        QCOMPARE(QSettings(exported, QSettings::IniFormat).value(QStringLiteral("max_recent_files")).toInt(), 42);

        dialog.resetToDefaults();
        QVERIFY(!check->isChecked());
        QCOMPARE(editor<QSpinBox>(dialog, QStringLiteral("max_recent_files"))->value(), 10);
        QVERIFY(iniText().contains(QStringLiteral("debug_mode=false")));

        // Imported INI values are strings ("true", "42") and still land right.
        dialog.importSettings(exported);
        QVERIFY(check->isChecked());
        QCOMPARE(editor<QSpinBox>(dialog, QStringLiteral("max_recent_files"))->value(), 42);
        QCOMPARE(getSettings()->value(QStringLiteral("max_recent_files")).toInt(), 42);
    }

    void expandAndCollapse()
    {
        SettingsDialog dialog(nullptr, QStringLiteral("Settings"), true);
        QTreeWidget *tree = dialog.treeWidget();
        dialog.show();
        QVERIFY(QTest::qWaitForWindowActive(&dialog));
        tree->setFocus();

        // Toolbar buttons.
        buttonWithText(dialog, QStringLiteral("Collapse All"))->click();
        QVERIFY(allExpanded(tree, false));
        buttonWithText(dialog, QStringLiteral("Expand All"))->click();
        QVERIFY(allExpanded(tree, true));

        // Shortcuts.
        QTest::keyClick(tree, Qt::Key_Minus);
        QVERIFY(allExpanded(tree, false));
        QTest::keyClick(tree, Qt::Key_Plus);
        QVERIFY(allExpanded(tree, true));

        // Context menu.
        QStringList entries;
        QTimer::singleShot(0, this, [&entries] {
            auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
            if (!menu)
                return;
            for (QAction *action : menu->actions())
                entries << action->text();
            menu->actions().at(1)->trigger();
            menu->close();
        });
        emit tree->customContextMenuRequested(QPoint(5, 5));
        QCOMPARE(entries, (QStringList{QStringLiteral("Expand All\t+"), QStringLiteral("Collapse All\t-")}));
        QVERIFY(allExpanded(tree, false));
    }
};

QTEST_MAIN(TstSettingsDialog)
#include "tst_settingsdialog.moc"

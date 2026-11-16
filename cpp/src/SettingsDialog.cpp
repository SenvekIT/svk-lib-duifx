#include "duifx/SettingsDialog.h"

#include "duifx/Settings.h"
#include "duifx/Tables.h"
#include "duifx/Theming.h"
#include "Logging.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPalette>
#include <QPushButton>
#include <QSettings>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTreeWidget>

#include <algorithm>
#include <utility>

namespace duifx {

namespace {

// Height of a setting row (column 1), as in Python.
const QSize kItemSizeHint(150, 30);

const QString kLogDirectoryKey = QStringLiteral("log_directory");

// str(Path.home() / "logs") in Python: native separators.
QString defaultLogDirectory()
{
    return QDir::toNativeSeparators(QDir::homePath() + QStringLiteral("/logs"));
}

QString colorName(const QVariant &value)
{
    if (value.metaType() == QMetaType::fromType<QColor>())
        return value.value<QColor>().name();
    return value.toString();
}

QString colorButtonStyle(const QString &name)
{
    return QStringLiteral("background-color: %1").arg(name);
}

} // namespace

SettingsDialog::SettingsDialog(QWidget *parent, const QString &windowTitle, bool createDefaults)
    : HeaderDialog(parent, windowTitle)
    , m_settings(getSettings())
{
    initTreeWidget();
    if (createDefaults)
        createDefaultSettings();
    loadSettings();
    createLogFolder();
}

SettingsDialog::~SettingsDialog() = default;

// --- tree -----------------------------------------------------------------------------

void SettingsDialog::initTreeWidget()
{
    m_tree = new QTreeWidget(this);
    m_tree->setHeaderLabels({QStringLiteral("Setting"), QStringLiteral("Value"), QString()});
    // Every column draggable; the value column takes the width left over (and
    // keeps doing so until it is dragged itself). The editors and restore
    // buttons are item widgets, which content sizing does not see, so the
    // widths are set here rather than measured.
    ColumnSizer::Options columns;
    columns.fill = 1;
    columns.autoFit = false;
    makeColumnsInteractive(m_tree, columns);
    QHeaderView *header = m_tree->header();
    header->resizeSection(0, 250);
    header->resizeSection(2, 40);
    m_tree->setAlternatingRowColors(true);

    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tree, &QWidget::customContextMenuRequested, this, &SettingsDialog::showTreeContextMenu);

    // '+' expands all, '-' collapses all.
    auto *expandShortcut = new QShortcut(QKeySequence(Qt::Key_Plus), m_tree);
    expandShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(expandShortcut, &QShortcut::activated, this, &SettingsDialog::expandAll);
    auto *collapseShortcut = new QShortcut(QKeySequence(Qt::Key_Minus), m_tree);
    collapseShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(collapseShortcut, &QShortcut::activated, this, &SettingsDialog::collapseAll);

    auto *toolbar = new QWidget;
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    auto *expandButton = new QPushButton(QStringLiteral("Expand All"));
    expandButton->setToolTip(QStringLiteral("Expand all categories (+)"));
    connect(expandButton, &QPushButton::clicked, this, &SettingsDialog::expandAll);
    auto *collapseButton = new QPushButton(QStringLiteral("Collapse All"));
    collapseButton->setToolTip(QStringLiteral("Collapse all categories (-)"));
    connect(collapseButton, &QPushButton::clicked, this, &SettingsDialog::collapseAll);
    toolbarLayout->addWidget(expandButton);
    toolbarLayout->addWidget(collapseButton);
    toolbarLayout->addStretch();

    addContentWidget(toolbar);
    addContentWidget(m_tree);
}

void SettingsDialog::showTreeContextMenu(const QPoint &position)
{
    QMenu menu(m_tree);
    menu.addAction(QStringLiteral("Expand All\t+"), this, &SettingsDialog::expandAll);
    menu.addAction(QStringLiteral("Collapse All\t-"), this, &SettingsDialog::collapseAll);
    menu.exec(m_tree->viewport()->mapToGlobal(position));
}

void SettingsDialog::expandAll()
{
    m_tree->expandAll();
}

void SettingsDialog::collapseAll()
{
    m_tree->collapseAll();
}

void SettingsDialog::createLogFolder()
{
    const QString folder = m_settings->value(kLogDirectoryKey, defaultLogDirectory()).toString();
    if (!QDir(folder).exists()) {
        qCInfo(lcDuifx, "Creating log folder at: %s", qPrintable(folder));
        QDir().mkpath(folder);
    }
}

void SettingsDialog::createDefaultSettings()
{
    QTreeWidgetItem *general = addCategory(QStringLiteral("General"));
    addComboSetting(general, QStringLiteral("language"), QStringLiteral("Language"),
                    {QStringLiteral("English"), QStringLiteral("Italiano"), QStringLiteral("Deutsch")},
                    QStringLiteral("English"));

    QTreeWidgetItem *advanced = addCategory(QStringLiteral("Advanced"));
    addBooleanSetting(advanced, QStringLiteral("debug_mode"), QStringLiteral("Debug Mode"), false);
    addIntegerSetting(advanced, QStringLiteral("max_recent_files"), QStringLiteral("Max Recent Files"), 10, 1, 50);
    addFolderSetting(advanced, kLogDirectoryKey, QStringLiteral("Log Directory"), defaultLogDirectory());

    expandAll();
}

// --- adding settings ------------------------------------------------------------------

QTreeWidgetItem *SettingsDialog::addCategory(const QString &name)
{
    auto *item = new QTreeWidgetItem(QStringList{name, QString()});
    item->setSizeHint(1, kItemSizeHint);
    item->setFlags(Qt::ItemIsEnabled); // not editable, not selectable
    const QBrush background = QGuiApplication::palette().alternateBase();
    item->setBackground(0, background);
    item->setBackground(1, background);
    m_tree->addTopLevelItem(item);
    return item;
}

QTreeWidgetItem *SettingsDialog::addSettingItem(QTreeWidgetItem *parent, const QString &label)
{
    auto *item = new QTreeWidgetItem(QStringList{label, QString()});
    item->setSizeHint(1, kItemSizeHint);
    if (parent)
        parent->addChild(item);
    else
        m_tree->addTopLevelItem(item);
    return item;
}

void SettingsDialog::registerSetting(const QString &key, const Entry &entry)
{
    if (!m_entries.contains(key))
        m_keys.append(key);
    m_entries.insert(key, entry);
    addRestoreButton(key);
}

void SettingsDialog::addBooleanSetting(QTreeWidgetItem *parent, const QString &key, const QString &label,
                                       bool defaultValue)
{
    QTreeWidgetItem *item = addSettingItem(parent, label);
    auto *checkbox = new QCheckBox;
    checkbox->setChecked(defaultValue);
    connect(checkbox, &QCheckBox::toggled, this, [this, key](bool checked) { onSettingChanged(key, checked); });
    checkbox->setToolTip(QStringLiteral("Default: %1").arg(defaultValue ? QStringLiteral("Enabled") : QStringLiteral("Disabled")));
    m_tree->setItemWidget(item, 1, checkbox);
    registerSetting(key, {item, checkbox, nullptr, nullptr, nullptr, defaultValue});
}

void SettingsDialog::addIntegerSetting(QTreeWidgetItem *parent, const QString &key, const QString &label,
                                       int defaultValue, int minValue, int maxValue)
{
    QTreeWidgetItem *item = addSettingItem(parent, label);
    auto *spinbox = new QSpinBox;
    spinbox->setRange(minValue, maxValue);
    spinbox->setValue(defaultValue);
    connect(spinbox, &QSpinBox::valueChanged, this, [this, key](int value) { onSettingChanged(key, value); });
    spinbox->setToolTip(QStringLiteral("Default: %1, Range: %2 - %3").arg(defaultValue).arg(minValue).arg(maxValue));
    m_tree->setItemWidget(item, 1, spinbox);
    registerSetting(key, {item, spinbox, nullptr, nullptr, nullptr, defaultValue});
}

void SettingsDialog::addTextSetting(QTreeWidgetItem *parent, const QString &key, const QString &label,
                                    const QString &defaultValue)
{
    QTreeWidgetItem *item = addSettingItem(parent, label);
    auto *lineEdit = new QLineEdit;
    lineEdit->setText(defaultValue);
    connect(lineEdit, &QLineEdit::textChanged, this, [this, key](const QString &text) { onSettingChanged(key, text); });
    lineEdit->setToolTip(QStringLiteral("Default: %1").arg(defaultValue));
    m_tree->setItemWidget(item, 1, lineEdit);
    registerSetting(key, {item, lineEdit, nullptr, nullptr, nullptr, defaultValue});
}

void SettingsDialog::addComboSetting(QTreeWidgetItem *parent, const QString &key, const QString &label,
                                     const QStringList &options, const QString &defaultValue)
{
    QTreeWidgetItem *item = addSettingItem(parent, label);
    auto *combobox = new QComboBox;
    combobox->addItems(options);
    combobox->setCurrentText(defaultValue);
    connect(combobox, &QComboBox::currentTextChanged, this,
            [this, key](const QString &text) { onSettingChanged(key, text); });
    combobox->setToolTip(QStringLiteral("Default: %1").arg(defaultValue));
    m_tree->setItemWidget(item, 1, combobox);
    registerSetting(key, {item, combobox, nullptr, nullptr, nullptr, defaultValue});
}

void SettingsDialog::addThemeSetting(QTreeWidgetItem *parent, const QString &label)
{
    addComboSetting(parent, QString::fromLatin1(kThemeSettingKey), label, themeLabels(), themeLabel(getTheme()));
}

void SettingsDialog::addColorSetting(QTreeWidgetItem *parent, const QString &key, const QString &label,
                                     const QColor &defaultValue)
{
    QTreeWidgetItem *item = addSettingItem(parent, label);
    const QString defaultName = defaultValue.name();

    auto *widget = new QWidget;
    auto *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *colorButton = new QPushButton;
    colorButton->setFixedSize(50, 25);
    colorButton->setStyleSheet(colorButtonStyle(defaultName));
    colorButton->setToolTip(QStringLiteral("Default: %1").arg(defaultName));
    auto *colorLabel = new QLabel(defaultName);

    // As in Python the dialog starts at the default colour, not the current one.
    connect(colorButton, &QPushButton::clicked, this, [this, key, defaultValue, colorButton, colorLabel] {
        const QColor chosen = QColorDialog::getColor(defaultValue, this, QStringLiteral("Choose Color"));
        if (chosen.isValid()) {
            colorButton->setStyleSheet(colorButtonStyle(chosen.name()));
            colorLabel->setText(chosen.name());
            onSettingChanged(key, chosen.name());
        }
    });

    layout->addWidget(colorButton);
    layout->addWidget(colorLabel);
    layout->addStretch();

    m_tree->setItemWidget(item, 1, widget);
    registerSetting(key, {item, widget, colorButton, colorLabel, nullptr, defaultName});
}

void SettingsDialog::addPathSetting(QTreeWidgetItem *parent, const QString &key, const QString &label,
                                     const QString &defaultValue, bool folder)
{
    QTreeWidgetItem *item = addSettingItem(parent, label);

    auto *widget = new QWidget;
    auto *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *lineEdit = new QLineEdit;
    lineEdit->setText(defaultValue);
    connect(lineEdit, &QLineEdit::textChanged, this, [this, key](const QString &text) { onSettingChanged(key, text); });

    auto *browseButton = new QPushButton(QStringLiteral("Browse..."));
    browseButton->setFixedWidth(80);
    connect(browseButton, &QPushButton::clicked, this, [this, label, lineEdit, folder] {
        const QString caption = QStringLiteral("Select %1").arg(label);
        const QString chosen = folder ? QFileDialog::getExistingDirectory(this, caption, lineEdit->text())
                                      : QFileDialog::getOpenFileName(this, caption, lineEdit->text());
        if (!chosen.isEmpty())
            lineEdit->setText(chosen);
    });

    layout->addWidget(lineEdit);
    layout->addWidget(browseButton);

    m_tree->setItemWidget(item, 1, widget);
    registerSetting(key, {item, lineEdit, nullptr, nullptr, nullptr, defaultValue});
}

void SettingsDialog::addFileSetting(QTreeWidgetItem *parent, const QString &key, const QString &label,
                                    const QString &defaultValue)
{
    addPathSetting(parent, key, label, defaultValue, false);
}

void SettingsDialog::addFolderSetting(QTreeWidgetItem *parent, const QString &key, const QString &label,
                                      const QString &defaultValue)
{
    addPathSetting(parent, key, label, defaultValue, true);
}

void SettingsDialog::addRestoreButton(const QString &key)
{
    Entry &entry = m_entries[key];
    auto *button = new QPushButton(QString(QChar(0x21BA))); // U+21BA, the anticlockwise arrow
    button->setObjectName(QStringLiteral("RestoreButton"));
    button->setToolTip(QStringLiteral("Restore"));
    button->setFixedSize(50, 20);
    button->setCursor(Qt::PointingHandCursor);
    button->setEnabled(false);
    connect(button, &QPushButton::clicked, this, [this, key] { restoreSetting(key); });
    m_tree->setItemWidget(entry.item, 2, button);
    entry.restore = button;
}

// --- state ----------------------------------------------------------------------------

QTreeWidgetItem *SettingsDialog::settingItem(const QString &key) const
{
    const auto it = m_entries.constFind(key);
    return it == m_entries.cend() ? nullptr : it->item;
}

QWidget *SettingsDialog::settingEditor(const QString &key) const
{
    const auto it = m_entries.constFind(key);
    return it == m_entries.cend() ? nullptr : it->editor;
}

QVariant SettingsDialog::settingDefault(const QString &key) const
{
    const auto it = m_entries.constFind(key);
    return it == m_entries.cend() ? QVariant() : it->defaultValue;
}

QPushButton *SettingsDialog::restoreButton(const QString &key) const
{
    const auto it = m_entries.constFind(key);
    return it == m_entries.cend() ? nullptr : it->restore;
}

QVariantMap SettingsDialog::changedSettings() const
{
    QVariantMap changed;
    for (const auto &[key, value] : m_changed)
        changed.insert(key, value);
    return changed;
}

bool SettingsDialog::isSettingModified(const QString &key) const
{
    const QPushButton *button = restoreButton(key);
    return button && button->isEnabled();
}

void SettingsDialog::updateItemStyle(const QString &key, bool modified)
{
    const auto it = m_entries.constFind(key);
    if (it == m_entries.cend())
        return;
    QFont font = it->item->font(0);
    font.setBold(modified);
    font.setItalic(modified);
    it->item->setFont(0, font);
    if (it->restore)
        it->restore->setEnabled(modified);
}

void SettingsDialog::showValue(const Entry &entry, const QVariant &value)
{
    if (entry.colorButton) {
        const QString name = colorName(value);
        entry.colorButton->setStyleSheet(colorButtonStyle(name));
        entry.colorLabel->setText(name);
    } else if (auto *checkbox = qobject_cast<QCheckBox *>(entry.editor)) {
        checkbox->setChecked(value.toBool());
    } else if (auto *spinbox = qobject_cast<QSpinBox *>(entry.editor)) {
        bool ok = false;
        const int number = value.toInt(&ok);
        spinbox->setValue(ok ? number : entry.defaultValue.toInt());
    } else if (auto *lineEdit = qobject_cast<QLineEdit *>(entry.editor)) {
        lineEdit->setText(value.toString());
    } else if (auto *combobox = qobject_cast<QComboBox *>(entry.editor)) {
        combobox->setCurrentText(value.toString());
    }
}

void SettingsDialog::takeChanged(const QString &key)
{
    m_changed.removeIf([&key](const auto &change) { return change.first == key; });
}

void SettingsDialog::onSettingChanged(const QString &key, const QVariant &value)
{
    auto it = std::find_if(m_changed.begin(), m_changed.end(), [&key](const auto &change) { return change.first == key; });
    if (it != m_changed.end())
        it->second = value;
    else
        m_changed.append({key, value});
    m_keysToRestore.removeAll(key);
    updateItemStyle(key, true);
}

void SettingsDialog::restoreSetting(const QString &key)
{
    const auto it = m_entries.constFind(key);
    if (it == m_entries.cend())
        return;
    {
        // The editor must not report the default as an edit.
        const QSignalBlocker blocker(it->editor);
        showValue(*it, it->defaultValue);
    }
    if (!m_keysToRestore.contains(key))
        m_keysToRestore.append(key);
    takeChanged(key);
    updateItemStyle(key, false);
}

// --- values ---------------------------------------------------------------------------

void SettingsDialog::accept()
{
    // Restored settings are removed, so the defaults apply on the next load.
    if (!m_keysToRestore.isEmpty()) {
        const QStringList restored = std::exchange(m_keysToRestore, {});
        for (const QString &key : restored) {
            m_settings->remove(key);
            takeChanged(key);
            qCInfo(lcDuifx, "Setting restored to default: %s", qPrintable(key));
        }
        m_settings->sync();
        for (const QString &key : restored) {
            if (m_entries.contains(key))
                emit settingChanged(key, m_entries.value(key).defaultValue);
        }
    }

    if (!m_changed.isEmpty()) {
        const auto changed = m_changed;
        for (const auto &[key, value] : changed) {
            m_settings->setValue(key, value);
            qCInfo(lcDuifx, "Setting changed: %s = %s", qPrintable(key), qPrintable(value.toString()));
        }
        m_settings->sync();
        for (const auto &[key, value] : changed)
            emit settingChanged(key, value);
        m_changed.clear();
    }
    HeaderDialog::accept();
}

QVariant SettingsDialog::getSetting(const QString &key, const QVariant &defaultValue, QMetaType type, bool sync) const
{
    if (sync)
        m_settings->sync();
    QVariant value = m_settings->value(key, defaultValue);
    if (!type.isValid() || value.metaType() == type)
        return value;
    // Python's value(type=...) raising for an unconvertible value: the default.
    if (!value.isValid() || !value.convert(type))
        return defaultValue;
    return value;
}

void SettingsDialog::setSetting(const QString &key, const QVariant &value)
{
    QVariant stored = value;
    const auto it = m_entries.constFind(key);
    if (it != m_entries.cend()) {
        // A QColor would be written as "@Variant(...)"; Python apps read "#rrggbb".
        if (it->colorButton)
            stored = colorName(value);
        showValue(*it, value);
    }
    m_settings->setValue(key, stored);
    m_settings->sync();
}

void SettingsDialog::loadSettings()
{
    for (const QString &key : std::as_const(m_keys)) {
        const Entry entry = m_entries.value(key);
        const bool hasSaved = m_settings->contains(key);
        showValue(entry, m_settings->value(key, entry.defaultValue));
        updateItemStyle(key, hasSaved);
    }
    m_changed.clear();
}

void SettingsDialog::resetToDefaults()
{
    for (const QString &key : std::as_const(m_keys))
        setSetting(key, m_entries.value(key).defaultValue);
}

void SettingsDialog::exportSettings(const QString &filePath) const
{
    QSettings exported(filePath, QSettings::IniFormat);
    for (const QString &key : m_settings->allKeys())
        exported.setValue(key, m_settings->value(key));
    exported.sync();
}

void SettingsDialog::importSettings(const QString &filePath)
{
    const QSettings imported(filePath, QSettings::IniFormat);
    for (const QString &key : imported.allKeys())
        setSetting(key, imported.value(key));
}

} // namespace duifx

from pathlib import Path
import logging
from typing import Any, Dict, Optional, Type, TypeVar
from PySide6.QtWidgets import (QTreeWidget, QTreeWidgetItem, QWidget, QHBoxLayout,
                              QCheckBox, QSpinBox, QComboBox, QLineEdit, QPushButton,
                              QLabel, QColorDialog, QFileDialog, QApplication, QDialog,
                              QHeaderView, QMenu)
from PySide6.QtCore import QSettings, Qt, Signal, QSize, QPoint
from PySide6.QtGui import QColor, QFont, QPalette, QIcon, QAction, QKeySequence, QShortcut

from duifx.gui.windows import HeaderDialog

logger = logging.getLogger(__name__)

T = TypeVar('T')

def get_settings() -> QSettings:
    """Creates QSettings in application directory"""
    app = QApplication.instance()
    if app:
        app_name = app.applicationName() or "DUIFx"
        org_name = app.organizationName() or "DUIFx"
    else:
        app_name = "DUIFx"
        org_name = "DUIFx"
    
    import sys

    # Settings file in application directory
    if sys.platform == "win32":
        # Windows: %APPDATA%/Organisation/Application/settings.ini
        settings_dir = Path.home() / "AppData" / "Roaming" / org_name / app_name
    else:
        # Linux/Mac: ~/.config/Organisation/Application/settings.ini
        settings_dir = Path.home() / ".config" / org_name / app_name

    settings_dir.mkdir(parents=True, exist_ok=True)
    settings_file = settings_dir / "settings.ini"

    return QSettings(str(settings_file), QSettings.Format.IniFormat)

class SettingsDialog(HeaderDialog):

    # Signal is emitted when a setting changes
    setting_changed = Signal(str, object)  # key, value

    def __init__(self, parent=None, window_title: str = "Settings", create_defaults: bool = False):
        super().__init__(parent, window_title)

        self.treeWidget = None
        # Initialize settings
        self.settings = get_settings()
        self.setting_items = {}  # key -> QTreeWidgetItem mapping
        self._restore_buttons = {}  # key -> QPushButton
        self.changed_setting_items = {}
        self._keys_to_restore = set()  # keys to remove from QSettings on accept

        self._init_tree_widget()

        # Add default settings if requested
        if create_defaults:
            self._create_default_settings()

        # Load settings
        self.load_settings()

        self._create_log_folder()

    def _init_tree_widget(self):
        """Initializes the tree widget"""
        self.treeWidget = QTreeWidget(self)

        # Configure tree widget
        self.treeWidget.setHeaderLabels(["Setting", "Value", ""])
        self.treeWidget.setColumnWidth(0, 250)
        header = self.treeWidget.header()
        header.setStretchLastSection(False)
        header.setSectionResizeMode(0, QHeaderView.ResizeMode.Interactive)
        header.setSectionResizeMode(1, QHeaderView.ResizeMode.Stretch)
        header.setSectionResizeMode(2, QHeaderView.ResizeMode.ResizeToContents)
        header.resizeSection(2, 40)
        self.treeWidget.setAlternatingRowColors(True)

        # Context menu for expand/collapse all
        self.treeWidget.setContextMenuPolicy(Qt.ContextMenuPolicy.CustomContextMenu)
        self.treeWidget.customContextMenuRequested.connect(self._show_tree_context_menu)

        # Keyboard shortcuts: '+' expands all, '-' collapses all
        expand_shortcut = QShortcut(QKeySequence(Qt.Key.Key_Plus), self.treeWidget)
        expand_shortcut.setContext(Qt.ShortcutContext.WidgetWithChildrenShortcut)
        expand_shortcut.activated.connect(self.expand_all)

        collapse_shortcut = QShortcut(QKeySequence(Qt.Key.Key_Minus), self.treeWidget)
        collapse_shortcut.setContext(Qt.ShortcutContext.WidgetWithChildrenShortcut)
        collapse_shortcut.activated.connect(self.collapse_all)

        # Toolbar with expand/collapse all buttons above the tree
        toolbar = QWidget()
        toolbar_layout = QHBoxLayout(toolbar)
        toolbar_layout.setContentsMargins(0, 0, 0, 0)

        expand_button = QPushButton("Expand All")
        expand_button.setToolTip("Expand all categories (+)")
        expand_button.clicked.connect(self.expand_all)

        collapse_button = QPushButton("Collapse All")
        collapse_button.setToolTip("Collapse all categories (-)")
        collapse_button.clicked.connect(self.collapse_all)

        toolbar_layout.addWidget(expand_button)
        toolbar_layout.addWidget(collapse_button)
        toolbar_layout.addStretch()

        # add widgets to layout
        self.add_content_widget(toolbar)
        self.add_content_widget(self.treeWidget)

    def _show_tree_context_menu(self, position: QPoint):
        """Shows a context menu with expand/collapse all actions."""
        menu = QMenu(self.treeWidget)

        expand_action = QAction("Expand All\t+", menu)
        expand_action.triggered.connect(self.expand_all)
        menu.addAction(expand_action)

        collapse_action = QAction("Collapse All\t-", menu)
        collapse_action.triggered.connect(self.collapse_all)
        menu.addAction(collapse_action)

        menu.exec(self.treeWidget.viewport().mapToGlobal(position))

    def collapse_all(self):
        """Collapses all categories in the tree widget."""
        self.treeWidget.collapseAll()

    def _create_log_folder(self):
        """Creates log folder if it does not exist"""
        log_file_path = self.settings.value("log_directory", str(Path.home() / "logs"), type=str)
        log_folder = Path(str(log_file_path))
        if not log_folder.exists():
            logger.info(f"Creating log folder at: {log_folder}")
            log_folder.mkdir(parents=True, exist_ok=True)

    def _create_default_settings(self):
        """Creates default settings"""

        # Category: General
        general_cat = self.add_category("General")
        self.add_combo_setting(general_cat, "language", "Language", ["English", "Italiano", "Deutsch"], "English")

        # Category: Advanced
        advanced_cat = self.add_category("Advanced")
        self.add_boolean_setting(advanced_cat, "debug_mode", "Debug Mode", False)
        self.add_integer_setting(advanced_cat, "max_recent_files", "Max Recent Files", 10, 1, 50)
        self.add_folder_setting(advanced_cat, "log_directory", "Log Directory", str(Path.home() / "logs"))

        # Expand tree
        self.expand_all()

    def expand_all(self):
        """Expands all categories in the tree widget"""
        self.treeWidget.expandAll()

    def add_category(self, name: str) -> QTreeWidgetItem:
        """Adds a category"""
        item = QTreeWidgetItem([name, ""])
        item.setSizeHint(1, QSize(150, 30))
        item.setFlags(Qt.ItemFlag.ItemIsEnabled)  # Not editable
        item.setBackground(0, QPalette().alternateBase())
        item.setBackground(1, QPalette().alternateBase())
        self.treeWidget.addTopLevelItem(item)
        return item

    def add_boolean_setting(self, parent: QTreeWidgetItem, key: str, label: str, default_value: bool):
        """Adds a boolean setting"""
        item = QTreeWidgetItem([label, ""])
        item.setSizeHint(1, QSize(150, 30))
        parent.addChild(item)

        # CheckBox Widget
        checkbox = QCheckBox()
        checkbox.setChecked(default_value)
        checkbox.toggled.connect(lambda checked: self._on_setting_changed(key, checked))

        checkbox.setToolTip(f"Default: {'Enabled' if default_value else 'Disabled'}")

        self.treeWidget.setItemWidget(item, 1, checkbox)
        self.setting_items[key] = (item, checkbox, default_value)
        self._add_restore_button(item, key)

    def add_integer_setting(self, parent: QTreeWidgetItem, key: str, label: str,
                            default_value: int, min_val: int = 0, max_val: int = 100):
        """Adds an integer setting"""
        item = QTreeWidgetItem([label, ""])
        item.setSizeHint(1, QSize(150, 30))
        parent.addChild(item)

        # SpinBox Widget
        spinbox = QSpinBox()
        spinbox.setRange(min_val, max_val)
        spinbox.setValue(default_value)
        spinbox.valueChanged.connect(lambda value: self._on_setting_changed(key, value))

        spinbox.setToolTip(f"Default: {default_value}, Range: {min_val} - {max_val}")

        self.treeWidget.setItemWidget(item, 1, spinbox)
        self.setting_items[key] = (item, spinbox, default_value)
        self._add_restore_button(item, key)

    def add_text_setting(self, parent: QTreeWidgetItem, key: str, label: str, default_value: str):
        """Adds a text setting"""
        item = QTreeWidgetItem([label, ""])
        item.setSizeHint(1, QSize(150, 30))
        parent.addChild(item)

        # LineEdit Widget
        lineedit = QLineEdit()
        lineedit.setText(default_value)
        lineedit.textChanged.connect(lambda text: self._on_setting_changed(key, text))

        lineedit.setToolTip(f"Default: {default_value}")

        self.treeWidget.setItemWidget(item, 1, lineedit)
        self.setting_items[key] = (item, lineedit, default_value)
        self._add_restore_button(item, key)

    def add_theme_setting(self, parent: QTreeWidgetItem,
                          label: str = "Appearance (restart required)"):
        """Adds the application theme selector.

        The options come from :func:`duifx.gui.theming.selectable_themes`,
        so themes that are not implemented yet are not offered. Applied on
        the next start -- a palette swap does not fully reach widgets that
        already exist.
        """
        from duifx.gui.theming import (
            THEME_LABELS, THEME_SETTING_KEY, get_theme, theme_labels,
        )

        self.add_combo_setting(
            parent, THEME_SETTING_KEY, label,
            theme_labels(), THEME_LABELS[get_theme()],
        )

    def add_combo_setting(self, parent: QTreeWidgetItem, key: str, label: str,
                          options: list, default_value: str):
        """Adds a ComboBox setting"""
        item = QTreeWidgetItem([label, ""])
        item.setSizeHint(1, QSize(150, 30))
        parent.addChild(item)

        # ComboBox Widget
        combobox = QComboBox()
        combobox.addItems(options)
        combobox.setCurrentText(default_value)
        combobox.currentTextChanged.connect(lambda text: self._on_setting_changed(key, text))

        combobox.setToolTip(f"Default: {default_value}")

        self.treeWidget.setItemWidget(item, 1, combobox)
        self.setting_items[key] = (item, combobox, default_value)
        self._add_restore_button(item, key)

    def add_color_setting(self, parent: QTreeWidgetItem, key: str, label: str, default_value: QColor):
        """Adds a color setting"""
        item = QTreeWidgetItem([label, ""])
        item.setSizeHint(1, QSize(150, 30))
        parent.addChild(item)

        # Color Button Widget
        widget = QWidget()
        layout = QHBoxLayout(widget)
        layout.setContentsMargins(0, 0, 0, 0)

        color_button = QPushButton()
        color_button.setFixedSize(50, 25)
        color_button.setStyleSheet(f"background-color: {default_value.name()}")

        color_button.setToolTip(f"Default: {default_value}")

        color_label = QLabel(default_value.name())

        def choose_color():
            color = QColorDialog.getColor(default_value, self, "Choose Color")
            if color.isValid():
                color_button.setStyleSheet(f"background-color: {color.name()}")
                color_label.setText(color.name())
                self._on_setting_changed(key, color.name())

        color_button.clicked.connect(choose_color)

        layout.addWidget(color_button)
        layout.addWidget(color_label)
        layout.addStretch()

        self.treeWidget.setItemWidget(item, 1, widget)
        self.setting_items[key] = (item, (color_button, color_label), default_value.name())
        self._add_restore_button(item, key)

    def add_file_setting(self, parent: QTreeWidgetItem, key: str, label: str, default_value: str):
        """Adds a file selection setting"""
        item = QTreeWidgetItem([label, ""])
        item.setSizeHint(1, QSize(150, 30))
        parent.addChild(item)

        # File Selection Widget
        widget = QWidget()
        layout = QHBoxLayout(widget)
        layout.setContentsMargins(0, 0, 0, 0)

        lineedit = QLineEdit()
        lineedit.setText(default_value)
        lineedit.textChanged.connect(lambda text: self._on_setting_changed(key, text))

        browse_button = QPushButton("Browse...")
        browse_button.setFixedWidth(80)

        def browse_file():
            file_path, _ = QFileDialog.getOpenFileName(
                self, f"Select {label}", lineedit.text()
            )
            if file_path:
                lineedit.setText(file_path)

        browse_button.clicked.connect(browse_file)

        layout.addWidget(lineedit)
        layout.addWidget(browse_button)

        self.treeWidget.setItemWidget(item, 1, widget)
        self.setting_items[key] = (item, lineedit, default_value)
        self._add_restore_button(item, key)

    def add_folder_setting(self, parent: QTreeWidgetItem, key: str, label: str, default_value: str):
        """Adds a folder selection setting"""
        item = QTreeWidgetItem([label, ""])
        item.setSizeHint(1, QSize(150, 30))
        if parent:
            parent.addChild(item)

        # Folder Selection Widget
        widget = QWidget()
        layout = QHBoxLayout(widget)
        layout.setContentsMargins(0, 0, 0, 0)

        lineedit = QLineEdit()
        lineedit.setText(default_value)
        lineedit.textChanged.connect(lambda text: self._on_setting_changed(key, text))

        browse_button = QPushButton("Browse...")
        browse_button.setFixedWidth(80)

        def browse_folder():
            folder_path = QFileDialog.getExistingDirectory(
                self, f"Select {label}", lineedit.text()
            )
            if folder_path:
                lineedit.setText(folder_path)

        browse_button.clicked.connect(browse_folder)

        layout.addWidget(lineedit)
        layout.addWidget(browse_button)

        self.treeWidget.setItemWidget(item, 1, widget)
        self.setting_items[key] = (item, lineedit, default_value)
        self._add_restore_button(item, key)

    def _add_restore_button(self, item: QTreeWidgetItem, key: str):
        """Adds a small restore button in column 2 for the given setting."""
        btn = QPushButton("\u21ba")
        btn.setObjectName("RestoreButton")
        btn.setToolTip("Restore")
        btn.setFixedSize(50, 20)
        btn.setCursor(Qt.CursorShape.PointingHandCursor)
        btn.setEnabled(False)
        btn.clicked.connect(lambda: self._restore_setting(key))
        self.treeWidget.setItemWidget(item, 2, btn)
        self._restore_buttons[key] = btn

    def _restore_setting(self, key: str):
        """Resets a single setting to its default value and marks it for removal from QSettings."""
        if key not in self.setting_items:
            return
        item, widget, default_value = self.setting_items[key]

        if isinstance(widget, tuple):  # Color widget
            color_button, color_label = widget
            color_button.setStyleSheet(f"background-color: {default_value}")
            color_label.setText(str(default_value))
        else:
            widget.blockSignals(True)
            if isinstance(widget, QCheckBox):
                widget.setChecked(default_value)
            elif isinstance(widget, QSpinBox):
                widget.setValue(default_value)
            elif isinstance(widget, QLineEdit):
                widget.setText(str(default_value))
            elif isinstance(widget, QComboBox):
                widget.setCurrentText(str(default_value))
            widget.blockSignals(False)

        self._keys_to_restore.add(key)
        self.changed_setting_items.pop(key, None)
        self._update_item_style(key, modified=False)

    def _update_item_style(self, key: str, modified: bool):
        """Sets bold+italic on the label and enables/disables the restore button."""
        if key not in self.setting_items:
            return
        item = self.setting_items[key][0]
        font = item.font(0)
        font.setBold(modified)
        font.setItalic(modified)
        item.setFont(0, font)
        if key in self._restore_buttons:
            self._restore_buttons[key].setEnabled(modified)

    def _on_setting_changed(self, key: str, value: Any):
        """Called when a setting changes"""
        self.changed_setting_items[key] = value
        self._keys_to_restore.discard(key)
        self._update_item_style(key, modified=True)

    def accept(self):
        # Remove restored settings from QSettings so defaults are used on next load
        if self._keys_to_restore:
            for key in self._keys_to_restore:
                self.settings.remove(key)
                self.changed_setting_items.pop(key, None)
                logger.info("Setting restored to default: %s", key)
            self.settings.sync()
            for key in self._keys_to_restore:
                if key in self.setting_items:
                    _, _, default_value = self.setting_items[key]
                    self.setting_changed.emit(key, default_value)
            self._keys_to_restore.clear()

        if self.changed_setting_items:
            for key, value in self.changed_setting_items.items():
                self.settings.setValue(key, value)
                logger.info("Setting changed: %s = %s", key, value)
            self.settings.sync()
            for key, value in self.changed_setting_items.items():
                self.setting_changed.emit(key, value)
            self.changed_setting_items.clear()
        super().accept()

    def get_setting(self, key: str, default_value: Any = None, value_type: Type[T] = None, sync: bool = False) -> Any:
        """Gets a setting"""
        if sync:
            self.settings.sync()

        try:
            return self.settings.value(key, default_value, type=value_type)
        except (ValueError, TypeError) as e:
            return default_value

    def set_setting(self, key: str, value: Any):
        """Sets a setting"""
        if key in self.setting_items:
            item, widget, default = self.setting_items[key]

            # Update widget
            if isinstance(widget, QCheckBox):
                widget.setChecked(bool(value))
            elif isinstance(widget, QSpinBox):
                widget.setValue(int(value))
            elif isinstance(widget, QLineEdit):
                widget.setText(str(value))
            elif isinstance(widget, QComboBox):
                widget.setCurrentText(str(value))
            elif isinstance(widget, tuple):  # Color widget
                color_button, color_label = widget
                color_button.setStyleSheet(f"background-color: {value}")
                color_label.setText(str(value))

        # Save to QSettings
        self.settings.setValue(key, value)
        self.settings.sync()

    def load_settings(self):
        """Loads all settings from QSettings"""
        for key, (item, widget, default_value) in self.setting_items.items():
            saved_value = self.settings.value(key, default_value)
            has_saved = self.settings.contains(key)

            # Update widget with saved value
            if isinstance(widget, QCheckBox):
                widget.setChecked(bool(self.settings.value(key, default_value, type=bool)))
            elif isinstance(widget, QSpinBox):
                widget.setValue(self.settings.value(key, default_value, type=int))
            elif isinstance(widget, QLineEdit):
                widget.setText(str(self.settings.value(key, default_value, type=str)))
            elif isinstance(widget, QComboBox):
                widget.setCurrentText(str(self.settings.value(key, default_value, type=str)))
            elif isinstance(widget, tuple):  # Color widget
                color_button, color_label = widget
                color_button.setStyleSheet(f"background-color: {saved_value}")
                color_label.setText(str(self.settings.value(key, default_value, type=str)))

            self._update_item_style(key, modified=has_saved)
        
        self.changed_setting_items.clear()

    def reset_to_defaults(self):
        """Resets all settings to default values"""
        for key, (item, widget, default_value) in self.setting_items.items():
            self.set_setting(key, default_value)

    def export_settings(self, file_path: str):
        """Exports settings to a file"""
        export_settings = QSettings(file_path, QSettings.Format.IniFormat)
        for key in self.settings.allKeys():
            export_settings.setValue(key, self.settings.value(key))
        export_settings.sync()

    def import_settings(self, file_path: str):
        """Imports settings from a file"""
        import_settings = QSettings(file_path, QSettings.Format.IniFormat)
        for key in import_settings.allKeys():
            value = import_settings.value(key)
            self.set_setting(key, value)
    



        

        

    

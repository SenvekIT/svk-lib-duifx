from pathlib import Path
from typing import Any, Dict, Optional
from PySide6.QtWidgets import (QTreeWidget, QTreeWidgetItem, QWidget, QHBoxLayout,
                              QCheckBox, QSpinBox, QComboBox, QLineEdit, QPushButton,
                              QLabel, QColorDialog, QFileDialog, QApplication, QDialog)
from PySide6.QtCore import QSettings, Qt, Signal, QSize
from PySide6.QtGui import QColor, QPalette, QIcon

from duifx.gui import HeaderDialog

RESOURCES_PATH = Path(__file__).parent / "resources"
IMAGES_PATH = RESOURCES_PATH / "image"
ICONS_PATH = RESOURCES_PATH / "icon"

def load_stylesheet():
    import os
    
    style_path = os.path.join(os.path.dirname(__file__), "resources", "modern_light.qss")
    
    try:
        with open(style_path, 'r', encoding='utf-8') as file:
            return file.read()
    except FileNotFoundError:
        # Return inline styles as fallback
        return """
        /* Fallback styles here */
        QMainWindow { background-color: #ffffff; }
        QPushButton { 
            background-color: #007bff; 
            color: white; 
            border: none; 
            padding: 8px 16px; 
            border-radius: 4px; 
        }
        """

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

        self.init_tree_widget()

        # Add default settings if requested
        if create_defaults:
            self._create_default_settings()

        # Load settings
        self.load_settings()

        self.create_log_folder()

    def init_tree_widget(self):
        """Initializes the tree widget"""
        self.treeWidget = QTreeWidget(self)

        # Configure tree widget
        self.treeWidget.setHeaderLabels(["Setting", "Value"])
        self.treeWidget.setColumnWidth(0, 250)
        self.treeWidget.setAlternatingRowColors(True)

        # add widget to layout
        self.add_content_widget(self.treeWidget)

    def create_log_folder(self):
        """Creates log folder if it does not exist"""
        log_file_path = self.settings.value("log_directory", str(Path.home() / "logs"))
        log_folder = Path(log_file_path)
        print(f"Creating log folder at: {log_folder}")
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
        item.setSizeHint(1, QSize(150, 35))
        item.setFlags(Qt.ItemFlag.ItemIsEnabled)  # Not editable
        item.setBackground(0, QPalette().alternateBase())
        item.setBackground(1, QPalette().alternateBase())
        self.treeWidget.addTopLevelItem(item)
        return item

    def add_boolean_setting(self, parent: QTreeWidgetItem, key: str, label: str, default_value: bool):
        """Adds a boolean setting"""
        item = QTreeWidgetItem([label, ""])
        item.setSizeHint(1, QSize(150, 35))
        parent.addChild(item)

        # CheckBox Widget
        checkbox = QCheckBox()
        checkbox.setChecked(default_value)
        checkbox.toggled.connect(lambda checked: self._on_setting_changed(key, checked))

        checkbox.setToolTip(f"Default: {'Enabled' if default_value else 'Disabled'}")

        self.treeWidget.setItemWidget(item, 1, checkbox)
        self.setting_items[key] = (item, checkbox, default_value)

    def add_integer_setting(self, parent: QTreeWidgetItem, key: str, label: str,
                            default_value: int, min_val: int = 0, max_val: int = 100):
        """Adds an integer setting"""
        item = QTreeWidgetItem([label, ""])
        item.setSizeHint(1, QSize(150, 35))
        parent.addChild(item)

        # SpinBox Widget
        spinbox = QSpinBox()
        spinbox.setRange(min_val, max_val)
        spinbox.setValue(default_value)
        spinbox.valueChanged.connect(lambda value: self._on_setting_changed(key, value))

        spinbox.setToolTip(f"Default: {default_value}, Range: {min_val} - {max_val}")

        self.treeWidget.setItemWidget(item, 1, spinbox)
        self.setting_items[key] = (item, spinbox, default_value)

    def add_text_setting(self, parent: QTreeWidgetItem, key: str, label: str, default_value: str):
        """Adds a text setting"""
        item = QTreeWidgetItem([label, ""])
        item.setSizeHint(1, QSize(150, 35))
        parent.addChild(item)

        # LineEdit Widget
        lineedit = QLineEdit()
        lineedit.setText(default_value)
        lineedit.textChanged.connect(lambda text: self._on_setting_changed(key, text))

        lineedit.setToolTip(f"Default: {default_value}")

        self.treeWidget.setItemWidget(item, 1, lineedit)
        self.setting_items[key] = (item, lineedit, default_value)

    def add_combo_setting(self, parent: QTreeWidgetItem, key: str, label: str,
                          options: list, default_value: str):
        """Adds a ComboBox setting"""
        item = QTreeWidgetItem([label, ""])
        item.setSizeHint(1, QSize(150, 35))
        parent.addChild(item)

        # ComboBox Widget
        combobox = QComboBox()
        combobox.addItems(options)
        combobox.setCurrentText(default_value)
        combobox.currentTextChanged.connect(lambda text: self._on_setting_changed(key, text))

        combobox.setToolTip(f"Default: {default_value}")

        self.treeWidget.setItemWidget(item, 1, combobox)
        self.setting_items[key] = (item, combobox, default_value)

    def add_color_setting(self, parent: QTreeWidgetItem, key: str, label: str, default_value: QColor):
        """Adds a color setting"""
        item = QTreeWidgetItem([label, ""])
        item.setSizeHint(1, QSize(150, 35))
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

    def add_file_setting(self, parent: QTreeWidgetItem, key: str, label: str, default_value: str):
        """Adds a file selection setting"""
        item = QTreeWidgetItem([label, ""])
        item.setSizeHint(1, QSize(150, 35))
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
    
    def add_folder_setting(self, parent: QTreeWidgetItem, key: str, label: str, default_value: str):
        """Adds a folder selection setting"""
        item = QTreeWidgetItem([label, ""])
        item.setSizeHint(1, QSize(150, 35))
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

    def _on_setting_changed(self, key: str, value: Any):
        """Called when a setting changes"""
        # Save to QSettings
        self.settings.setValue(key, value)
        self.settings.sync()  # Write to disk immediately

        # Emit signal
        self.setting_changed.emit(key, value)

    def get_setting(self, key: str, default_value: Any = None) -> Any:
        """Gets a setting"""
        return self.settings.value(key, default_value)

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

            # Update widget with saved value
            if isinstance(widget, QCheckBox):
                widget.setChecked(saved_value == 'true')
            elif isinstance(widget, QSpinBox):
                widget.setValue(int(saved_value))
            elif isinstance(widget, QLineEdit):
                widget.setText(str(saved_value))
            elif isinstance(widget, QComboBox):
                widget.setCurrentText(str(saved_value))
            elif isinstance(widget, tuple):  # Color widget
                color_button, color_label = widget
                color_button.setStyleSheet(f"background-color: {saved_value}")
                color_label.setText(str(saved_value))

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
    



        

        

    

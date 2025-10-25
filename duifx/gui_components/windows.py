import sys
from pathlib import Path
import logging

from PySide6.QtWidgets import QPushButton, QToolButton, QSizeGrip, QMainWindow, QWidget, QVBoxLayout
from PySide6.QtCore import QDir, QEvent, Qt, QSize, QSettings, QTimer, Signal as Signal
from PySide6.QtGui import QIcon, QCursor, QColor
from PySide6.QtCore import Qt

from duifx.gui_components.ui.ui_main_window import Ui_MainWindow
from duifx.gui_components.resources.theme import palette

RESOURCES_PATH = Path(__file__).parent / "resources"
IMAGES_PATH = RESOURCES_PATH / "image"
ICONS_PATH = RESOURCES_PATH / "icon"

def load_stylesheet():
    """Load the modern light theme stylesheet"""
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

class Container(QWidget):
    def __init__(self, content=None):
        super().__init__()
        self.content = content

        self.layout = QVBoxLayout()
        if content:
            self.layout.addWidget(content)
        self.setLayout(self.layout)

    def set_content(self, content):
        if self.content:
            self.layout.removeWidget(self.content)
            self.content.deleteLater()
        self.content = content
        self.layout.addWidget(content)

class HeaderMenuButton(QToolButton):
    def __init__(self, icon_path: str = "", label: str = "", tooltip= "", parent=None):
        super().__init__(parent)

        icon_path = str(ICONS_PATH / Path(icon_path))

        if icon_path:
            icon = QIcon(icon_path)
            self.setIcon(icon)
            self.setIconSize(QSize(32, 32))  # Make icon larger
        if label:
            self.setText(label)
        if tooltip:
            self.setToolTip(tooltip)

        self.setToolButtonStyle(Qt.ToolButtonStyle.ToolButtonTextUnderIcon)
        self.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))
        self.setMinimumSize(QSize(60, 50))

class MainWindow(QMainWindow):    
    
    # Signals
    window_closed = Signal()
    
    def __init__(self, description: str = ""):
        super().__init__()

        self._ui = Ui_MainWindow()
        self._ui.setupUi(self)

        self.setWindowTitle(description)  # set custom design title

        stylesheet = load_stylesheet()
        self.setStyleSheet(stylesheet)


        self.show()

    def add_content_widget(self, content: QWidget) -> None:
        self._ui.stackedWidgetContent.addWidget(content)

    def add_menu_button(self, button: HeaderMenuButton, selected: bool = False) -> None:
        self._ui.headerContentLayout.addWidget(button, 0, Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignVCenter)

        #button.clicked.connect(self._update_button_style)

        if selected:
            button.clicked.emit()  # simulate a button click if default selected

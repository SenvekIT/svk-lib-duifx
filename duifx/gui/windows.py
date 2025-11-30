import sys
import ctypes
from typing import Callable, Optional
from pathlib import Path

from PySide6.QtWidgets import QToolButton, QMainWindow, QWidget, QVBoxLayout, QDialog, QLabel
from PySide6.QtCore import Qt, QSize, Signal, Slot
from PySide6.QtGui import QIcon, QCursor, QPixmap, QPainter
from PySide6.QtSvg import QSvgRenderer

from duifx.gui.ui.ui_main_window import Ui_MainWindow
from duifx.gui.ui.ui_info_dialog import Ui_InfoDialog

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

def set_svg_on_label(label: QLabel, svg_path: str, size: QSize | None = None) -> None:
    """
    Rendert die SVG aus `svg_path` in ein QPixmap und setzt es auf `label`.
    Für Ressourcenpfade z.B. `:/icons/icon/Tick Box.svg` verwenden.
    """
    renderer = QSvgRenderer(svg_path)
    if size is None:
        default = renderer.defaultSize()
        if default.isValid():
            size = default
        else:
            size = QSize(label.width() or 64, label.height() or 64)

    pixmap = QPixmap(size)
    pixmap.fill(Qt.transparent)

    painter = QPainter(pixmap)
    renderer.render(painter)
    painter.end()

    label.setPixmap(pixmap)

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

class SidebarMenuButton(QToolButton):
    def __init__(self, icon_path: str = "", tooltip= "", parent=None):
        super().__init__(parent)

        if icon_path:
            icon = QIcon(icon_path)
            self.setIcon(icon)
            self.setIconSize(QSize(24, 24))  # Make icon larger
        if tooltip:
            self.setToolTip(tooltip)

        self.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))
        

class MainWindow(QMainWindow):        
    # Signals
    window_closed = Signal()
    
    def __init__(self, description: str = "", user_model_id : str = "DUIFx.Application.1.0"):
        super().__init__()

        # Windows-spezifische Taskleisten-Konfiguration
        if sys.platform == "win32":
            ctypes.windll.shell32.SetCurrentProcessExplicitAppUserModelID(user_model_id)

        self._ui = Ui_MainWindow()
        self._ui.setupUi(self)

        self.setWindowTitle(description)  # set custom design title
        self.setWindowIcon(QIcon(":icons/icon/Dx.svg")) # set a default icon

        from duifx.gui import load_stylesheet

        stylesheet = load_stylesheet()
        self.setStyleSheet(stylesheet)

        self.show()

    def add_content_widget(self, content: QWidget) -> None:
        self._ui.stackedWidgetContent.addWidget(content)

    def add_menu_button(self, button: HeaderMenuButton, slot: Optional[Callable] = None) -> None:
        self._ui.headerContentLayout.addWidget(button, 0, Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignVCenter)

        if slot:
            button.clicked.connect(slot)

    def add_sidebar_button(self, button: SidebarMenuButton, slot: Optional[Callable] = None) -> None:
        self._ui.sidebarBottomLayout.addWidget(button, 0, Qt.AlignmentFlag.AlignBottom | Qt.AlignmentFlag.AlignHCenter)
        
        if slot:
            button.clicked.connect(slot)

    def add_header_logo(self, logo_path: str, size: QSize, format: str = "svg") -> None:
        label_logo = QLabel()

        if format == "svg":
            set_svg_on_label(label_logo, logo_path, QSize(128, 32))
        elif format == "png":
            pixmap = QPixmap(logo_path)
            pixmap = pixmap.scaled(size, Qt.KeepAspectRatio, Qt.SmoothTransformation)
            label_logo.setPixmap(pixmap)

        self._ui.headerLogoLayout.addWidget(label_logo, 0, Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
class InfoDialog(QDialog):
    def __init__(self, parent=None, window_title: str = "Information"):
        super().__init__(parent)

        self._ui = Ui_InfoDialog()
        self._ui.setupUi(self)

        self.setWindowTitle(window_title)
        self.setWindowIcon(QIcon(":icons/icon/Info.svg")) # set a default icon

        self._ui.labelMainTitle.setProperty("class", "title")
        self._ui.labelSubTitle.setProperty("class", "subtitle")

        from duifx.gui import load_stylesheet
        
        stylesheet = load_stylesheet()
        self.setStyleSheet(stylesheet)

        self.resize(400, 400)
    
    def set_main_title(self, title: str) -> None:
        self._ui.labelMainTitle.setText(title)

    def set_sub_title(self, subtitle: str) -> None:
        self._ui.labelSubTitle.setText(subtitle)

    def set_info_text(self, text: str) -> None:
        self._ui.textEditInfo.setPlainText(text)

class HeaderDialog(QDialog):
    def __init__(self, parent=None, window_title: str = "Settings"):
        super().__init__(parent)

        from duifx.gui.ui.ui_settings_dialog import Ui_SettingsDialog

        self._ui = Ui_SettingsDialog()
        self._ui.setupUi(self)

        self.setWindowTitle(window_title)
        self.setWindowIcon(QIcon(":icons/icon/Settings.svg")) # set a default icon
        # Show maximize and minimize buttons
        self.setWindowFlags(self.windowFlags() | Qt.WindowType.WindowMinMaxButtonsHint)

        self._ui.labelMainTitle.setProperty("class", "title")
        self._ui.labelSubTitle.setProperty("class", "subtitle")

        stylesheet = load_stylesheet()
        self.setStyleSheet(stylesheet)

        # Connect OK and Cancel buttons
        self._ui.buttonBox.accepted.connect(self.accept)
        self._ui.buttonBox.rejected.connect(self.reject)

    def set_main_title(self, title: str) -> None:
        self._ui.labelMainTitle.setText(title)

    def set_sub_title(self, subtitle: str) -> None:
        self._ui.labelSubTitle.setText(subtitle)

    def add_content_widget(self, content: QWidget) -> None:
        """
        :param content: QWidget to add content to
        """
        self._ui.contentLayout.addWidget(content)





import sys
import ctypes
from typing import Callable, Optional
import logging

from PySide6.QtWidgets import QApplication, QToolButton, QMainWindow, QWidget, QVBoxLayout, QDialog, QLabel, QTabWidget, QSplitter
from PySide6.QtCore import Qt, QSize, Signal, Slot
from PySide6.QtGui import QIcon, QCursor, QPixmap

from duifx.gui.ui.ui_main_window import Ui_MainWindow
from duifx.gui.ui.ui_info_dialog import Ui_InfoDialog
from duifx.gui.utils import set_svg_on_label, load_stylesheet

logger = logging.getLogger(__name__)


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
    def __init__(self, description: str = "", user_model_id : str = "DUIFx.Application.1.0"):
        super().__init__()

        # Windows-spezifische Taskleisten-Konfiguration
        if sys.platform == "win32":
            ctypes.windll.shell32.SetCurrentProcessExplicitAppUserModelID(user_model_id)

        self._ui = Ui_MainWindow()
        self._ui.setupUi(self)

        self.setWindowTitle(description)  # set custom design title
        self.setWindowIcon(QIcon(":icons/icon/Dx.svg")) # set a default icon

        stylesheet = load_stylesheet()
        self.setStyleSheet(stylesheet)

        # Sidebar fixed to 50px; remaining space belongs to main area/splitter
        self._ui.sidebar.setMinimumWidth(50)
        self._ui.sidebar.setMaximumWidth(50)
        self._ui.horizontalLayout_8.setStretch(0, 0)
        self._ui.horizontalLayout_8.setStretch(1, 1)

        # initialize bottom tab panel with a splitter so the height can be resized via handle
        self._ui.bottomPanel.setMinimumHeight(0)
        self._ui.bottomPanel.setMaximumHeight(16777215)

        self._bottom_splitter = QSplitter(Qt.Orientation.Vertical, self._ui.verticalWidget)
        self._bottom_splitter.setChildrenCollapsible(False)
        self._bottom_splitter.setHandleWidth(6)
        self._bottom_splitter.setStretchFactor(0, 4)
        self._bottom_splitter.setStretchFactor(1, 1)

        layout = self._ui.main_content
        layout.removeWidget(self._ui.stackedContent)
        layout.removeWidget(self._ui.bottomPanel)

        self._bottom_splitter.addWidget(self._ui.stackedContent)
        self._bottom_splitter.addWidget(self._ui.bottomPanel)
        layout.addWidget(self._bottom_splitter)

        self._default_bottom_height = 200
        self._bottom_panel_height = self._default_bottom_height
        self._bottom_splitter.splitterMoved.connect(self._on_bottom_splitter_moved)
        self._set_bottom_panel_height(0)

        # Restore saved window position/size before showing
        self._restore_window_geometry()

        # Save geometry when the application is about to quit.
        # closeEvent is NOT called when the app exits via QApplication.quit()
        # or sys.exit(), so aboutToQuit is the only reliable hook.
        app = QApplication.instance()
        if app:
            app.aboutToQuit.connect(self._save_window_geometry)

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

    def add_bottom_panel_tab(self, widget: QWidget, text: str, closable: bool = False, initial_height: int | None = None) -> None:
        tw: QTabWidget = self._ui.tabWidgetBottomPanel

        if initial_height is None:
            initial_height = self._bottom_panel_height or self._default_bottom_height

        for i in range(tw.count()):
            if tw.tabText(i) == text:
                tw.setCurrentIndex(i)
                self._set_bottom_panel_height(initial_height)
                return

        tw.setTabsClosable(closable)
        tw.addTab(widget, text)
        tw.setCurrentWidget(widget)
        self._bottom_panel_height = initial_height
        self._set_bottom_panel_height(initial_height)

    def set_bottom_panel_tab(self, text: str):
        tw: QTabWidget = self._ui.tabWidgetBottomPanel

        for i in range(tw.count()):
            if tw.tabText(i) == text:
                tw.setCurrentIndex(i)
                break

        return

    def _set_bottom_panel_height(self, height: int) -> None:
        height = max(height, 0)

        sizes = self._bottom_splitter.sizes()
        total = sum(sizes)
        if total <= 0:
            total = (self.height() or 0) or (height + 400)

        top_height = max(total - height, 0)
        self._bottom_splitter.setSizes([top_height, height])

    def _on_bottom_splitter_moved(self, _pos: int, _index: int) -> None:
        sizes = self._bottom_splitter.sizes()
        if len(sizes) >= 2:
            self._bottom_panel_height = sizes[1]

    def closeEvent(self, event) -> None:
        """Save window geometry before closing."""
        self._save_window_geometry()        
        super().closeEvent(event)

    def _save_window_geometry(self) -> None:
        """Save window position and size to application settings."""
        from duifx.gui.settings import get_settings

        settings = get_settings()
        settings.beginGroup("MainWindow")

        # Save normal (non-maximized) geometry so restore works correctly
        if not self.isMaximized():
            settings.setValue("pos_x", self.x())
            settings.setValue("pos_y", self.y())
            settings.setValue("width", self.width())
            settings.setValue("height", self.height())

        settings.setValue("maximized", self.isMaximized())
        settings.endGroup()
        settings.sync()

        logger.debug(
            "Window geometry saved: x=%s, y=%s, w=%s, h=%s, maximized=%s",
            self.x(), self.y(), self.width(), self.height(), self.isMaximized()
        )

    def _restore_window_geometry(self) -> None:
        """Restore window position and size from application settings.

        If the saved position would place the title bar outside all available
        screens (e.g. a monitor was disconnected or resolution changed), the
        window is repositioned so that the title bar remains visible.
        """
        from duifx.gui.settings import get_settings

        settings = get_settings()
        settings.beginGroup("MainWindow")

        has_saved = settings.contains("pos_x")

        if has_saved:
            pos_x = int(settings.value("pos_x", 0))
            pos_y = int(settings.value("pos_y", 0))
            width = int(settings.value("width", self.width()))
            height = int(settings.value("height", self.height()))
            maximized = settings.value("maximized", False, type=bool)

            # Validate position: ensure the title bar is visible on a screen
            self._ensure_title_bar_visible(pos_x, pos_y, width, height)

            if maximized:
                self.showMaximized()

            logger.debug(
                "Window geometry restored: x=%s, y=%s, w=%s, h=%s, maximized=%s",
                pos_x, pos_y, width, height, maximized
            )

        settings.endGroup()

    def _ensure_title_bar_visible(self, x: int, y: int, width: int, height: int) -> None:
        """Ensure the window title bar is visible on at least one screen.

        The title bar area (top ~40 px of the window) is checked against all
        available screen geometries.  If it does not intersect any screen the
        window is centred on the primary screen instead.
        """
        from PySide6.QtCore import QRect

        screens = QApplication.screens()
        if not screens:
            return

        # Title bar region: full window width, top 40 pixels
        title_bar_height = 40
        title_bar_rect = QRect(x, y, max(width, 1), title_bar_height)

        visible_on_screen = any(
            screen.availableGeometry().intersects(title_bar_rect)
            for screen in screens
        )

        if visible_on_screen:
            self.move(x, y)
            self.resize(width, height)
        else:
            # Fallback: place on primary screen
            primary = QApplication.primaryScreen()
            if primary:
                avail = primary.availableGeometry()
                # Clamp size to available screen size
                w = min(width, avail.width())
                h = min(height, avail.height())
                new_x = avail.x() + (avail.width() - w) // 2
                new_y = avail.y() + (avail.height() - h) // 2
                self.move(new_x, new_y)
                self.resize(w, h)

            logger.info("Saved window position was off-screen; window repositioned to primary screen.")


class InfoDialog(QDialog):
    def __init__(self, parent=None, window_title: str = "Information"):
        super().__init__(parent)

        self._ui = Ui_InfoDialog()
        self._ui.setupUi(self)

        self.setWindowTitle(window_title)
        self.setWindowIcon(QIcon(":icons/icon/Info.svg")) # set a default icon

        self._ui.labelMainTitle.setProperty("class", "title")
        self._ui.labelSubTitle.setProperty("class", "subtitle")

        self._ui.textEditInfo.setReadOnly(True)
        
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
    def __init__(self, parent=None, window_title: str = "Settings", icon_path: str = ":icons/icon/Settings.svg"):
        super().__init__(parent)

        from duifx.gui.ui.ui_settings_dialog import Ui_SettingsDialog

        self._ui = Ui_SettingsDialog()
        self._ui.setupUi(self)

        self.setWindowTitle(window_title)
        self.setWindowIcon(QIcon(icon_path)) # set a default icon
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

    def hide_cancel_button(self) -> None:
        self._ui.buttonBox.button(self._ui.buttonBox.StandardButton.Cancel).hide()

    def hide_ok_button(self) -> None:
        self._ui.buttonBox.button(self._ui.buttonBox.StandardButton.Ok).hide()





import sys
import ctypes
from typing import Callable, Optional
import logging

from PySide6.QtWidgets import (
    QApplication, QToolButton, QMainWindow, QWidget, QHBoxLayout, QVBoxLayout,
    QDialog, QLabel,
)
from PySide6.QtCore import Qt, QSize, Signal, Slot
from PySide6.QtGui import QIcon, QCursor

from duifx.gui.ui.ui_main_window import Ui_MainWindow
from duifx.gui.ui.ui_info_dialog import Ui_InfoDialog
from duifx.gui.logo import DxLogo
from duifx.gui.utils import load_stylesheet

logger = logging.getLogger(__name__)

#: Default gap between the header logo and the right window edge.
DEFAULT_LOGO_RIGHT_MARGIN = 20

class MenuButton(QToolButton):
    def __init__(self, icon_path: str = "", label: str = "", tooltip: str = "",
                 icon_size: QSize | None = None, parent=None):
        super().__init__(parent)

        if icon_path:
            self.setIcon(QIcon(icon_path))

        if label:
            self.setText(label)
            self.setToolButtonStyle(Qt.ToolButtonStyle.ToolButtonTextUnderIcon)
            self.setIconSize(icon_size or QSize(32, 32))
            self.setMinimumSize(QSize(60, 50))
        else:
            self.setIconSize(icon_size or QSize(24, 24))

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

    def set_content_widget(self, index: int) -> None:
        self._ui.stackedWidgetContent.setCurrentIndex(index)

    def get_content_widget(self) -> QWidget | None:
        return self._ui.stackedWidgetContent.currentWidget()
    
    def set_right_sidebar_size(self, width: int) -> None:
        self._ui.rightSidebar.setFixedWidth(width)
    
    def add_content_right_sidebar(self, content: QWidget) -> None:
        self._ui.stackedWidgetRightSidebar.addWidget(content)

    def set_right_sidebar_widget(self, index: int) -> None:
        self._ui.stackedWidgetRightSidebar.setCurrentIndex(index)

    def get_right_sidebar_widget(self) -> QWidget | None:
        return self._ui.stackedWidgetRightSidebar.currentWidget()

    def add_menu_button(self, button: MenuButton, slot: Optional[Callable] = None) -> None:
        self._ui.headerContentLayout.addWidget(button, 0, Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignVCenter)

        if slot:
            button.clicked.connect(slot)

    def add_sidebar_bottom_button(self, button: MenuButton, slot: Optional[Callable] = None) -> None:
        self._ui.sidebarBottomLayout.addWidget(button, 0, Qt.AlignmentFlag.AlignBottom | Qt.AlignmentFlag.AlignHCenter)
        
        if slot:
            button.clicked.connect(slot)

    def add_sidebar_top_button(self, button: MenuButton, slot: Optional[Callable] = None) -> None:
        self._ui.sidebarTopLayout.addWidget(button, 0, Qt.AlignmentFlag.AlignTop | Qt.AlignmentFlag.AlignHCenter)
        
        if slot:
            button.clicked.connect(slot)

    def add_header_logo(
        self,
        logo_path: str,
        size: QSize | None = None,
        format: str | None = None,
        right_margin: int = DEFAULT_LOGO_RIGHT_MARGIN,
    ) -> bool:
        """Show a logo in the top right of the header.

        Handles SVG, PNG, JPEG and anything else Qt reads — the format is
        detected from the file, so applications do not have to say. The logo
        keeps its aspect ratio, is fitted into *size* (height only is enough,
        the width follows), and is painted at the display's own resolution so
        it stays sharp on a scaled screen. Returns False when the file cannot
        be read.

        ``format`` is accepted for backwards compatibility and ignored.
        ``right_margin`` keeps the logo off the window edge; pass 0 to butt it
        against the frame.
        """
        if format is not None:
            logger.debug("add_header_logo: 'format' is ignored, the file decides")

        logo = DxLogo(logo_path, size)
        if not logo.is_valid():
            logger.warning("Header logo could not be loaded: %s", logo_path)
            return False

        # The margin lives on a container: DxLogo paints across its whole rect,
        # so its own contentsMargins would not inset the drawing.
        container = QWidget()
        box = QHBoxLayout(container)
        box.setContentsMargins(0, 0, max(0, right_margin), 0)
        box.setSpacing(0)
        box.addWidget(logo)

        self._ui.headerLogoLayout.addWidget(
            container, 0, Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
        return True

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
        self.setWindowIcon(QIcon(":icons/icon/info.svg")) # set a default icon

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
    def __init__(self, parent=None, window_title: str = "Settings", icon_path: str = ":icons/icon/settings.svg"):
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





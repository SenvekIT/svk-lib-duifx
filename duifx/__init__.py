"""DUIFx – A modern PySide6-based GUI and utility framework."""

from duifx.version import __version__, __author__, __email__

from duifx.gui.windows import (
    MainWindow,
    HeaderMenuButton,
    SidebarMenuButton,
    InfoDialog,
    HeaderDialog,
)
from duifx.gui.settings import get_settings, SettingsDialog
from duifx.gui.splash import DxSplashScreen
from duifx.gui.resources.theme import palette
from duifx.gui.utils import get_base_path, set_svg_on_label, load_stylesheet
from duifx.gui.widgets import LogViewerWidget, QtLogHandler

__all__ = [
    "__version__",
    "__author__",
    "__email__",
    "MainWindow",
    "HeaderMenuButton",
    "SidebarMenuButton",
    "InfoDialog",
    "HeaderDialog",
    "get_settings",
    "SettingsDialog",
    "DxSplashScreen",
    "palette",
    "get_base_path",
    "set_svg_on_label",
    "load_stylesheet",
    "LogViewerWidget",
    "QtLogHandler",
]


def __dir__():
    return sorted(__all__)
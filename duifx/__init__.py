"""DUIFx – A modern PySide6-based GUI and utility framework."""

from duifx.version import __version__, __author__, __email__

from duifx.gui.windows import (
    MainWindow,
    MenuButton,
    InfoDialog,
    HeaderDialog,
)
from duifx.gui.settings import get_settings, SettingsDialog
from duifx.gui.splash import DxSplashScreen
from duifx.gui.resources.theme import palette
from duifx.gui.theming import Theme, apply_theme, get_theme, set_theme
from duifx.gui.utils import get_base_path, set_svg_on_label, load_stylesheet
from duifx.gui.widgets import (
    LogViewerWidget,
    QtLogHandler,
    ThreadMonitorWidget,
    LOG_RETENTION_SESSIONS_DEFAULT,
    prune_session_logs,
)

__all__ = [
    "__version__",
    "__author__",
    "__email__",
    "MainWindow",
    "MenuButton",
    "InfoDialog",
    "HeaderDialog",
    "get_settings",
    "SettingsDialog",
    "DxSplashScreen",
    "palette",
    "Theme",
    "apply_theme",
    "get_theme",
    "set_theme",
    "get_base_path",
    "set_svg_on_label",
    "load_stylesheet",
    "LogViewerWidget",
    "QtLogHandler",
    "ThreadMonitorWidget",
    "LOG_RETENTION_SESSIONS_DEFAULT",
    "prune_session_logs",
]


def __dir__():
    return sorted(__all__)
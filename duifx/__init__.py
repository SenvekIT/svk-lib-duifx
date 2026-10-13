"""DUIFx – A modern PySide6-based GUI and utility framework."""

from duifx.version import __version__, __author__, __email__

from duifx.gui.windows import (
    MainWindow,
    MenuButton,
    InfoDialog,
    HeaderDialog,
    DEFAULT_LOGO_RIGHT_MARGIN,
)
from duifx.gui.logo import DxLogo, DEFAULT_LOGO_HEIGHT
from duifx.gui.settings import get_settings, SettingsDialog
from duifx.gui.splash import DxSplashScreen
from duifx.gui.resources.theme import palette
from duifx.gui.colors import (
    Token,
    PLOT_SERIES,
    StateColors,
    state_colors,
    color,
    color_hex,
    css,
    plot_series_color,
    active_theme,
    set_active_theme,
)
from duifx.gui.theming import Theme, apply_theme, get_theme, set_theme
from duifx.gui.utils import get_base_path, set_svg_on_label, load_stylesheet
from duifx.gui.widgets import (
    LogViewerWidget,
    QtLogHandler,
    ThreadMonitorWidget,
    LOG_RETENTION_SESSIONS_DEFAULT,
    prune_session_logs,
)
from duifx.gui.tables import ColumnSizer, make_columns_interactive

#: Public names of duifx.gui.plot, resolved lazily by __getattr__.
_PLOT_EXPORTS = (
    "TimeSeriesPlot",
    "ScatterPlot",
    "CompassDial",
    "ThresholdBar",
    "PlotStyle",
    "default_plot_style",
    "set_default_plot_style",
    "axis_label",
    "compass_point",
    "link_x_axes",
)

__all__ = [
    "__version__",
    "__author__",
    "__email__",
    "MainWindow",
    "MenuButton",
    "InfoDialog",
    "HeaderDialog",
    "DxLogo",
    "DEFAULT_LOGO_HEIGHT",
    "DEFAULT_LOGO_RIGHT_MARGIN",
    "get_settings",
    "SettingsDialog",
    "DxSplashScreen",
    "palette",
    "Token",
    "PLOT_SERIES",
    "StateColors",
    "state_colors",
    "color",
    "color_hex",
    "css",
    "plot_series_color",
    "active_theme",
    "set_active_theme",
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
    "ColumnSizer",
    "make_columns_interactive",
    *_PLOT_EXPORTS,
]


def __getattr__(name):
    # The plot widgets load pyqtgraph and numpy; import them only on first use
    # so applications without plots do not pay for it at startup.
    if name in _PLOT_EXPORTS:
        from duifx.gui import plot
        value = getattr(plot, name)
        globals()[name] = value
        return value
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")


def __dir__():
    return sorted(__all__)
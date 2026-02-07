"""DUIFx – A modern PySide6-based GUI and utility framework."""

import importlib

from duifx.version import __version__, __author__, __email__

__all__ = [
    "__version__",
    "__author__",
    "__email__",
    # gui.windows
    "MainWindow",
    "HeaderMenuButton",
    "Container",
    "SidebarMenuButton",
    "InfoDialog",
    "HeaderDialog",
    # gui.settings
    "get_settings",
    "SettingsDialog",
    # gui.splash
    "DxSplashScreen",
    # gui.resources.theme
    "palette",
    # gui.utils
    "get_base_path",
    "set_svg_on_label",
    "load_stylesheet",
    # gui.widgets
    "LogViewerWidget",
    "QtLogHandler",
]

# Lazy symbol-level imports – GUI modules are only loaded on first access.
_LAZY_IMPORTS: dict[str, str] = {
    "MainWindow": "duifx.gui.windows",
    "HeaderMenuButton": "duifx.gui.windows",
    "Container": "duifx.gui.windows",
    "SidebarMenuButton": "duifx.gui.windows",
    "InfoDialog": "duifx.gui.windows",
    "HeaderDialog": "duifx.gui.windows",
    "get_settings": "duifx.gui.settings",
    "SettingsDialog": "duifx.gui.settings",
    "DxSplashScreen": "duifx.gui.splash",
    "palette": "duifx.gui.resources.theme",
    "get_base_path": "duifx.gui.utils",
    "set_svg_on_label": "duifx.gui.utils",
    "load_stylesheet": "duifx.gui.utils",
    "LogViewerWidget": "duifx.gui.widgets",
    "QtLogHandler": "duifx.gui.widgets",
}


def __getattr__(name: str):
    if name in _LAZY_IMPORTS:
        module = importlib.import_module(_LAZY_IMPORTS[name])
        return getattr(module, name)
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")


def __dir__():
    return sorted(__all__)
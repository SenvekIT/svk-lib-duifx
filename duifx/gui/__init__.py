# Import main components for easier access
try:
    from .windows import MainWindow, HeaderMenuButton, Container, SidebarMenuButton, InfoDialog, HeaderDialog
    from .settings import get_settings, SettingsDialog, load_stylesheet
    from .splash import DxSplashScreen
    from .resources.theme import palette
except ImportError as e:
    # Handle import errors gracefully during package installation
    print("Import error in duifx.gui package components:", e)

__all__ = [
    "MainWindow",
    "HeaderMenuButton",
    "Container",    
    "SidebarMenuButton",
    "InfoDialog",
    "HeaderDialog",
    "get_settings",
    "SettingsDialog",
    "load_stylesheet",
    "DxSplashScreen",
    "palette",
]
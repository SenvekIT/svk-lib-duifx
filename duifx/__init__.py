"""
DUIFx - A modern PySide6-based GUI and utility framework.

This package provides a comprehensive set of GUI components and utilities
for creating modern desktop applications with PySide6.
"""

__version__ = "0.2.0"
__author__ = "David Senoner Industrial Software & Electronics"
__email__ = "david.senoner@davtec.it"

# Import main components for easier access
try:
    # Relative imports für kompilierte Module
    from duifx.gui.windows import MainWindow, HeaderMenuButton, Container, SidebarMenuButton, InfoDialog, HeaderDialog
    from duifx.gui.settings import get_settings, SettingsDialog
    from duifx.gui.splash import DxSplashScreen
    from duifx.gui.resources.theme import palette
    from duifx.gui.utils import get_resource_path
    
    __all__ = [
        "__version__",
        "__author__",
        "__email__",
        "MainWindow",
        "HeaderMenuButton",
        "Container",
        "SidebarMenuButton",
        "InfoDialog",
        "HeaderDialog",
        "SettingsDialog",
        "DxSplashScreen",
        "get_settings",
        "palette",
        "get_resource_path",
    ]
except ImportError as e:
    # Handle import errors gracefully during package installation
    import sys
    print(f"Warning: Could not import GUI components: {e}", file=sys.stderr)
    __all__ = [
        "__version__",
        "__author__",
        "__email__",
    ]
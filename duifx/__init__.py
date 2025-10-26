"""
DUIFx - A modern PySide6-based GUI framework.

This package provides a comprehensive set of GUI components and utilities
for creating modern desktop applications with PySide6.
"""

__version__ = "0.1.0"
__author__ = "David Senoner"
__email__ = "david.senoner@davtec.it"

# Import main components for easier access
try:
    from .gui_components.windows import MainWindow, HeaderMenuButton, Container, SidebarMenuButton, SettingsDialog, InfoDialog
    from .gui_components.resources.theme import palette
except ImportError:
    # Handle import errors gracefully during package installation
    pass

__all__ = [
    "__version__",
    "__author__",
    "__email__",
    "MainWindow",
    "HeaderMenuButton",
    "Container",
    "SidebarMenuButton",
    "SettingsDialog",
    "InfoDialog",
]
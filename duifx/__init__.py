"""
DUIFx - A modern PySide6-based GUI and utility framework.

This package provides a comprehensive set of GUI components and utilities
for creating modern desktop applications with PySide6.
"""

# Import main components for easier access
try:
    from .gui.windows import MainWindow, HeaderMenuButton, Container, SidebarMenuButton, SettingsDialog, InfoDialog
    from .gui.resources.theme import palette
except ImportError:
    # Handle import errors gracefully during package installation
    pass

__all__ = [
    "MainWindow",
    "HeaderMenuButton",
    "Container",
    "SidebarMenuButton",
    "SettingsDialog",
    "InfoDialog",
]
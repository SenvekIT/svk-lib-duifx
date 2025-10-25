"""
DUIFx - A modern PyQt6-based GUI framework.

This package provides a comprehensive set of GUI components and utilities
for creating modern desktop applications with PyQt6.
"""

__version__ = "0.1.0"
__author__ = "David Senoner"
__email__ = "your.email@example.com"

# Import main components for easier access
try:
    from .gui_components.windows import MainWindow
    from .gui_components.widgets.test_manager import TestManager
    from .gui_components.resources.theme import ThemeManager
except ImportError:
    # Handle import errors gracefully during package installation
    pass

__all__ = [
    "__version__",
    "__author__",
    "__email__",
    "MainWindow",
    "TestManager", 
    "ThemeManager",
]
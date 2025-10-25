"""
Main entry point for the DUIFx application.
"""

import sys
from PyQt6.QtWidgets import QApplication
from duifx.gui_components.windows import MainWindow
from duifx.gui_components.widgets.test_manager import TestManagerWidget
from duifx.gui_components.windows import MainWindow, Container, HeaderMenuButton

VERSION = "1.0.0"  # Version number
BUILD = "21.10.2025"  # Build date
TITLE = "Application Title"  # Title of the application
ORGANISATION = "Organisation name"  # Organisation name
DOMAIN = "www.domain.it"  # Organisation domain
AUTHOR = "Senoner David"  # Author name

def main():
    """Main entry point for the DUIFx application."""
    app = QApplication(sys.argv)

    # create main window
    main_window = MainWindow(TITLE + " v" + VERSION)
    testmanager_container = Container()

    testmanager_widget = TestManagerWidget(container=testmanager_container)
    testmanager_widget.set_main_title("CANOpen Test Manager")
    testmanager_widget.set_sub_title("Manage and execute CANOpen tests")

    # add test manager content to main window
    main_window.add_content_widget(testmanager_container)

    main_window.add_menu_button(HeaderMenuButton(icon_path="icon_0_32x32_143.png", label=""), selected=True)
    main_window.add_menu_button(HeaderMenuButton(icon_path="icon_0_32x32_528.png", label=""), selected=True)
    
    # Start the application event loop
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
"""
Main entry point for the DUIFx application.
"""

import sys
import logging
from PySide6.QtWidgets import QApplication
from PySide6.QtCore import QTimer
from duifx.gui_components.splash import DxSplashScreen

logger = logging.getLogger(__name__)
logging.basicConfig(
    level=logging.DEBUG,
    format="%(asctime)s - %(name)-11s - %(levelname)-7s - %(message)s",
)

VERSION = "1.0.0"
BUILD = "21.10.2025"
TITLE = "Application Title"
ORGANISATION = "Organisation name"
DOMAIN = "www.domain.it"
AUTHOR = "Senoner David"

def load_application_modules(splash, app):
    """Lädt die Anwendungsmodule mit Progress-Updates"""
    
    # Schritt 1: GUI Komponenten
    splash.showMessage("Loading GUI components...")
    splash.set_progress(25)
    app.processEvents()
    
    from duifx.gui_components.windows import (MainWindow, Container, HeaderMenuButton, 
                                              SidebarMenuButton, SettingsDialog, InfoDialog
                                              )
    
    # Schritt 2: Weitere Module (falls vorhanden)
    splash.showMessage("Loading additional modules...")
    splash.set_progress(50)
    app.processEvents()
    
    # Hier könntest du weitere schwere Module laden
    # import heavy_module_1
    # import heavy_module_2
    
    # Schritt 3: Konfiguration
    splash.showMessage("Loading configuration...")
    splash.set_progress(75)
    app.processEvents()
    
    # Konfiguration laden...
    
    splash.showMessage("Initializing application...")
    splash.set_progress(100)
    app.processEvents()
    
    return MainWindow, Container, HeaderMenuButton, SidebarMenuButton, SettingsDialog, InfoDialog

def on_settings_button_clicked():
    """Öffnet den Einstellungsdialog"""
    settings_dialog = SettingsDialog()
    settings_dialog.set_main_title("Application Settings")
    settings_dialog.set_sub_title("Configure application preferences")
    if settings_dialog.exec() == SettingsDialog.DialogCode.Accepted:
        # Settings were saved/accepted
        logger.info("Settings dialog accepted")
    else:
        # Settings were cancelled
        logger.info("Settings dialog cancelled")

def on_info_button_clicked():
    
    """Öffnet den Info-Dialog"""
    info_dialog = InfoDialog()
    info_dialog.set_main_title("About Application")
    info_dialog.set_sub_title(f"{TITLE} v{VERSION}")
    info_dialog.set_info_text(f"""Build Date: {BUILD}\nSoftware version: {VERSION}\nAuthor: {AUTHOR}\n""")
    info_dialog.exec()

def main():
    """Main entry point for the DUIFx application."""
    # App erstellen
    app = QApplication(sys.argv)

    # Show Splash Screen
    splash = DxSplashScreen(app, TITLE, "© 2025 All Rights Reserved")
    
    # App-Metadaten setzen
    app.setApplicationName(TITLE)
    app.setApplicationVersion(VERSION)
    app.setOrganizationName(ORGANISATION)
    app.setOrganizationDomain(DOMAIN)

    # Module laden
    (MainWindow, Container, HeaderMenuButton, SidebarMenuButton, SettingsDialog, InfoDialog) = load_application_modules(splash, app)

    # Wait 1 second before closing splash screen
    import time
    time.sleep(1.5)
    splash.close()
    
    # Main Window erstellen
    main_window = MainWindow(TITLE + " v" + VERSION)
    testmanager_container = Container()

    # add test manager content to main window
    main_window.add_content_widget(testmanager_container)

    main_window.add_menu_button(HeaderMenuButton(icon_path="Home.svg", label="Home"), )

    main_window.add_sidebar_button(SidebarMenuButton(icon_path="Settings.svg", tooltip="Settings"), slot=on_settings_button_clicked)
    main_window.add_sidebar_button(SidebarMenuButton(icon_path="Info.svg", tooltip="Info"), slot=on_info_button_clicked)
    
    # Splash Screen schließen
    QTimer.singleShot(1000, splash.close)  # Nach 1 Sekunde schließen
    
    # Start the application event loop
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
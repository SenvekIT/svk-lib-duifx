"""
Main entry point for the DUIFx application.
"""

import sys
from PySide6.QtWidgets import QApplication
from PySide6.QtCore import QTimer
from duifx.gui_components.splash import create_advanced_splash_screen

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
    
    from duifx.gui_components.windows import MainWindow, Container, HeaderMenuButton
    
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
    
    return MainWindow, Container, HeaderMenuButton

def main():
    """Main entry point for the DUIFx application."""
    # App erstellen
    app = QApplication(sys.argv)
    
    # App-Metadaten setzen
    app.setApplicationName(TITLE)
    app.setApplicationVersion(VERSION)
    app.setOrganizationName(ORGANISATION)
    app.setOrganizationDomain(DOMAIN)
    
    # Splash Screen sofort anzeigen
    splash = create_advanced_splash_screen(app)
    splash.showMessage("Starting application...")
    splash.set_progress(0)
    
    # Module laden
    MainWindow, Container, HeaderMenuButton = load_application_modules(splash, app)
    
    # Main Window erstellen
    main_window = MainWindow(TITLE + " v" + VERSION)
    testmanager_container = Container()

    # add test manager content to main window
    main_window.add_content_widget(testmanager_container)

    main_window.add_menu_button(HeaderMenuButton(icon_path="Home.svg", label="Home"), selected=True)
    main_window.add_menu_button(HeaderMenuButton(icon_path="Settings.svg", label="Settings"), selected=True)
    
    # Splash Screen schließen
    QTimer.singleShot(1000, splash.close)  # Nach 1 Sekunde schließen
    
    # Start the application event loop
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
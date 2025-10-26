from PySide6.QtWidgets import QSplashScreen, QProgressBar, QLabel, QVBoxLayout, QWidget
from PySide6.QtGui import QPixmap, QFont, QPainter
from PySide6.QtCore import Qt, QTimer
from pathlib import Path
from duifx.gui_components.resources.theme import palette

class CustomSplashScreen(QSplashScreen):
    def __init__(self, pixmap):
        super().__init__(pixmap)
        self.progress_value = 0
        
        # Schrift für Text
        self.font = QFont("Segoe UI", 12)
        self.message = "Loading..."
        
    def showMessage(self, message, alignment=Qt.AlignmentFlag.AlignBottom, color=Qt.GlobalColor.darkGray):
        self.message = message
        super().showMessage(message, alignment, color)
        
    def set_progress(self, value):
        """Setzt den Progress Wert (0-100)"""
        self.progress_value = value
        self.repaint()
        
    def drawContents(self, painter):
        """Custom Drawing für Progress Bar"""
        super().drawContents(painter)
        
        # Progress Bar zeichnen
        painter.setFont(self.font)
        
        # Progress Bar Position
        rect = self.rect()
        bar_width = rect.width() - 40
        bar_height = 10
        bar_x = 20
        bar_y = rect.height() - 50
        
        # Background der Progress Bar
        painter.fillRect(bar_x, bar_y, bar_width, bar_height, Qt.GlobalColor.lightGray)
        
        # Progress der Progress Bar
        progress_width = int((bar_width * self.progress_value) / 100)
        painter.fillRect(bar_x, bar_y, progress_width, bar_height, Qt.GlobalColor.darkGray)

def create_advanced_splash_screen(app):
    """Erstellt einen erweiterten Splash Screen mit Progress Bar"""
    # Splash Screen Bild laden
    splash_path = Path(__file__).parent.parent / "gui_components" / "resources" / "image" / "splash.png"
    
    if splash_path.exists():
        pixmap = QPixmap(str(splash_path))
        pixmap = pixmap.scaled(700, 400, Qt.AspectRatioMode.IgnoreAspectRatio, Qt.TransformationMode.SmoothTransformation)
    else:
        # Fallback: Einfaches Bild erstellen
        pixmap = QPixmap(500, 350)
        pixmap.fill(Qt.GlobalColor.lightGray)
        
        # Logo/Text auf Pixmap zeichnen
        painter = QPainter(pixmap)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        painter.setRenderHint(QPainter.RenderHint.TextAntialiasing)
        painter.setPen(Qt.GlobalColor.white)
        painter.setFont(QFont("Segoe UI", 16, QFont.Weight.Bold))
        painter.drawText(pixmap.rect(), Qt.AlignmentFlag.AlignCenter, "DUIFx Framework")
        painter.end()
    
    # Custom Splash Screen erstellen
    splash = CustomSplashScreen(pixmap)
    splash.setWindowFlags(Qt.WindowType.WindowStaysOnTopHint | Qt.WindowType.FramelessWindowHint)
    
    splash.show()
    app.processEvents()
    
    return splash
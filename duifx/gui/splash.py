from pathlib import Path
from PySide6.QtWidgets import QSplashScreen, QApplication
from PySide6.QtGui import QPixmap, QFont, QPainter
from PySide6.QtCore import Qt, QTimer, QRectF
from PySide6.QtSvg import QSvgRenderer

class DxSplashScreen(QSplashScreen):
    def __init__(self, app, title, sub_title):
        self.progress_value = 0
        self.font = QFont("Segoe UI", 10)
        self.title = title
        self.sub_title = sub_title
        self.message = "Loading..."

        pixmap = self._create_splash_screen(app)
        super().__init__(pixmap)

        self.showMessage("Starting application...")
        self.set_progress(0)

        self.setWindowFlags(Qt.WindowType.WindowStaysOnTopHint | Qt.WindowType.FramelessWindowHint)
        self.show()
        app.processEvents()
        
    def showMessage(self, message, alignment=Qt.AlignmentFlag.AlignBottom, color=Qt.GlobalColor.darkGray):
        self.message = message
        
    def set_progress(self, value):
        """Sets progress value (0-100)"""
        if self.progress_value != value:
            self.progress_value = value
            self.repaint()
        
    def drawContents(self, painter):
        """Custom drawing for progress bar"""
        super().drawContents(painter)
        
        painter.setFont(self.font)
        rect = self.rect()
        
        # Draw message
        painter.drawText(40, rect.height() - 30, self.message)
         
        # Draw progress bar
        bar_width = rect.width() - 80
        bar_x, bar_y, bar_height = 40, rect.height() - 50, 3
        
        painter.fillRect(bar_x, bar_y, bar_width, bar_height, Qt.GlobalColor.lightGray)
        
        if self.progress_value > 0:
            progress_width = (bar_width * self.progress_value) // 100
            painter.fillRect(bar_x, bar_y, progress_width, bar_height, Qt.GlobalColor.darkGray)
    
    def _create_splash_screen(self, app):
        """Creates a splash screen with progress bar"""
        
        screen = app.primaryScreen()
        device_pixel_ratio = screen.devicePixelRatio()
        
        base_width, base_height = 450, 250
        pixmap = QPixmap(int(base_width * device_pixel_ratio), int(base_height * device_pixel_ratio))
        pixmap.setDevicePixelRatio(device_pixel_ratio)
        pixmap.fill(Qt.GlobalColor.transparent)
        
        painter = QPainter(pixmap)
        painter.setRenderHints(
            QPainter.RenderHint.Antialiasing |
            QPainter.RenderHint.TextAntialiasing |
            QPainter.RenderHint.SmoothPixmapTransform
        )
        
        # Draw background
        painter.setBrush(Qt.GlobalColor.white)
        painter.setPen(Qt.PenStyle.NoPen)
        painter.drawRoundedRect(QRectF(0, 0, base_width, base_height), 15, 15)

        from duifx.gui.ui import resource_rc
        
        # Render SVG icon
        svg_renderer = QSvgRenderer(":icons/icon/Dx.svg")
        svg_renderer.render(painter, QRectF(30, 77, 96, 96))
        
        # Draw text
        painter.setPen(Qt.GlobalColor.black)
        text_x = 130
        
        # Title
        title_font = QFont("Segoe UI", 16, QFont.Weight.Bold)
        painter.setFont(title_font)
        painter.drawText(QRectF(text_x, 105, 290, 40), 
                        Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignVCenter, 
                        self.title)
        
        # Copyright
        copyright_font = QFont("Segoe UI", 10)
        painter.setFont(copyright_font)
        painter.drawText(QRectF(text_x, 132, 290, 40), 
                        Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignVCenter, 
                        self.sub_title)
        
        painter.end()
    
        return pixmap

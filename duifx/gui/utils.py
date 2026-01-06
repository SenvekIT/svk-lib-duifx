from pathlib import Path
import sys
from PySide6.QtWidgets import QWidget, QLabel
from PySide6.QtCore import QSize, Qt
from PySide6.QtGui import QPixmap, QPainter
from PySide6.QtSvg import QSvgRenderer

def get_base_path() -> Path:
    """Get base path depending on whether running in PyInstaller bundle or development."""
    if getattr(sys, 'frozen', False):
        # PyInstaller bundle
        return Path(sys._MEIPASS)
    else:
        # Development
        return Path(__file__).parent

def set_svg_on_label(label: QLabel, svg_path: str, size: QSize | None = None) -> None:
    """
    Rendert die SVG aus `svg_path` in ein QPixmap und setzt es auf `label`.
    Für Ressourcenpfade z.B. `:/icons/icon/Tick Box.svg` verwenden.
    """
    renderer = QSvgRenderer(svg_path)
    if size is None:
        default = renderer.defaultSize()
        if default.isValid():
            size = default
        else:
            size = QSize(label.width() or 64, label.height() or 64)

    pixmap = QPixmap(size)
    pixmap.fill(Qt.transparent)

    painter = QPainter(pixmap)
    renderer.render(painter)
    painter.end()

    label.setPixmap(pixmap)
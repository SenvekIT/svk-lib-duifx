"""Shared utility helpers for DUIFx GUI components."""

from __future__ import annotations

import logging
import sys
from pathlib import Path
from typing import Optional

from PySide6.QtCore import QSize, Qt
from PySide6.QtGui import QPainter, QPixmap
from PySide6.QtSvg import QSvgRenderer
from PySide6.QtWidgets import QLabel

logger = logging.getLogger(__name__)


def get_base_path() -> Path:
    """Get base path depending on whether running in PyInstaller bundle or development."""
    if getattr(sys, "frozen", False):
        return Path(sys._MEIPASS)
    return Path(__file__).parent


def load_stylesheet() -> Optional[str]:
    """Load the default QSS stylesheet from the resources directory."""
    base_path = get_base_path()
    if getattr(sys, "frozen", False):
        base_path = base_path / "duifx" / "gui"

    qss_path = base_path / "resources" / "modern_light.qss"
    if qss_path.exists():
        logger.debug("Loading stylesheet from %s", qss_path)
        return qss_path.read_text(encoding="utf-8")
    return None


def set_svg_on_label(label: QLabel, svg_path: str, size: QSize | None = None) -> None:
    """Render an SVG file onto a QLabel as a QPixmap."""
    renderer = QSvgRenderer(svg_path)
    if size is None:
        default = renderer.defaultSize()
        size = default if default.isValid() else QSize(label.width() or 64, label.height() or 64)

    pixmap = QPixmap(size)
    pixmap.fill(Qt.transparent)

    painter = QPainter(pixmap)
    renderer.render(painter)
    painter.end()

    label.setPixmap(pixmap)
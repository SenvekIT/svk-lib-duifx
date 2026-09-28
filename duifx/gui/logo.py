"""Logo display that stays sharp, whatever the source format is.

One widget for SVG, PNG, JPEG and anything else Qt can read, so an application
only has to say *which file* and *how big*, not how to get it onto the screen.

Why a widget and not a QLabel with a pixmap — which is the obvious approach and
the one this replaces: ``QLabel.setPixmap()`` normalises the pixmap to its
device-independent size, so a render made at three times the logical size comes
out pixel-identical to one made at the logical size. The extra resolution is
discarded, and on a 150 % or 200 % display the logo is visibly soft. Painting in
``paintEvent`` instead happens at the device resolution: an SVG is re-rendered
from its vector source at whatever the screen currently asks for, and a raster
image is scaled from its native pixels. Both keep their aspect ratio, and both
follow the window when it is dragged onto a differently scaled monitor.
"""

from __future__ import annotations

import logging
from pathlib import Path
from typing import Optional

from PySide6.QtCore import QRectF, QSize, Qt
from PySide6.QtGui import QPainter, QPixmap
from PySide6.QtSvg import QSvgRenderer
from PySide6.QtWidgets import QSizePolicy, QWidget

logger = logging.getLogger(__name__)

#: Used when the caller gives no size. Fits the standard 80 px header.
DEFAULT_LOGO_HEIGHT = 40

_SVG_SUFFIXES = (".svg", ".svgz")


def _looks_like_svg(path: Path) -> bool:
    """Decide by extension, then by content.

    The content check catches an SVG saved under a different extension, which
    is common enough when a logo comes out of a design tool.
    """
    if path.suffix.lower() in _SVG_SUFFIXES:
        return True
    try:
        head = path.open("rb").read(1024).lstrip()
    except OSError:
        return False
    return head.startswith(b"<svg") or (head.startswith(b"<?xml") and b"<svg" in head)


class DxLogo(QWidget):
    """A logo scaled into a bounding box, rendered at full device resolution.

    Parameters
    ----------
    logo_path:
        File to display. SVG is preferred — it stays sharp at any size — but
        any format Qt can read works.
    size:
        Bounding box. The logo is fitted into it keeping its aspect ratio, so
        it is normally enough to give the height you have room for and a
        generous width. When omitted, the logo is scaled to
        ``DEFAULT_LOGO_HEIGHT``.

    ``is_valid()`` reports whether the file could be read at all; a widget that
    could not read its source paints nothing rather than showing a broken box.
    """

    def __init__(
        self,
        logo_path: str | Path,
        size: Optional[QSize] = None,
        parent: Optional[QWidget] = None,
    ) -> None:
        super().__init__(parent)

        self._renderer: Optional[QSvgRenderer] = None
        self._pixmap: Optional[QPixmap] = None
        self._native = QSize()

        self.setSizePolicy(QSizePolicy.Policy.Fixed, QSizePolicy.Policy.Fixed)
        self.setAttribute(Qt.WidgetAttribute.WA_TranslucentBackground, True)
        self.setLogo(logo_path, size)

    # ------------------------------------------------------------------ #
    #  Public API                                                         #
    # ------------------------------------------------------------------ #

    def setLogo(self, logo_path: str | Path, size: Optional[QSize] = None) -> bool:
        """Load *logo_path* and scale it into *size*. False if it is unreadable."""
        path = Path(logo_path)
        self._renderer = None
        self._pixmap = None
        self._native = QSize()

        if not path.exists():
            logger.warning("Logo file not found: %s", path)
            self.setFixedSize(0, 0)
            self.update()
            return False

        if _looks_like_svg(path):
            renderer = QSvgRenderer(str(path))
            if renderer.isValid():
                # The widget's own rect decides the size, so let the renderer
                # fill it and keep the ratio rather than stretching.
                renderer.setAspectRatioMode(Qt.AspectRatioMode.KeepAspectRatio)
                self._renderer = renderer
                self._native = renderer.defaultSize()
        if self._renderer is None:
            pixmap = QPixmap(str(path))
            if not pixmap.isNull():
                self._pixmap = pixmap
                self._native = pixmap.size()

        if not self._native.isValid() or self._native.height() <= 0:
            logger.warning("Logo file could not be read as an image: %s", path)
            self.setFixedSize(0, 0)
            self.update()
            return False

        self.setFixedSize(self._fitted_size(size))
        self.update()
        return True

    def is_valid(self) -> bool:
        return self._renderer is not None or self._pixmap is not None

    def nativeSize(self) -> QSize:
        """Size of the source, in its own units (SVG) or pixels (raster)."""
        return QSize(self._native)

    def sizeHint(self) -> QSize:
        return self.size()

    # ------------------------------------------------------------------ #
    #  Internals                                                          #
    # ------------------------------------------------------------------ #

    def _fitted_size(self, size: Optional[QSize]) -> QSize:
        """Largest size with the source's aspect ratio that fits in *size*."""
        ratio = self._native.width() / self._native.height()
        if size is None or not size.isValid() or size.height() <= 0:
            height = DEFAULT_LOGO_HEIGHT
            return QSize(max(1, round(height * ratio)), height)

        width = round(size.height() * ratio)
        if size.width() > 0 and width > size.width():
            return QSize(size.width(), max(1, round(size.width() / ratio)))
        return QSize(max(1, width), size.height())

    def paintEvent(self, event) -> None:  # noqa: N802 - Qt naming
        if not self.is_valid():
            return
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.SmoothPixmapTransform, True)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing, True)
        if self._renderer is not None:
            self._renderer.render(painter, QRectF(self.rect()))
        else:
            # drawPixmap(QRect, QPixmap) scales the source into the rect; the
            # widget's device pixel ratio is already in the painter transform,
            # so this uses the source's full resolution.
            painter.drawPixmap(self.rect(), self._pixmap)
        painter.end()


__all__ = ["DxLogo", "DEFAULT_LOGO_HEIGHT"]

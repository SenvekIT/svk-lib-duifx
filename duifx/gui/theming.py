"""Application theme (colour scheme) selection.

Qt follows the operating system's light/dark setting by default. The
stylesheet shipped with DUIFx is a light one: it pins the text colour but
leaves backgrounds to the palette, so on a dark desktop the styled widgets
stay light while plain ones turn dark -- dark text on a dark background.
Pinning the colour scheme here removes that dependency on the OS setting.

The theme is stored in the application settings and applied at startup,
before any window is built::

    from duifx import apply_theme
    apply_theme(app)

:class:`Theme` also carries ``DARK``, which is **not implemented yet**: the
shipped stylesheet and the hardcoded colours across the widgets are all
light-theme. It is therefore not offered by :func:`selectable_themes` and
falls back to ``LIGHT`` when applied.
"""

from __future__ import annotations

import logging
from enum import Enum
from typing import Optional

from PySide6.QtCore import Qt
from PySide6.QtWidgets import QApplication

logger = logging.getLogger(__name__)

#: Settings key holding the selected theme.
THEME_SETTING_KEY = "appearance/theme"


class Theme(str, Enum):
    """Available colour schemes."""

    LIGHT = "light"
    #: Reserved -- see the module docstring. Falls back to LIGHT.
    DARK = "dark"
    SYSTEM = "system"


DEFAULT_THEME = Theme.LIGHT

#: Human-readable labels, also what ends up in the settings file.
THEME_LABELS: dict[Theme, str] = {
    Theme.LIGHT: "Light",
    Theme.DARK: "Dark",
    Theme.SYSTEM: "Follow system setting",
}

#: Qt colour scheme per theme. ``Unknown`` hands control back to the OS.
_COLOR_SCHEMES: dict[Theme, Qt.ColorScheme] = {
    Theme.LIGHT: Qt.ColorScheme.Light,
    Theme.SYSTEM: Qt.ColorScheme.Unknown,
}


def selectable_themes() -> list[Theme]:
    """Themes that may be offered to the user (excludes unimplemented ones)."""
    return [Theme.LIGHT, Theme.SYSTEM]


def theme_labels() -> list[str]:
    """Labels of :func:`selectable_themes`, in display order."""
    return [THEME_LABELS[t] for t in selectable_themes()]


def parse_theme(value: object) -> Theme:
    """Return the :class:`Theme` for *value*, tolerating labels and case.

    Accepts the enum value (``"light"``), the display label
    (``"Follow system setting"``), or a :class:`Theme`. Unknown input
    yields :data:`DEFAULT_THEME`.
    """
    if isinstance(value, Theme):
        return value
    if value is None:
        return DEFAULT_THEME
    text = str(value).strip().lower()
    if not text:
        return DEFAULT_THEME
    for theme in Theme:
        if text == theme.value or text == THEME_LABELS[theme].lower():
            return theme
    logger.warning("Unknown theme %r, falling back to %s", value, DEFAULT_THEME.value)
    return DEFAULT_THEME


def get_theme() -> Theme:
    """Return the theme stored in the application settings."""
    from duifx.gui.settings import get_settings

    return parse_theme(get_settings().value(THEME_SETTING_KEY, DEFAULT_THEME.value))


def set_theme(theme: Theme | str) -> None:
    """Persist *theme*. Takes effect on the next application start."""
    from duifx.gui.settings import get_settings

    resolved = parse_theme(theme)
    settings = get_settings()
    settings.setValue(THEME_SETTING_KEY, THEME_LABELS[resolved])
    settings.sync()


def apply_theme(app: Optional[QApplication] = None,
                theme: Theme | str | None = None) -> Theme:
    """Apply *theme* (or the stored one) to *app*.

    Call before creating windows: a palette change is only partly picked up
    by widgets that already exist.

    Returns the theme actually applied, which differs from the request when
    an unimplemented one was asked for.
    """
    app = app or QApplication.instance()
    if app is None:
        raise RuntimeError("apply_theme() needs a QApplication instance")

    requested = parse_theme(theme) if theme is not None else get_theme()
    effective = requested
    if requested not in _COLOR_SCHEMES:
        logger.warning(
            "Theme '%s' is not implemented yet, using '%s' instead.",
            requested.value, DEFAULT_THEME.value,
        )
        effective = DEFAULT_THEME

    style_hints = app.styleHints()
    # setColorScheme() is Qt 6.8+; older builds simply keep following the OS.
    if not hasattr(style_hints, "setColorScheme"):
        logger.warning(
            "Qt build has no QStyleHints.setColorScheme(); the theme setting "
            "cannot be enforced and the OS setting stays in charge.",
        )
        return effective

    style_hints.setColorScheme(_COLOR_SCHEMES[effective])
    logger.debug("Theme applied: %s", effective.value)
    return effective

"""Central colour palette: named base colours and semantic colour tokens.

Widgets should not carry their own hex literals. They ask for a *role* --
"state ok", "muted text", "accent button" -- and get the colour the current
theme assigns to it, so every DUIFx application draws its states, hints and
buttons the same way::

    from duifx.gui.colors import Token, color, css, state_colors

    label.setStyleSheet(f"color: {css(Token.TEXT_MUTED)};")
    item.setForeground(state_colors.color(entry.state))
    pen = pg.mkPen(color(Token.PLOT_AXIS_X), width=1)

Two layers:

* :data:`base` -- the named base colours: the Tailwind scales of
  :class:`~duifx.gui.resources.theme.Palette`, plus the few colours that are
  not on a scale (the stylesheet's text and chrome greys, the signal colours
  for states, the plot colours).
* :class:`Token` -- the semantic roles. Each one has a value per theme
  (:data:`_VALUES`), built from :data:`base`.

The theme a token resolves against is :func:`active_theme`: the stored theme
setting, reduced to ``LIGHT`` or ``DARK``. ``DARK`` values are defined but
provisional -- :mod:`duifx.gui.theming` does not implement a dark theme yet,
so :func:`active_theme` returns ``LIGHT`` until it does.

Plot colours are the same in both themes: plots keep pyqtgraph's black
background, and the series colours match the EdgePulse firmware's monitor so
screenshots from either tool read the same way.
"""

from __future__ import annotations

import logging
from enum import Enum
from typing import Mapping, Optional

from PySide6.QtGui import QColor

from duifx.gui.resources.theme import Palette
from duifx.gui.theming import Theme

logger = logging.getLogger(__name__)


# =========================================================================== #
#  Base palette
# =========================================================================== #


class BasePalette(Palette):
    """Named base colours: :class:`Palette`'s scales plus off-scale colours.

    Tokens should be the only consumers; widgets use :class:`Token`.
    """

    # Neutrals the stylesheet (modern_light.qss) uses and widgets must match.
    ink = "#212529"        # default text colour, ``* { color: ... }``
    chrome = "#e9ecef"     # sidebar, footer and toolbar background

    # Signal colours for states -- the status language of the applications.
    signal_green = "#27ae60"
    signal_orange = "#e67e22"
    signal_red = "#e74c3c"

    # Plot colours, as in the firmware's monitor (drawn on black).
    plot_red = "#e65050"
    plot_green = "#50c850"
    plot_blue = "#5a8cff"
    plot_amber = "#ffc83c"
    plot_pink = "#ff78c8"
    plot_mint = "#5ac88c"
    plot_light = "#dcdcdc"
    plot_axis = "#969696"   # pyqtgraph's default foreground
    plot_black = "#000000"  # pyqtgraph's default background

    _EXTRA_NAMES = (
        "ink", "chrome", "signal_green", "signal_orange", "signal_red",
        "plot_red", "plot_green", "plot_blue", "plot_amber", "plot_pink",
        "plot_mint", "plot_light", "plot_axis", "plot_black",
    )

    @classmethod
    def get_color_dict(cls) -> dict[str, str]:
        """All base colours as flat dictionary, scales and extras."""
        colors = super().get_color_dict()
        colors.update({name: getattr(cls, name) for name in cls._EXTRA_NAMES})
        return colors


base = BasePalette()


# =========================================================================== #
#  Semantic tokens
# =========================================================================== #


class Token(str, Enum):
    """Semantic colour roles. Resolve with :func:`color` or :func:`css`."""

    # States: connection, device and value status.
    STATE_OK = "state_ok"                # connected, operational, value good
    STATE_BUSY = "state_busy"            # scanning, connecting
    STATE_WARNING = "state_warning"      # needs attention, not good enough yet
    STATE_ERROR = "state_error"          # error, fault, heartbeat lost
    STATE_INACTIVE = "state_inactive"    # disconnected, offline, not valid
    STATE_INFO = "state_info"            # neutral notice

    # Text.
    TEXT_HEADING = "text_heading"        # titles, item names
    TEXT_PRIMARY = "text_primary"        # body text, values
    TEXT_SECONDARY = "text_secondary"    # labels, section headers
    TEXT_MUTED = "text_muted"            # hints, captions, descriptions
    TEXT_PLACEHOLDER = "text_placeholder"  # "nothing selected" placeholders
    TEXT_DISABLED = "text_disabled"      # disabled entries
    TEXT_WARNING = "text_warning"        # warning sentences (readable orange)
    TEXT_ON_ACCENT = "text_on_accent"    # text on ACCENT / DANGER buttons

    # Surfaces and lines.
    SURFACE = "surface"                  # input fields, plain content
    SURFACE_ALT = "surface_alt"          # tables, tooltips on chrome
    SURFACE_MUTED = "surface_muted"      # toolbars, cards, table header rows (stylesheet chrome)
    BORDER = "border"                    # input borders
    BORDER_STRONG = "border_strong"      # tooltip and emphasised borders
    HIGHLIGHT_CHANGED = "highlight_changed"  # background of a changed value

    # Overlays for flat buttons (translucent).
    OVERLAY_HOVER = "overlay_hover"
    OVERLAY_PRESSED = "overlay_pressed"

    # Buttons.
    ACCENT = "accent"
    ACCENT_HOVER = "accent_hover"
    ACCENT_PRESSED = "accent_pressed"
    DANGER = "danger"
    DANGER_HOVER = "danger_hover"
    DANGER_PRESSED = "danger_pressed"
    CONTROL_DISABLED = "control_disabled"

    # Indicator dots (bit LEDs) without ok/error meaning.
    INDICATOR_ON = "indicator_on"
    INDICATOR_OFF = "indicator_off"

    # Plots. Series 1-3 double as the x/y/z axis colours.
    PLOT_SERIES_1 = "plot_series_1"
    PLOT_SERIES_2 = "plot_series_2"
    PLOT_SERIES_3 = "plot_series_3"
    PLOT_SERIES_4 = "plot_series_4"
    PLOT_SERIES_5 = "plot_series_5"
    PLOT_SERIES_6 = "plot_series_6"
    PLOT_AXIS_X = "plot_series_1"        # alias of PLOT_SERIES_1
    PLOT_AXIS_Y = "plot_series_2"        # alias of PLOT_SERIES_2
    PLOT_AXIS_Z = "plot_series_3"        # alias of PLOT_SERIES_3
    PLOT_MAGNITUDE = "plot_magnitude"    # |v|, sigma and other derived traces
    PLOT_BACKGROUND = "plot_background"
    PLOT_AXIS = "plot_axis"              # axis lines, ticks, labels
    PLOT_GRID = "plot_grid"              # grid lines (translucent axis colour)


#: Plot series in cycle order, see :func:`plot_series_color`.
PLOT_SERIES: tuple[Token, ...] = (
    Token.PLOT_SERIES_1, Token.PLOT_SERIES_2, Token.PLOT_SERIES_3,
    Token.PLOT_SERIES_4, Token.PLOT_SERIES_5, Token.PLOT_SERIES_6,
)

_g, _b, _r, _y = base.gray, base.blue, base.red, base.yellow

#: Token values as ``(light, dark)``. ``#aarrggbb`` marks translucent ones.
#: The dark column is provisional, see the module docstring.
_VALUES: dict[Token, tuple[str, str]] = {
    Token.STATE_OK:          (base.signal_green, base.signal_green),
    Token.STATE_BUSY:        (base.signal_orange, base.signal_orange),
    Token.STATE_WARNING:     (base.signal_orange, base.signal_orange),
    Token.STATE_ERROR:       (base.signal_red, base.signal_red),
    Token.STATE_INACTIVE:    (_g.s400, _g.s500),
    Token.STATE_INFO:        (_b.s500, _b.s400),

    Token.TEXT_HEADING:      (_g.s900, _g.s50),
    Token.TEXT_PRIMARY:      (base.ink, _g.s100),
    Token.TEXT_SECONDARY:    (_g.s600, _g.s300),
    Token.TEXT_MUTED:        (_g.s500, _g.s400),
    Token.TEXT_PLACEHOLDER:  (_g.s400, _g.s500),
    Token.TEXT_DISABLED:     (_g.s300, _g.s600),
    Token.TEXT_WARNING:      (_y.s700, _y.s400),
    Token.TEXT_ON_ACCENT:    (base.white, base.white),

    Token.SURFACE:           (base.white, _g.s900),
    Token.SURFACE_ALT:       (_g.s50, _g.s800),
    Token.SURFACE_MUTED:     (base.chrome, _g.s800),
    Token.BORDER:            (_g.s300, _g.s600),
    Token.BORDER_STRONG:     (_g.s400, _g.s500),
    Token.HIGHLIGHT_CHANGED: (_y.s100, _y.s900),

    Token.OVERLAY_HOVER:     ("#14ffffff", "#14ffffff"),
    Token.OVERLAY_PRESSED:   ("#28ffffff", "#28ffffff"),

    Token.ACCENT:            (_b.s600, _b.s500),
    Token.ACCENT_HOVER:      (_b.s700, _b.s400),
    Token.ACCENT_PRESSED:    (_b.s800, _b.s600),
    Token.DANGER:            (_r.s600, _r.s500),
    Token.DANGER_HOVER:      (_r.s700, _r.s400),
    Token.DANGER_PRESSED:    (_r.s800, _r.s600),
    Token.CONTROL_DISABLED:  (_g.s400, _g.s600),

    Token.INDICATOR_ON:      (_g.s900, _g.s100),
    Token.INDICATOR_OFF:     (_g.s300, _g.s600),

    Token.PLOT_SERIES_1:     (base.plot_red, base.plot_red),
    Token.PLOT_SERIES_2:     (base.plot_green, base.plot_green),
    Token.PLOT_SERIES_3:     (base.plot_blue, base.plot_blue),
    Token.PLOT_SERIES_4:     (base.plot_amber, base.plot_amber),
    Token.PLOT_SERIES_5:     (base.plot_pink, base.plot_pink),
    Token.PLOT_SERIES_6:     (base.plot_mint, base.plot_mint),
    Token.PLOT_MAGNITUDE:    (base.plot_light, base.plot_light),
    Token.PLOT_BACKGROUND:   (base.plot_black, base.plot_black),
    Token.PLOT_AXIS:         (base.plot_axis, base.plot_axis),
    # pyqtgraph draws the grid with the axis pen at alpha 0.3 (77 / 255).
    Token.PLOT_GRID:         ("#4d969696", "#4d969696"),
}

del _g, _b, _r, _y


# =========================================================================== #
#  Theme resolution
# =========================================================================== #

#: Theme the tokens resolve against; ``None`` until first use.
_active: Optional[Theme] = None


def _resolve_theme() -> Theme:
    """Reduce the stored theme setting to ``LIGHT`` or ``DARK``."""
    from duifx.gui.theming import get_theme, selectable_themes

    try:
        theme = get_theme()
    except Exception:  # settings unavailable: keep drawing
        logger.debug("Theme setting unreadable, colours use LIGHT", exc_info=True)
        return Theme.LIGHT

    if theme is Theme.SYSTEM:
        theme = _system_theme()
    # Unimplemented themes fall back like apply_theme() does.
    if theme not in selectable_themes():
        return Theme.LIGHT
    return theme


def _system_theme() -> Theme:
    """The operating system's colour scheme, ``LIGHT`` if unknown."""
    from PySide6.QtCore import Qt
    from PySide6.QtGui import QGuiApplication

    app = QGuiApplication.instance()
    hints = app.styleHints() if app is not None else None
    if hints is not None and hasattr(hints, "colorScheme"):
        if hints.colorScheme() == Qt.ColorScheme.Dark:
            return Theme.DARK
    return Theme.LIGHT


def active_theme() -> Theme:
    """Theme the colour tokens currently resolve against (LIGHT or DARK).

    Resolved from the theme setting on first use and kept for the rest of the
    process -- a theme change takes effect on the next start anyway (see
    :func:`duifx.gui.theming.set_theme`).
    """
    global _active
    if _active is None:
        _active = _resolve_theme()
    return _active


def set_active_theme(theme: Theme | str | None) -> None:
    """Pin the theme colours resolve against; ``None`` re-reads the setting.

    Only needed when :func:`~duifx.gui.theming.apply_theme` was given an
    explicit theme that differs from the stored one, or in tests.
    """
    global _active
    if theme is None:
        _active = None
        return
    from duifx.gui.theming import parse_theme

    resolved = parse_theme(theme)
    _active = Theme.DARK if resolved is Theme.DARK else Theme.LIGHT


# =========================================================================== #
#  Accessors
# =========================================================================== #


def _value(token: Token, theme: Theme | None) -> str:
    light, dark = _VALUES[Token(token)]
    return dark if (theme or active_theme()) is Theme.DARK else light


def color(token: Token, alpha: int | None = None,
          theme: Theme | None = None) -> QColor:
    """Return *token*'s colour for *theme* (default: :func:`active_theme`).

    *alpha* (0-255) overrides the token's own opacity, e.g. for translucent
    scatter points.
    """
    qcolor = QColor(_value(token, theme))
    if alpha is not None:
        qcolor.setAlpha(alpha)
    return qcolor


def color_hex(token: Token, theme: Theme | None = None) -> str:
    """Return *token*'s colour as ``#rrggbb`` (opacity dropped)."""
    return color(token, theme=theme).name()


def css(token: Token, alpha: int | None = None,
        theme: Theme | None = None) -> str:
    """Return *token*'s colour for a Qt stylesheet.

    ``#rrggbb`` for opaque colours, ``rgba(r, g, b, a)`` for translucent ones
    (Qt stylesheets do not read ``#aarrggbb``).
    """
    qcolor = color(token, alpha, theme)
    if qcolor.alpha() == 255:
        return qcolor.name()
    return (f"rgba({qcolor.red()}, {qcolor.green()}, {qcolor.blue()}, "
            f"{qcolor.alpha()})")


def plot_series_color(index: int, alpha: int | None = None) -> QColor:
    """Colour of the *index*-th plot series, cycling through :data:`PLOT_SERIES`."""
    return color(PLOT_SERIES[index % len(PLOT_SERIES)], alpha)


# =========================================================================== #
#  State colours
# =========================================================================== #

#: State names the applications report, lower case. Covers CAN network,
#: Modbus master and CANopen node states.
DEFAULT_STATE_TOKENS: dict[str, Token] = {
    "connected": Token.STATE_OK,
    "operational": Token.STATE_OK,
    "online": Token.STATE_OK,
    "active": Token.STATE_OK,
    "ok": Token.STATE_OK,
    "scanning": Token.STATE_BUSY,
    "connecting": Token.STATE_BUSY,
    "warning": Token.STATE_WARNING,
    "error": Token.STATE_ERROR,
    "fault": Token.STATE_ERROR,
    "heartbeat lost": Token.STATE_ERROR,
    "disconnected": Token.STATE_INACTIVE,
    "offline": Token.STATE_INACTIVE,
    "inactive": Token.STATE_INACTIVE,
    "unknown": Token.STATE_INACTIVE,
}


class StateColors:
    """Maps state names to colours, case-insensitively.

    Reads like the ``dict[str, str]`` of hex colours it replaces::

        state_colors["Connected"]             # '#27ae60'
        state_colors.get(entry.state)         # default token for unknown ones
        state_colors.color(entry.state)       # QColor

    *mapping* adds to (or overrides) :data:`DEFAULT_STATE_TOKENS`; unknown
    states resolve to *default*.
    """

    def __init__(self, mapping: Mapping[str, Token] | None = None,
                 default: Token = Token.TEXT_SECONDARY) -> None:
        self._tokens = dict(DEFAULT_STATE_TOKENS)
        self._tokens.update({k.strip().lower(): v for k, v in (mapping or {}).items()})
        self._default = default

    def token(self, state: object) -> Token:
        """Token for *state*, the default token if it is unknown."""
        return self._tokens.get(str(state).strip().lower(), self._default)

    def color(self, state: object) -> QColor:
        """Colour for *state* as :class:`QColor`."""
        return color(self.token(state))

    def hex(self, state: object) -> str:
        """Colour for *state* as ``#rrggbb``."""
        return color_hex(self.token(state))

    def get(self, state: object, default: str | None = None) -> str:
        """Hex colour for *state*; *default* (a hex string) if it is unknown."""
        key = str(state).strip().lower()
        if key not in self._tokens and default is not None:
            return default
        return self.hex(state)

    def __getitem__(self, state: object) -> str:
        return self.hex(state)

    def __contains__(self, state: object) -> bool:
        return str(state).strip().lower() in self._tokens


#: Shared instance with :data:`DEFAULT_STATE_TOKENS`.
state_colors = StateColors()

"""Plot widgets for DUIFx: live time series, XY scatter, compass dial, gate bar.

Data-agnostic building blocks for instrument views. They know about samples,
series, ranges, labels and units -- never about where the numbers come from --
so an application only decides *what* to plot and feeds the values in.

* :class:`TimeSeriesPlot` -- named series over time with a rolling window,
  for live sensor traces. A series can be a line, a dashed line or points, and
  can break at a wrap-around (a heading jumping from 359 to 0 degrees).
* :class:`ScatterPlot` -- aspect-locked XY point clouds with an optional
  reference circle, for magnetometer and similar calibration checks.
* :class:`CompassDial` -- a heading dial with any number of needles and an
  uncertainty wedge.
* :class:`ThresholdBar` -- a progress bar judged against a gate.

The two plots draw with ``pyqtgraph`` (and therefore ``numpy``): live data at
several hundred samples per second, zoom, pan and auto-ranging are what it is
built for, and redoing that on bare QPainter would be a project of its own.
Neither is a hard dependency of DUIFx (``pip install duifx[plot]``). Without
them this module still imports, the plots show a hint instead of curves and
accept data silently -- so a minimal install keeps every other number on
screen. :class:`CompassDial` and :class:`ThresholdBar` need only Qt.

Redrawing is throttled: feeding data only fills buffers and marks the plot
dirty, and a timer redraws at most every ``refresh_ms`` -- and not at all while
the widget is hidden, e.g. on another tab. Sensors deliver far faster than a
screen can usefully repaint.

Every colour lives in one :class:`PlotStyle`, defaulting to the DUIFx palette
(:mod:`duifx.gui.colors`). Widgets take one, or fall back to
:func:`default_plot_style`, and can be restyled at runtime with ``set_style``.
"""

from __future__ import annotations

import logging
import math
from collections import deque
from dataclasses import dataclass, field
from typing import Callable, Iterable, Mapping, Optional, Sequence

from PySide6.QtCore import QPointF, QRectF, QSize, Qt, QTimer
from PySide6.QtGui import QColor, QFont, QPainter, QPainterPath, QPen, QPolygonF
from PySide6.QtWidgets import QLabel, QProgressBar, QSizePolicy, QVBoxLayout, QWidget

from duifx.gui import colors
from duifx.gui.colors import Token

logger = logging.getLogger(__name__)

# PySide6 is imported above on purpose: pyqtgraph binds to whichever Qt
# binding is already loaded, and must not pick up a PyQt that happens to be
# installed alongside.
try:
    import numpy as np
    import pyqtgraph as pg
    HAVE_PYQTGRAPH = True
except ImportError:  # pragma: no cover - depends on the install
    np = None
    pg = None
    HAVE_PYQTGRAPH = False
    logger.info("pyqtgraph/numpy not installed -- duifx plots show a hint instead of curves")

#: Shown in place of a plot when pyqtgraph is missing.
UNAVAILABLE_TEXT = "Live plots need pyqtgraph and numpy:  pip install duifx[plot]"

#: Redraw interval of the plots. Matches a 10 Hz display refresh, which is
#: plenty for the eye and leaves the GUI thread free for everything else.
DEFAULT_REFRESH_MS = 100

#: Default buffer length per time series.
DEFAULT_MAX_SAMPLES = 20000

#: Default buffer length per scatter series. A few thousand points already
#: show the shape of a cloud; more only cost redraw time.
DEFAULT_MAX_POINTS = 3000


# ---------------------------------------------------------------------------
# Style
# ---------------------------------------------------------------------------

def _token(token: Token) -> Callable[[], str]:
    """Field default that resolves *token* when a style is created.

    Resolving lazily (not at import) lets the active theme -- read from the
    settings -- decide, and keeps importing this module free of side effects.
    """
    return lambda: colors.color_hex(token)


def _series_default() -> tuple[str, ...]:
    return tuple(colors.color_hex(token) for token in colors.PLOT_SERIES)


@dataclass(frozen=True)
class PlotStyle:
    """All colours and stroke sizes of the plot widgets, in one place.

    Colours are anything :class:`QColor` accepts, normally ``"#rrggbb"``.
    The defaults come from the DUIFx palette (:mod:`duifx.gui.colors`): the
    dark instrument look of the EdgePulse DevStream view, with the firmware
    monitor's series colours, so screenshots of either tool read the same
    way. Derive variants with :func:`dataclasses.replace`.
    """

    #: Plot canvas and dial face.
    background: str = field(default_factory=_token(Token.PLOT_BACKGROUND))
    #: Axis lines and tick labels. The grid is drawn in this colour too, at
    #: :attr:`grid_alpha`.
    axis: str = field(default_factory=_token(Token.PLOT_AXIS))
    #: Axis labels, legend entries, dial labels and readout.
    text: str = field(default_factory=_token(Token.PLOT_AXIS))
    #: Plot titles.
    title: str = field(default_factory=_token(Token.PLOT_AXIS))
    #: Opacity of the grid lines, 0..1.
    grid_alpha: float = 0.3
    #: Colour cycle for series without an explicit colour, in the order of
    #: :data:`duifx.gui.colors.PLOT_SERIES`: the first three are the
    #: x / y / z axis colours, then amber, pink and teal.
    series: tuple[str, ...] = field(default_factory=_series_default)
    #: Derived traces such as a magnitude ``|v|``.
    magnitude: str = field(default_factory=_token(Token.PLOT_MAGNITUDE))
    #: Reference lines and circles (expected values, calibration radius).
    reference: str = field(default_factory=_token(Token.PLOT_SERIES_4))
    #: Main needle of :class:`CompassDial`.
    accent: str = field(default_factory=_token(Token.PLOT_SERIES_4))
    #: Opacity of scatter points, 0..255. Overlapping points then build up
    #: density instead of a solid blob.
    marker_alpha: int = 150
    #: Opacity of the dial's uncertainty wedge, 0..255.
    wedge_alpha: int = 70
    #: Line width in pixels.
    line_width: float = 1.0
    #: Diameter of scatter points in pixels.
    marker_size: float = 3.0
    #: :class:`ThresholdBar` chunk when the gate is met.
    ok: str = field(default_factory=_token(Token.STATE_OK))
    #: :class:`ThresholdBar` chunk when the gate is missed.
    warn: str = field(default_factory=_token(Token.STATE_WARNING))
    #: Hint text when pyqtgraph is missing.
    muted: str = field(default_factory=_token(Token.TEXT_MUTED))

    def series_color(self, index: int) -> str:
        """Colour of the *index*-th series, cycling through :attr:`series`."""
        return self.series[index % len(self.series)]


#: Created on first use, see :func:`_token`.
_default_style: Optional[PlotStyle] = None


def default_plot_style() -> PlotStyle:
    """The style widgets use when none is passed."""
    global _default_style
    if _default_style is None:
        _default_style = PlotStyle()
    return _default_style


def set_default_plot_style(style: PlotStyle) -> None:
    """Replace the style used by widgets created *from now on*.

    Existing widgets keep theirs; restyle them with ``set_style``.
    """
    global _default_style
    _default_style = style


def _qcolor(spec: str, alpha: Optional[int] = None) -> QColor:
    color = QColor(spec)
    if alpha is not None:
        color.setAlpha(alpha)
    return color


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def axis_label(text: str, unit: str = "") -> str:
    """``"text [unit]"``, or just *text* without a unit.

    Units go in brackets rather than through pyqtgraph's ``units`` argument,
    which would rescale the axis with SI prefixes -- a field in µT shown as
    "mT" or a heading in "k°" helps nobody.
    """
    if not unit:
        return text
    return f"{text} [{unit}]" if text else f"[{unit}]"


_POINTS_8 = ("N", "NE", "E", "SE", "S", "SW", "W", "NW")
_POINTS_16 = ("N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
              "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW")


def compass_point(heading: float, points: int = 8) -> str:
    """Name of the compass point nearest to *heading* (degrees, 0 = north).

    *points* is 4, 8 or 16.
    """
    if points == 16:
        names = _POINTS_16
    elif points == 4:
        names = _POINTS_8[::2]
    else:
        names = _POINTS_8
    step = 360.0 / len(names)
    return names[int((heading % 360.0) / step + 0.5) % len(names)]


def link_x_axes(*plots: "_PlotBase") -> None:
    """Link the x axes of *plots* to the first one: zoom and pan move all."""
    if len(plots) < 2 or not HAVE_PYQTGRAPH:
        return
    first = plots[0].plot_item
    for plot in plots[1:]:
        plot.plot_item.setXLink(first)


def _break_wraps(x, y, period: float):
    """Insert NaN gaps where *y* wraps, so a line does not cross the plot.

    A jump of more than half a period between neighbours is taken as a wrap
    (359 -> 1 degrees), not as motion.
    """
    if len(y) < 2:
        return x, y
    jumps = np.nonzero(np.abs(np.diff(y)) > period / 2.0)[0] + 1
    if not jumps.size:
        return x, y
    return np.insert(x, jumps, np.nan), np.insert(y, jumps, np.nan)


# ---------------------------------------------------------------------------
# Plot base
# ---------------------------------------------------------------------------

class _Series:
    """One named series: its look and its buffers."""

    __slots__ = ("key", "label", "color", "index", "dashed", "points", "width",
                 "size", "alpha", "wrap", "x", "y", "item")

    def __init__(self, key: str, label: str, color: Optional[str], index: int,
                 dashed: bool, points: bool, width: Optional[float],
                 size: Optional[float], alpha: Optional[int], wrap: Optional[float],
                 maxlen: int) -> None:
        self.key = key
        self.label = label
        self.color = color
        self.index = index
        self.dashed = dashed
        self.points = points
        self.width = width
        self.size = size
        self.alpha = alpha
        self.wrap = wrap
        self.x: deque = deque(maxlen=maxlen)
        self.y: deque = deque(maxlen=maxlen)
        self.item = None


class _PlotBase(QWidget):
    """A pyqtgraph plot inside a QWidget, with series, styling and throttling.

    Wrapping rather than subclassing ``pg.PlotWidget`` keeps this module
    importable without pyqtgraph and the public API independent of it;
    :attr:`plot_item` is the escape hatch for anything not covered here.
    """

    def __init__(self, title: str, style: Optional[PlotStyle], legend: bool,
                 refresh_ms: int, parent: Optional[QWidget]) -> None:
        super().__init__(parent)
        self._style = style or default_plot_style()
        self._title = title
        self._labels: dict[str, tuple[str, str]] = {}
        self._series: dict[str, _Series] = {}
        self._references: dict[str, tuple[object, Optional[str], bool]] = {}
        self._dirty = False

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)

        if HAVE_PYQTGRAPH:
            self._widget = pg.PlotWidget()
            self._item = self._widget.getPlotItem()
            self._legend = self._item.addLegend(offset=(10, 10)) if legend else None
            layout.addWidget(self._widget)
            self._hint = None
        else:
            self._widget = None
            self._item = None
            self._legend = None
            self._hint = QLabel(UNAVAILABLE_TEXT)
            self._hint.setWordWrap(True)
            self._hint.setAlignment(Qt.AlignmentFlag.AlignCenter)
            layout.addWidget(self._hint)

        self._timer = QTimer(self)
        self._timer.setSingleShot(True)
        self._timer.setInterval(max(0, refresh_ms))
        self._timer.timeout.connect(self._on_timer)

        self._apply_style()

    # -- properties --------------------------------------------------------

    @property
    def available(self) -> bool:
        """False when pyqtgraph is missing and the widget shows a hint only."""
        return self._item is not None

    @property
    def plot_item(self):
        """The underlying ``pg.PlotItem``, or None without pyqtgraph."""
        return self._item

    @property
    def plot_style(self) -> PlotStyle:
        """The :class:`PlotStyle` in use."""
        return self._style

    # -- labels ------------------------------------------------------------

    def set_title(self, title: str) -> None:
        self._title = title
        if self._item is not None:
            self._item.setTitle(title or None, color=self._style.title)

    def set_axis_label(self, side: str, text: str, unit: str = "") -> None:
        """Label the ``"left"``, ``"bottom"``, ``"right"`` or ``"top"`` axis."""
        self._labels[side] = (text, unit)
        if self._item is not None:
            self._item.setLabel(side, axis_label(text, unit), color=self._style.text)

    # -- style -------------------------------------------------------------

    def set_style(self, style: PlotStyle) -> None:
        """Restyle the plot and every series without an explicit colour."""
        self._style = style
        self._apply_style()

    def _apply_style(self) -> None:
        style = self._style
        if self._item is None:
            self._hint.setStyleSheet(f"color: {style.muted}; font-style: italic;")
            return
        self._widget.setBackground(style.background)
        for side in ("left", "bottom", "right", "top"):
            axis = self._item.getAxis(side)
            axis.setPen(style.axis)
            axis.setTextPen(style.axis)
        self._item.showGrid(x=True, y=True, alpha=style.grid_alpha)
        if self._legend is not None:
            self._legend.setLabelTextColor(style.text)
        self.set_title(self._title)
        for side, (text, unit) in self._labels.items():
            self.set_axis_label(side, text, unit)
        for series in self._series.values():
            self._style_series(series)
        for key, (line, color, dashed) in self._references.items():
            line.setPen(self._ref_pen(color, dashed))

    def _series_color(self, series: _Series) -> str:
        return series.color or self._style.series_color(series.index)

    def _style_series(self, series: _Series) -> None:
        if series.item is None:
            return
        color = self._series_color(series)
        if series.points:
            series.item.setPen(None)
            series.item.setSymbol("o")
            series.item.setSymbolSize(series.size or self._style.marker_size)
            series.item.setSymbolPen(None)
            alpha = self._style.marker_alpha if series.alpha is None else series.alpha
            series.item.setSymbolBrush(_qcolor(color, alpha))
        else:
            series.item.setSymbol(None)
            series.item.setPen(pg.mkPen(
                _qcolor(color, series.alpha), width=series.width or self._style.line_width,
                style=Qt.PenStyle.DashLine if series.dashed else Qt.PenStyle.SolidLine))

    def _ref_pen(self, color: Optional[str], dashed: bool):
        return pg.mkPen(color or self._style.reference, width=self._style.line_width,
                        style=Qt.PenStyle.DashLine if dashed else Qt.PenStyle.SolidLine)

    # -- series ------------------------------------------------------------

    def _add_series(self, key: str, label: Optional[str], color: Optional[str],
                    dashed: bool, points: bool, width: Optional[float],
                    size: Optional[float], alpha: Optional[int], wrap: Optional[float],
                    maxlen: int) -> None:
        if key in self._series:
            self.remove_series(key)
        index = max((s.index for s in self._series.values()), default=-1) + 1
        series = _Series(key, key if label is None else label, color, index,
                         dashed, points, width, size, alpha, wrap, maxlen)
        if self._item is not None:
            # An empty name keeps the series out of the legend.
            series.item = pg.PlotDataItem(name=series.label or None)
            self._item.addItem(series.item)
            self._style_series(series)
        self._series[key] = series

    def remove_series(self, key: str) -> None:
        series = self._series.pop(key, None)
        if series is not None and series.item is not None:
            self._item.removeItem(series.item)

    def series_keys(self) -> list[str]:
        """Keys of the series, in the order they were added."""
        return list(self._series)

    def has_series(self, key: str) -> bool:
        return key in self._series

    def drawn_data(self, key: str) -> tuple[list[float], list[float]]:
        """The points series *key* draws, as ``(x, y)`` lists.

        That is the buffer after the plot is done with it: cut to the window,
        in relative time, with NaN gaps at wraps. A pending redraw is brought
        forward, visible or not. Empty without pyqtgraph.
        """
        series = self._series[key]
        if self._dirty:
            self.redraw()
        if series.item is None:
            return [], []
        x, y = series.item.getData()
        return ([] if x is None else x.tolist()), ([] if y is None else y.tolist())

    def clear(self) -> None:
        """Empty every series, keeping the series themselves."""
        for series in self._series.values():
            series.x.clear()
            series.y.clear()
        self.schedule_redraw()

    def _get_series(self, key: Optional[str]) -> _Series:
        """The series *key*; the first one, or a new default one, for None."""
        if key is None:
            if self._series:
                return next(iter(self._series.values()))
            key = "data"
        if key not in self._series:
            self._add_default_series(key)
        return self._series[key]

    def _add_default_series(self, key: str) -> None:
        raise NotImplementedError

    def _set_series_data(self, key: Optional[str], xs: Iterable[float],
                         ys: Iterable[float]) -> None:
        series = self._get_series(key)
        series.x.clear()
        series.y.clear()
        series.x.extend(xs)
        series.y.extend(ys)
        if len(series.x) != len(series.y):
            raise ValueError(f"series {series.key!r}: x and y differ in length")
        self.schedule_redraw()

    # -- redraw ------------------------------------------------------------

    def schedule_redraw(self) -> None:
        """Mark the plot dirty; it redraws within ``refresh_ms``."""
        self._dirty = True
        if self._item is not None and not self._timer.isActive():
            self._timer.start()

    def redraw(self) -> None:
        """Redraw now, visible or not."""
        self._dirty = False
        if self._item is not None:
            self._render()

    def _on_timer(self) -> None:
        # Hidden (another tab, a collapsed splitter): stay dirty and catch up
        # in showEvent instead of spending time on pixels nobody sees.
        if self._dirty and self.isVisible():
            self.redraw()

    def showEvent(self, event) -> None:
        super().showEvent(event)
        if self._dirty:
            self.schedule_redraw()

    def _render(self) -> None:
        raise NotImplementedError

    @staticmethod
    def _arrays(series: _Series):
        n = min(len(series.x), len(series.y))
        x = np.fromiter(series.x, dtype=float, count=n)
        y = np.fromiter(series.y, dtype=float, count=n)
        return x, y


# ---------------------------------------------------------------------------
# Time series
# ---------------------------------------------------------------------------

class TimeSeriesPlot(_PlotBase):
    """Named series over time, scrolling with a fixed window.

    Feed samples incrementally with :meth:`append` / :meth:`extend`, or hand
    over whole buffers (e.g. ring buffers kept elsewhere) with
    :meth:`set_data`. Samples outside the window are kept but not drawn;
    each series keeps at most *max_samples*.

    With *relative_time* the newest sample sits at x = 0 and the window runs
    to ``-window``, so the axis reads "seconds ago" whatever the device clock
    says. "Newest" is the latest timestamp of this plot's series unless
    :meth:`set_now` pins it -- which is how several plots fed from one source
    stay aligned.

    Parameters
    ----------
    title:
        Plot title.
    y_label, y_unit:
        Left axis label and unit, shown as ``"label [unit]"``.
    x_label, x_unit:
        Bottom axis.
    window:
        Visible time span. None shows everything and auto-ranges x.
    max_samples:
        Buffer length per series.
    legend:
        Show a legend of the labelled series.
    relative_time:
        Plot time relative to the newest sample (see above).
    style:
        Colours; :func:`default_plot_style` when None.
    refresh_ms:
        Minimum interval between redraws.
    """

    def __init__(self, title: str = "", y_label: str = "", y_unit: str = "", *,
                 x_label: str = "time", x_unit: str = "s",
                 window: Optional[float] = 10.0,
                 max_samples: int = DEFAULT_MAX_SAMPLES,
                 legend: bool = True,
                 relative_time: bool = True,
                 style: Optional[PlotStyle] = None,
                 refresh_ms: int = DEFAULT_REFRESH_MS,
                 parent: Optional[QWidget] = None) -> None:
        super().__init__(title, style, legend, refresh_ms, parent)
        self._window = window
        self._max_samples = max_samples
        self._relative = relative_time
        self._now: Optional[float] = None
        self._y_range: Optional[tuple[float, float]] = None
        # Reference line values, kept apart from the items so they read back
        # the same without pyqtgraph.
        self._reference_values: dict[str, Optional[float]] = {}
        self.set_axis_label("left", y_label, y_unit)
        self.set_axis_label("bottom", x_label, x_unit)

    # -- configuration -----------------------------------------------------

    def add_series(self, key: str, label: Optional[str] = None, *,
                   color: Optional[str] = None, dashed: bool = False,
                   points: bool = False, width: Optional[float] = None,
                   size: Optional[float] = None, alpha: Optional[int] = None,
                   wrap: Optional[float] = None) -> None:
        """Add (or replace) the series *key*.

        Parameters
        ----------
        label:
            Legend entry; defaults to *key*, ``""`` keeps it out of the legend.
        color:
            Fixed colour; otherwise the next one of the style's cycle.
        dashed:
            Dashed line, e.g. for a derived quantity such as a magnitude.
        points:
            Draw points instead of a line.
        width, size, alpha:
            Line width, point size and opacity (0..255) overriding the style.
        wrap:
            Period of a wrapping quantity, e.g. 360 for a heading: the line
            breaks at a wrap instead of drawing a stroke across the plot.
        """
        self._add_series(key, label, color, dashed, points, width, size, alpha,
                         wrap, self._max_samples)

    def _add_default_series(self, key: str) -> None:
        self.add_series(key)

    def set_y_label(self, text: str, unit: str = "") -> None:
        self.set_axis_label("left", text, unit)

    def set_window(self, seconds: Optional[float]) -> None:
        """Visible time span; None shows everything."""
        self._window = seconds
        if seconds is None and self._item is not None:
            self._item.enableAutoRange(axis="x")
        self.schedule_redraw()

    @property
    def window(self) -> Optional[float]:
        return self._window

    def set_y_range(self, low: Optional[float] = None, high: Optional[float] = None) -> None:
        """Fix the y range, or auto-range it again with no arguments."""
        if low is None or high is None:
            self._y_range = None
            if self._item is not None:
                self._item.enableAutoRange(axis="y")
        else:
            self._y_range = (low, high)
            if self._item is not None:
                self._item.setYRange(low, high, padding=0)

    def set_now(self, t: Optional[float]) -> None:
        """Pin the time the window ends at; None follows the newest sample."""
        self._now = t
        self.schedule_redraw()

    def link_x(self, other: "TimeSeriesPlot") -> None:
        """Zoom and pan the x axis together with *other*."""
        link_x_axes(other, self)

    # -- reference lines ---------------------------------------------------

    def add_reference_line(self, key: str, value: Optional[float] = None, *,
                           color: Optional[str] = None, dashed: bool = True) -> None:
        """A horizontal line at *value*, e.g. an expected magnitude.

        Hidden while *value* is None; move it with :meth:`set_reference`.
        """
        self.remove_reference_line(key)
        self._reference_values[key] = None
        if self._item is not None:
            line = pg.InfiniteLine(angle=0, movable=False, pen=self._ref_pen(color, dashed))
            self._item.addItem(line)
            self._references[key] = (line, color, dashed)
        self.set_reference(key, value)

    def set_reference(self, key: str, value: Optional[float]) -> None:
        if key not in self._reference_values:
            return
        self._reference_values[key] = value
        entry = self._references.get(key)
        if entry is None:
            return
        line = entry[0]
        if value is None:
            line.setVisible(False)
        else:
            line.setValue(value)
            line.setVisible(True)

    def reference(self, key: str) -> Optional[float]:
        """Value of the reference line *key*; None while hidden or unknown."""
        return self._reference_values.get(key)

    def remove_reference_line(self, key: str) -> None:
        self._reference_values.pop(key, None)
        entry = self._references.pop(key, None)
        if entry is not None:
            self._item.removeItem(entry[0])

    # -- data --------------------------------------------------------------

    def append(self, t: float, values: Mapping[str, float] | float) -> None:
        """Add one sample at time *t*.

        *values* maps series keys to values; unknown keys get a new series
        with default looks. A plain number goes to the first series.
        """
        if not isinstance(values, Mapping):
            values = {self._get_series(None).key: values}
        for key, value in values.items():
            series = self._get_series(key)
            series.x.append(t)
            series.y.append(value)
        self.schedule_redraw()

    def extend(self, t: Sequence[float], values: Mapping[str, Sequence[float]]) -> None:
        """Add a block of samples: *t* and, per key, as many values."""
        for key, ys in values.items():
            if len(ys) != len(t):
                raise ValueError(f"series {key!r}: {len(ys)} values for {len(t)} times")
            series = self._get_series(key)
            series.x.extend(t)
            series.y.extend(ys)
        self.schedule_redraw()

    def set_data(self, key: str, t: Iterable[float], y: Iterable[float]) -> None:
        """Replace the whole series *key* (created if new)."""
        self._set_series_data(key, t, y)

    def latest_time(self) -> Optional[float]:
        """The newest timestamp of any series, or None when empty."""
        ends = [s.x[-1] for s in self._series.values() if s.x]
        return max(ends) if ends else None

    # -- drawing -----------------------------------------------------------

    def _render(self) -> None:
        now = self._now if self._now is not None else self.latest_time()
        for series in self._series.values():
            if now is None or not series.x:
                series.item.setData([], [])
                continue
            x, y = self._arrays(series)
            if self._window is not None:
                keep = x >= now - self._window
                x, y = x[keep], y[keep]
            if self._relative:
                x = x - now
            if series.wrap:
                x, y = _break_wraps(x, y, series.wrap)
                series.item.setData(x, y, connect="finite")
            else:
                series.item.setData(x, y)
        if self._window is not None and now is not None:
            end = 0.0 if self._relative else now
            self._item.setXRange(end - self._window, end, padding=0)


# ---------------------------------------------------------------------------
# XY scatter
# ---------------------------------------------------------------------------

class ScatterPlot(_PlotBase):
    """XY point clouds, aspect-locked by default, with a reference circle.

    Made for calibration checks: a magnetometer turned in every direction
    traces a circle, and how round and how centred it is shows the remaining
    hard- and soft-iron error -- which is only visible when a circle looks
    like a circle, hence the locked aspect ratio. :meth:`set_reference_circle`
    draws the radius the data should have.

    Each series keeps the newest *max_points* points.

    Parameters
    ----------
    title:
        Plot title.
    x_label, y_label, unit:
        Axis labels; *unit* applies to both.
    max_points:
        Buffer length per series.
    aspect_locked:
        Keep one unit on x as long as one unit on y.
    legend:
        Show a legend of the labelled series.
    style, refresh_ms:
        As for :class:`TimeSeriesPlot`.
    """

    def __init__(self, title: str = "", x_label: str = "", y_label: str = "",
                 unit: str = "", *,
                 max_points: int = DEFAULT_MAX_POINTS,
                 aspect_locked: bool = True,
                 legend: bool = False,
                 style: Optional[PlotStyle] = None,
                 refresh_ms: int = DEFAULT_REFRESH_MS,
                 parent: Optional[QWidget] = None) -> None:
        super().__init__(title, style, legend, refresh_ms, parent)
        self._max_points = max_points
        self._circles: dict[str, tuple[float, float, float]] = {}
        if self._item is not None:
            self._item.setAspectLocked(aspect_locked)
        self.set_axis_labels(x_label, y_label, unit)

    def set_axis_labels(self, x_label: Optional[str] = None, y_label: Optional[str] = None,
                        unit: Optional[str] = None) -> None:
        """Change any of the axis labels or the shared unit."""
        x_text, x_unit = self._labels.get("bottom", ("", ""))
        y_text, y_unit = self._labels.get("left", ("", ""))
        self.set_axis_label("bottom", x_text if x_label is None else x_label,
                            x_unit if unit is None else unit)
        self.set_axis_label("left", y_text if y_label is None else y_label,
                            y_unit if unit is None else unit)

    def add_series(self, key: str, label: Optional[str] = None, *,
                   color: Optional[str] = None, size: Optional[float] = None,
                   alpha: Optional[int] = None, line: bool = False) -> None:
        """Add (or replace) the point series *key*.

        *line* connects the points instead -- for a trajectory rather than a
        cloud.
        """
        self._add_series(key, label, color, False, not line, None, size, alpha,
                         None, self._max_points)

    def _add_default_series(self, key: str) -> None:
        self.add_series(key, "")

    # -- data --------------------------------------------------------------

    def append(self, x: float, y: float, key: Optional[str] = None) -> None:
        """Add one point to *key* (the first series when None)."""
        series = self._get_series(key)
        series.x.append(x)
        series.y.append(y)
        self.schedule_redraw()

    def extend(self, xs: Sequence[float], ys: Sequence[float], key: Optional[str] = None) -> None:
        if len(xs) != len(ys):
            raise ValueError("x and y differ in length")
        series = self._get_series(key)
        series.x.extend(xs)
        series.y.extend(ys)
        self.schedule_redraw()

    def set_data(self, xs: Iterable[float], ys: Iterable[float], key: Optional[str] = None) -> None:
        """Replace the points of *key*; only the newest *max_points* are kept."""
        self._set_series_data(key, xs, ys)

    # -- reference circle --------------------------------------------------

    def set_reference_circle(self, radius: Optional[float],
                             center: tuple[float, float] = (0.0, 0.0), *,
                             key: str = "reference", color: Optional[str] = None,
                             dashed: bool = True) -> None:
        """Draw a circle of *radius* around *center*; None removes it."""
        if radius is None or radius <= 0:
            self._circles.pop(key, None)
            entry = self._references.pop(key, None)
            if entry is not None:
                self._item.removeItem(entry[0])
            return
        self._circles[key] = (radius, center[0], center[1])
        if self._item is not None:
            if key in self._references:
                item = self._references[key][0]
            else:
                item = pg.PlotDataItem()
                self._item.addItem(item)
            self._references[key] = (item, color, dashed)
            item.setPen(self._ref_pen(color, dashed))
        self.schedule_redraw()

    def reference_circle(self, key: str = "reference") -> Optional[tuple[float, tuple[float, float]]]:
        """``(radius, (cx, cy))`` of the reference circle *key*, or None."""
        circle = self._circles.get(key)
        return None if circle is None else (circle[0], (circle[1], circle[2]))

    # -- drawing -----------------------------------------------------------

    def _render(self) -> None:
        for series in self._series.values():
            if not series.x:
                series.item.setData([], [])
                continue
            series.item.setData(*self._arrays(series))
        angle = np.linspace(0.0, 2.0 * np.pi, 181)
        for key, (radius, cx, cy) in self._circles.items():
            self._references[key][0].setData(cx + radius * np.cos(angle),
                                             cy + radius * np.sin(angle))


# ---------------------------------------------------------------------------
# Compass dial
# ---------------------------------------------------------------------------

class CompassDial(QWidget):
    """A north-up compass rose with rotating needles.

    Headings are in degrees, clockwise from north. The first needle uses the
    style's accent colour, further ones the series cycle. An uncertainty
    (± degrees) draws a wedge around its needle; a readout under the centre
    shows the first needle's heading and compass point unless replaced with
    :meth:`set_readout`. Needles with a label are listed in a legend under
    the rose, one per line in the needle's colour.

    Parameters
    ----------
    points:
        Compass points labelled on the rose: 4 or 8.
    show_readout:
        Show the numeric readout.
    show_legend:
        Show the legend of the labelled needles.
    style:
        Colours; :func:`default_plot_style` when None.
    """

    def __init__(self, *, points: int = 8, show_readout: bool = True,
                 show_legend: bool = True,
                 style: Optional[PlotStyle] = None,
                 parent: Optional[QWidget] = None) -> None:
        super().__init__(parent)
        self._style = style or default_plot_style()
        self._points = 8 if points >= 8 else 4
        self._show_readout = show_readout
        self._show_legend = show_legend
        self._readout: Optional[str] = None
        # key -> [heading, uncertainty, colour, label]
        self._needles: dict[str, list] = {}
        self.add_needle("heading")
        policy = QSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        policy.setHeightForWidth(True)
        self.setSizePolicy(policy)

    @property
    def plot_style(self) -> PlotStyle:
        return self._style

    def set_style(self, style: PlotStyle) -> None:
        self._style = style
        self.update()

    def add_needle(self, key: str, *, color: Optional[str] = None,
                   label: str = "") -> None:
        """Add (or restyle) the needle *key*, hidden until it has a heading."""
        old = self._needles.get(key)
        self._needles[key] = [old[0] if old else None, old[1] if old else None, color, label]
        self.updateGeometry()   # the legend may have grown
        self.update()

    def remove_needle(self, key: str) -> None:
        self._needles.pop(key, None)
        self.updateGeometry()
        self.update()

    def set_heading(self, heading: Optional[float], key: str = "heading") -> None:
        """Point needle *key* at *heading*; None hides it."""
        if key not in self._needles:
            self.add_needle(key)
        self._needles[key][0] = None if heading is None else heading % 360.0
        self.update()

    def heading(self, key: str = "heading") -> Optional[float]:
        needle = self._needles.get(key)
        return needle[0] if needle else None

    def set_uncertainty(self, degrees: Optional[float], key: str = "heading") -> None:
        """± *degrees* wedge around needle *key*; None removes it."""
        if key not in self._needles:
            self.add_needle(key)
        self._needles[key][1] = None if degrees is None else abs(degrees)
        self.update()

    def set_readout(self, text: Optional[str]) -> None:
        """Replace the readout text; None restores the automatic one."""
        self._readout = text
        self.update()

    def clear(self) -> None:
        """Hide every needle and wedge."""
        for needle in self._needles.values():
            needle[0] = None
            needle[1] = None
        self.update()

    # -- geometry ------------------------------------------------------------

    def sizeHint(self) -> QSize:
        return QSize(200, 200 + self._legend_height())

    def minimumSizeHint(self) -> QSize:
        return QSize(100, 100 + self._legend_height())

    def hasHeightForWidth(self) -> bool:
        return True

    def heightForWidth(self, width: int) -> int:
        return width + self._legend_height()

    def _legend_entries(self) -> list[tuple[int, list]]:
        """``(index, needle)`` of the needles the legend lists."""
        if not self._show_legend:
            return []
        return [(i, n) for i, n in enumerate(self._needles.values()) if n[3]]

    def _legend_height(self) -> int:
        """Pixels the legend takes under the rose, 0 without one."""
        entries = len(self._legend_entries())
        if not entries:
            return 0
        return entries * self.fontMetrics().height() + 4

    # -- painting ------------------------------------------------------------

    def _needle_color(self, index: int, needle: list) -> str:
        if needle[2]:
            return needle[2]
        return self._style.accent if index == 0 else self._style.series_color(index - 1)

    def _paint_legend(self, painter: QPainter, top: float) -> None:
        """One line per labelled needle: a short stroke and the label."""
        metrics = self.fontMetrics()
        line_h = metrics.height()
        swatch = line_h * 1.2
        entries = self._legend_entries()
        width = swatch + 6 + max(metrics.horizontalAdvance(n[3]) for _, n in entries)
        left = max(2.0, (self.width() - width) / 2.0)
        painter.setFont(self.font())
        for row, (index, needle) in enumerate(entries):
            color = _qcolor(self._needle_color(index, needle))
            y = top + row * line_h
            painter.setPen(QPen(color, 2.0, Qt.PenStyle.SolidLine, Qt.PenCapStyle.RoundCap))
            painter.drawLine(QPointF(left, y + line_h / 2.0), QPointF(left + swatch, y + line_h / 2.0))
            painter.setPen(_qcolor(self._style.text))
            painter.drawText(QRectF(left + swatch + 6, y, self.width() - left - swatch - 6, line_h),
                             Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignVCenter, needle[3])

    def _automatic_readout(self) -> str:
        if not self._needles:
            return ""
        heading = next(iter(self._needles.values()))[0]
        return "" if heading is None else f"{heading:.1f}°  {compass_point(heading, 16)}"

    def paintEvent(self, event) -> None:
        style = self._style
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        painter.fillRect(self.rect(), _qcolor(style.background))

        # The legend gets a strip at the bottom, the rose what is left.
        legend_h = self._legend_height()
        rose_h = self.height() - legend_h
        if legend_h:
            self._paint_legend(painter, rose_h)
        side = min(self.width(), rose_h)
        radius = side / 2.0 - max(4.0, side * 0.03)
        if radius <= 10:
            painter.end()
            return
        center = QPointF(self.width() / 2.0, rose_h / 2.0)

        def at(angle_deg: float, r: float) -> QPointF:
            a = math.radians(angle_deg)
            return QPointF(center.x() + r * math.sin(a), center.y() - r * math.cos(a))

        # Ring and ticks: long every 30 degrees, medium every 10, short every 5.
        axis = _qcolor(style.axis)
        painter.setPen(QPen(axis, 1.5))
        painter.setBrush(Qt.BrushStyle.NoBrush)
        painter.drawEllipse(center, radius, radius)
        for deg in range(0, 360, 5):
            length = radius * (0.10 if deg % 30 == 0 else 0.06 if deg % 10 == 0 else 0.03)
            painter.setPen(QPen(axis, 1.5 if deg % 30 == 0 else 1.0))
            painter.drawLine(at(deg, radius), at(deg, radius - length))

        # Labels: the compass points, and the degrees at the remaining 30s.
        label_r = radius * 0.76
        font = QFont(self.font())
        font.setPixelSize(max(8, int(radius * 0.13)))
        font.setBold(True)
        small = QFont(font)
        small.setBold(False)
        small.setPixelSize(max(7, int(radius * 0.085)))
        step = 360 // self._points
        names = _POINTS_8 if self._points == 8 else _POINTS_8[::2]
        text_color = _qcolor(style.text)
        for deg in range(0, 360, 30 if step == 90 else 15):
            if deg % step == 0:
                painter.setFont(font if deg % 90 == 0 else small)
                text = names[deg // step]
            elif deg % 30 == 0:
                painter.setFont(small)
                text = str(deg)
            else:
                continue
            painter.setPen(text_color)
            p = at(deg, label_r)
            box = QRectF(p.x() - radius * 0.2, p.y() - radius * 0.1, radius * 0.4, radius * 0.2)
            painter.drawText(box, Qt.AlignmentFlag.AlignCenter, text)

        # Uncertainty wedges first, so the needles stay on top.
        painter.setPen(Qt.PenStyle.NoPen)
        for index, needle in enumerate(self._needles.values()):
            heading, spread = needle[0], needle[1]
            if heading is None or not spread:
                continue
            spread = min(spread, 180.0)
            painter.setBrush(_qcolor(self._needle_color(index, needle), style.wedge_alpha))
            path = QPainterPath(center)
            ring = QRectF(center.x() - radius, center.y() - radius, 2 * radius, 2 * radius)
            # Qt counts angles counter-clockwise from 3 o'clock.
            path.arcTo(ring, 90.0 - (heading - spread), -2.0 * spread)
            path.closeSubpath()
            painter.drawPath(path)

        # Needles: a line with an arrow head, and a short tail.
        for index, needle in enumerate(self._needles.values()):
            heading = needle[0]
            if heading is None:
                continue
            color = _qcolor(self._needle_color(index, needle))
            width = max(2.0, radius * 0.025)
            painter.setPen(QPen(color, width, Qt.PenStyle.SolidLine, Qt.PenCapStyle.RoundCap))
            tip = at(heading, radius * 0.88)
            painter.drawLine(at(heading + 180.0, radius * 0.18), at(heading, radius * 0.70))
            painter.setPen(Qt.PenStyle.NoPen)
            painter.setBrush(color)
            painter.drawPolygon(QPolygonF([
                tip,
                at(heading + 7.0, radius * 0.66),
                at(heading - 7.0, radius * 0.66),
            ]))

        painter.setBrush(axis)
        painter.setPen(Qt.PenStyle.NoPen)
        painter.drawEllipse(center, radius * 0.04, radius * 0.04)

        if self._show_readout:
            text = self._automatic_readout() if self._readout is None else self._readout
            if text:
                readout = QFont(self.font())
                readout.setPixelSize(max(8, int(radius * 0.12)))
                painter.setFont(readout)
                painter.setPen(text_color)
                box = QRectF(center.x() - radius * 0.6, center.y() + radius * 0.22,
                             radius * 1.2, radius * 0.24)
                painter.drawText(box, Qt.AlignmentFlag.AlignCenter, text)
        painter.end()


# ---------------------------------------------------------------------------
# Threshold bar
# ---------------------------------------------------------------------------

class ThresholdBar(QProgressBar):
    """A progress bar judged against a gate.

    The bar runs to the physical full scale; the gate is what matters, so the
    chunk turns ``style.ok`` once the value meets it and ``style.warn`` while
    it does not, and the text names both: ``"143.0°  (gate ≥ 180°)"``.
    *higher_is_better* False flips the test, for limits like a maximum error.

    Parameters
    ----------
    maximum:
        Full scale.
    gate:
        Threshold; None shows the value without judging it.
    unit:
        Appended to value and gate, e.g. ``"°"``.
    decimals:
        Decimals of the value in the text.
    """

    #: QProgressBar is integer-only; values are scaled by this before setting.
    _SCALE = 1000

    def __init__(self, maximum: float = 100.0, gate: Optional[float] = None, *,
                 unit: str = "", decimals: int = 1, higher_is_better: bool = True,
                 style: Optional[PlotStyle] = None,
                 parent: Optional[QWidget] = None) -> None:
        super().__init__(parent)
        self._style = style or default_plot_style()
        self._gate = gate
        self._unit = unit
        self._decimals = decimals
        self._higher_is_better = higher_is_better
        self._value: Optional[float] = None
        self.setTextVisible(True)
        self.set_maximum(maximum)

    @property
    def plot_style(self) -> PlotStyle:
        return self._style

    def set_style(self, style: PlotStyle) -> None:
        self._style = style
        self.set_value(self._value)

    def set_maximum(self, maximum: float) -> None:
        self.setRange(0, max(1, int(maximum * self._SCALE)))
        self.set_value(self._value)

    def set_gate(self, gate: Optional[float]) -> None:
        self._gate = gate
        self.set_value(self._value)

    def value_f(self) -> Optional[float]:
        """The value as set, unscaled; None when cleared."""
        return self._value

    def gate_met(self) -> Optional[bool]:
        """Whether the value meets the gate; None without value or gate."""
        if self._value is None or self._gate is None:
            return None
        if self._higher_is_better:
            return self._value >= self._gate
        return self._value <= self._gate

    def set_value(self, value: Optional[float]) -> None:
        """Show *value*; None clears the bar to a dash."""
        self._value = value
        if value is None:
            self.setValue(0)
            self.setFormat("—")
            self.setStyleSheet("")
            return
        self.setValue(max(0, min(int(value * self._SCALE), self.maximum())))
        text = f"{value:.{self._decimals}f}{self._unit}"
        met = self.gate_met()
        if met is None:
            self.setFormat(text)
            self.setStyleSheet("")
            return
        relation = "≥" if self._higher_is_better else "≤"
        self.setFormat(f"{text}  (gate {relation} {self._gate:g}{self._unit})")
        color = self._style.ok if met else self._style.warn
        self.setStyleSheet(f"QProgressBar::chunk {{ background-color: {color}; }}")

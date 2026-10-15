"""A text console that looks and lines up like a real terminal (PuTTY).

Why a widget of its own: the application stylesheet sets ``font-family`` and
``font-size`` on ``*``, and a stylesheet beats ``QWidget.setFont()``. A
``QPlainTextEdit`` given Consolas therefore still renders in the proportional
UI font, and column-aligned output - a command list, a table of values -
comes out ragged. The widgets here carry a style sheet of their own, which Qt
always prefers to the inherited application one, so they keep a fixed-pitch
font whatever the application stylesheet says.

What makes it a terminal and not a text box:

* PuTTY's look by default: Courier New 10 pt, light grey on black, a
  selection that inverts the colours (:class:`TerminalStyle`);
* tab stops every 8 columns;
* output is written as it arrives, a line that is not finished yet (a prompt
  such as ``$ ``) included; ``\\r\\n`` and ``\\n`` end a line, a lone ``\\r``
  returns to the start of the line and overwrites it, ``\\b`` erases the
  character before;
* lines wrap at the window edge anywhere, not at word boundaries - or not at
  all (``wrap=False``) with a horizontal scroll bar;
* it follows the end of the output while the view is at the bottom, and stays
  where it is while the user scrolls back.

Typical use::

    view = TerminalView()
    entry = TerminalLineEdit()
    port.text_received.connect(view.append_text)
"""

from __future__ import annotations

from dataclasses import dataclass, replace

from PySide6.QtCore import QEvent, Slot
from PySide6.QtGui import QFont, QTextCursor, QTextOption
from PySide6.QtWidgets import QLineEdit, QPlainTextEdit, QTextEdit, QWidget

__all__ = [
    "TerminalStyle",
    "default_terminal_style",
    "set_default_terminal_style",
    "terminal_font",
    "terminal_stylesheet",
    "apply_terminal_style",
    "apply_fixed_pitch",
    "TerminalView",
    "TerminalLineEdit",
]

#: Columns between two tab stops, as in a VT100 / PuTTY.
TAB_COLUMNS = 8


@dataclass(frozen=True)
class TerminalStyle:
    """Font and colours of the terminal widgets. The defaults are PuTTY's."""

    font_families: tuple[str, ...] = ("Courier New", "Consolas", "DejaVu Sans Mono", "monospace")
    point_size: int = 10
    foreground: str = "#bbbbbb"
    background: str = "#000000"
    selection_foreground: str = "#000000"
    selection_background: str = "#bbbbbb"
    border: str = "#3c3c3c"
    padding_px: int = 3


_default_style = TerminalStyle()


def default_terminal_style() -> TerminalStyle:
    """The style new terminal widgets take."""
    return _default_style


def set_default_terminal_style(style: TerminalStyle | None = None, **changes) -> TerminalStyle:
    """Set the style for terminal widgets created from now on: a whole
    :class:`TerminalStyle`, or single fields (``point_size=11``) changed on
    the current one. Returns the new default."""
    global _default_style
    _default_style = replace(style or _default_style, **changes)
    return _default_style


def terminal_font(style: TerminalStyle | None = None) -> QFont:
    """A fixed-pitch QFont for *style*: the first of its families the system
    has, otherwise whatever monospace font Qt finds."""
    style = style or _default_style
    font = QFont()
    font.setFamilies([f for f in style.font_families if f != "monospace"])
    font.setStyleHint(QFont.StyleHint.TypeWriter)
    font.setFixedPitch(True)
    font.setPointSize(style.point_size)
    return font


def terminal_stylesheet(widget_type: str, style: TerminalStyle | None = None) -> str:
    """The style sheet for one widget class (``"QPlainTextEdit"`` ...)."""
    style = style or _default_style
    families = ", ".join(f if f == "monospace" else f'"{f}"' for f in style.font_families)
    return (
        f"{widget_type} {{"
        f" font-family: {families};"
        f" font-size: {style.point_size}pt;"
        f" color: {style.foreground};"
        f" background-color: {style.background};"
        f" selection-color: {style.selection_foreground};"
        f" selection-background-color: {style.selection_background};"
        f" border: 1px solid {style.border};"
        f" border-radius: 0px;"
        f" padding: {style.padding_px}px;"
        f" }}"
        f" {widget_type}:focus {{ border: 1px solid {style.border}; }}"
        f" {widget_type}:disabled {{ color: {style.foreground}; background-color: {style.background}; }}"
    )


def apply_terminal_style(widget: QWidget, style: TerminalStyle | None = None) -> None:
    """Give any text widget (a QPlainTextEdit, QTextEdit or QLineEdit made
    elsewhere) the terminal font and colours."""
    style = style or _default_style
    for cls in (QPlainTextEdit, QTextEdit, QLineEdit):
        if isinstance(widget, cls):
            widget_type = cls.__name__
            break
    else:
        widget_type = type(widget).__name__
    widget.setFont(terminal_font(style))
    widget.setStyleSheet(terminal_stylesheet(widget_type, style))
    if isinstance(widget, (QPlainTextEdit, QTextEdit)):
        _set_tab_stops(widget)


def apply_fixed_pitch(widget: QWidget, point_size: int | None = None, bold: bool = False,
                      style: TerminalStyle | None = None) -> None:
    """Only the terminal's fixed-pitch font, not its colours: for a widget
    that keeps the application look but must line up - values in columns, a
    readout. Replaces the widget's own style sheet."""
    style = style or _default_style
    size = point_size or style.point_size
    families = ", ".join(f if f == "monospace" else f'"{f}"' for f in style.font_families)
    font = terminal_font(style)
    font.setPointSize(size)
    font.setBold(bold)
    widget.setFont(font)
    widget.setStyleSheet(f"font-family: {families}; font-size: {size}pt;"
                         + (" font-weight: bold;" if bold else ""))
    if isinstance(widget, (QPlainTextEdit, QTextEdit)):
        _set_tab_stops(widget)


def _set_tab_stops(widget: QPlainTextEdit | QTextEdit) -> None:
    widget.setTabStopDistance(widget.fontMetrics().horizontalAdvance(" ") * TAB_COLUMNS)


class TerminalView(QPlainTextEdit):
    """Read-only terminal output; see the module docstring.

    Parameters
    ----------
    max_lines:
        Lines kept; the oldest are dropped beyond it (0 = no limit).
    wrap:
        Wrap at the window edge (anywhere, like a terminal) or not at all.
    style:
        Font and colours; :func:`default_terminal_style` when None.
    """

    def __init__(self, parent: QWidget | None = None, *, max_lines: int = 5000, wrap: bool = True,
                 style: TerminalStyle | None = None) -> None:
        super().__init__(parent)
        self.setReadOnly(True)
        self.setUndoRedoEnabled(False)
        self.setMaximumBlockCount(max_lines)
        self.setWrapping(wrap)
        apply_terminal_style(self, style)
        self._autoscroll = True
        # Column the next character overwrites after a lone CR, -1 = append.
        self._overwrite_from = -1

    # ------------------------------------------------------------------ #

    @property
    def autoscroll(self) -> bool:
        """Follow the end of the output (only while the view is at the
        bottom: scrolling back keeps the view where the user put it)."""
        return self._autoscroll

    @autoscroll.setter
    def autoscroll(self, on: bool) -> None:
        self._autoscroll = bool(on)
        if self._autoscroll:
            self.scroll_to_end()

    def setWrapping(self, wrap: bool) -> None:  # noqa: N802 - Qt naming
        """Wrap anywhere at the window edge, or not at all."""
        if wrap:
            self.setLineWrapMode(QPlainTextEdit.LineWrapMode.WidgetWidth)
            self.setWordWrapMode(QTextOption.WrapMode.WrapAnywhere)
        else:
            self.setLineWrapMode(QPlainTextEdit.LineWrapMode.NoWrap)

    def scroll_to_end(self) -> None:
        bar = self.verticalScrollBar()
        bar.setValue(bar.maximum())

    # ------------------------------------------------------------------ #

    @Slot(str)
    def append_text(self, text: str) -> None:
        """Write *text* at the end as a terminal would: unfinished lines show
        at once, CR LF / LF end a line, a lone CR returns to the line start,
        BS erases the character before."""
        if not text:
            return
        bar = self.verticalScrollBar()
        follow = self._autoscroll and bar.value() >= bar.maximum() - 2
        cursor = QTextCursor(self.document())
        cursor.movePosition(QTextCursor.MoveOperation.End)
        cursor.beginEditBlock()
        run: list[str] = []
        text = text.replace("\r\n", "\n")
        for ch in text:
            if ch == "\n":
                self._flush(cursor, run)
                cursor.movePosition(QTextCursor.MoveOperation.End)
                cursor.insertBlock()
                self._overwrite_from = -1
            elif ch == "\r":
                self._flush(cursor, run)
                self._overwrite_from = 0
            elif ch == "\b":
                self._flush(cursor, run)
                self._backspace(cursor)
            else:
                run.append(ch)
        self._flush(cursor, run)
        cursor.endEditBlock()
        if follow:
            self.scroll_to_end()

    def append_line(self, line: str) -> None:
        """A whole line of its own (a note of the application): starts on a
        new line if the output stopped in the middle of one."""
        cursor = QTextCursor(self.document())
        cursor.movePosition(QTextCursor.MoveOperation.End)
        prefix = "\n" if cursor.block().length() > 1 else ""
        self.append_text(prefix + line + "\n")

    def clear(self) -> None:  # noqa: D401 - Qt naming
        super().clear()
        self._overwrite_from = -1

    def _flush(self, cursor: QTextCursor, run: list[str]) -> None:
        if not run:
            return
        chunk = "".join(run)
        run.clear()
        if self._overwrite_from >= 0:
            # After a lone CR: replace the line from that column on.
            block = self.document().lastBlock()
            cursor.setPosition(block.position() + min(self._overwrite_from, block.length() - 1))
            end = min(block.position() + block.length() - 1, cursor.position() + len(chunk))
            cursor.setPosition(end, QTextCursor.MoveMode.KeepAnchor)
            cursor.insertText(chunk)
            self._overwrite_from += len(chunk)
            if self._overwrite_from >= block.length() - 1:
                self._overwrite_from = -1
        else:
            cursor.movePosition(QTextCursor.MoveOperation.End)
            cursor.insertText(chunk)

    def _backspace(self, cursor: QTextCursor) -> None:
        cursor.movePosition(QTextCursor.MoveOperation.End)
        if cursor.positionInBlock() > 0:
            cursor.deletePreviousChar()

    def changeEvent(self, event: QEvent) -> None:  # noqa: N802 - Qt naming
        super().changeEvent(event)
        if event.type() in (QEvent.Type.FontChange, QEvent.Type.StyleChange):
            _set_tab_stops(self)


class TerminalLineEdit(QLineEdit):
    """A one-line input in the terminal's font and colours."""

    def __init__(self, parent: QWidget | None = None, *, style: TerminalStyle | None = None) -> None:
        super().__init__(parent)
        apply_terminal_style(self, style)

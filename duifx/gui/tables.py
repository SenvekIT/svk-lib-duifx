"""Column widths the user can drag, sized to their content until he does.

``QHeaderView.ResizeMode.ResizeToContents`` and ``Stretch`` give good widths
but lock the columns: the header cannot be dragged. ``Interactive`` can be
dragged, but leaves every column at the default width however wide its content
is. :class:`ColumnSizer` combines the two:

* every section is ``Interactive``, so every column can be dragged;
* each column is sized to its content (header text included) whenever the
  content changes -- throttled, so a table refreshed ten times a second does
  not measure itself ten times a second -- until the user drags that column;
  from then on the user's width is kept;
* one column (the last by default) fills the width left over, like
  ``Stretch``, but stays draggable; once dragged, it keeps its width too.

A double-click on a section's edge still sizes that column to its content, as
Qt does for interactive sections.

Typical use, one line per view::

    make_columns_interactive(table)                  # last column fills
    make_columns_interactive(tree, fill=1)           # the second one fills
    make_columns_interactive(table, fill=None)       # nothing fills

Works for ``QTableView`` / ``QTableWidget`` (horizontal header) and
``QTreeView`` / ``QTreeWidget`` (``header()``).
"""

from __future__ import annotations

from typing import Optional

from PySide6.QtCore import QEvent, QObject, Qt, QTimer
from PySide6.QtWidgets import QAbstractItemView, QApplication, QHeaderView, QTableView, QTreeView

__all__ = ["ColumnSizer", "make_columns_interactive"]

#: Content changes are measured at most this often.
DEFAULT_FIT_INTERVAL_MS = 250


def _header(view: QAbstractItemView) -> QHeaderView:
    if isinstance(view, QTableView):
        return view.horizontalHeader()
    if isinstance(view, QTreeView):
        return view.header()
    raise TypeError(f"no column header on {type(view).__name__}")


class ColumnSizer(QObject):
    """Interactive columns sized to their content; see the module docstring.

    Parented to *view*, so it lives exactly as long as the view.

    Parameters
    ----------
    view:
        A table or tree view.
    fill:
        Column that takes the width left over: an index, -1 for the last
        column, None for none.
    auto_fit:
        Size the columns to their content again whenever the model changes
        (rows inserted or removed, data changed, reset). Without it, call
        :meth:`fit` yourself.
    max_width:
        Upper limit of a width sized to content, so one long value does not
        push everything else out of sight. None for no limit. The user can
        still drag wider.
    interval_ms:
        Throttle of the automatic fit.
    """

    def __init__(self, view: QAbstractItemView, *, fill: Optional[int] = -1,
                 auto_fit: bool = True, max_width: Optional[int] = 400,
                 interval_ms: int = DEFAULT_FIT_INTERVAL_MS) -> None:
        super().__init__(view)
        self._view = view
        self._header = _header(view)
        self._fill = fill
        self._max_width = max_width
        self._user: set[int] = set()
        self._busy = False
        self._model = None

        self._header.setStretchLastSection(False)
        self._header.setSectionResizeMode(QHeaderView.ResizeMode.Interactive)
        self._header.sectionResized.connect(self._on_section_resized)
        self._viewport = view.viewport()
        self._viewport.installEventFilter(self)

        self._timer = QTimer(self)
        self._timer.setSingleShot(True)
        self._timer.setInterval(max(0, interval_ms))
        self._timer.timeout.connect(self.fit)
        if auto_fit:
            self._watch_model()
            # A view replacing its model (setModel) gets the new one watched.
            self._header.sectionCountChanged.connect(lambda *_: self._watch_model())
        self.fit()

    # -- public ------------------------------------------------------------

    @property
    def user_sized(self) -> frozenset[int]:
        """The columns the user has dragged; they keep their width."""
        return frozenset(self._user)

    def fill_column(self) -> Optional[int]:
        """The column taking the left-over width, or None."""
        count = self._header.count()
        if self._fill is None or count == 0:
            return None
        column = count - 1 if self._fill < 0 else self._fill
        return column if 0 <= column < count else None

    def fit(self) -> None:
        """Size every column the user has not dragged to its content now,
        then let the fill column take what is left."""
        self._timer.stop()
        header = self._header
        fill = self.fill_column()
        self._busy = True
        try:
            for column in range(header.count()):
                if column in self._user or header.isSectionHidden(column):
                    continue
                width = max(self._view.sizeHintForColumn(column), header.sectionSizeHint(column))
                if self._max_width is not None and column != fill:
                    width = min(width, self._max_width)
                width = max(width, header.minimumSectionSize())
                if header.sectionSize(column) != width:
                    header.resizeSection(column, width)
        finally:
            self._busy = False
        self._fill_rest()

    def reset(self) -> None:
        """Forget the widths the user dragged and size everything afresh."""
        self._user.clear()
        self.fit()

    def schedule_fit(self) -> None:
        """Fit within the throttle interval (several calls, one fit)."""
        try:
            if not self._timer.isActive():
                self._timer.start()
        except RuntimeError:            # a model outliving its view
            pass

    # -- internals -----------------------------------------------------------

    def _watch_model(self) -> None:
        model = self._view.model()
        if model is None or model is self._model:
            return
        if self._model is not None:
            for signal in self._model_signals(self._model):
                try:
                    signal.disconnect(self.schedule_fit)
                except (RuntimeError, TypeError):
                    pass
        self._model = model
        for signal in self._model_signals(model):
            signal.connect(self.schedule_fit)

    @staticmethod
    def _model_signals(model):
        return (model.modelReset, model.layoutChanged, model.rowsInserted,
                model.rowsRemoved, model.dataChanged, model.headerDataChanged)

    def _fill_rest(self) -> None:
        """Give the fill column the width the others leave, at least its own
        content width; a horizontal scroll bar takes over below that."""
        column = self.fill_column()
        if column is None or column in self._user:
            return
        header = self._header
        others = sum(header.sectionSize(c) for c in range(header.count())
                     if c != column and not header.isSectionHidden(c))
        content = max(self._view.sizeHintForColumn(column), header.sectionSizeHint(column),
                      header.minimumSectionSize())
        width = max(content, self._view.viewport().width() - others)
        if header.sectionSize(column) != width:
            self._busy = True
            try:
                header.resizeSection(column, width)
            finally:
                self._busy = False

    def _on_section_resized(self, column: int, _old: int, _new: int) -> None:
        if self._busy:
            return
        # Only a drag counts as the user's choice - not the view resizing a
        # section itself, nor a programmatic resize between two fits.
        if QApplication.mouseButtons() & Qt.MouseButton.LeftButton:
            self._user.add(column)
            if column != self.fill_column():
                self._fill_rest()

    def eventFilter(self, obj, event) -> bool:
        if event.type() == QEvent.Type.Resize and obj is self._viewport:
            try:
                self._fill_rest()
            except RuntimeError:        # the view is being destroyed
                pass
        return False


def make_columns_interactive(view: QAbstractItemView, *, fill: Optional[int] = -1,
                             auto_fit: bool = True,
                             max_width: Optional[int] = 400) -> ColumnSizer:
    """Make *view*'s columns draggable, sized to their content until the user
    drags them, with column *fill* (default the last) taking the left-over
    width. Returns the :class:`ColumnSizer` (also reachable as a child of
    *view*); keeping a reference is optional."""
    return ColumnSizer(view, fill=fill, auto_fit=auto_fit, max_width=max_width)

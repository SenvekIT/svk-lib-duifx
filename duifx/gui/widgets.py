"""Log viewer widgets for DUIFx.

Provides a performant log viewer with level filters, automatic file persistence
(JSON Lines with per-session timestamped files), an in-memory ring buffer
with batched UI updates and scroll-back loading of older entries from disk.

Every start writes a new session file and nothing ever removed them, so the log
directory grew without bound (896 files / 85 MB on one development machine after
four months).  Startup now prunes the oldest sessions down to
``log_retention_sessions``.
"""

from __future__ import annotations

import json
import logging
import os
import re
import threading
from datetime import datetime
from functools import lru_cache
from pathlib import Path
from typing import List, TypedDict

from PySide6.QtCore import (
    Qt,
    QObject,
    Signal,
    QAbstractTableModel,
    QModelIndex,
    QTimer,
    QThread,
)
from PySide6.QtGui import QColor, QPainter
from PySide6.QtWidgets import (
    QApplication,
    QWidget,
    QVBoxLayout,
    QHBoxLayout,
    QCheckBox,
    QTableView,
    QHeaderView,
    QSizePolicy,
    QPushButton,
    QFileDialog,
    QLabel,
    QMessageBox,
    QLineEdit,
    QAbstractItemView,
    QStyle,
    QStyleOptionSlider,
    QStyledItemDelegate,
    QScrollBar,
)

# ---------------------------------------------------------------------------
# Lightweight log entry dict (replaces heavy logging.LogRecord in storage)
# ---------------------------------------------------------------------------

class _LogEntry(TypedDict):
    ts: float
    name: str
    lvl: str
    lvl_no: int
    msg: str


def _record_to_entry(record: logging.LogRecord) -> _LogEntry:
    return _LogEntry(
        ts=record.created,
        name=record.name,
        lvl=record.levelname,
        lvl_no=record.levelno,
        msg=record.getMessage(),
    )


#: Plain-text lines as written by ``LogViewerWidget._save_logs``:
#: ``2026-08-03 11:50:13,123 - app.core.x - DEBUG   - message``
_TEXT_LINE_RE = re.compile(
    r"^(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2},\d{3})"
    r" - (\S+)\s+- (\w+)\s+- (.*)$",
    re.DOTALL,
)


def _parse_log_line(raw: str) -> _LogEntry | None:
    """Turn one line of a log file into an entry, or return None.

    Accepts both formats the application produces: the JSON Lines session files
    and the plain-text files written by "Save Log".  Loading a log the user
    saved themselves is the obvious thing to try, so refusing it would be a
    poor surprise.
    """
    raw = raw.strip()
    if not raw:
        return None

    if raw.startswith("{"):
        try:
            entry = json.loads(raw)
            # A foreign JSON file would parse but lack the fields we index on.
            return _LogEntry(
                ts=float(entry["ts"]),
                name=str(entry["name"]),
                lvl=str(entry["lvl"]),
                lvl_no=int(entry["lvl_no"]),
                msg=str(entry["msg"]),
            )
        except (json.JSONDecodeError, KeyError, TypeError, ValueError):
            return None

    match = _TEXT_LINE_RE.match(raw)
    if not match:
        return None

    ts_str, name, level, msg = match.groups()
    try:
        ts = datetime.strptime(ts_str, "%Y-%m-%d %H:%M:%S,%f").timestamp()
    except ValueError:
        return None

    # getLevelName maps a known name to its number and echoes the name back
    # otherwise; anything unknown is treated as INFO so it stays visible.
    lvl_no = logging.getLevelName(level)
    if not isinstance(lvl_no, int):
        lvl_no = logging.INFO

    return _LogEntry(ts=ts, name=name, lvl=level, lvl_no=lvl_no, msg=msg)


# ---------------------------------------------------------------------------
# Session log retention
# ---------------------------------------------------------------------------

#: Number of session files kept when the setting is absent.  0 disables pruning.
LOG_RETENTION_SESSIONS_DEFAULT = 30

#: Above this many entries, loading a file asks for confirmation first. Reading
#: is not the cost -- 300 000 entries parse in about a second -- but they also
#: occupy roughly 130 MB, so it should be a deliberate choice.
LOG_LOAD_WARN_ENTRIES = 300_000

#: Name pattern of the per-session files, used both for writing and for pruning.
LOG_SESSION_PREFIX = "app_"
LOG_SESSION_SUFFIX = ".jsonl"


def prune_session_logs(log_dir: str, keep: int) -> int:
    """Delete all but the *keep* newest session logs in *log_dir*.

    Only files matching the session naming pattern are considered, so anything
    else the user keeps in that directory is left alone.  Sorting is by name:
    the timestamp is fixed-width and zero-padded, so lexical order is
    chronological order, and unlike mtime it survives a file copy.

    Returns the number of files removed.  Failures are logged and swallowed --
    housekeeping must never keep the application from starting.
    """
    if keep <= 0:
        return 0

    try:
        entries = sorted(
            name for name in os.listdir(log_dir)
            if name.startswith(LOG_SESSION_PREFIX) and name.endswith(LOG_SESSION_SUFFIX)
        )
    except OSError as exc:
        logging.getLogger(__name__).warning("Cannot list log directory %s: %s", log_dir, exc)
        return 0

    # The file for the session about to start does not exist yet, so keep one
    # slot free for it: with keep=1 the previous session still goes away.
    excess = len(entries) - max(keep - 1, 0)
    if excess <= 0:
        return 0

    removed = 0
    for name in entries[:excess]:
        try:
            os.remove(os.path.join(log_dir, name))
            removed += 1
        except OSError as exc:
            logging.getLogger(__name__).warning("Cannot delete old log %s: %s", name, exc)

    return removed


# ---------------------------------------------------------------------------
# JSON Lines file handler -- writes one JSON object per line
# ---------------------------------------------------------------------------

class _JsonLinesFileHandler(logging.FileHandler):
    """FileHandler that writes ``_LogEntry`` dicts as JSON Lines.

    Each application session creates a new timestamped file so that
    previous session logs are preserved on disk.
    """

    def __init__(self, path: str):
        super().__init__(path, mode="a", encoding="utf-8")

    def emit(self, record: logging.LogRecord) -> None:
        entry = _record_to_entry(record)
        try:
            self.stream.write(json.dumps(entry, ensure_ascii=False) + "\n")
            self.stream.flush()
        except Exception:
            self.handleError(record)


# ---------------------------------------------------------------------------
# Qt signal bridge
# ---------------------------------------------------------------------------

class _LogEmitter(QObject):
    record_emitted = Signal(object)


class QtLogHandler(logging.Handler):
    """Logging handler that forwards records into the Qt event loop."""

    def __init__(self, emitter: _LogEmitter):
        super().__init__()
        self._emitter = emitter

    def emit(self, record: logging.LogRecord) -> None:
        self._emitter.record_emitted.emit(record)


# ---------------------------------------------------------------------------
# Virtual table model -- cached record list, batched updates
# ---------------------------------------------------------------------------

_COLUMNS = ("Time", "Module", "Level", "Message")

#: Row background per severity, so errors and warnings stand out while
#: scrolling. Matched by threshold (highest first) rather than by exact
#: level number, so custom levels in between are coloured too.
_LEVEL_BACKGROUNDS: tuple[tuple[int, QColor], ...] = (
    (logging.CRITICAL, QColor("#f5c2c7")),   # red, stronger than ERROR
    (logging.ERROR,    QColor("#f8d7da")),   # red
    (logging.WARNING,  QColor("#faffb2")),   # orange
)
#: Tinted backgrounds are light, so the text colour is pinned to a dark one
#: instead of inheriting a palette that may render it unreadable.
_LEVEL_FOREGROUND = QColor("#212529")


def _level_background(lvl_no: int) -> QColor | None:
    for threshold, color in _LEVEL_BACKGROUNDS:
        if lvl_no >= threshold:
            return color
    return None


#: Marks on the overview scrollbar. Saturated on purpose: these are a few
#: pixels tall and have to read against the scrollbar groove, unlike the
#: pale row tints above.
_LEVEL_MARKERS: tuple[tuple[int, QColor], ...] = (
    (logging.CRITICAL, QColor("#b71c1c")),
    (logging.ERROR,    QColor("#e53935")),
    (logging.WARNING,  QColor("#fb8c00")),
)


def _marker_color(lvl_no: int) -> QColor | None:
    for threshold, color in _LEVEL_MARKERS:
        if lvl_no >= threshold:
            return color
    return None


class _LogItemDelegate(QStyledItemDelegate):
    """Paints the severity background behind each cell.

    The view carries a ``QTableView::item`` stylesheet rule, which routes
    item painting through QStyleSheetStyle -- and that ignores
    ``Qt.BackgroundRole`` entirely. Filling the rect here keeps both the
    stylesheet and the per-level tint. Selected rows are left to the style
    so the selection stays visible.
    """

    def paint(self, painter, option, index) -> None:
        if not option.state & QStyle.StateFlag.State_Selected:
            background = index.data(Qt.ItemDataRole.BackgroundRole)
            if background is not None:
                painter.fillRect(option.rect, background)
        super().paint(painter, option, index)


class _OverviewScrollBar(QScrollBar):
    """Vertical scrollbar that marks where the warnings and errors are.

    The same idea as the overview ruler in editors like VS Code: every
    visible row at WARNING or above leaves a coloured tick at its relative
    position in the log, so the interesting parts can be found -- and
    jumped to -- without scrolling through everything.
    """

    #: Rebuilds are coalesced. During live logging the model changes on
    #: every batch flush (50 ms), and rescanning a six-figure row count that
    #: often would cost more than the ruler is worth.
    _REBUILD_DELAY_MS = 250
    _MARK_HEIGHT = 2

    def __init__(self, model: "_LogTableModel", parent: QWidget | None = None):
        super().__init__(Qt.Orientation.Vertical, parent)
        self._model = model
        self._marks: List[tuple[int, int]] = []

        self._rebuild_timer = QTimer(self)
        self._rebuild_timer.setSingleShot(True)
        self._rebuild_timer.setInterval(self._REBUILD_DELAY_MS)
        self._rebuild_timer.timeout.connect(self._rebuild_marks)

        for signal in (model.rowsInserted, model.rowsRemoved,
                       model.modelReset, model.layoutChanged):
            signal.connect(self._schedule_rebuild)

    # -- mark maintenance ------------------------------------------------------

    def _schedule_rebuild(self, *_args) -> None:
        if not self._rebuild_timer.isActive():
            self._rebuild_timer.start()

    def _rebuild_marks(self) -> None:
        self._marks = self._model.marker_rows()
        self.update()

    # -- painting --------------------------------------------------------------

    def _groove_rect(self):
        option = QStyleOptionSlider()
        self.initStyleOption(option)
        return self.style().subControlRect(
            QStyle.ComplexControl.CC_ScrollBar, option,
            QStyle.SubControl.SC_ScrollBarGroove, self,
        )

    def paintEvent(self, event) -> None:
        super().paintEvent(event)
        if not self._marks:
            return
        total = self._model.rowCount()
        if total <= 0:
            return
        groove = self._groove_rect()
        if groove.height() <= self._MARK_HEIGHT:
            return

        # Collapse to one mark per pixel row, strongest severity winning:
        # a 100 000-row log would otherwise repaint the same few hundred
        # pixels thousands of times.
        span = groove.height() - self._MARK_HEIGHT
        buckets: dict[int, int] = {}
        for row, lvl_no in self._marks:
            if row >= total:
                continue
            y = groove.top() + int(span * row / total)
            if buckets.get(y, 0) < lvl_no:
                buckets[y] = lvl_no
        if not buckets:
            return

        # Drawn over the handle, not beside it: a mark hidden under the
        # handle is a mark the user cannot find.
        width = max(3, groove.width() // 2)
        x = groove.right() - width + 1

        painter = QPainter(self)
        for y, lvl_no in buckets.items():
            color = _marker_color(lvl_no)
            if color is not None:
                painter.fillRect(x, y, width, self._MARK_HEIGHT, color)
        painter.end()


class _LogTableModel(QAbstractTableModel):
    """Model backed by a flat record list with cached lookups and batched inserts."""

    def __init__(self, parent: QObject | None = None):
        super().__init__(parent)
        # Flat record storage: file scroll-back entries + live entries
        self._file_records: List[_LogEntry] = []
        self._live_records: List[_LogEntry] = []
        self._live_max: int = 5_000

        # Cached merged view -- invalidated on mutation
        self._cache: List[_LogEntry] | None = None
        self._filtered: List[int] = []

        # Pending batch of live entries not yet pushed to the view
        self._pending: List[_LogEntry] = []

        # Filter state (set externally before calling rebuild_filter)
        self.show_debug = True
        self.show_info = True
        self.show_warning = True
        self.show_error = True
        self.show_unseen_only = False
        self.seen_count: int = 0
        self.search_text: str = ""

        # File scroll-back state
        self._log_path: str = ""
        self._file_read_pos: int = 0
        self._file_exhausted: bool = True

        # Review state -- a log file loaded for inspection.  While active the
        # view shows the loaded file instead of the session; incoming records
        # keep filling _live_records so returning to live is instant.
        self._review_active: bool = False
        self._review_records: List[_LogEntry] = []
        self._review_path: str = ""
        self._review_read_pos: int = 0
        self._review_exhausted: bool = True
        self._live_missed: int = 0

    # -- cached record list ----------------------------------------------------

    def _all_records(self) -> List[_LogEntry]:
        if self._cache is None:
            if self._review_active:
                self._cache = list(self._review_records)
            else:
                self._cache = self._file_records + self._live_records
        return self._cache

    def _invalidate_cache(self) -> None:
        self._cache = None

    # -- visibility check ------------------------------------------------------

    def _is_visible(self, entry: _LogEntry, global_idx: int) -> bool:
        lvl = entry["lvl_no"]
        if lvl >= logging.ERROR and not self.show_error:
            return False
        if logging.WARNING <= lvl < logging.ERROR and not self.show_warning:
            return False
        if logging.INFO <= lvl < logging.WARNING and not self.show_info:
            return False
        if lvl < logging.INFO and not self.show_debug:
            return False
        if self.show_unseen_only and global_idx < self.seen_count:
            return False
        if self.search_text:
            needle = self.search_text
            # Check cheap fields first, format timestamp only if needed
            if (needle in entry["name"].lower()
                    or needle in entry["lvl"].lower()
                    or needle in entry["msg"].lower()):
                return True
            ts_str = datetime.fromtimestamp(entry["ts"]).strftime("%Y-%m-%d %H:%M:%S")
            return needle in ts_str
        return True

    # -- QAbstractTableModel interface ----------------------------------------

    def rowCount(self, parent: QModelIndex = QModelIndex()) -> int:
        return len(self._filtered)

    def columnCount(self, parent: QModelIndex = QModelIndex()) -> int:
        return len(_COLUMNS)

    def headerData(self, section: int, orientation: Qt.Orientation, role: int = Qt.ItemDataRole.DisplayRole):
        if role == Qt.ItemDataRole.DisplayRole and orientation == Qt.Orientation.Horizontal:
            return _COLUMNS[section]
        return None

    def data(self, index: QModelIndex, role: int = Qt.ItemDataRole.DisplayRole):
        if not index.isValid():
            return None
        if role not in (Qt.ItemDataRole.DisplayRole, Qt.ItemDataRole.UserRole,
                        Qt.ItemDataRole.BackgroundRole,
                        Qt.ItemDataRole.ForegroundRole):
            return None
        all_recs = self._all_records()
        filtered_idx = self._filtered[index.row()]
        if filtered_idx >= len(all_recs):
            return None
        entry = all_recs[filtered_idx]
        col = index.column()
        if role == Qt.ItemDataRole.BackgroundRole:
            return _level_background(entry["lvl_no"])
        if role == Qt.ItemDataRole.ForegroundRole:
            # Only override where a tinted background was applied.
            if _level_background(entry["lvl_no"]) is not None:
                return _LEVEL_FOREGROUND
            return None
        if role == Qt.ItemDataRole.UserRole:
            # Raw sortable values
            if col == 0:
                return entry["ts"]
            if col == 1:
                return entry["name"]
            if col == 2:
                return entry["lvl_no"]
            if col == 3:
                return entry["msg"]
            return None
        # DisplayRole
        if col == 0:
            return datetime.fromtimestamp(entry["ts"]).strftime("%Y-%m-%d %H:%M:%S")
        if col == 1:
            return entry["name"]
        if col == 2:
            return entry["lvl"]
        if col == 3:
            return entry["msg"]
        return None

    def marker_rows(self) -> List[tuple[int, int]]:
        """Return ``(row, lvl_no)`` for every visible row at WARNING or above.

        Feeds the overview scrollbar. Reads the record list directly rather
        than going through :meth:`data`, which costs far more per row and is
        paid across the whole (possibly six-figure) filtered list.
        """
        all_recs = self._all_records()
        total = len(all_recs)
        out: List[tuple[int, int]] = []
        for row, idx in enumerate(self._filtered):
            if idx < total:
                lvl_no = all_recs[idx]["lvl_no"]
                if lvl_no >= logging.WARNING:
                    out.append((row, lvl_no))
        return out

    # -- batched live append ---------------------------------------------------

    def queue_entry(self, entry: _LogEntry) -> None:
        """Queue an entry for the next batch flush."""
        self._pending.append(entry)

    def flush_pending(self) -> None:
        """Push all queued entries into the model in one batch."""
        if not self._pending:
            return

        batch = self._pending
        self._pending = []

        # Trim live records if over limit (drop oldest)
        overflow = len(self._live_records) + len(batch) - self._live_max
        if overflow > 0:
            self._live_records = self._live_records[overflow:]

        self._live_records.extend(batch)

        # While a file is under review the live entries are collected but not
        # shown: mixing them into a log from another day would produce a table
        # that reads as one continuous run but is not.  They stay in the ring
        # buffer and appear as soon as the user returns to live.
        if self._review_active:
            self._live_missed += len(batch)
            return

        self._invalidate_cache()

        # Compute new visible indices for the batch
        all_recs = self._all_records()
        base = len(all_recs) - len(batch)
        new_visible = [
            i for i in range(base, len(all_recs))
            if self._is_visible(all_recs[i], i)
        ]

        if new_visible:
            pos = len(self._filtered)
            self.beginInsertRows(QModelIndex(), pos, pos + len(new_visible) - 1)
            self._filtered.extend(new_visible)
            self.endInsertRows()

    # -- full filter rebuild ---------------------------------------------------

    def rebuild_filter(self) -> None:
        self.beginResetModel()
        all_recs = self._all_records()
        self._filtered = [i for i, e in enumerate(all_recs) if self._is_visible(e, i)]
        self.endResetModel()

    # -- scroll-back from file ------------------------------------------------

    def set_log_path(self, path: str) -> None:
        self._log_path = path
        if path and os.path.isfile(path):
            self._file_read_pos = os.path.getsize(path)
            self._file_exhausted = self._file_read_pos == 0
        else:
            self._file_read_pos = 0
            self._file_exhausted = True

    # Scroll-back reads from the session file normally and from the reviewed
    # file while one is loaded.  Both use the same backward reader, so the state
    # it needs is selected here instead of duplicating the reader.

    @property
    def _src_path(self) -> str:
        return self._review_path if self._review_active else self._log_path

    @property
    def _src_pos(self) -> int:
        return self._review_read_pos if self._review_active else self._file_read_pos

    @_src_pos.setter
    def _src_pos(self, value: int) -> None:
        if self._review_active:
            self._review_read_pos = value
        else:
            self._file_read_pos = value

    @property
    def file_exhausted(self) -> bool:
        return self._review_exhausted if self._review_active else self._file_exhausted

    @file_exhausted.setter
    def file_exhausted(self, value: bool) -> None:
        if self._review_active:
            self._review_exhausted = value
        else:
            self._file_exhausted = value

    @property
    def review_active(self) -> bool:
        return self._review_active

    @property
    def review_path(self) -> str:
        return self._review_path

    @property
    def live_missed(self) -> int:
        """Records that arrived while a file was under review."""
        return self._live_missed

    def active_log_path(self) -> str:
        """Path of the file currently backing the view."""
        return self._review_path if self._review_active else self._log_path

    def has_records(self) -> bool:
        return bool(self._file_records or self._live_records or self._pending)

    # -- review mode -----------------------------------------------------------

    def enter_review(self, path: str, count: int = 0) -> int:
        """Show *path* instead of the running session.

        The whole file is read in one forward pass. That is far cheaper than it
        sounds -- 407 ms for 120 000 entries -- and about forty times faster
        than walking the same file backwards in chunks, which needs one open and
        a seek per block.

        *count* limits how many entries are kept, counting from the end; 0 means
        the whole file. It is the caller's safety valve for very large files,
        not the normal case.

        Returns the number of entries loaded. Zero means the file held nothing
        this widget can display, and review mode is not entered.
        """
        if not path or not os.path.isfile(path):
            return 0

        records: List[_LogEntry] = []
        try:
            with open(path, "r", encoding="utf-8", errors="replace") as fh:
                for line in fh:
                    entry = _parse_log_line(line)
                    if entry is not None:
                        records.append(entry)
        except OSError as exc:
            logging.getLogger(__name__).warning(
                "Cannot read log file %s: %s", path, exc)
            return 0

        if not records:
            return 0

        if count > 0 and len(records) > count:
            records = records[-count:]

        self.beginResetModel()
        self._review_active = True
        self._review_records = records
        self._review_path = path
        # Everything is in memory, so there is nothing left to scroll back into.
        self._review_read_pos = 0
        self._review_exhausted = True
        self._live_missed = 0
        self._invalidate_cache()
        self._filtered = []
        self.endResetModel()

        return len(records)

    @staticmethod
    def count_log_entries(path: str) -> int:
        """Count the lines of *path* without parsing them.

        Used to warn before loading a very large file: reading bytes and
        counting newlines costs a fraction of building the entries.
        """
        total = 0
        try:
            with open(path, "rb") as fh:
                while True:
                    block = fh.read(1 << 20)
                    if not block:
                        break
                    total += block.count(b"\n")
        except OSError:
            return 0
        return total

    def exit_review(self) -> None:
        """Drop the reviewed file and show the running session again."""
        if not self._review_active:
            return

        self.beginResetModel()
        self._review_active = False
        self._review_records = []
        self._review_path = ""
        self._review_read_pos = 0
        self._review_exhausted = True
        self._live_missed = 0
        self._invalidate_cache()
        self.endResetModel()
        self.rebuild_filter()

    def load_older_entries(self, count: int = 500) -> int:
        """Read up to *count* older entries from the JSONL log file.

        Returns the number of entries actually loaded.
        """
        if self.file_exhausted or not self._src_path:
            return 0

        loaded: List[_LogEntry] = []
        chunk_size = 8192
        remainder = b""

        def decode(raw: bytes) -> _LogEntry | None:
            return _parse_log_line(raw.decode("utf-8", errors="replace"))

        try:
            with open(self._src_path, "rb") as fh:
                pos = self._src_pos
                while len(loaded) < count and pos > 0:
                    read_size = min(chunk_size, pos)
                    pos -= read_size
                    fh.seek(pos)
                    chunk = fh.read(read_size) + remainder
                    lines = chunk.split(b"\n")
                    remainder = lines[0]
                    # Every line of this chunk is consumed, even once *count* is
                    # reached: _src_pos below is the start of the chunk, so
                    # stopping mid-chunk would move the read position past
                    # entries that were never returned and lose them for good.
                    # Overshooting by at most one chunk costs nothing.
                    for raw_line in reversed(lines[1:]):
                        entry = decode(raw_line)
                        if entry is None:
                            continue
                        loaded.append(entry)

                if pos == 0 and remainder.strip():
                    entry = decode(remainder)
                    if entry is not None:
                        loaded.append(entry)
                    remainder = b""

                self._src_pos = pos + len(remainder)
                if self._src_pos <= 0:
                    self.file_exhausted = True
        except OSError:
            self.file_exhausted = True
            return 0

        if not loaded:
            self.file_exhausted = True
            return 0

        # loaded is newest-first; reverse to chronological order
        loaded.reverse()

        # Deduplicate against existing records
        if self._review_active:
            if self._review_records:
                oldest_ts = self._review_records[0]["ts"]
                loaded = [e for e in loaded if e["ts"] < oldest_ts]
        elif self._file_records:
            oldest_ts = self._file_records[0]["ts"]
            loaded = [e for e in loaded if e["ts"] < oldest_ts]
        elif self._live_records:
            oldest_ts = self._live_records[0]["ts"]
            loaded = [e for e in loaded if e["ts"] < oldest_ts]

        if not loaded:
            return 0

        n = len(loaded)
        self.beginInsertRows(QModelIndex(), 0, n - 1)
        if self._review_active:
            self._review_records = loaded + self._review_records
        else:
            self._file_records = loaded + self._file_records
        self._invalidate_cache()
        # Shift existing filtered indices by n
        self._filtered = [i + n for i in self._filtered]
        # Add newly visible file entries at the front
        new_indices = [
            i for i, e in enumerate(loaded)
            if self._is_visible(e, i)
        ]
        self._filtered = new_indices + self._filtered
        self.endInsertRows()
        return n


# ---------------------------------------------------------------------------
# Main widget
# ---------------------------------------------------------------------------

class LogViewerWidget(QWidget):
    """Performant log viewer with level filters, file persistence, and scroll-back."""

    BUFFER_MAX = 5_000
    _BATCH_INTERVAL_MS = 50  # flush pending entries every 50ms

    def __init__(self, parent: QWidget | None = None):
        super().__init__(parent)

        self._emitter = _LogEmitter()
        self._emitter.record_emitted.connect(self._append_record)
        self._handler = QtLogHandler(self._emitter)
        self._formatter = logging.Formatter("%(asctime)s - %(name)-11s - %(levelname)-7s - %(message)s")

        self._model = _LogTableModel(parent=self)
        self._model._live_max = self.BUFFER_MAX

        # --- batch flush timer ------------------------------------------------
        self._flush_timer = QTimer(self)
        self._flush_timer.setInterval(self._BATCH_INTERVAL_MS)
        self._flush_timer.timeout.connect(self._flush_and_scroll)

        # --- file persistence ------------------------------------------------
        self._file_handler: _JsonLinesFileHandler | None = None
        self._setup_file_handler()

        # --- filter widgets ---------------------------------------------------
        self._checkbox_error = QCheckBox("Error")
        self._checkbox_warning = QCheckBox("Warning")
        self._checkbox_info = QCheckBox("Info")
        self._checkbox_debug = QCheckBox("Debug")
        self._checkbox_new_only = QCheckBox("Unseen")

        for cb in (self._checkbox_debug, self._checkbox_info,
                   self._checkbox_warning, self._checkbox_error,
                   self._checkbox_new_only):
            cb.setSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Fixed)

        for cb in (self._checkbox_debug, self._checkbox_info,
                   self._checkbox_warning, self._checkbox_error):
            cb.setChecked(True)
            cb.toggled.connect(self._on_filter_changed)

        self._checkbox_new_only.setChecked(False)
        self._checkbox_new_only.toggled.connect(self._on_filter_changed)

        self._btn_mark_seen = QPushButton("\u2713 Mark seen")
        self._btn_mark_seen.setToolTip("Mark all current logs as seen")
        self._btn_mark_seen.clicked.connect(self._mark_seen)

        self._btn_save = QPushButton("Save Log")
        self._btn_save.clicked.connect(self._save_logs)

        self._btn_load = QPushButton("Load Log")
        self._btn_load.setToolTip("Open a saved log file for inspection")
        self._btn_load.clicked.connect(self._load_logs)

        self._search_input = QLineEdit()
        self._search_input.setPlaceholderText("Search...")
        self._search_input.setClearButtonEnabled(True)
        # Rebuilding the filter scans every record. That is 48 ms at 120 000
        # entries -- fine once, wasteful on every keystroke -- so typing is
        # collected and applied when it pauses.
        self._search_timer = QTimer(self)
        self._search_timer.setSingleShot(True)
        self._search_timer.setInterval(200)
        self._search_timer.timeout.connect(self._on_filter_changed)
        self._search_input.textChanged.connect(lambda _: self._search_timer.start())

        filters_layout = QHBoxLayout()
        filters_layout.addWidget(self._checkbox_debug)
        filters_layout.addWidget(self._checkbox_info)
        filters_layout.addWidget(self._checkbox_warning)
        filters_layout.addWidget(self._checkbox_error)
        filters_layout.addWidget(self._checkbox_new_only)
        filters_layout.addWidget(self._search_input, 1)
        filters_layout.addWidget(self._btn_mark_seen)
        filters_layout.addWidget(self._btn_load)
        filters_layout.addWidget(self._btn_save)

        # --- review banner ----------------------------------------------------
        #
        # Hidden while live.  It is the only thing telling the user that what
        # they see is a file and not the running application, so it also carries
        # the way back.
        self._review_label = QLabel()
        self._review_label.setWordWrap(False)
        self._btn_back_live = QPushButton("← Back to Live")
        self._btn_back_live.clicked.connect(self._exit_review)

        self._review_bar = QWidget()
        review_layout = QHBoxLayout(self._review_bar)
        review_layout.setContentsMargins(6, 3, 6, 3)
        review_layout.addWidget(self._review_label, 1)
        review_layout.addWidget(self._btn_back_live)
        self._review_bar.setStyleSheet(
            "QWidget { background: palette(highlight); }"
            "QLabel { color: palette(highlighted-text); font-weight: bold; }"
        )
        self._review_bar.setVisible(False)

        # Refreshes the "N new entries" counter while reviewing.
        self._review_timer = QTimer(self)
        self._review_timer.setInterval(1000)
        self._review_timer.timeout.connect(self._update_review_label)

        # --- virtual table view -----------------------------------------------
        #
        # The model is used directly: no proxy and no view-side sorting.
        #
        # Both used to be here, and sorting made every model reset re-order the
        # whole table through Python -- 220 ms at 5 000 rows, 31 s at 120 000,
        # paid again on every filter change or keystroke in the search box. It
        # bought nothing: records are appended in chronological order and column
        # 0 is the timestamp, so the sort only reconfirmed the order they were
        # already in. Filtering and searching live in the model itself, which is
        # what the proxy would otherwise have been for.
        #
        # If sorting by column is ever wanted, reorder _filtered in the model --
        # do not hand a large virtual table back to QSortFilterProxyModel.
        self._table = QTableView()
        self._table.setModel(self._model)
        self._table.setSortingEnabled(False)
        self._table.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        header = self._table.horizontalHeader()
        header.setSectionResizeMode(0, QHeaderView.ResizeMode.ResizeToContents)
        header.setSectionResizeMode(1, QHeaderView.ResizeMode.ResizeToContents)
        header.setSectionResizeMode(2, QHeaderView.ResizeMode.ResizeToContents)
        header.setSectionResizeMode(3, QHeaderView.ResizeMode.Stretch)
        header.setMinimumSectionSize(30)
        header.setStyleSheet("QHeaderView::section { padding: 2px 4px; font-size: 14px; }")
        self._table.verticalHeader().setVisible(False)
        self._table.setShowGrid(False)
        self._table.verticalHeader().setDefaultSectionSize(18)
        self._table.setSelectionBehavior(QAbstractItemView.SelectionBehavior.SelectRows)
        self._table.setEditTriggers(QAbstractItemView.EditTrigger.NoEditTriggers)
        self._table.setStyleSheet(
            "QTableView { font-family: Consolas, 'Courier New', monospace; font-size: 12px; }"
            "QTableView::item { padding: 0px 4px; border: none; }"
        )
        # The ::item rule above routes painting through QStyleSheetStyle,
        # which ignores Qt.BackgroundRole -- the delegate paints the
        # per-severity tint instead.
        self._table.setItemDelegate(_LogItemDelegate(self._table))

        # Installed before the valueChanged connection below, which would
        # otherwise bind to the scrollbar this one replaces.
        self._table.setVerticalScrollBar(_OverviewScrollBar(self._model, self._table))

        # scroll-back trigger
        self._table.verticalScrollBar().valueChanged.connect(self._on_scroll)
        self._loading_older = False

        layout = QVBoxLayout()
        layout.addLayout(filters_layout)
        layout.addWidget(self._review_bar)
        layout.addWidget(self._table)
        self.setLayout(layout)

    # --- file handler setup --------------------------------------------------

    def _setup_file_handler(self) -> None:
        log_dir = str(Path.home() / "logs")
        keep = LOG_RETENTION_SESSIONS_DEFAULT
        try:
            from duifx.gui.settings import get_settings
            settings = get_settings()
            log_dir = settings.value("log_directory", log_dir, type=str)
            keep = settings.value("log_retention_sessions", keep, type=int)
        except Exception:
            pass

        logging.debug(f"Setting up log file handler with directory: {log_dir}")

        session_name = datetime.now().strftime(
            LOG_SESSION_PREFIX + "%Y%m%d_%H%M%S" + LOG_SESSION_SUFFIX)
        log_path = os.path.join(str(log_dir), session_name)
        os.makedirs(str(log_dir), exist_ok=True)

        # Prune before opening the new file so the current session is never a
        # candidate for deletion.
        removed = prune_session_logs(str(log_dir), keep)
        if removed:
            logging.debug(f"Removed {removed} old session log(s), keeping the last {keep}")

        self._file_handler = _JsonLinesFileHandler(log_path)
        self._model.set_log_path(log_path)

    # --- public API -----------------------------------------------------------

    @property
    def handler(self) -> logging.Handler:
        """Return the handler that forwards records to this widget."""
        return self._handler

    @property
    def file_handler(self) -> logging.Handler | None:
        """Return the file handler for automatic log persistence."""
        return self._file_handler

    def attach_logger(self, logger: logging.Logger) -> None:
        """Attach this widget's handlers to a logger."""
        logger.addHandler(self._handler)
        if self._file_handler:
            logger.addHandler(self._file_handler)

    # --- record handling ------------------------------------------------------

    def _append_record(self, record: logging.LogRecord) -> None:
        entry = _record_to_entry(record)
        self._model.queue_entry(entry)
        if not self._flush_timer.isActive():
            self._flush_timer.start()

    def _flush_and_scroll(self) -> None:
        """Flush pending entries and auto-scroll if near bottom."""
        if not self._model._pending:
            self._flush_timer.stop()
            return
        # While reviewing, flush_pending only fills the ring buffer and inserts
        # no rows, so the scroll position must be left exactly where the user
        # put it.
        if self._model.review_active:
            self._model.flush_pending()
            return
        sb = self._table.verticalScrollBar()
        was_at_bottom = sb.value() >= sb.maximum() - 5
        self._model.flush_pending()
        if was_at_bottom:
            self._table.scrollToBottom()

    # --- load / review --------------------------------------------------------

    def _load_logs(self) -> None:
        """Open a saved log file in place of the running session."""
        if self._model.review_active:
            # Already reviewing: swapping one file for another loses nothing,
            # so go straight to the file dialog.
            pass
        elif self._model.has_records():
            session_path = self._model._log_path or "(no session file)"
            box = QMessageBox(self)
            box.setIcon(QMessageBox.Icon.Question)
            box.setWindowTitle("Replace displayed logs?")
            box.setText("Replace the logs currently shown with a file?")
            box.setInformativeText(
                "The current logs are not lost. Every entry of this session is "
                "already written to disk as it happens:\n\n"
                f"{session_path}\n\n"
                "New entries keep being recorded while you look at the loaded "
                "file, and \"Back to Live\" brings this session back."
            )
            box.setStandardButtons(
                QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.Cancel
            )
            box.setDefaultButton(QMessageBox.StandardButton.Cancel)
            box.button(QMessageBox.StandardButton.Yes).setText("Load File")
            if box.exec() != QMessageBox.StandardButton.Yes:
                return

        start_dir = ""
        if self._model._log_path:
            start_dir = os.path.dirname(self._model._log_path)

        path, _ = QFileDialog.getOpenFileName(
            self, "Load log file", start_dir,
            "Log Files (*.jsonl *.log);;Session Logs (*.jsonl);;"
            "Saved Logs (*.log);;All Files (*)",
        )
        if not path:
            return

        # A very large file is loaded whole only on request: it is a second of
        # parsing and a few hundred MB, and the user should decide.
        limit = 0
        total = self._model.count_log_entries(path)
        if total > LOG_LOAD_WARN_ENTRIES:
            box = QMessageBox(self)
            box.setIcon(QMessageBox.Icon.Question)
            box.setWindowTitle("Large log file")
            box.setText(f"This file holds about {total:,} entries.")
            box.setInformativeText(
                "Loading all of it takes a moment and a few hundred megabytes "
                "of memory.\n\n"
                f"You can also load just the most recent "
                f"{LOG_LOAD_WARN_ENTRIES:,} entries."
            )
            load_all = box.addButton("Load All", QMessageBox.ButtonRole.AcceptRole)
            load_tail = box.addButton("Load Recent", QMessageBox.ButtonRole.AcceptRole)
            box.addButton(QMessageBox.StandardButton.Cancel)
            box.setDefaultButton(load_tail)
            box.exec()
            clicked = box.clickedButton()
            if clicked is load_tail:
                limit = LOG_LOAD_WARN_ENTRIES
            elif clicked is not load_all:
                return

        loaded = self._model.enter_review(path, limit)
        if loaded == 0:
            QMessageBox.warning(
                self, "Nothing to show",
                f"No log entries could be read from:\n{path}\n\n"
                "Expected a session file (.jsonl) or a log saved with "
                "\"Save Log\".",
            )
            self._update_review_state()
            return

        self._sync_filter_state()
        self._model.rebuild_filter()
        self._table.scrollToBottom()
        self._update_review_state()

    def _exit_review(self) -> None:
        self._model.exit_review()
        self._sync_filter_state()
        self._model.rebuild_filter()
        self._table.scrollToBottom()
        self._update_review_state()

    def _update_review_state(self) -> None:
        active = self._model.review_active
        self._review_bar.setVisible(active)
        # Marking entries of a foreign file as "seen" would move the unseen
        # boundary of the live session, so that button rests while reviewing.
        self._btn_mark_seen.setEnabled(not active)
        self._checkbox_new_only.setEnabled(not active)
        if active:
            self._update_review_label()
            self._review_timer.start()
        else:
            self._review_timer.stop()

    def _update_review_label(self) -> None:
        if not self._model.review_active:
            return
        name = os.path.basename(self._model.review_path)
        missed = self._model.live_missed
        suffix = f" — {missed} new live entr{'y' if missed == 1 else 'ies'} waiting" if missed else ""
        self._review_label.setText(f"Reviewing file: {name}{suffix}")

    # --- filter handling ------------------------------------------------------

    def _sync_filter_state(self) -> None:
        self._model.show_debug = self._checkbox_debug.isChecked()
        self._model.show_info = self._checkbox_info.isChecked()
        self._model.show_warning = self._checkbox_warning.isChecked()
        self._model.show_error = self._checkbox_error.isChecked()
        self._model.show_unseen_only = self._checkbox_new_only.isChecked()
        self._model.search_text = self._search_input.text().strip().lower()

    def _on_filter_changed(self) -> None:
        self._sync_filter_state()
        self._model.rebuild_filter()

    def _mark_seen(self) -> None:
        if self._model.review_active:
            return
        all_count = len(self._model._file_records) + len(self._model._live_records)
        self._model.seen_count = all_count
        if self._checkbox_new_only.isChecked():
            self._model.rebuild_filter()

    # --- scroll-back ----------------------------------------------------------

    def _on_scroll(self, value: int) -> None:
        if value == 0 and not self._loading_older and not self._model.file_exhausted:
            self._loading_older = True
            sb = self._table.verticalScrollBar()
            old_max = sb.maximum()
            loaded = self._model.load_older_entries(500)
            if loaded > 0:
                new_max = sb.maximum()
                sb.setValue(sb.value() + (new_max - old_max))
            self._loading_older = False

    # --- save logs ------------------------------------------------------------

    def _format_entry(self, entry: _LogEntry) -> str:
        ts_str = datetime.fromtimestamp(entry["ts"]).strftime("%Y-%m-%d %H:%M:%S,%f")[:-3]
        return f"{ts_str} - {entry['name']:<11s} - {entry['lvl']:<7s} - {entry['msg']}"

    def _save_logs(self) -> None:
        default_name = datetime.now().strftime("logs_%Y%m%d_%H%M%S.log")
        path, _ = QFileDialog.getSaveFileName(
            self, "Save logs", default_name, "Log Files (*.log);;All Files (*)"
        )
        if not path:
            return

        # Flush pending entries to the model and disk file first
        self._model.flush_pending()
        if self._file_handler:
            self._file_handler.acquire()
            try:
                self._file_handler.flush()
            finally:
                self._file_handler.release()

        # Save what is on screen: the reviewed file while one is loaded, the
        # running session otherwise.
        reviewing = self._model.review_active
        log_path = self._model.active_log_path()
        lines: list[str] = []
        last_file_ts = 0.0

        # 1) Read all entries from the backing file (authoritative disk copy)
        if log_path and os.path.isfile(log_path):
            try:
                with open(log_path, "r", encoding="utf-8") as fh:
                    for raw_line in fh:
                        entry = _parse_log_line(raw_line)
                        if entry is None:
                            continue
                        last_file_ts = entry["ts"]
                        lines.append(self._format_entry(entry))
            except OSError:
                pass

        # 2) Append any live entries not yet flushed to the disk file.  Skipped
        #    while reviewing: those entries belong to the session, not to the
        #    file being saved.
        if not reviewing:
            for entry in self._model._live_records:
                if entry["ts"] > last_file_ts:
                    lines.append(self._format_entry(entry))

        try:
            with open(path, "w", encoding="utf-8") as f:
                f.write("\n".join(lines))
        except OSError as exc:
            logging.getLogger(__name__).error("Failed to save logs: %s", exc)


# ---------------------------------------------------------------------------
# Thread monitor -- debug widget
# ---------------------------------------------------------------------------

_THREAD_COLUMNS = ("Name", "ID", "Type", "Status", "Daemon", "Priority", "Target")


class _ThreadEntry(TypedDict):
    name: str
    ident: str
    type: str
    status: str
    daemon: str
    priority: str
    target: str


class _ThreadTableModel(QAbstractTableModel):
    """Model that snapshots all running Qt and Python threads on each refresh."""

    def __init__(self, parent: QObject | None = None):
        super().__init__(parent)
        self._entries: List[_ThreadEntry] = []
        self._filtered: List[int] = []
        self.search_text: str = ""

    # -- data collection -------------------------------------------------------

    def refresh(self) -> None:
        """Re-scan all threads and update the table."""
        entries: List[_ThreadEntry] = []
        seen_idents: set[int] = set()

        # --- Qt threads via QApplication.findChildren(QThread) ----------------
        app = QApplication.instance()
        if app is not None:
            for qt_thread in app.findChildren(QThread):
                tid = int(qt_thread.currentThreadId()) if qt_thread == QThread.currentThread() else 0
                priority_map = {
                    QThread.Priority.IdlePriority: "Idle",
                    QThread.Priority.LowestPriority: "Lowest",
                    QThread.Priority.LowPriority: "Low",
                    QThread.Priority.NormalPriority: "Normal",
                    QThread.Priority.HighPriority: "High",
                    QThread.Priority.HighestPriority: "Highest",
                    QThread.Priority.TimeCriticalPriority: "TimeCritical",
                    QThread.Priority.InheritPriority: "Inherit",
                }
                prio = priority_map.get(qt_thread.priority(), "Unknown")
                status = "Running" if qt_thread.isRunning() else "Finished"
                entries.append(_ThreadEntry(
                    name=qt_thread.objectName() or f"QThread-{id(qt_thread):#x}",
                    ident=str(tid) if tid else "-",
                    type="Qt",
                    status=status,
                    daemon="-",
                    priority=prio,
                    target=qt_thread.metaObject().className(),
                ))
                if tid:
                    seen_idents.add(tid)

        # --- Python threading threads -----------------------------------------
        for t in threading.enumerate():
            tid = t.native_id if hasattr(t, "native_id") and t.native_id else t.ident
            if tid and tid in seen_idents:
                continue
            status = "Running" if t.is_alive() else "Finished"
            target_name = ""
            if hasattr(t, "_target") and t._target is not None:
                target_name = getattr(t._target, "__qualname__", str(t._target))
            entries.append(_ThreadEntry(
                name=t.name,
                ident=str(tid) if tid else "-",
                type="Python",
                status=status,
                daemon="Yes" if t.daemon else "No",
                priority="-",
                target=target_name or "-",
            ))

        self._entries = entries
        self._rebuild_filter()

    # -- filter ----------------------------------------------------------------

    def _rebuild_filter(self) -> None:
        self.beginResetModel()
        if not self.search_text:
            self._filtered = list(range(len(self._entries)))
        else:
            needle = self.search_text
            self._filtered = [
                i for i, e in enumerate(self._entries)
                if (needle in e["name"].lower()
                    or needle in e["type"].lower()
                    or needle in e["status"].lower()
                    or needle in e["target"].lower())
            ]
        self.endResetModel()

    # -- QAbstractTableModel interface -----------------------------------------

    def rowCount(self, parent: QModelIndex = QModelIndex()) -> int:
        return len(self._filtered)

    def columnCount(self, parent: QModelIndex = QModelIndex()) -> int:
        return len(_THREAD_COLUMNS)

    def headerData(self, section: int, orientation: Qt.Orientation, role: int = Qt.ItemDataRole.DisplayRole):
        if role == Qt.ItemDataRole.DisplayRole and orientation == Qt.Orientation.Horizontal:
            return _THREAD_COLUMNS[section]
        return None

    def data(self, index: QModelIndex, role: int = Qt.ItemDataRole.DisplayRole):
        if not index.isValid() or role != Qt.ItemDataRole.DisplayRole:
            return None
        entry = self._entries[self._filtered[index.row()]]
        col = index.column()
        if col == 0:
            return entry["name"]
        if col == 1:
            return entry["ident"]
        if col == 2:
            return entry["type"]
        if col == 3:
            return entry["status"]
        if col == 4:
            return entry["daemon"]
        if col == 5:
            return entry["priority"]
        if col == 6:
            return entry["target"]
        return None


class ThreadMonitorWidget(QWidget):
    """Debug widget that lists all running Qt and Python threads in a table."""

    _REFRESH_INTERVAL_MS = 1000

    def __init__(self, parent: QWidget | None = None):
        super().__init__(parent)

        self._model = _ThreadTableModel(parent=self)

        # --- refresh timer ----------------------------------------------------
        self._refresh_timer = QTimer(self)
        self._refresh_timer.setInterval(self._REFRESH_INTERVAL_MS)
        self._refresh_timer.timeout.connect(self._refresh)

        # --- toolbar ----------------------------------------------------------
        self._btn_refresh = QPushButton("Refresh")
        self._btn_refresh.setToolTip("Refresh thread list now")
        self._btn_refresh.clicked.connect(self._refresh)

        self._cb_auto = QCheckBox("Auto")
        self._cb_auto.setToolTip("Auto-refresh every second")
        self._cb_auto.setChecked(True)
        self._cb_auto.toggled.connect(self._on_auto_toggled)

        self._search_input = QLineEdit()
        self._search_input.setPlaceholderText("Filter threads...")
        self._search_input.setClearButtonEnabled(True)
        self._search_input.textChanged.connect(self._on_search_changed)

        toolbar = QHBoxLayout()
        toolbar.addWidget(self._btn_refresh)
        toolbar.addWidget(self._cb_auto)
        toolbar.addWidget(self._search_input, 1)

        # --- table view -------------------------------------------------------
        self._table = QTableView()
        self._table.setModel(self._model)
        self._table.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        header = self._table.horizontalHeader()
        header.setSectionResizeMode(0, QHeaderView.ResizeMode.Stretch)       # Name
        header.setSectionResizeMode(1, QHeaderView.ResizeMode.ResizeToContents)  # ID
        header.setSectionResizeMode(2, QHeaderView.ResizeMode.ResizeToContents)  # Type
        header.setSectionResizeMode(3, QHeaderView.ResizeMode.ResizeToContents)  # Status
        header.setSectionResizeMode(4, QHeaderView.ResizeMode.ResizeToContents)  # Daemon
        header.setSectionResizeMode(5, QHeaderView.ResizeMode.ResizeToContents)  # Priority
        header.setSectionResizeMode(6, QHeaderView.ResizeMode.Stretch)       # Target
        header.setMinimumSectionSize(40)
        self._table.verticalHeader().setVisible(False)
        self._table.setShowGrid(False)
        self._table.verticalHeader().setDefaultSectionSize(22)
        self._table.setSelectionBehavior(QAbstractItemView.SelectionBehavior.SelectRows)
        self._table.setEditTriggers(QAbstractItemView.EditTrigger.NoEditTriggers)
        self._table.setStyleSheet(
            "QTableView { font-family: Consolas, 'Courier New', monospace; font-size: 12px; }"
            "QTableView::item { padding: 0px 4px; border: none; }"
        )

        # --- layout -----------------------------------------------------------
        layout = QVBoxLayout()
        layout.addLayout(toolbar)
        layout.addWidget(self._table)
        self.setLayout(layout)

        # initial data + start timer
        self._refresh()
        self._refresh_timer.start()

    # -- slots -----------------------------------------------------------------

    def _refresh(self) -> None:
        self._model.refresh()

    def _on_auto_toggled(self, checked: bool) -> None:
        if checked:
            self._refresh_timer.start()
        else:
            self._refresh_timer.stop()

    def _on_search_changed(self, text: str) -> None:
        self._model.search_text = text.strip().lower()
        self._model._rebuild_filter()

__all__ = ["LogViewerWidget", "QtLogHandler", "ThreadMonitorWidget"]

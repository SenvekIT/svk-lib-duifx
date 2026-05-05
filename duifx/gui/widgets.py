"""Log viewer widgets for DUIFx.

Provides a performant log viewer with level filters, automatic file persistence
(JSON Lines with per-session timestamped files), an in-memory ring buffer
with batched UI updates and scroll-back loading of older entries from disk.
"""

from __future__ import annotations

import json
import logging
import os
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
    QSortFilterProxyModel,
    QTimer,
    QThread,
)
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
    QLineEdit,
    QAbstractItemView,
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

    # -- cached record list ----------------------------------------------------

    def _all_records(self) -> List[_LogEntry]:
        if self._cache is None:
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
        if role not in (Qt.ItemDataRole.DisplayRole, Qt.ItemDataRole.UserRole):
            return None
        all_recs = self._all_records()
        filtered_idx = self._filtered[index.row()]
        if filtered_idx >= len(all_recs):
            return None
        entry = all_recs[filtered_idx]
        col = index.column()
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

    @property
    def file_exhausted(self) -> bool:
        return self._file_exhausted

    def load_older_entries(self, count: int = 500) -> int:
        """Read up to *count* older entries from the JSONL log file.

        Returns the number of entries actually loaded.
        """
        if self._file_exhausted or not self._log_path:
            return 0

        loaded: List[_LogEntry] = []
        chunk_size = 8192
        remainder = b""

        try:
            with open(self._log_path, "rb") as fh:
                pos = self._file_read_pos
                while len(loaded) < count and pos > 0:
                    read_size = min(chunk_size, pos)
                    pos -= read_size
                    fh.seek(pos)
                    chunk = fh.read(read_size) + remainder
                    lines = chunk.split(b"\n")
                    remainder = lines[0]
                    for raw_line in reversed(lines[1:]):
                        raw_line = raw_line.strip()
                        if not raw_line:
                            continue
                        try:
                            entry = json.loads(raw_line)
                            loaded.append(entry)
                            if len(loaded) >= count:
                                break
                        except (json.JSONDecodeError, KeyError):
                            continue

                if pos == 0 and remainder.strip():
                    try:
                        loaded.append(json.loads(remainder))
                    except (json.JSONDecodeError, KeyError):
                        pass
                    remainder = b""

                self._file_read_pos = pos + len(remainder)
                if self._file_read_pos <= 0:
                    self._file_exhausted = True
        except OSError:
            self._file_exhausted = True
            return 0

        if not loaded:
            self._file_exhausted = True
            return 0

        # loaded is newest-first; reverse to chronological order
        loaded.reverse()

        # Deduplicate against existing records
        if self._file_records:
            oldest_ts = self._file_records[0]["ts"]
            loaded = [e for e in loaded if e["ts"] < oldest_ts]
        elif self._live_records:
            oldest_ts = self._live_records[0]["ts"]
            loaded = [e for e in loaded if e["ts"] < oldest_ts]

        if not loaded:
            return 0

        n = len(loaded)
        self.beginInsertRows(QModelIndex(), 0, n - 1)
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

        self._search_input = QLineEdit()
        self._search_input.setPlaceholderText("Search...")
        self._search_input.setClearButtonEnabled(True)
        self._search_input.textChanged.connect(self._on_filter_changed)

        filters_layout = QHBoxLayout()
        filters_layout.addWidget(self._checkbox_debug)
        filters_layout.addWidget(self._checkbox_info)
        filters_layout.addWidget(self._checkbox_warning)
        filters_layout.addWidget(self._checkbox_error)
        filters_layout.addWidget(self._checkbox_new_only)
        filters_layout.addWidget(self._search_input, 1)
        filters_layout.addWidget(self._btn_mark_seen)
        filters_layout.addWidget(self._btn_save)

        # --- virtual table view -----------------------------------------------
        self._proxy = QSortFilterProxyModel(self)
        self._proxy.setSourceModel(self._model)
        self._proxy.setSortRole(Qt.ItemDataRole.UserRole)

        self._table = QTableView()
        self._table.setModel(self._proxy)
        self._table.setSortingEnabled(True)
        self._table.sortByColumn(0, Qt.SortOrder.AscendingOrder)
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

        # scroll-back trigger
        self._table.verticalScrollBar().valueChanged.connect(self._on_scroll)
        self._loading_older = False

        layout = QVBoxLayout()
        layout.addLayout(filters_layout)
        layout.addWidget(self._table)
        self.setLayout(layout)

    # --- file handler setup --------------------------------------------------

    def _setup_file_handler(self) -> None:
        try:
            from duifx.gui.settings import get_settings
            settings = get_settings()
            log_dir = settings.value("log_directory", str(Path.home() / "logs"), type=str)
        except Exception:
            log_dir = str(Path.home() / "logs")

        logging.debug(f"Setting up log file handler with directory: {log_dir}")

        session_name = datetime.now().strftime("app_%Y%m%d_%H%M%S.jsonl")
        log_path = os.path.join(str(log_dir), session_name)
        os.makedirs(str(log_dir), exist_ok=True)

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
        sb = self._table.verticalScrollBar()
        was_at_bottom = sb.value() >= sb.maximum() - 5
        self._model.flush_pending()
        if was_at_bottom:
            self._table.scrollToBottom()

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

        log_path = self._model._log_path
        lines: list[str] = []
        last_file_ts = 0.0

        # 1) Read all entries from the JSONL session file (authoritative disk copy)
        if log_path and os.path.isfile(log_path):
            try:
                with open(log_path, "r", encoding="utf-8") as fh:
                    for raw_line in fh:
                        raw_line = raw_line.strip()
                        if not raw_line:
                            continue
                        try:
                            entry: _LogEntry = json.loads(raw_line)
                            last_file_ts = entry["ts"]
                            lines.append(self._format_entry(entry))
                        except (json.JSONDecodeError, KeyError):
                            continue
            except OSError:
                pass

        # 2) Append any live entries not yet flushed to the disk file
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

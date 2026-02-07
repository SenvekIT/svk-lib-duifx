"""Log viewer widgets for DUIFx.

Provides a simple log viewer with level filters (debug, info, warning, error).
"""

from __future__ import annotations

import logging
from datetime import datetime
from typing import List

from PySide6.QtCore import Qt, QObject, Signal
from PySide6.QtWidgets import (
    QWidget,
    QVBoxLayout,
    QHBoxLayout,
    QCheckBox,
    QTableWidget,
    QTableWidgetItem,
    QHeaderView,
    QSizePolicy,
    QPushButton,
    QFileDialog,
)


class _LogEmitter(QObject):
    record_emitted = Signal(object)


class QtLogHandler(logging.Handler):
    """Logging handler that forwards records into the Qt event loop."""

    def __init__(self, emitter: _LogEmitter):
        super().__init__()
        self._emitter = emitter

    def emit(self, record: logging.LogRecord) -> None:
        self._emitter.record_emitted.emit(record)


class LogViewerWidget(QWidget):
    """Minimal log viewer with level filters."""

    def __init__(self, parent: QWidget | None = None):
        super().__init__(parent)

        self._emitter = _LogEmitter()
        self._emitter.record_emitted.connect(self._append_record)
        self._handler = QtLogHandler(self._emitter)
        self._formatter = logging.Formatter("%(asctime)s - %(name)-11s - %(levelname)-7s - %(message)s")

        self._records: List[logging.LogRecord] = []

        self._checkbox_error = QCheckBox("Error")
        self._checkbox_warning = QCheckBox("Warning")
        self._checkbox_info = QCheckBox("Info")
        self._checkbox_debug = QCheckBox("Debug")

        for checkbox in (self._checkbox_debug, self._checkbox_info, self._checkbox_warning, self._checkbox_error):
            checkbox.setChecked(True)
            checkbox.toggled.connect(self._rebuild_table)

        self._btn_save = QPushButton("Save Log")
        self._btn_save.clicked.connect(self._save_logs)

        filters_layout = QHBoxLayout()
        filters_layout.addWidget(self._checkbox_debug)
        filters_layout.addWidget(self._checkbox_info)
        filters_layout.addWidget(self._checkbox_warning)
        filters_layout.addWidget(self._checkbox_error)
        filters_layout.addStretch()
        filters_layout.addWidget(self._btn_save)

        self._table = QTableWidget(0, 4)
        self._table.setHorizontalHeaderLabels(["Time", "Module", "Level", "Message"])
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
        self._table.setSelectionBehavior(QTableWidget.SelectionBehavior.SelectRows)
        self._table.setEditTriggers(QTableWidget.EditTrigger.NoEditTriggers)
        self._table.setStyleSheet(
            "QTableWidget { font-family: Consolas, 'Courier New', monospace; font-size: 12px; }"
            "QTableWidget::item { padding: 0px 4px; border: none; }"
        )

        layout = QVBoxLayout()
        layout.addLayout(filters_layout)
        layout.addWidget(self._table)
        self.setLayout(layout)

    @property
    def handler(self) -> logging.Handler:
        """Return the handler that forwards records to this widget."""
        return self._handler

    def attach_logger(self, logger: logging.Logger) -> None:
        """Attach this widget's handler to a logger."""
        logger.addHandler(self._handler)

    def _append_record(self, record: logging.LogRecord) -> None:
        self._records.append(record)
        if self._is_visible(record):
            self._add_row(record)

    def _is_visible(self, record: logging.LogRecord) -> bool:
        if record.levelno >= logging.ERROR:
            return self._checkbox_error.isChecked()
        if record.levelno >= logging.WARNING:
            return self._checkbox_warning.isChecked()
        if record.levelno >= logging.INFO:
            return self._checkbox_info.isChecked()
        return self._checkbox_debug.isChecked()

    def _add_row(self, record: logging.LogRecord) -> None:
        row = self._table.rowCount()
        self._table.insertRow(row)

        timestamp = datetime.fromtimestamp(record.created).strftime("%Y-%m-%d %H:%M:%S")
        level_item = QTableWidgetItem(record.levelname)
        logger_item = QTableWidgetItem(record.name)
        msg_item = QTableWidgetItem(record.getMessage())

        level_item.setData(Qt.ItemDataRole.UserRole, record.levelno)

        self._table.setItem(row, 0, QTableWidgetItem(timestamp))
        self._table.setItem(row, 1, logger_item)
        self._table.setItem(row, 2, level_item)
        self._table.setItem(row, 3, msg_item)
        self._table.scrollToBottom()

    def _rebuild_table(self) -> None:
        self._table.setRowCount(0)
        for record in self._records:
            if self._is_visible(record):
                self._add_row(record)

    def _save_logs(self) -> None:
        path, _ = QFileDialog.getSaveFileName(self, "Save logs", "logs.log", "Log Files (*.log);;All Files (*)")
        if not path:
            return

        lines = [self._formatter.format(record) for record in self._records]
        try:
            with open(path, "w", encoding="utf-8") as f:
                f.write("\n".join(lines))
        except OSError:
            pass


__all__ = ["LogViewerWidget", "QtLogHandler"]

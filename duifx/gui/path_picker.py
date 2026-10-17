"""A path in a line edit with a *Browse...* button beside it.

The pair every settings page and export form builds by hand: the path can be
typed or pasted, or chosen in the platform's file dialog, which opens where
the current path is (or the nearest folder of it that exists, or a start
folder). :class:`PathPicker` emits ``path_changed`` once per real change -
after a choice in the dialog, when editing ends with another text than
before, or from ``set_path`` - not on every keystroke.

Typical use::

    picker = PathPicker(PathPicker.DIRECTORY, caption="Recordings folder")
    picker.set_path(settings.value("recordings_dir", default))
    picker.path_changed.connect(lambda p: settings.setValue("recordings_dir", p))

    PathPicker(PathPicker.OPEN_FILE, caption="Firmware", file_filter="Firmware (*.bin)")
    PathPicker(PathPicker.SAVE_FILE, caption="Export to", file_filter="CSV (*.csv)")
"""

from __future__ import annotations

import os
from pathlib import Path
from typing import Optional

from PySide6.QtCore import Signal
from PySide6.QtWidgets import QFileDialog, QHBoxLayout, QLineEdit, QPushButton, QWidget

__all__ = ["PathPicker"]


class PathPicker(QWidget):
    """Line edit + *Browse...* for a directory, a file to open or a file to save."""

    DIRECTORY = "directory"
    OPEN_FILE = "open_file"
    SAVE_FILE = "save_file"
    MODES = (DIRECTORY, OPEN_FILE, SAVE_FILE)

    #: The path changed (by the dialog, by editing, or by set_path).
    path_changed = Signal(str)

    def __init__(self, mode: str = DIRECTORY, parent: Optional[QWidget] = None, *,
                 caption: str = "", file_filter: str = "", placeholder: str = "",
                 button_text: str = "Browse…", start_dir: str = "") -> None:
        super().__init__(parent)
        if mode not in self.MODES:
            raise ValueError(f"PathPicker mode must be one of {self.MODES}, not {mode!r}")
        self._mode = mode
        self._caption = caption
        self._filter = file_filter
        self._start_dir = start_dir
        self._last = ""

        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(6)

        self._edit = QLineEdit()
        self._edit.setPlaceholderText(placeholder)
        self._edit.editingFinished.connect(self._on_editing_finished)
        layout.addWidget(self._edit, 1)

        self._button = QPushButton(button_text)
        self._button.clicked.connect(self.browse)
        layout.addWidget(self._button)

    # ------------------------------------------------------------------ #

    @property
    def mode(self) -> str:
        return self._mode

    @property
    def line_edit(self) -> QLineEdit:
        return self._edit

    @property
    def button(self) -> QPushButton:
        return self._button

    def path(self) -> str:
        """The path as shown, whitespace stripped."""
        return self._edit.text().strip()

    def set_path(self, path: str) -> None:
        """Show *path*; emits ``path_changed`` when it differs from before."""
        path = str(path or "")
        self._edit.setText(path)
        self._commit(path.strip())

    def set_start_dir(self, folder: str) -> None:
        """Where the dialog opens while the path is empty or nowhere on disk."""
        self._start_dir = str(folder or "")

    def set_placeholder(self, text: str) -> None:
        self._edit.setPlaceholderText(text)

    def browse(self) -> None:
        """Open the dialog; a choice becomes the path."""
        start = self.dialog_start()
        if self._mode == self.DIRECTORY:
            chosen = QFileDialog.getExistingDirectory(self, self._caption or "Choose folder", start)
        elif self._mode == self.OPEN_FILE:
            chosen, _ = QFileDialog.getOpenFileName(self, self._caption or "Open file", start, self._filter)
        else:
            chosen, _ = QFileDialog.getSaveFileName(self, self._caption or "Save as", start, self._filter)
        if chosen:
            self.set_path(os.path.normpath(chosen))

    def dialog_start(self) -> str:
        """The current path where it exists (for a file: the file itself, or
        its folder), else its nearest existing folder, else the start folder,
        else the home folder."""
        path = self.path()
        if path:
            p = Path(path)
            if p.exists() and (self._mode != self.DIRECTORY or p.is_dir()):
                return str(p)
            for parent in p.parents:
                if str(parent) not in ("", ".") and parent.is_dir():
                    if self._mode == self.SAVE_FILE:
                        return str(parent / p.name)
                    return str(parent)
        if self._start_dir and os.path.isdir(self._start_dir):
            return self._start_dir
        return str(Path.home())

    # ------------------------------------------------------------------ #

    def _on_editing_finished(self) -> None:
        self._commit(self.path())

    def _commit(self, path: str) -> None:
        if path != self._last:
            self._last = path
            self.path_changed.emit(path)

# DUIFx

**DUIFx** is a PySide6-based user interface package providing windows, dialogs
and widgets with predefined styles.

## Install

Directly from GitHub:

```bash
pip install git+https://github.com/SenvekIT/svk-lib-duifx.git
```

A specific tag or branch:

```bash
pip install git+https://github.com/SenvekIT/svk-lib-duifx.git@v1.1.0
```

From a `requirements.txt`:

```
duifx @ git+https://github.com/SenvekIT/svk-lib-duifx.git
```

When the package is used as a git submodule, install it in editable mode from its path instead:

```
-e packages/duifx
```

## Usage

```python
import sys
from PySide6.QtWidgets import QApplication
from duifx import MainWindow, MenuButton, apply_theme

app = QApplication(sys.argv)
apply_theme(app)                      # light / system, see Theming

window = MainWindow("My Application")
window.add_menu_button(
    MenuButton(icon_path=":icons/icon/Settings.svg", label="Settings"),
    slot=lambda: print("clicked"),
)
window.show()
sys.exit(app.exec())
```

See [ICONS.md](ICONS.md) for the icon reference.

### Navigation

The top entries of the left sidebar switch pages; the active one gets a light
background (`Token.SURFACE_SELECTED`). Header menu buttons and the sidebar
bottom buttons are actions (dialogs) and never become active.

```python
window.add_content_right_sidebar(nodes_page)
window.add_content_right_sidebar(channels_page)

# Linked to its page: a click shows it, and the entry is active whenever the
# page is shown, also by set_right_sidebar_widget() / set_content_widget().
window.add_sidebar_top_button(MenuButton(":icons/icon/home.svg", tooltip="Home"),
                              page=nodes_page)
window.add_sidebar_top_button(MenuButton(":icons/icon/Channels.svg", tooltip="Channels"),
                              page=channels_page)

# An action in the top sidebar that opens a dialog:
window.add_sidebar_top_button(MenuButton(":icons/icon/search.svg"), slot=open_dialog,
                              navigation=False)
```

Without `page=` (only a `slot`) the click alone moves the highlight and the
first entry added starts active; an application that then switches pages in
code calls `window.set_active_sidebar_button(button)`.

### Theming

The colour scheme is pinned by the application instead of following the
operating system:

```python
from duifx import Theme, apply_theme, set_theme

apply_theme(app)                      # applies the stored theme at startup
set_theme(Theme.SYSTEM)               # persist; takes effect on next start
```

`Theme.LIGHT` and `Theme.SYSTEM` are available; `Theme.DARK` is reserved and
currently falls back to light. In a `SettingsDialog`, add the selector with
`self.add_theme_setting(category)`.

### Terminal

A console that looks and lines up like PuTTY (Courier New 10 pt, light grey on
black, tab stops every 8 columns), whatever the application stylesheet says:

```python
from duifx import TerminalLineEdit, TerminalView, apply_fixed_pitch

view = TerminalView()                 # output; append_text() as text arrives
entry = TerminalLineEdit()            # input in the same font and colours
apply_fixed_pitch(label)              # just the fixed-pitch font, own colours
```

Do not use `setFont()` for a monospace text box: the stylesheet's `*` rule
overrides it.

## Development

```bash
git clone https://github.com/SenvekIT/svk-lib-duifx.git
cd svk-lib-duifx
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements-dev.txt
pip install -e .
```

### Compile the Qt resource file

Required after adding, removing or renaming icons or stylesheets:

```bash
pyside6-rcc -o duifx/gui/ui/resource_rc.py duifx/gui/resources/resource.qrc
```

From the E-MEC application repository the same command reads:

```bash
pyside6-rcc -o packages/duifx/duifx/gui/ui/resource_rc.py packages/duifx/duifx/gui/resources/resource.qrc
```

### Build the package

```bash
python -m build
```

The wheel and source distribution are written to `dist/`.

## License

Released under the MIT License, see [LICENSE](LICENSE).

This package depends on [Qt for Python (PySide6)](https://doc.qt.io/qtforpython-6/),
which is licensed separately under the LGPLv3.

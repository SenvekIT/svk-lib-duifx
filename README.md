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

### Path picker

A path in a line edit with a *Browse…* button, for a folder, a file to open or
a file to save. `path_changed` comes once per real change (dialog choice,
editing finished with another text, `set_path()`), not per keystroke:

```python
from duifx import PathPicker

picker = PathPicker(PathPicker.DIRECTORY, caption="Recordings folder")
picker.set_path(folder)
picker.path_changed.connect(on_folder)
PathPicker(PathPicker.OPEN_FILE, caption="Firmware", file_filter="Firmware (*.bin)")
```

The dialog opens at the current path, or its nearest existing folder, or
`set_start_dir()`, or the home folder.

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

## C++ (Qt Widgets)

`cpp/` holds DUIFx for C++/Qt 6 with the same widgets and behaviour as the
Python package. Both share the stylesheet, the icons (`:/icons/icon/<name>.svg`)
and the Qt Designer forms in `duifx/gui`, and they read and write the same
settings file and the same session log files, so a Python and a C++
application of one organisation see each other's settings and logs.

Requirements: CMake 3.21, a C++20 compiler (MSVC 2022, GCC 11+), Qt 6.5 or
newer with Qt SVG (Qt 6.8+ to pin the colour scheme). No other dependencies;
the plots are drawn with QPainter (Qt Charts / Qt Graphs are GPL-only and are
not used).

Use it from an application as a subdirectory, e.g. this repository as a git
submodule:

```cmake
add_subdirectory(external/svk-lib-duifx/cpp duifx)
target_link_libraries(myapp PRIVATE duifx::duifx)
```

```cpp
#include <duifx/Duifx.h>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    duifx::applyTheme(&app);                        // apply_theme(app)

    duifx::MainWindow window(QStringLiteral("My Application"));
    window.addMenuButton(new duifx::MenuButton(QStringLiteral(":/icons/icon/settings.svg"),
                                               QStringLiteral("Settings")),
                         [] { qDebug("clicked"); });
    return app.exec();
}
```

The API follows the Python one with Qt naming: `add_sidebar_top_button` is
`addSidebarTopButton`, `setting_changed` is the signal `settingChanged`,
keyword arguments become `Options` structs (`TimeSeriesPlot::Options`,
`PathPicker::Options`, ...). Each header in `cpp/include/duifx` names the
Python module it ports and lists where C++ differs. The Python `logging`
handlers are replaced by Qt's logging: `LogViewerWidget::attachLogger()`
captures `qDebug()` ... `qCritical()` per logging category.

Build, test and run the demo on Windows:

```powershell
.\cpp\scripts\build.ps1                 # Debug build + tests
.\cpp\build\windows-msvc-debug\bin\duifx_demo.exe
```

On Linux: `cmake --preset linux-gcc-debug` in `cpp/`, then
`cmake --build --preset linux-gcc-debug` and
`ctest --test-dir build/linux-gcc-debug` (with `QT_QPA_PLATFORM=offscreen`
without a display).

When a module changes in Python, the C++ counterpart changes in the same pull
request, so both stay alike.

## License

Released under the MIT License, see [LICENSE](LICENSE).

This package depends on [Qt for Python (PySide6)](https://doc.qt.io/qtforpython-6/),
which is licensed separately under the LGPLv3.

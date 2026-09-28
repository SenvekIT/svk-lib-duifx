# Changelog

All notable changes to DUIFx are documented here.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Changed

- Replaced the bundled icon set with
  [Flat Color Icons](https://github.com/icons8/flat-color-icons) (MIT). The
  previous set could not be redistributed. Third-party brand and product logos
  were left out: the artwork may be licensed while the trademark it depicts is
  not.
  **Breaking:** icon resource paths changed, e.g. `:icons/icon/Settings.svg`
  is now `:icons/icon/settings.svg`. See [ICONS.md](ICONS.md) for the full list.
- Scroll areas and their content are transparent, so a scrolled panel takes the
  background of the container it sits in instead of the palette. Previously such
  panels showed a grey — or, with a dark desktop palette, dark — background
  inside an otherwise light window.
- `MainWindow.add_header_logo()` now uses `DxLogo` and detects the file format
  itself, so an application only says which file and how big. It also returns
  whether the logo could be loaded, and inserts a right margin (20 px by
  default, `right_margin=0` restores the old flush placement).
  Previously the two branches behaved differently and neither was good: `svg`
  ignored the requested size and drew into a fixed 128x32 box with
  `IgnoreAspectRatio`, squashing anything that was not 4:1, and `png` went
  through `QLabel.setPixmap()`, which normalises the pixmap to its
  device-independent size — a render at 3x came out identical to one at 1x, so
  the logo was soft on a 150 % or 200 % display.
  The `format` argument is still accepted and now ignored; existing calls keep
  working.

### Added

- `DxLogo`, a widget that shows a logo from any format Qt can read — SVG, PNG,
  JPEG — scaled into a bounding box with its aspect ratio kept, and painted at
  the display's own resolution so it stays sharp on a scaled screen and when
  the window moves to a differently scaled monitor.
- `THIRD-PARTY-LICENSES.md` documenting the bundled icons and the PySide6
  (LGPLv3) dependency.
- Project documentation: `CONTRIBUTING.md`, `SECURITY.md`,
  `CODE_OF_CONDUCT.md` and this changelog.

### Fixed

- Package metadata: declared the MIT licence as an SPDX expression (PEP 639),
  corrected `requires-python` to `>=3.10` (the code uses `X | None`
  annotations at runtime and never worked on 3.8/3.9), dropped the unused
  `pyusb` dependency, and pointed the project URLs at this repository.

### Removed

- `designer.lnk`, a Windows shortcut containing a local development path.

## [1.3.0]

### Added

- Theme management: `apply_theme()`, `get_theme()`, `set_theme()` and the
  `Theme` enum pin the application colour scheme instead of following the
  operating system, which previously left unstyled widgets dark on a dark
  desktop while styled ones stayed light. `SettingsDialog.add_theme_setting()`
  adds the selector. `Theme.DARK` is reserved and currently falls back to light.
- Log viewer: rows at WARNING and above are tinted by severity, and the vertical
  scrollbar shows their position as an overview ruler.

## [1.2.0]

### Added

- Log viewer: session log pruning and a confirmation prompt before loading
  large log files.

## [1.1.0]

### Added

- Log viewer: review mode for inspecting saved log files, and session log
  retention.

## [1.0.0]

First stable release.

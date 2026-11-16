#pragma once

// Shared helpers. Port of duifx/gui/utils.py.

#include <QSize>
#include <QString>

class QLabel;

namespace duifx {

/// Registers the DUIFx resources (icons ":/icons/icon/<name>.svg" and the
/// stylesheet). DUIFx is a static library, so the linker would drop the
/// resource initialisers; every DUIFx widget and applyTheme() call this.
/// An application that loads DUIFx icons before using either calls it itself.
void initResources();

/// The application's folder: the counterpart of get_base_path(), which in
/// Python is the package or the PyInstaller bundle.
QString basePath();

/// The DUIFx stylesheet (modern_light.qss); empty if it cannot be read.
QString loadStylesheet();

/// Renders an SVG onto label as a pixmap; size defaults to the SVG's own size.
void setSvgOnLabel(QLabel *label, const QString &svgPath, QSize size = {});

/// The library version, the one of duifx/version.py.
QString version();

} // namespace duifx

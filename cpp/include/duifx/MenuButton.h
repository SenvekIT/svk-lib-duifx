#pragma once

// Tool button of the header and the sidebars. Port of MenuButton in
// duifx/gui/windows.py.

#include <QSize>
#include <QToolButton>

namespace duifx {

class MenuButton : public QToolButton
{
    Q_OBJECT

public:
    /// With a label: text under a 32 px icon (header style), at least 60x50.
    /// Without: a 24 px icon (sidebar style). iconSize overrides the icon size.
    explicit MenuButton(const QString &iconPath = {}, const QString &label = {}, const QString &tooltip = {},
                        QSize iconSize = {}, QWidget *parent = nullptr);
};

} // namespace duifx

#pragma once

// Logo display that stays sharp, whatever the source format is. Port of
// duifx/gui/logo.py.
//
// A widget rather than a QLabel with a pixmap: QLabel::setPixmap() normalises
// the pixmap to its device-independent size, so on a 150 % or 200 % display
// the logo turns soft. Painting in paintEvent() happens at the device
// resolution: an SVG is re-rendered from its vector source, a raster image is
// scaled from its native pixels; both keep their aspect ratio.

#include <QPixmap>
#include <QSize>
#include <QWidget>

#include <memory>

class QSvgRenderer;

namespace duifx {

/// Used when the caller gives no size. Fits the standard 80 px header.
inline constexpr int kDefaultLogoHeight = 40;

/// A logo scaled into a bounding box, rendered at full device resolution.
/// SVG is preferred, any format Qt reads works. isValid() reports whether the
/// file could be read; an unreadable logo paints nothing.
class DxLogo : public QWidget
{
    Q_OBJECT

public:
    /// size is the bounding box; the logo keeps its aspect ratio, so the height
    /// you have room for and a generous width are enough. Invalid size: scaled
    /// to kDefaultLogoHeight.
    explicit DxLogo(const QString &logoPath, QSize size = {}, QWidget *parent = nullptr);
    ~DxLogo() override;

    /// Loads logoPath and scales it into size. False if it is unreadable.
    bool setLogo(const QString &logoPath, QSize size = {});

    bool isValid() const;

    /// Size of the source, in its own units (SVG) or pixels (raster).
    QSize nativeSize() const { return m_native; }

    QSize sizeHint() const override { return size(); }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QSize fittedSize(QSize size) const;

    std::unique_ptr<QSvgRenderer> m_renderer;
    QPixmap m_pixmap;
    QSize m_native;
};

} // namespace duifx

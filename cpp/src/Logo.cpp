#include "duifx/Logo.h"

#include "duifx/Utils.h"
#include "Logging.h"

#include <algorithm>
#include <QFile>
#include <QFileInfo>
#include <QPainter>
#include <QSvgRenderer>

#include <cmath>

namespace duifx {

namespace {

/// By extension, then by content: catches an SVG saved under another extension.
bool looksLikeSvg(const QString &path)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    if (suffix == QLatin1StringView("svg") || suffix == QLatin1StringView("svgz"))
        return true;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    const QByteArray head = file.read(1024).trimmed();
    return head.startsWith("<svg") || (head.startsWith("<?xml") && head.contains("<svg"));
}

} // namespace

DxLogo::DxLogo(const QString &logoPath, QSize size, QWidget *parent)
    : QWidget(parent)
{
    initResources();
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setLogo(logoPath, size);
}

DxLogo::~DxLogo() = default;

bool DxLogo::setLogo(const QString &logoPath, QSize size)
{
    m_renderer.reset();
    m_pixmap = QPixmap();
    m_native = QSize();

    if (!QFileInfo::exists(logoPath)) {
        qCWarning(lcDuifx, "Logo file not found: %s", qPrintable(logoPath));
        setFixedSize(0, 0);
        update();
        return false;
    }

    if (looksLikeSvg(logoPath)) {
        auto renderer = std::make_unique<QSvgRenderer>(logoPath);
        if (renderer->isValid()) {
            // The widget's own rect decides the size: fill it, keep the ratio.
            renderer->setAspectRatioMode(Qt::KeepAspectRatio);
            m_native = renderer->defaultSize();
            m_renderer = std::move(renderer);
        }
    }
    if (!m_renderer) {
        QPixmap pixmap(logoPath);
        if (!pixmap.isNull()) {
            m_pixmap = pixmap;
            m_native = pixmap.size();
        }
    }

    if (!m_native.isValid() || m_native.height() <= 0) {
        qCWarning(lcDuifx, "Logo file could not be read as an image: %s", qPrintable(logoPath));
        m_renderer.reset();
        m_pixmap = QPixmap();
        setFixedSize(0, 0);
        update();
        return false;
    }

    setFixedSize(fittedSize(size));
    update();
    return true;
}

bool DxLogo::isValid() const
{
    return m_renderer || !m_pixmap.isNull();
}

QSize DxLogo::fittedSize(QSize size) const
{
    const double ratio = double(m_native.width()) / double(m_native.height());
    if (!size.isValid() || size.height() <= 0) {
        const int height = kDefaultLogoHeight;
        return {std::max(1, int(std::lround(height * ratio))), height};
    }
    const int width = int(std::lround(size.height() * ratio));
    if (size.width() > 0 && width > size.width())
        return {size.width(), std::max(1, int(std::lround(size.width() / ratio)))};
    return {std::max(1, width), size.height()};
}

void DxLogo::paintEvent(QPaintEvent *)
{
    if (!isValid())
        return;
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setRenderHint(QPainter::Antialiasing, true);
    if (m_renderer)
        m_renderer->render(&painter, QRectF(rect()));
    else
        // The device pixel ratio is in the painter transform: full source resolution.
        painter.drawPixmap(rect(), m_pixmap);
}

} // namespace duifx

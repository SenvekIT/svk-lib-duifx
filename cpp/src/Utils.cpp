#include "duifx/Utils.h"

#include "Logging.h"

#include <QCoreApplication>
#include <QFile>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>

#include <mutex>

// Q_INIT_RESOURCE must be used outside any namespace.
static void duifxInitResources()
{
    Q_INIT_RESOURCE(duifx_icons);
    Q_INIT_RESOURCE(duifx_styles);
}

namespace duifx {

Q_LOGGING_CATEGORY(lcDuifx, "duifx")

void initResources()
{
    static std::once_flag once;
    std::call_once(once, duifxInitResources);
}

QString basePath()
{
    return QCoreApplication::instance() ? QCoreApplication::applicationDirPath() : QString();
}

QString loadStylesheet()
{
    initResources();
    QFile file(QStringLiteral(":/duifx/modern_light.qss"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(lcDuifx, "Stylesheet not found in the resources");
        return {};
    }
    qCDebug(lcDuifx, "Loading stylesheet from %s", qPrintable(file.fileName()));
    return QString::fromUtf8(file.readAll());
}

void setSvgOnLabel(QLabel *label, const QString &svgPath, QSize size)
{
    initResources();
    QSvgRenderer renderer(svgPath);
    if (!size.isValid()) {
        const QSize native = renderer.defaultSize();
        size = native.isValid() ? native : QSize(label->width() ? label->width() : 64, label->height() ? label->height() : 64);
    }
    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    renderer.render(&painter);
    painter.end();
    label->setPixmap(pixmap);
}

QString version()
{
    return QStringLiteral(DUIFX_VERSION_STRING);
}

} // namespace duifx

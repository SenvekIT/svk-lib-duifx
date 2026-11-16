#include "duifx/SplashScreen.h"

#include "duifx/Utils.h"

#include <QApplication>
#include <QPainter>
#include <QScreen>
#include <QSvgRenderer>

namespace duifx {

DxSplashScreen::DxSplashScreen(QApplication *app, const QString &title, const QString &subTitle)
    : QSplashScreen(createPixmap(title, subTitle))
{
    setMessage(QStringLiteral("Starting application..."));
    setProgress(0);
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
    show();
    if (app)
        app->processEvents();
}

void DxSplashScreen::setMessage(const QString &message)
{
    m_message = message;
}

void DxSplashScreen::setProgress(int value)
{
    if (m_progress != value) {
        m_progress = value;
        repaint();
    }
}

void DxSplashScreen::drawContents(QPainter *painter)
{
    QSplashScreen::drawContents(painter);

    painter->setFont(QFont(QStringLiteral("Segoe UI"), 10));
    const QRect area = rect();

    // Message.
    painter->drawText(40, area.height() - 30, m_message);

    // Progress bar.
    const int barWidth = area.width() - 80;
    const int barX = 40;
    const int barY = area.height() - 50;
    const int barHeight = 3;
    painter->fillRect(barX, barY, barWidth, barHeight, Qt::lightGray);
    if (m_progress > 0)
        painter->fillRect(barX, barY, (barWidth * m_progress) / 100, barHeight, Qt::darkGray);
}

QPixmap DxSplashScreen::createPixmap(const QString &title, const QString &subTitle)
{
    initResources();
    QScreen *screen = QGuiApplication::primaryScreen();
    const qreal ratio = screen ? screen->devicePixelRatio() : 1.0;

    const int baseWidth = 450;
    const int baseHeight = 250;
    QPixmap pixmap(int(baseWidth * ratio), int(baseHeight * ratio));
    pixmap.setDevicePixelRatio(ratio);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

    // Background.
    painter.setBrush(Qt::white);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(QRectF(0, 0, baseWidth, baseHeight), 15, 15);

    // Logo.
    QSvgRenderer logo(QStringLiteral(":/icons/icon/Dx.svg"));
    logo.render(&painter, QRectF(30, 77, 96, 96));

    // Title and subtitle.
    painter.setPen(Qt::black);
    const int textX = 130;
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 16, QFont::Bold));
    painter.drawText(QRectF(textX, 105, 290, 40), Qt::AlignLeft | Qt::AlignVCenter, title);
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 10));
    painter.drawText(QRectF(textX, 132, 290, 40), Qt::AlignLeft | Qt::AlignVCenter, subTitle);
    painter.end();
    return pixmap;
}

} // namespace duifx

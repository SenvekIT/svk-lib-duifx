#pragma once

// Splash screen with the DUIFx logo, a title, a subtitle, a message and a thin
// progress bar. Port of duifx/gui/splash.py.

#include <QSplashScreen>

class QApplication;

namespace duifx {

class DxSplashScreen : public QSplashScreen
{
    Q_OBJECT

public:
    /// Builds the splash, shows it and processes pending events, so it is on
    /// screen when the constructor returns.
    DxSplashScreen(QApplication *app, const QString &title, const QString &subTitle);

    /// Stores the message drawn above the progress bar (shown with the next
    /// repaint, e.g. by setProgress()).
    void setMessage(const QString &message);
    QString message() const { return m_message; }

    /// As in Python: stores the message (alignment and colour are ignored, the
    /// splash draws its own layout). Hides QSplashScreen::showMessage().
    void showMessage(const QString &message, int alignment = Qt::AlignBottom, const QColor &color = Qt::darkGray)
    {
        Q_UNUSED(alignment);
        Q_UNUSED(color);
        setMessage(message);
    }

    /// Progress 0-100; repaints when it changes.
    void setProgress(int value);
    int progress() const { return m_progress; }

protected:
    void drawContents(QPainter *painter) override;

private:
    static QPixmap createPixmap(const QString &title, const QString &subTitle);

    int m_progress = 0;
    QString m_message = QStringLiteral("Loading...");
};

} // namespace duifx

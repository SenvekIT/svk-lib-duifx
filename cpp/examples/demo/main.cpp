// DUIFx demo: the main window with navigation, a live plot, the log viewer,
// the settings dialog and the about dialog.

#include <duifx/Duifx.h>

#include <QApplication>
#include <QLabel>
#include <QLoggingCategory>
#include <QTimer>

#include <cmath>

Q_LOGGING_CATEGORY(lcDemo, "demo")

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("Senvek"));
    QApplication::setApplicationName(QStringLiteral("DUIFx Demo"));
    duifx::applyTheme(&app);

    duifx::DxSplashScreen splash(&app, QStringLiteral("DUIFx Demo"), QStringLiteral("© Senvek SRL"));
    splash.setProgress(50);

    duifx::MainWindow window(QStringLiteral("DUIFx Demo"));

    // Page 1: a live plot.
    duifx::TimeSeriesPlot::Options plotOptions;
    plotOptions.title = QStringLiteral("Acceleration");
    plotOptions.yLabel = QStringLiteral("a");
    plotOptions.yUnit = QStringLiteral("m/s²");
    auto *plot = new duifx::TimeSeriesPlot(plotOptions);
    plot->addSeries(QStringLiteral("x"), {.label = QStringLiteral("X")});
    plot->addSeries(QStringLiteral("y"), {.label = QStringLiteral("Y")});

    // Page 2: the log viewer, fed by Qt's logging.
    auto *logs = new duifx::LogViewerWidget;
    logs->attachLogger();

    window.addContentRightSidebar(plot);
    window.addContentRightSidebar(logs);
    window.addSidebarTopButton(new duifx::MenuButton(QStringLiteral(":/icons/icon/line_chart.svg"), {}, QStringLiteral("Plot")), {},
                               plot);
    window.addSidebarTopButton(new duifx::MenuButton(QStringLiteral(":/icons/icon/document.svg"), {}, QStringLiteral("Log")), {},
                               logs);

    window.addMenuButton(new duifx::MenuButton(QStringLiteral(":/icons/icon/settings.svg"), QStringLiteral("Settings")), [&window] {
        duifx::SettingsDialog dialog(&window, QStringLiteral("Settings"), true);
        dialog.exec();
    });
    window.addMenuButton(new duifx::MenuButton(QStringLiteral(":/icons/icon/info.svg"), QStringLiteral("About")), [&window] {
        duifx::InfoDialog dialog(&window, QStringLiteral("About"));
        dialog.setMainTitle(QStringLiteral("DUIFx Demo"));
        dialog.setSubTitle(QStringLiteral("Version ") + duifx::version());
        dialog.setInfoText(QStringLiteral("The C++ port of the Senvek user interface framework."));
        dialog.exec();
    });
    window.addHeaderLogo(QStringLiteral(":/icons/icon/Dx.svg"), QSize(0, 40));

    // Samples at 50 Hz, a log line per second.
    double t = 0.0;
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&] {
        t += 0.02;
        plot->append(t, {{QStringLiteral("x"), std::sin(t)}, {QStringLiteral("y"), 0.5 * std::cos(3 * t)}});
        if (std::fmod(t, 1.0) < 0.02)
            qCInfo(lcDemo, "t = %.0f s", t);
        if (std::fmod(t, 7.0) < 0.02)
            qCWarning(lcDemo, "a warning every 7 s");
    });
    timer.start(20);

    splash.finish(&window);
    return app.exec();
}

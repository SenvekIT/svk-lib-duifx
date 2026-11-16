// DUIFx demo: the main window with navigation, header buttons and dialogs.

#include <duifx/Dialogs.h>
#include <duifx/MainWindow.h>
#include <duifx/MenuButton.h>
#include <duifx/SplashScreen.h>
#include <duifx/Theming.h>
#include <duifx/Utils.h>

#include <QApplication>
#include <QLabel>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("Senvek"));
    QApplication::setApplicationName(QStringLiteral("DUIFx Demo"));
    duifx::applyTheme(&app);

    duifx::DxSplashScreen splash(&app, QStringLiteral("DUIFx Demo"), QStringLiteral("© Senvek SRL"));
    splash.setProgress(50);

    duifx::MainWindow window(QStringLiteral("DUIFx Demo"));
    auto *home = new QLabel(QStringLiteral("Home page"));
    auto *channels = new QLabel(QStringLiteral("Channels page"));
    home->setAlignment(Qt::AlignCenter);
    channels->setAlignment(Qt::AlignCenter);
    window.addContentRightSidebar(home);
    window.addContentRightSidebar(channels);
    window.addSidebarTopButton(new duifx::MenuButton(QStringLiteral(":/icons/icon/home.svg"), {}, QStringLiteral("Home")), {}, home);
    window.addSidebarTopButton(new duifx::MenuButton(QStringLiteral(":/icons/icon/Channels.svg"), {}, QStringLiteral("Channels")), {},
                               channels);

    window.addMenuButton(new duifx::MenuButton(QStringLiteral(":/icons/icon/info.svg"), QStringLiteral("About")), [&window] {
        duifx::InfoDialog dialog(&window, QStringLiteral("About"));
        dialog.setMainTitle(QStringLiteral("DUIFx Demo"));
        dialog.setSubTitle(QStringLiteral("Version ") + duifx::version());
        dialog.setInfoText(QStringLiteral("The C++ port of the Senvek user interface framework."));
        dialog.exec();
    });
    window.addHeaderLogo(QStringLiteral(":/icons/icon/Dx.svg"), QSize(0, 40));

    splash.finish(&window);
    return app.exec();
}

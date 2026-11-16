// Colours, theme, settings, resources, main window and dialogs.

#include "TestSupport.h"

#include <duifx/Colors.h>
#include <duifx/Dialogs.h>
#include <duifx/Logo.h>
#include <duifx/MainWindow.h>
#include <duifx/MenuButton.h>
#include <duifx/Palette.h>
#include <duifx/Settings.h>
#include <duifx/SplashScreen.h>
#include <duifx/Theming.h>
#include <duifx/Utils.h>

#include <QApplication>
#include <QFile>
#include <QIcon>
#include <QLabel>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QtTest>

using namespace duifx;

class TstCore : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        duifx_test::isolateSettings();
        setActiveTheme(std::nullopt);
    }

    // --- palette and tokens ---

    void paletteDictionary()
    {
        const auto colors = palette::colorDict();
        QCOMPARE(colors.value(QStringLiteral("gray_50")), QStringLiteral("#f9fafb"));
        QCOMPARE(colors.value(QStringLiteral("teal_900")), QStringLiteral("#134e4a"));
        QCOMPARE(colors.value(QStringLiteral("primary")), QStringLiteral("#3b82f6"));
        QCOMPARE(colors.size(), 8 * 10 + 8);
        const auto all = base::colorDict();
        QCOMPARE(all.value(QStringLiteral("signal_green")), QStringLiteral("#27ae60"));
        QCOMPARE(all.size(), 8 * 10 + 8 + 14);
    }

    void tokenValues()
    {
        QCOMPARE(colorHex(Token::StateOk), QStringLiteral("#27ae60"));
        QCOMPARE(colorHex(Token::TextPrimary), QStringLiteral("#212529"));
        QCOMPARE(colorHex(Token::TextPrimary, Theme::Dark), QStringLiteral("#f3f4f6"));
        QCOMPARE(colorHex(Token::SurfaceSelected), QStringLiteral("#dbeafe"));
        // Translucent tokens: rgba() for stylesheets.
        QCOMPARE(css(Token::OverlayHover), QStringLiteral("rgba(255, 255, 255, 20)"));
        QCOMPARE(css(Token::PlotGrid), QStringLiteral("rgba(150, 150, 150, 77)"));
        QCOMPARE(css(Token::Accent), QStringLiteral("#2563eb"));
        QCOMPARE(color(Token::Accent, 10).alpha(), 10);
        QCOMPARE(css(Token::SurfaceSelected, std::nullopt, Theme::Dark), QStringLiteral("rgba(59, 130, 246, 64)"));
    }

    void tokenNames()
    {
        QCOMPARE(tokenName(Token::StateOk), QStringLiteral("state_ok"));
        QCOMPARE(tokenName(Token::PlotAxisY), QStringLiteral("plot_series_2"));
        QCOMPARE(*tokenFromName(QStringLiteral("surface_selected_hover")), Token::SurfaceSelectedHover);
        QCOMPARE(*tokenFromName(QStringLiteral("plot_axis_z")), Token::PlotSeries3);
        QVERIFY(!tokenFromName(QStringLiteral("nope")));
        for (int i = 0; i < kTokenCount; ++i)
            QCOMPARE(*tokenFromName(tokenName(Token(i))), Token(i));
    }

    void plotSeriesCycle()
    {
        QCOMPARE(plotSeriesColor(0), color(Token::PlotSeries1));
        QCOMPARE(plotSeriesColor(6), color(Token::PlotSeries1));
        QCOMPARE(plotSeriesColor(-1), color(Token::PlotSeries6));
        QCOMPARE(plotSeriesColor(3, 100).alpha(), 100);
    }

    void stateColorLookup()
    {
        const StateColors &states = stateColors();
        QCOMPARE(states[QStringLiteral("Connected")], QStringLiteral("#27ae60"));
        QCOMPARE(states.token(QStringLiteral("  heartbeat LOST ")), Token::StateError);
        QCOMPARE(states.token(QStringLiteral("whatever")), Token::TextSecondary);
        QVERIFY(states.contains(QStringLiteral("SCANNING")));
        QCOMPARE(states.get(QStringLiteral("whatever"), QStringLiteral("#123456")), QStringLiteral("#123456"));
        QCOMPARE(states.get(QStringLiteral("error"), QStringLiteral("#123456")), QStringLiteral("#e74c3c"));

        const StateColors custom({{QStringLiteral("Booting"), Token::StateBusy}}, Token::StateInfo);
        QCOMPARE(custom.token(QStringLiteral("booting")), Token::StateBusy);
        QCOMPARE(custom.token(QStringLiteral("x")), Token::StateInfo);
    }

    // --- theme ---

    void themeParsing()
    {
        QCOMPARE(parseTheme(QStringLiteral("light")), Theme::Light);
        QCOMPARE(parseTheme(QStringLiteral("Follow system setting")), Theme::System);
        QCOMPARE(parseTheme(QStringLiteral(" DARK ")), Theme::Dark);
        QCOMPARE(parseTheme(QString()), Theme::Light);
        QCOMPARE(parseTheme(QStringLiteral("purple")), Theme::Light);
        QCOMPARE(themeLabels(), (QStringList{QStringLiteral("Light"), QStringLiteral("Follow system setting")}));
    }

    void themeIsStoredLikePython()
    {
        setTheme(Theme::System);
        QCOMPARE(getSettings()->value(QLatin1StringView(kThemeSettingKey)).toString(), QStringLiteral("Follow system setting"));
        QCOMPARE(getTheme(), Theme::System);
        // Dark is not implemented: applied as light.
        QCOMPARE(applyTheme(nullptr, Theme::Dark), Theme::Light);
        QCOMPARE(applyTheme(nullptr, Theme::System), Theme::System);
        setTheme(Theme::Light);
        QCOMPARE(applyTheme(), Theme::Light);
        setActiveTheme(Theme::Dark);
        QCOMPARE(activeTheme(), Theme::Dark);
        setActiveTheme(std::nullopt);
        QCOMPARE(activeTheme(), Theme::Light);
    }

    // --- settings and resources ---

    void settingsFileLikePython()
    {
        const QString path = settingsFilePath();
#ifdef Q_OS_WIN
        QVERIFY2(path.endsWith(QStringLiteral("/AppData/Roaming/DUIFxTests/tests/settings.ini")), qPrintable(path));
#else
        QVERIFY2(path.endsWith(QStringLiteral("/.config/DUIFxTests/tests/settings.ini")), qPrintable(path));
#endif
        QCOMPARE(getSettings()->format(), QSettings::IniFormat);
    }

    void resourcesAreShared()
    {
        const QString qss = loadStylesheet();
        QVERIFY(qss.contains(QStringLiteral("QLabel[class=\"title\"]")));
        // The icons of resource.qrc, by the Python paths.
        for (const char *icon : {"Dx.svg", "home.svg", "info.svg", "settings.svg", "arrow-down-icon.svg"})
            QVERIFY2(QFile::exists(QStringLiteral(":/icons/icon/") + QLatin1StringView(icon)), icon);
        QVERIFY(!QIcon(QStringLiteral(":icons/icon/home.svg")).isNull());
        QVERIFY(!version().isEmpty());
    }

    void svgOnLabel()
    {
        QLabel label;
        setSvgOnLabel(&label, QStringLiteral(":/icons/icon/Dx.svg"), QSize(20, 10));
        QCOMPARE(label.pixmap().size(), QSize(20, 10));
    }

    // --- logo and splash ---

    void logoKeepsAspectRatio()
    {
        QTemporaryDir dir;
        const QString file = dir.filePath(QStringLiteral("logo.png"));
        QPixmap source(200, 50);
        source.fill(Qt::red);
        QVERIFY(source.save(file));

        DxLogo logo(file, QSize(400, 20));
        QVERIFY(logo.isValid());
        QCOMPARE(logo.nativeSize(), QSize(200, 50));
        QCOMPARE(logo.size(), QSize(80, 20));
        QVERIFY(logo.setLogo(file, QSize(40, 100))); // width-bound
        QCOMPARE(logo.size(), QSize(40, 10));
        QVERIFY(logo.setLogo(file));
        QCOMPARE(logo.size(), QSize(160, kDefaultLogoHeight));

        // An SVG under another extension is still recognised.
        QFile svg(QStringLiteral(":/icons/icon/Dx.svg"));
        QVERIFY(svg.open(QIODevice::ReadOnly));
        const QString renamed = dir.filePath(QStringLiteral("logo.dat"));
        QFile out(renamed);
        QVERIFY(out.open(QIODevice::WriteOnly));
        out.write(svg.readAll());
        out.close();
        QVERIFY(logo.setLogo(renamed, QSize(0, 30)));

        QVERIFY(!logo.setLogo(dir.filePath(QStringLiteral("missing.svg"))));
        QVERIFY(!logo.isValid());
        QCOMPARE(logo.size(), QSize(0, 0));
    }

    void splashProgress()
    {
        DxSplashScreen splash(qobject_cast<QApplication *>(QCoreApplication::instance()), QStringLiteral("Title"),
                              QStringLiteral("Sub"));
        QCOMPARE(splash.message(), QStringLiteral("Starting application..."));
        splash.showMessage(QStringLiteral("Loading devices"));
        QCOMPARE(splash.message(), QStringLiteral("Loading devices"));
        splash.setProgress(40);
        QCOMPARE(splash.progress(), 40);
        QVERIFY(splash.isVisible());
    }

    // --- main window: the Python CI smoke test ---

    void sidebarNavigation()
    {
        MainWindow window(QStringLiteral("CI"));
        QVERIFY(window.isVisible());
        auto *a = new QLabel(QStringLiteral("a"));
        auto *b = new QLabel(QStringLiteral("b"));
        window.addContentRightSidebar(a);
        window.addContentRightSidebar(b);

        auto *home = new MenuButton({}, {}, QStringLiteral("Home"));
        auto *other = new MenuButton({}, {}, QStringLiteral("Other"));
        window.addSidebarTopButton(home, {}, a);
        window.addSidebarTopButton(other, {}, b);
        auto *action = new MenuButton({}, QStringLiteral("Dialog"));
        int actions = 0;
        window.addMenuButton(action, [&] { ++actions; });

        QCOMPARE(window.activeSidebarButton(), home);
        other->click();
        QCOMPARE(window.activeSidebarButton(), other);
        QVERIFY(!home->isChecked());
        QCOMPARE(window.rightSidebarWidget(), b);
        window.setRightSidebarWidget(0);
        QCOMPARE(window.activeSidebarButton(), home);
        action->click();
        QCOMPARE(actions, 1);
        QVERIFY(!action->isCheckable());
        QCOMPARE(window.activeSidebarButton(), home);
    }

    void navigationWithoutPages()
    {
        MainWindow window;
        auto *first = new MenuButton(QStringLiteral(":/icons/icon/home.svg"));
        auto *second = new MenuButton();
        auto *dialog = new MenuButton();
        int slotRuns = 0;
        window.addSidebarTopButton(first);
        window.addSidebarTopButton(second, [&] { ++slotRuns; });
        window.addSidebarTopButton(dialog, {}, nullptr, false);
        QCOMPARE(window.activeSidebarButton(), first); // first entry starts active
        second->click();
        QCOMPARE(slotRuns, 1);
        QCOMPARE(window.activeSidebarButton(), second);
        dialog->click();
        QCOMPARE(window.activeSidebarButton(), second);
        window.setActiveSidebarButton(dialog); // no navigation entry: ignored
        QCOMPARE(window.activeSidebarButton(), second);
        window.setActiveSidebarButton(nullptr);
        QCOMPARE(window.activeSidebarButton(), nullptr);
        QCOMPARE(window.property("styleSheet").toString().contains(QStringLiteral("[navigation=\"true\"]:checked")), true);
    }

    void pageReplacedByOneWithoutEntry()
    {
        MainWindow window;
        auto *page = new QLabel;
        auto *loose = new QLabel;
        window.addContentWidget(page);
        window.addContentWidget(loose);
        auto *entry = new MenuButton();
        window.addSidebarTopButton(entry, {}, page);
        QCOMPARE(window.activeSidebarButton(), entry);
        window.setContentWidget(1);
        QCOMPARE(window.contentWidget(), loose);
        QCOMPARE(window.activeSidebarButton(), nullptr);
        window.setContentWidget(0);
        QCOMPARE(window.activeSidebarButton(), entry);
    }

    void geometryIsSavedAndRestored()
    {
        {
            MainWindow window;
            window.showNormal();
            window.resize(640, 480);
            window.close();
        }
        const auto settings = getSettings();
        QCOMPARE(settings->value(QStringLiteral("MainWindow/width")).toInt(), 640);
        QCOMPARE(settings->value(QStringLiteral("MainWindow/maximized")).toBool(), false);
        MainWindow restored;
        QCOMPARE(restored.size(), QSize(640, 480));
    }

    void headerLogo()
    {
        MainWindow window;
        QVERIFY(window.addHeaderLogo(QStringLiteral(":/icons/icon/Dx.svg"), QSize(0, 32)));
        QVERIFY(!window.addHeaderLogo(QStringLiteral("does-not-exist.png")));
        QCOMPARE(window.findChildren<DxLogo *>().size(), 1);
    }

    // --- dialogs ---

    void infoDialog()
    {
        InfoDialog dialog(nullptr, QStringLiteral("About"));
        dialog.setMainTitle(QStringLiteral("Main"));
        dialog.setSubTitle(QStringLiteral("Sub"));
        dialog.setInfoText(QStringLiteral("Text"));
        QCOMPARE(dialog.windowTitle(), QStringLiteral("About"));
        QCOMPARE(dialog.findChild<QLabel *>(QStringLiteral("labelMainTitle"))->property("class").toString(), QStringLiteral("title"));
    }

    void headerDialog()
    {
        HeaderDialog dialog;
        auto *content = new QLabel(QStringLiteral("content"));
        dialog.addContentWidget(content);
        QCOMPARE(content->parentWidget() != nullptr, true);
        dialog.hideOkButton();
        dialog.hideCancelButton();
        QVERIFY(dialog.windowFlags().testFlag(Qt::WindowMaximizeButtonHint));
    }
};

QTEST_MAIN(TstCore)
#include "tst_core.moc"

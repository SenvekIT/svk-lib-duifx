#include "duifx/MainWindow.h"

#include "duifx/Colors.h"
#include "duifx/Logo.h"
#include "duifx/MenuButton.h"
#include "duifx/Settings.h"
#include "duifx/Utils.h"
#include "Logging.h"

#include "ui_main_window.h"

#include <algorithm>
#include <QApplication>
#include <QButtonGroup>
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QScreen>
#include <QStackedWidget>
#include <QStyle>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shobjidl.h>
#endif

namespace duifx {

QString navigationStylesheet()
{
    // The ID selector outranks the stylesheet's generic QToolButton:checked
    // (solid accent, white text), which is meant for toggle buttons.
    const QString entry = QStringLiteral("#sidebar QToolButton[%1=\"true\"]").arg(QLatin1StringView(kNavigationProperty));
    return QStringLiteral("\n%1:checked {\n"
                          "    background-color: %2;\n"
                          "    color: %3;\n"
                          "}\n"
                          "%1:checked:hover {\n"
                          "    background-color: %4;\n"
                          "}\n")
        .arg(entry, css(Token::SurfaceSelected), css(Token::TextPrimary), css(Token::SurfaceSelectedHover));
}

// --- MenuButton ---------------------------------------------------------------------

MenuButton::MenuButton(const QString &iconPath, const QString &label, const QString &tooltip, QSize iconSize,
                       QWidget *parent)
    : QToolButton(parent)
{
    initResources();
    if (!iconPath.isEmpty())
        setIcon(QIcon(iconPath));

    if (!label.isEmpty()) {
        setText(label);
        setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        setIconSize(iconSize.isValid() ? iconSize : QSize(32, 32));
        setMinimumSize(QSize(60, 50));
    } else {
        setIconSize(iconSize.isValid() ? iconSize : QSize(24, 24));
    }

    if (!tooltip.isEmpty())
        setToolTip(tooltip);

    setCursor(QCursor(Qt::PointingHandCursor));
}

// --- MainWindow ---------------------------------------------------------------------

MainWindow::MainWindow(const QString &description, const QString &userModelId)
    : m_ui(std::make_unique<Ui::MainWindow>())
{
    initResources();

#ifdef Q_OS_WIN
    // Own taskbar entry and icon instead of the host process's.
    SetCurrentProcessExplicitAppUserModelID(reinterpret_cast<PCWSTR>(userModelId.utf16()));
#else
    Q_UNUSED(userModelId);
#endif

    m_ui->setupUi(this);
    setWindowTitle(description);
    setWindowIcon(QIcon(QStringLiteral(":/icons/icon/Dx.svg")));

    // Navigation: the sidebar top entries switch pages, exactly one of them is
    // active (checked) and highlighted.
    m_navGroup = new QButtonGroup(this);
    m_navGroup->setExclusive(true);
    watchStack(m_ui->stackedWidgetContent);
    watchStack(m_ui->stackedWidgetRightSidebar);

    setStyleSheet(loadStylesheet() + navigationStylesheet());

    // Sidebar fixed to 50 px; the remaining space belongs to the main area.
    m_ui->sidebar->setMinimumWidth(50);
    m_ui->sidebar->setMaximumWidth(50);
    m_ui->horizontalLayout_8->setStretch(0, 0);
    m_ui->horizontalLayout_8->setStretch(1, 1);

    restoreWindowGeometry();

    // closeEvent() is not called when the application quits through
    // QApplication::quit(); aboutToQuit is the reliable hook.
    if (QCoreApplication::instance())
        connect(qApp, &QCoreApplication::aboutToQuit, this, &MainWindow::saveWindowGeometry);

    show();
}

MainWindow::~MainWindow()
{
    // QWidget deletes the children only after the members are gone, and the
    // dying buttons and stacks would still call into the navigation. Delete
    // them now, while the members live, with the navigation switched off.
    m_destroying = true;
    const QList<QWidget *> children = findChildren<QWidget *>(Qt::FindDirectChildrenOnly);
    qDeleteAll(children);
}

void MainWindow::addContentWidget(QWidget *content)
{
    m_ui->stackedWidgetContent->addWidget(content);
}

void MainWindow::setContentWidget(int index)
{
    m_ui->stackedWidgetContent->setCurrentIndex(index);
}

QWidget *MainWindow::contentWidget() const
{
    return m_ui->stackedWidgetContent->currentWidget();
}

void MainWindow::setRightSidebarSize(int width)
{
    m_ui->rightSidebar->setFixedWidth(width);
}

void MainWindow::addContentRightSidebar(QWidget *content)
{
    m_ui->stackedWidgetRightSidebar->addWidget(content);
}

void MainWindow::setRightSidebarWidget(int index)
{
    m_ui->stackedWidgetRightSidebar->setCurrentIndex(index);
}

QWidget *MainWindow::rightSidebarWidget() const
{
    return m_ui->stackedWidgetRightSidebar->currentWidget();
}

void MainWindow::addMenuButton(MenuButton *button, const std::function<void()> &slot)
{
    m_ui->headerContentLayout->addWidget(button, 0, Qt::AlignLeft | Qt::AlignVCenter);
    if (slot)
        connect(button, &QAbstractButton::clicked, this, slot);
}

void MainWindow::addSidebarBottomButton(MenuButton *button, const std::function<void()> &slot)
{
    m_ui->sidebarBottomLayout->addWidget(button, 0, Qt::AlignBottom | Qt::AlignHCenter);
    if (slot)
        connect(button, &QAbstractButton::clicked, this, slot);
}

void MainWindow::addSidebarTopButton(MenuButton *button, const std::function<void()> &slot, QWidget *page,
                                     bool navigation)
{
    m_ui->sidebarTopLayout->addWidget(button, 0, Qt::AlignTop | Qt::AlignHCenter);

    if (navigation) {
        addNavigationButton(button, page);
    } else if (page) {
        QPointer<QWidget> target(page);
        connect(button, &QAbstractButton::clicked, this, [this, target] {
            if (target)
                showPage(target);
        });
    }

    if (slot)
        connect(button, &QAbstractButton::clicked, this, slot);
}

void MainWindow::setActiveSidebarButton(QAbstractButton *button)
{
    if (!button)
        clearActiveNavigation();
    else if (m_navGroup->buttons().contains(button))
        button->setChecked(true);
    else
        qCDebug(lcDuifx, "setActiveSidebarButton: the button is no navigation entry");
}

QAbstractButton *MainWindow::activeSidebarButton() const
{
    return m_navGroup->checkedButton();
}

void MainWindow::addNavigationButton(QAbstractButton *button, QWidget *page)
{
    button->setCheckable(true);
    button->setProperty(kNavigationProperty, true);
    // The property selector is matched at polish time.
    button->style()->unpolish(button);
    button->style()->polish(button);
    m_navGroup->addButton(button);
    connect(button, &QObject::destroyed, this, [this, button] { m_navPages.remove(button); });

    if (!page) {
        if (!m_navGroup->checkedButton() && !navigationPagesShown())
            button->setChecked(true);
        return;
    }

    m_navPages.insert(button, page);
    if (auto *stack = qobject_cast<QStackedWidget *>(page->parentWidget()))
        watchStack(stack);
    // Connected before the application's slot: the page is showing (and the
    // entry active) by the time the slot runs.
    QPointer<QWidget> target(page);
    connect(button, &QAbstractButton::clicked, this, [this, target] {
        if (target)
            showPage(target);
    });

    if (isPageShown(page) || !m_navGroup->checkedButton())
        button->setChecked(true);
}

void MainWindow::watchStack(QStackedWidget *stack)
{
    if (m_navStacks.contains(stack))
        return;
    m_navStacks.insert(stack);
    connect(stack, &QStackedWidget::currentChanged, this, [this] { syncNavigation(); });
    connect(stack, &QObject::destroyed, this, [this, stack] { m_navStacks.remove(stack); });
}

bool MainWindow::isPageShown(const QWidget *page)
{
    const auto *stack = page ? qobject_cast<const QStackedWidget *>(page->parentWidget()) : nullptr;
    return stack && stack->currentWidget() == page;
}

bool MainWindow::navigationPagesShown() const
{
    for (const QPointer<QWidget> &page : m_navPages) {
        if (isPageShown(page))
            return true;
    }
    return false;
}

void MainWindow::showPage(QWidget *page)
{
    if (auto *stack = qobject_cast<QStackedWidget *>(page->parentWidget()))
        stack->setCurrentWidget(page);
    else
        qCWarning(lcDuifx, "Navigation page is not in a stacked widget");
}

void MainWindow::syncNavigation()
{
    if (m_destroying)
        return;
    // Move the highlight to the entry whose page is now shown.
    for (auto it = m_navPages.cbegin(); it != m_navPages.cend(); ++it) {
        if (isPageShown(it.value())) {
            it.key()->setChecked(true);
            return;
        }
    }
    // The active entry's page was replaced by one without an entry.
    if (m_navPages.contains(m_navGroup->checkedButton()))
        clearActiveNavigation();
}

void MainWindow::clearActiveNavigation()
{
    QAbstractButton *checked = m_navGroup->checkedButton();
    if (!checked)
        return;
    // An exclusive group refuses to uncheck its last checked button.
    m_navGroup->setExclusive(false);
    checked->setChecked(false);
    m_navGroup->setExclusive(true);
}

bool MainWindow::addHeaderLogo(const QString &logoPath, QSize size, int rightMargin)
{
    auto *logo = new DxLogo(logoPath, size);
    if (!logo->isValid()) {
        qCWarning(lcDuifx, "Header logo could not be loaded: %s", qPrintable(logoPath));
        delete logo;
        return false;
    }

    // The margin lives on a container: DxLogo paints across its whole rect.
    auto *container = new QWidget;
    auto *box = new QHBoxLayout(container);
    box->setContentsMargins(0, 0, std::max(0, rightMargin), 0);
    box->setSpacing(0);
    box->addWidget(logo);
    m_ui->headerLogoLayout->addWidget(container, 0, Qt::AlignRight | Qt::AlignVCenter);
    return true;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveWindowGeometry();
    QMainWindow::closeEvent(event);
}

void MainWindow::saveWindowGeometry()
{
    const auto settings = getSettings();
    settings->beginGroup(QStringLiteral("MainWindow"));
    // The normal (non-maximised) geometry, so a restore works.
    if (!isMaximized()) {
        settings->setValue(QStringLiteral("pos_x"), x());
        settings->setValue(QStringLiteral("pos_y"), y());
        settings->setValue(QStringLiteral("width"), width());
        settings->setValue(QStringLiteral("height"), height());
    }
    settings->setValue(QStringLiteral("maximized"), isMaximized());
    settings->endGroup();
    settings->sync();
    qCDebug(lcDuifx, "Window geometry saved: x=%d, y=%d, w=%d, h=%d, maximized=%d", x(), y(), width(), height(),
            int(isMaximized()));
}

void MainWindow::restoreWindowGeometry()
{
    const auto settings = getSettings();
    settings->beginGroup(QStringLiteral("MainWindow"));
    if (settings->contains(QStringLiteral("pos_x"))) {
        const int posX = settings->value(QStringLiteral("pos_x"), 0).toInt();
        const int posY = settings->value(QStringLiteral("pos_y"), 0).toInt();
        const int w = settings->value(QStringLiteral("width"), width()).toInt();
        const int h = settings->value(QStringLiteral("height"), height()).toInt();
        const bool maximized = settings->value(QStringLiteral("maximized"), false).toBool();

        ensureTitleBarVisible(posX, posY, w, h);
        if (maximized)
            showMaximized();
        qCDebug(lcDuifx, "Window geometry restored: x=%d, y=%d, w=%d, h=%d, maximized=%d", posX, posY, w, h,
                int(maximized));
    }
    settings->endGroup();
}

void MainWindow::ensureTitleBarVisible(int x, int y, int width, int height)
{
    const QList<QScreen *> screens = QGuiApplication::screens();
    if (screens.isEmpty())
        return;

    // Title bar region: full window width, top 40 pixels.
    const QRect titleBar(x, y, std::max(width, 1), 40);
    bool visible = false;
    for (const QScreen *screen : screens)
        visible = visible || screen->availableGeometry().intersects(titleBar);

    if (visible) {
        move(x, y);
        resize(width, height);
        return;
    }
    // Off-screen (monitor removed, resolution changed): centre on the primary screen.
    if (const QScreen *primary = QGuiApplication::primaryScreen()) {
        const QRect available = primary->availableGeometry();
        const int w = std::min(width, available.width());
        const int h = std::min(height, available.height());
        move(available.x() + (available.width() - w) / 2, available.y() + (available.height() - h) / 2);
        resize(w, h);
    }
    qCInfo(lcDuifx, "Saved window position was off-screen; window repositioned to primary screen.");
}

} // namespace duifx

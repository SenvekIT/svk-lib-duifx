#pragma once

// The DUIFx main window: header with menu buttons and logo, a 50 px left
// sidebar (navigation on top, actions at the bottom), a content stack and a
// right sidebar stack. Port of MainWindow in duifx/gui/windows.py; the layout
// is the shared Qt Designer form duifx/gui/ui/main_window.ui.

#include <QHash>
#include <QMainWindow>
#include <QPointer>
#include <QSet>
#include <QSize>

#include <functional>
#include <memory>

class QAbstractButton;
class QButtonGroup;
class QStackedWidget;

namespace Ui {
class MainWindow;
}

namespace duifx {

class MenuButton;

/// Default gap between the header logo and the right window edge.
inline constexpr int kDefaultLogoRightMargin = 20;

/// Dynamic property set on the sidebar entries that switch pages; the
/// stylesheet gives the active one (":checked") a light background.
inline constexpr const char *kNavigationProperty = "navigation";

/// Stylesheet rules for the active navigation entry, built from the colour
/// tokens so the tint follows the active theme.
QString navigationStylesheet();

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /// description is the window title. userModelId groups the taskbar entry
    /// on Windows (SetCurrentProcessExplicitAppUserModelID). Restores the saved
    /// geometry and, as in Python, shows the window.
    explicit MainWindow(const QString &description = {},
                        const QString &userModelId = QStringLiteral("DUIFx.Application.1.0"));
    ~MainWindow() override;

    // --- content and right sidebar stacks ---
    void addContentWidget(QWidget *content);
    void setContentWidget(int index);
    QWidget *contentWidget() const;

    void setRightSidebarSize(int width);
    void addContentRightSidebar(QWidget *content);
    void setRightSidebarWidget(int index);
    QWidget *rightSidebarWidget() const;

    // --- buttons ---
    /// A header button (an action, never active).
    void addMenuButton(MenuButton *button, const std::function<void()> &slot = {});

    /// A button at the bottom of the left sidebar (an action, never active).
    void addSidebarBottomButton(MenuButton *button, const std::function<void()> &slot = {});

    /// An entry at the top of the left sidebar: the navigation. Exactly one
    /// entry is active; a click makes its entry active, and the first entry
    /// added starts active.
    ///
    /// page links the entry to a widget in one of the window's stacks (or any
    /// QStackedWidget it already sits in): a click shows it (before slot runs),
    /// and the entry becomes active whenever the page is shown -- also through
    /// setContentWidget(), setRightSidebarWidget() or the stack itself. Without
    /// page only the click moves the highlight; an application that switches
    /// pages in code then calls setActiveSidebarButton().
    ///
    /// navigation = false adds a plain action button that never becomes active.
    void addSidebarTopButton(MenuButton *button, const std::function<void()> &slot = {}, QWidget *page = nullptr,
                             bool navigation = true);

    /// Makes button the active navigation entry; nullptr clears it. Buttons
    /// that are no navigation entries are ignored.
    void setActiveSidebarButton(QAbstractButton *button);

    /// The active navigation entry of the sidebar, or nullptr.
    QAbstractButton *activeSidebarButton() const;

    /// Shows a logo (SVG, PNG, JPEG, ...) in the top right of the header,
    /// fitted into size (the height alone is enough) and sharp on scaled
    /// screens. False when the file cannot be read.
    bool addHeaderLogo(const QString &logoPath, QSize size = {}, int rightMargin = kDefaultLogoRightMargin);

protected:
    void closeEvent(QCloseEvent *event) override;

    /// The widgets of main_window.ui, for applications that extend the layout.
    Ui::MainWindow *ui() const { return m_ui.get(); }

private:
    void addNavigationButton(QAbstractButton *button, QWidget *page);
    void watchStack(QStackedWidget *stack);
    static bool isPageShown(const QWidget *page);
    bool navigationPagesShown() const;
    void showPage(QWidget *page);
    void syncNavigation();
    void clearActiveNavigation();

    void saveWindowGeometry();
    void restoreWindowGeometry();
    void ensureTitleBarVisible(int x, int y, int width, int height);

    std::unique_ptr<Ui::MainWindow> m_ui;
    QButtonGroup *m_navGroup = nullptr;
    QHash<QAbstractButton *, QPointer<QWidget>> m_navPages;
    QSet<QStackedWidget *> m_navStacks;
    bool m_destroying = false;
};

} // namespace duifx

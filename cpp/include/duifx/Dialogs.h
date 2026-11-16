#pragma once

// InfoDialog and HeaderDialog. Port of duifx/gui/windows.py; the layouts are
// the shared forms info_dialog.ui and settings_dialog.ui.

#include <QDialog>

#include <memory>

namespace Ui {
class InfoDialog;
class SettingsDialog;
} // namespace Ui

namespace duifx {

/// A title, a subtitle and a read-only text.
class InfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit InfoDialog(QWidget *parent = nullptr, const QString &windowTitle = QStringLiteral("Information"));
    ~InfoDialog() override;

    void setMainTitle(const QString &title);
    void setSubTitle(const QString &subtitle);
    void setInfoText(const QString &text);

protected:
    Ui::InfoDialog *ui() const { return m_ui.get(); }

private:
    std::unique_ptr<Ui::InfoDialog> m_ui;
};

/// A dialog with a title, a subtitle, a content area and OK / Cancel; the base
/// of SettingsDialog. Minimise and maximise buttons are shown.
class HeaderDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HeaderDialog(QWidget *parent = nullptr, const QString &windowTitle = QStringLiteral("Settings"),
                          const QString &iconPath = QStringLiteral(":/icons/icon/settings.svg"));
    ~HeaderDialog() override;

    void setMainTitle(const QString &title);
    void setSubTitle(const QString &subtitle);

    /// Appends content below the titles.
    void addContentWidget(QWidget *content);

    void hideCancelButton();
    void hideOkButton();

protected:
    Ui::SettingsDialog *ui() const { return m_ui.get(); }

private:
    std::unique_ptr<Ui::SettingsDialog> m_ui;
};

} // namespace duifx

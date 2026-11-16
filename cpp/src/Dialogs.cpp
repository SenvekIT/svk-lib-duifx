#include "duifx/Dialogs.h"

#include "duifx/Utils.h"

#include "ui_info_dialog.h"
#include "ui_settings_dialog.h"

#include <QPushButton>

namespace duifx {

// --- InfoDialog ---------------------------------------------------------------------

InfoDialog::InfoDialog(QWidget *parent, const QString &windowTitle)
    : QDialog(parent)
    , m_ui(std::make_unique<Ui::InfoDialog>())
{
    initResources();
    m_ui->setupUi(this);
    setWindowTitle(windowTitle);
    setWindowIcon(QIcon(QStringLiteral(":/icons/icon/info.svg")));

    m_ui->labelMainTitle->setProperty("class", QStringLiteral("title"));
    m_ui->labelSubTitle->setProperty("class", QStringLiteral("subtitle"));
    m_ui->textEditInfo->setReadOnly(true);

    setStyleSheet(loadStylesheet());
    resize(400, 400);
}

InfoDialog::~InfoDialog() = default;

void InfoDialog::setMainTitle(const QString &title)
{
    m_ui->labelMainTitle->setText(title);
}

void InfoDialog::setSubTitle(const QString &subtitle)
{
    m_ui->labelSubTitle->setText(subtitle);
}

void InfoDialog::setInfoText(const QString &text)
{
    m_ui->textEditInfo->setPlainText(text);
}

// --- HeaderDialog -------------------------------------------------------------------

HeaderDialog::HeaderDialog(QWidget *parent, const QString &windowTitle, const QString &iconPath)
    : QDialog(parent)
    , m_ui(std::make_unique<Ui::SettingsDialog>())
{
    initResources();
    m_ui->setupUi(this);
    setWindowTitle(windowTitle);
    setWindowIcon(QIcon(iconPath));
    setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint);

    m_ui->labelMainTitle->setProperty("class", QStringLiteral("title"));
    m_ui->labelSubTitle->setProperty("class", QStringLiteral("subtitle"));

    setStyleSheet(loadStylesheet());

    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

HeaderDialog::~HeaderDialog() = default;

void HeaderDialog::setMainTitle(const QString &title)
{
    m_ui->labelMainTitle->setText(title);
}

void HeaderDialog::setSubTitle(const QString &subtitle)
{
    m_ui->labelSubTitle->setText(subtitle);
}

void HeaderDialog::addContentWidget(QWidget *content)
{
    m_ui->contentLayout->addWidget(content);
}

void HeaderDialog::hideCancelButton()
{
    m_ui->buttonBox->button(QDialogButtonBox::Cancel)->hide();
}

void HeaderDialog::hideOkButton()
{
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->hide();
}

} // namespace duifx

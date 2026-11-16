#include "duifx/PathPicker.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>

namespace duifx {

namespace {

// str(Path(p)) / os.path.normpath(p): clean, with the platform's separators.
QString normalised(const QString &path)
{
    return QDir::toNativeSeparators(QDir::cleanPath(path));
}

} // namespace

PathPicker::PathPicker(Mode mode, QWidget *parent)
    : PathPicker(mode, Options{}, parent)
{
}

PathPicker::PathPicker(Mode mode, const Options &options, QWidget *parent)
    : QWidget(parent)
    , m_mode(mode)
    , m_caption(options.caption)
    , m_filter(options.fileFilter)
    , m_startDir(options.startDir)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    m_edit = new QLineEdit;
    m_edit->setPlaceholderText(options.placeholder);
    connect(m_edit, &QLineEdit::editingFinished, this, [this] { commit(path()); });
    layout->addWidget(m_edit, 1);

    m_button = new QPushButton(options.buttonText);
    connect(m_button, &QPushButton::clicked, this, &PathPicker::browse);
    layout->addWidget(m_button);
}

QString PathPicker::path() const
{
    return m_edit->text().trimmed();
}

void PathPicker::setPath(const QString &path)
{
    m_edit->setText(path);
    commit(path.trimmed());
}

void PathPicker::setStartDir(const QString &folder)
{
    m_startDir = folder;
}

void PathPicker::setPlaceholder(const QString &text)
{
    m_edit->setPlaceholderText(text);
}

void PathPicker::browse()
{
    const QString start = dialogStart();
    QString chosen;
    switch (m_mode) {
    case Directory:
        chosen = QFileDialog::getExistingDirectory(
            this, m_caption.isEmpty() ? QStringLiteral("Choose folder") : m_caption, start);
        break;
    case OpenFile:
        chosen = QFileDialog::getOpenFileName(this, m_caption.isEmpty() ? QStringLiteral("Open file") : m_caption,
                                              start, m_filter);
        break;
    case SaveFile:
        chosen = QFileDialog::getSaveFileName(this, m_caption.isEmpty() ? QStringLiteral("Save as") : m_caption,
                                              start, m_filter);
        break;
    }
    if (!chosen.isEmpty())
        setPath(normalised(chosen));
}

QString PathPicker::dialogStart() const
{
    const QString current = path();
    if (!current.isEmpty()) {
        const QString cleaned = QDir::cleanPath(QDir::fromNativeSeparators(current));
        const QFileInfo info(cleaned);
        if (info.exists() && (m_mode != Directory || info.isDir()))
            return normalised(cleaned);
        // The nearest existing folder above it ("." -- a bare relative name -- does not count).
        QString child = cleaned;
        while (true) {
            const QString parent = QFileInfo(child).path();
            if (parent == child)
                break;
            child = parent;
            if (parent.isEmpty() || parent == QLatin1String("."))
                continue;
            if (QFileInfo(parent).isDir()) {
                if (m_mode == SaveFile)
                    return normalised(parent + QLatin1Char('/') + info.fileName());
                return normalised(parent);
            }
        }
    }
    if (!m_startDir.isEmpty() && QFileInfo(m_startDir).isDir())
        return m_startDir;
    return normalised(QDir::homePath());
}

void PathPicker::commit(const QString &path)
{
    if (path != m_last) {
        m_last = path;
        emit pathChanged(path);
    }
}

} // namespace duifx

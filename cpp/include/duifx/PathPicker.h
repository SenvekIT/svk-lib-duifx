#pragma once

// A path in a line edit with a "Browse…" button beside it. Port of
// duifx/gui/path_picker.py.
//
// The path can be typed or pasted, or chosen in the platform's file dialog,
// which opens where the current path is (or the nearest folder of it that
// exists, or a start folder). pathChanged is emitted once per real change --
// after a choice in the dialog, when editing ends with another text than
// before, or from setPath() -- not on every keystroke.
//
//     auto *picker = new duifx::PathPicker(duifx::PathPicker::Directory, {.caption = QStringLiteral("Recordings folder")});
//     picker->setPath(settings->value("recordings_dir", fallback).toString());
//     connect(picker, &duifx::PathPicker::pathChanged, this, [&](const QString &p) { settings->setValue("recordings_dir", p); });
//
//     new duifx::PathPicker(duifx::PathPicker::OpenFile, {.caption = QStringLiteral("Firmware"), .fileFilter = QStringLiteral("Firmware (*.bin)")});
//     new duifx::PathPicker(duifx::PathPicker::SaveFile, {.caption = QStringLiteral("Export to"), .fileFilter = QStringLiteral("CSV (*.csv)")});

#include <QString>
#include <QWidget>

class QLineEdit;
class QPushButton;

namespace duifx {

class PathPicker : public QWidget
{
    Q_OBJECT

public:
    /// What the dialog chooses. Python: PathPicker.DIRECTORY / OPEN_FILE / SAVE_FILE.
    enum Mode {
        Directory,
        OpenFile,
        SaveFile,
    };
    Q_ENUM(Mode)

    struct Options
    {
        /// Dialog title; empty for "Choose folder" / "Open file" / "Save as".
        QString caption;
        /// File dialog name filter, e.g. "Firmware (*.bin)".
        QString fileFilter;
        QString placeholder;
        QString buttonText = QStringLiteral("Browse\u2026");
        /// Where the dialog opens while the path is empty or nowhere on disk.
        QString startDir;
    };

    explicit PathPicker(Mode mode = Directory, QWidget *parent = nullptr);
    PathPicker(Mode mode, const Options &options, QWidget *parent = nullptr);

    Mode mode() const { return m_mode; }
    QLineEdit *lineEdit() const { return m_edit; }
    QPushButton *button() const { return m_button; }

    /// The path as shown, whitespace stripped.
    QString path() const;

    /// Shows path; emits pathChanged when it differs from before.
    void setPath(const QString &path);

    /// Where the dialog opens while the path is empty or nowhere on disk.
    void setStartDir(const QString &folder);

    void setPlaceholder(const QString &text);

    /// The current path where it exists (for a file: the file itself, or its
    /// folder), else its nearest existing folder (for SaveFile with the file
    /// name appended), else the start folder, else the home folder.
    QString dialogStart() const;

public slots:
    /// Opens the dialog; a choice becomes the path (native separators).
    void browse();

signals:
    /// The path changed (by the dialog, by editing, or by setPath()).
    void pathChanged(const QString &path);

private:
    void commit(const QString &path);

    Mode m_mode;
    QString m_caption;
    QString m_filter;
    QString m_startDir;
    QString m_last;
    QLineEdit *m_edit = nullptr;
    QPushButton *m_button = nullptr;
};

} // namespace duifx

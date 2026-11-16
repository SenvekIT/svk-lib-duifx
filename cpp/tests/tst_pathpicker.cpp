// PathPicker: one pathChanged per real change, where the dialog opens, browse().

#include "TestSupport.h"

#include <duifx/PathPicker.h>

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

using namespace duifx;

namespace {

QString native(const QString &path)
{
    return QDir::toNativeSeparators(QDir::cleanPath(path));
}

} // namespace

class TstPathPicker : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() { duifx_test::isolateSettings(); }

    void widgetsAndOptions()
    {
        PathPicker plain;
        QCOMPARE(plain.mode(), PathPicker::Directory);
        QCOMPARE(plain.button()->text(), QStringLiteral("Browse\u2026"));
        QCOMPARE(plain.path(), QString());

        PathPicker::Options options;
        options.placeholder = QStringLiteral("Pick one");
        options.buttonText = QStringLiteral("\u2026");
        PathPicker save(PathPicker::SaveFile, options);
        QCOMPARE(save.mode(), PathPicker::SaveFile);
        QCOMPARE(save.button()->text(), QStringLiteral("\u2026"));
        QCOMPARE(save.lineEdit()->placeholderText(), QStringLiteral("Pick one"));
        save.setPlaceholder(QStringLiteral("Other"));
        QCOMPARE(save.lineEdit()->placeholderText(), QStringLiteral("Other"));
    }

    void pathChangedOncePerRealChange()
    {
        PathPicker picker;
        QSignalSpy changed(&picker, &PathPicker::pathChanged);

        picker.setPath(QStringLiteral("  /data/a  "));
        QCOMPARE(picker.path(), QStringLiteral("/data/a"));
        QCOMPARE(changed.count(), 1);
        QCOMPARE(changed.last().at(0).toString(), QStringLiteral("/data/a"));
        picker.setPath(QStringLiteral("/data/a"));
        QCOMPARE(changed.count(), 1);

        // Typing does not emit; finishing the edit does, once.
        picker.lineEdit()->setText(QStringLiteral("/data/b"));
        QCOMPARE(changed.count(), 1);
        emit picker.lineEdit()->editingFinished();
        QCOMPARE(changed.count(), 2);
        QCOMPARE(changed.last().at(0).toString(), QStringLiteral("/data/b"));
        emit picker.lineEdit()->editingFinished();
        QCOMPARE(changed.count(), 2);

        picker.setPath(QString());
        QCOMPARE(changed.count(), 3);
        QCOMPARE(changed.last().at(0).toString(), QString());
    }

    void dialogStartFollowsThePath()
    {
        QTemporaryDir dir;
        const QString folder = dir.path();
        const QString file = dir.filePath(QStringLiteral("data.bin"));
        QFile out(file);
        QVERIFY(out.open(QIODevice::WriteOnly));
        out.close();

        PathPicker directory(PathPicker::Directory);
        QCOMPARE(directory.dialogStart(), native(QDir::homePath()));
        directory.setStartDir(folder);
        QCOMPARE(directory.dialogStart(), folder);
        directory.setStartDir(dir.filePath(QStringLiteral("missing")));
        QCOMPARE(directory.dialogStart(), native(QDir::homePath()));
        directory.setPath(folder);
        QCOMPARE(directory.dialogStart(), native(folder));
        // A file is no folder: its folder.
        directory.setPath(file);
        QCOMPARE(directory.dialogStart(), native(folder));
        directory.setPath(dir.filePath(QStringLiteral("a/b/c")));
        QCOMPARE(directory.dialogStart(), native(folder));

        PathPicker open(PathPicker::OpenFile);
        open.setPath(file);
        QCOMPARE(open.dialogStart(), native(file));
        open.setPath(dir.filePath(QStringLiteral("gone/fw.bin")));
        QCOMPARE(open.dialogStart(), native(folder));

        // Saving keeps the file name for a path whose folder exists.
        PathPicker save(PathPicker::SaveFile);
        save.setPath(dir.filePath(QStringLiteral("new/export.csv")));
        QCOMPARE(save.dialogStart(), native(folder + QStringLiteral("/export.csv")));
        save.setPath(file);
        QCOMPARE(save.dialogStart(), native(file));

        // A relative name whose only parent is "." falls back.
        save.setPath(QStringLiteral("nowhere-at-all-xyz.csv"));
        QCOMPARE(save.dialogStart(), native(QDir::homePath()));
    }

    void browseTakesTheChoice()
    {
        QTemporaryDir dir;
        const QString file = dir.filePath(QStringLiteral("fw.bin"));
        QFile out(file);
        QVERIFY(out.open(QIODevice::WriteOnly));
        out.close();

        PathPicker::Options options;
        options.caption = QStringLiteral("Firmware");
        options.fileFilter = QStringLiteral("Firmware (*.bin)");
        PathPicker picker(PathPicker::OpenFile, options);
        picker.setStartDir(dir.path());
        QSignalSpy changed(&picker, &PathPicker::pathChanged);

        // Without a native dialog (offscreen) the Qt file dialog opens; accept it.
        QString caption;
        auto *poll = new QTimer(&picker);
        connect(poll, &QTimer::timeout, poll, [poll, &caption, file] {
            auto *dialog = qobject_cast<QFileDialog *>(QApplication::activeModalWidget());
            if (!dialog)
                return;
            poll->stop();
            caption = dialog->windowTitle();
            dialog->selectFile(file);
            static_cast<QDialog *>(dialog)->accept(); // QFileDialog::accept() is protected
        });
        poll->start(20);
        picker.button()->click();

        QCOMPARE(caption, QStringLiteral("Firmware"));
        QCOMPARE(changed.count(), 1);
        QCOMPARE(picker.path(), native(file));
    }
};

QTEST_MAIN(TstPathPicker)
#include "tst_pathpicker.moc"

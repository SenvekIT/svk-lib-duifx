// Terminal widgets: PuTTY style, style sheets, tab stops and the terminal
// semantics of TerminalView::appendText (CR, LF, BS, unfinished lines).

#include "TestSupport.h"

#include <duifx/Terminal.h>

#include <QFontMetrics>
#include <QLabel>
#include <QScrollBar>
#include <QTextEdit>
#include <QtTest>

using namespace duifx;

namespace {

const QString kFamilies = QStringLiteral("\"Courier New\", \"Consolas\", \"DejaVu Sans Mono\", monospace");

qreal expectedTabStop(const QWidget &widget)
{
    return widget.fontMetrics().horizontalAdvance(QLatin1Char(' ')) * kTabColumns;
}

class OwnLineEdit : public QLineEdit
{
};

} // namespace

class TstTerminal : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() { duifx_test::isolateSettings(); }

    void defaultStyleIsPutty()
    {
        const TerminalStyle style = defaultTerminalStyle();
        QCOMPARE(style, TerminalStyle());
        QCOMPARE(style.fontFamilies.first(), QStringLiteral("Courier New"));
        QCOMPARE(style.pointSize, 10);
        QCOMPARE(style.foreground, QStringLiteral("#bbbbbb"));
        QCOMPARE(style.background, QStringLiteral("#000000"));
        QCOMPARE(style.selectionForeground, QStringLiteral("#000000"));
        QCOMPARE(style.selectionBackground, QStringLiteral("#bbbbbb"));
        QCOMPARE(style.border, QStringLiteral("#3c3c3c"));
        QCOMPARE(style.paddingPx, 3);
    }

    void fontAndStylesheet()
    {
        const QFont font = terminalFont();
        QCOMPARE(font.families(), (QStringList{QStringLiteral("Courier New"), QStringLiteral("Consolas"),
                                                QStringLiteral("DejaVu Sans Mono")}));
        QVERIFY(font.fixedPitch());
        QCOMPARE(font.styleHint(), QFont::TypeWriter);
        QCOMPARE(font.pointSize(), 10);

        // Character for character the Python style sheet.
        QCOMPARE(terminalStylesheet(QStringLiteral("QPlainTextEdit")),
                 QStringLiteral("QPlainTextEdit { font-family: ") + kFamilies
                     + QStringLiteral("; font-size: 10pt; color: #bbbbbb; background-color: #000000;"
                                      " selection-color: #000000; selection-background-color: #bbbbbb;"
                                      " border: 1px solid #3c3c3c; border-radius: 0px; padding: 3px; }"
                                      " QPlainTextEdit:focus { border: 1px solid #3c3c3c; }"
                                      " QPlainTextEdit:disabled { color: #bbbbbb; background-color: #000000; }"));

        TerminalStyle custom;
        custom.pointSize = 12;
        custom.foreground = QStringLiteral("#00ff00");
        QVERIFY(terminalStylesheet(QStringLiteral("QLineEdit"), custom).contains(QStringLiteral("font-size: 12pt; color: #00ff00;")));
        QCOMPARE(terminalFont(custom).pointSize(), 12);
    }

    void defaultStyleCanBeChanged()
    {
        TerminalStyle style = defaultTerminalStyle();
        style.pointSize = 11;
        QCOMPARE(setDefaultTerminalStyle(style).pointSize, 11);
        QCOMPARE(defaultTerminalStyle().pointSize, 11);
        TerminalView view;
        QVERIFY(view.styleSheet().contains(QStringLiteral("font-size: 11pt;")));
        // An explicit style wins over the default.
        TerminalLineEdit edit(nullptr, TerminalStyle());
        QVERIFY(edit.styleSheet().contains(QStringLiteral("font-size: 10pt;")));
        setDefaultTerminalStyle(TerminalStyle());
        QCOMPARE(defaultTerminalStyle(), TerminalStyle());
    }

    void applyToOtherWidgets()
    {
        QTextEdit text;
        applyTerminalStyle(&text);
        QVERIFY(text.styleSheet().startsWith(QStringLiteral("QTextEdit {")));
        text.ensurePolished();
        QCOMPARE(text.tabStopDistance(), expectedTabStop(text));

        OwnLineEdit own; // a subclass is styled by its Qt class
        applyTerminalStyle(&own);
        QVERIFY(own.styleSheet().startsWith(QStringLiteral("QLineEdit {")));

        QLabel label;
        applyTerminalStyle(&label);
        QVERIFY(label.styleSheet().startsWith(QStringLiteral("QLabel {")));

        TerminalLineEdit entry;
        QVERIFY(entry.styleSheet().startsWith(QStringLiteral("QLineEdit {")));
        QVERIFY(entry.font().fixedPitch());

        // Fixed pitch only: no colours.
        QLabel readout;
        applyFixedPitch(&readout, 0, true);
        QCOMPARE(readout.styleSheet(),
                 QStringLiteral("font-family: ") + kFamilies + QStringLiteral("; font-size: 10pt; font-weight: bold;"));
        QVERIFY(readout.font().bold());
        applyFixedPitch(&readout, 14);
        QCOMPARE(readout.styleSheet(), QStringLiteral("font-family: ") + kFamilies + QStringLiteral("; font-size: 14pt;"));
        readout.ensurePolished(); // the style sheet's font applies on polish
        QCOMPARE(readout.font().pointSize(), 14);
    }

    void viewDefaults()
    {
        TerminalView view;
        QVERIFY(view.isReadOnly());
        QVERIFY(!view.isUndoRedoEnabled());
        QCOMPARE(view.maximumBlockCount(), 5000);
        QCOMPARE(view.lineWrapMode(), QPlainTextEdit::WidgetWidth);
        QCOMPARE(view.wordWrapMode(), QTextOption::WrapAnywhere);
        QVERIFY(view.autoscroll());
        QVERIFY(view.styleSheet().startsWith(QStringLiteral("QPlainTextEdit {")));
        view.ensurePolished();
        QCOMPARE(view.tabStopDistance(), expectedTabStop(view));

        // Tab stops follow a new font size.
        TerminalStyle big;
        big.pointSize = 20;
        view.setStyleSheet(terminalStylesheet(QStringLiteral("QPlainTextEdit"), big));
        view.ensurePolished();
        QCOMPARE(view.tabStopDistance(), expectedTabStop(view));

        TerminalView::Options options;
        options.maxLines = 3;
        options.wrap = false;
        TerminalView limited(options);
        QCOMPARE(limited.lineWrapMode(), QPlainTextEdit::NoWrap);
        for (int i = 0; i < 10; ++i)
            limited.appendText(QStringLiteral("line %1\n").arg(i));
        QVERIFY(limited.blockCount() <= 3);
        QVERIFY(limited.toPlainText().endsWith(QStringLiteral("line 9\n")));
    }

    void appendTextLikeATerminal()
    {
        TerminalView view;
        view.appendText(QStringLiteral("$ "));
        QCOMPARE(view.toPlainText(), QStringLiteral("$ ")); // unfinished line shows at once
        view.appendText(QStringLiteral("ls\r\nfile\tsize\n"));
        QCOMPARE(view.toPlainText(), QStringLiteral("$ ls\nfile\tsize\n"));
        view.appendText(QString());
        QCOMPARE(view.blockCount(), 3);

        // A lone CR overwrites the line.
        view.clear();
        view.appendText(QStringLiteral("progress 10%\rprogress 20%"));
        QCOMPARE(view.toPlainText(), QStringLiteral("progress 20%"));
        view.appendText(QStringLiteral("\rab"));
        QCOMPARE(view.toPlainText(), QStringLiteral("abogress 20%"));
        view.appendText(QStringLiteral("\n"));
        view.appendText(QStringLiteral("next"));
        QCOMPARE(view.toPlainText(), QStringLiteral("abogress 20%\nnext"));

        // Overwriting past the end of the line appends.
        view.clear();
        view.appendText(QStringLiteral("abc\rXYZW"));
        QCOMPARE(view.toPlainText(), QStringLiteral("XYZW"));
        view.appendText(QStringLiteral("!"));
        QCOMPARE(view.toPlainText(), QStringLiteral("XYZW!"));

        // CR and LF in separate chunks still make one line end.
        view.clear();
        view.appendText(QStringLiteral("a\r"));
        view.appendText(QStringLiteral("\nb"));
        QCOMPARE(view.toPlainText(), QStringLiteral("a\nb"));

        // BS erases within the line, never the line break.
        view.clear();
        view.appendText(QStringLiteral("abc\b\bX"));
        QCOMPARE(view.toPlainText(), QStringLiteral("aX"));
        view.appendText(QStringLiteral("\n\b\b"));
        QCOMPARE(view.toPlainText(), QStringLiteral("aX\n"));
    }

    void appendLineStartsOnItsOwnLine()
    {
        TerminalView view;
        view.appendText(QStringLiteral("$ "));
        view.appendLine(QStringLiteral("[disconnected]"));
        QCOMPARE(view.toPlainText(), QStringLiteral("$ \n[disconnected]\n"));
        view.appendLine(QStringLiteral("[connected]"));
        QCOMPARE(view.toPlainText(), QStringLiteral("$ \n[disconnected]\n[connected]\n"));
    }

    void followsTheEndOnlyAtTheBottom()
    {
        TerminalView view;
        view.resize(300, 120);
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        QScrollBar *bar = view.verticalScrollBar();
        for (int i = 0; i < 100; ++i)
            view.appendText(QStringLiteral("line %1\n").arg(i));
        QVERIFY(bar->maximum() > 0);
        QCOMPARE(bar->value(), bar->maximum());

        // Scrolled back: stays.
        bar->setValue(0);
        view.appendText(QStringLiteral("more\n"));
        QCOMPARE(bar->value(), 0);

        // Switching autoscroll on jumps to the end.
        view.setAutoscroll(true);
        QCOMPARE(bar->value(), bar->maximum());

        // Off: stays even at the bottom.
        view.setAutoscroll(false);
        const int before = bar->value();
        for (int i = 0; i < 20; ++i)
            view.appendText(QStringLiteral("x\n"));
        QCOMPARE(bar->value(), before);
        QVERIFY(bar->value() < bar->maximum());
    }
};

QTEST_MAIN(TstTerminal)
#include "tst_terminal.moc"

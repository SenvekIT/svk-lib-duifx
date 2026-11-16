#include "duifx/Terminal.h"

#include <QEvent>
#include <QFontMetrics>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextEdit>
#include <QTextOption>

#include <algorithm>
#include <utility>

namespace duifx {

namespace {

const QString kMonospace = QStringLiteral("monospace");

TerminalStyle &defaultStyleStorage()
{
    static TerminalStyle style;
    return style;
}

const TerminalStyle &resolve(const std::optional<TerminalStyle> &style)
{
    return style ? *style : defaultStyleStorage();
}

// CSS font-family list: names quoted, the generic "monospace" not.
QString cssFamilies(const TerminalStyle &style)
{
    QStringList families;
    for (const QString &family : style.fontFamilies)
        families << (family == kMonospace ? family : QLatin1Char('"') + family + QLatin1Char('"'));
    return families.join(QStringLiteral(", "));
}

// Stylesheet type selector of widget: the Qt text widget it is, else its own
// class ("ns::Name" is written "ns--Name" in a stylesheet).
QString widgetTypeName(const QWidget *widget)
{
    for (const char *type : {"QPlainTextEdit", "QTextEdit", "QLineEdit"}) {
        if (widget->inherits(type))
            return QString::fromLatin1(type);
    }
    return QString::fromLatin1(widget->metaObject()->className()).replace(QStringLiteral("::"), QStringLiteral("--"));
}

} // namespace

TerminalStyle defaultTerminalStyle()
{
    return defaultStyleStorage();
}

TerminalStyle setDefaultTerminalStyle(const TerminalStyle &style)
{
    defaultStyleStorage() = style;
    return style;
}

QFont terminalFont(const std::optional<TerminalStyle> &style)
{
    const TerminalStyle &s = resolve(style);
    QStringList families = s.fontFamilies;
    families.removeAll(kMonospace);
    QFont font;
    font.setFamilies(families);
    font.setStyleHint(QFont::TypeWriter);
    font.setFixedPitch(true);
    font.setPointSize(s.pointSize);
    return font;
}

QString terminalStylesheet(const QString &widgetType, const std::optional<TerminalStyle> &style)
{
    const TerminalStyle &s = resolve(style);
    return QStringLiteral("%1 { font-family: %2; font-size: %3pt; color: %4; background-color: %5;"
                          " selection-color: %6; selection-background-color: %7; border: 1px solid %8;"
                          " border-radius: 0px; padding: %9px; }")
               .arg(widgetType, cssFamilies(s), QString::number(s.pointSize), s.foreground, s.background,
                    s.selectionForeground, s.selectionBackground, s.border, QString::number(s.paddingPx))
        + QStringLiteral(" %1:focus { border: 1px solid %2; }").arg(widgetType, s.border)
        + QStringLiteral(" %1:disabled { color: %2; background-color: %3; }").arg(widgetType, s.foreground, s.background);
}

void applyTerminalStyle(QWidget *widget, const std::optional<TerminalStyle> &style)
{
    if (!widget)
        return;
    const TerminalStyle &s = resolve(style);
    widget->setFont(terminalFont(s));
    widget->setStyleSheet(terminalStylesheet(widgetTypeName(widget), s));
    applyTerminalTabStops(widget);
}

void applyFixedPitch(QWidget *widget, int pointSize, bool bold, const std::optional<TerminalStyle> &style)
{
    if (!widget)
        return;
    const TerminalStyle &s = resolve(style);
    const int size = pointSize > 0 ? pointSize : s.pointSize;
    QFont font = terminalFont(s);
    font.setPointSize(size);
    font.setBold(bold);
    widget->setFont(font);
    widget->setStyleSheet(QStringLiteral("font-family: %1; font-size: %2pt;").arg(cssFamilies(s)).arg(size)
                          + (bold ? QStringLiteral(" font-weight: bold;") : QString()));
    applyTerminalTabStops(widget);
}

void applyTerminalTabStops(QWidget *widget)
{
    if (auto *plain = qobject_cast<QPlainTextEdit *>(widget))
        plain->setTabStopDistance(plain->fontMetrics().horizontalAdvance(QLatin1Char(' ')) * kTabColumns);
    else if (auto *rich = qobject_cast<QTextEdit *>(widget))
        rich->setTabStopDistance(rich->fontMetrics().horizontalAdvance(QLatin1Char(' ')) * kTabColumns);
}

// --- TerminalView ---------------------------------------------------------------------

TerminalView::TerminalView(QWidget *parent)
    : TerminalView(Options{}, parent)
{
}

TerminalView::TerminalView(const Options &options, QWidget *parent)
    : QPlainTextEdit(parent)
{
    setReadOnly(true);
    setUndoRedoEnabled(false);
    setMaximumBlockCount(options.maxLines);
    setWrapping(options.wrap);
    applyTerminalStyle(this, options.style);
}

void TerminalView::setAutoscroll(bool on)
{
    m_autoscroll = on;
    if (m_autoscroll)
        scrollToEnd();
}

void TerminalView::setWrapping(bool wrap)
{
    if (wrap) {
        setLineWrapMode(QPlainTextEdit::WidgetWidth);
        setWordWrapMode(QTextOption::WrapAnywhere);
    } else {
        setLineWrapMode(QPlainTextEdit::NoWrap);
    }
}

void TerminalView::scrollToEnd()
{
    QScrollBar *bar = verticalScrollBar();
    bar->setValue(bar->maximum());
}

void TerminalView::appendText(const QString &text)
{
    if (text.isEmpty())
        return;
    const QScrollBar *bar = verticalScrollBar();
    const bool follow = m_autoscroll && bar->value() >= bar->maximum() - 2;
    QTextCursor cursor(document());
    cursor.movePosition(QTextCursor::End);
    cursor.beginEditBlock();
    QString run;
    QString normalised = text;
    normalised.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    for (const QChar ch : std::as_const(normalised)) {
        if (ch == QLatin1Char('\n')) {
            flush(cursor, run);
            cursor.movePosition(QTextCursor::End);
            cursor.insertBlock();
            m_overwriteFrom = -1;
        } else if (ch == QLatin1Char('\r')) {
            flush(cursor, run);
            m_overwriteFrom = 0;
        } else if (ch == QLatin1Char('\b')) {
            flush(cursor, run);
            cursor.movePosition(QTextCursor::End);
            if (cursor.positionInBlock() > 0)
                cursor.deletePreviousChar();
        } else {
            run.append(ch);
        }
    }
    flush(cursor, run);
    cursor.endEditBlock();
    if (follow)
        scrollToEnd();
}

void TerminalView::appendLine(const QString &line)
{
    QTextCursor cursor(document());
    cursor.movePosition(QTextCursor::End);
    const QString prefix = cursor.block().length() > 1 ? QStringLiteral("\n") : QString();
    appendText(prefix + line + QLatin1Char('\n'));
}

void TerminalView::clear()
{
    QPlainTextEdit::clear();
    m_overwriteFrom = -1;
}

void TerminalView::flush(QTextCursor &cursor, QString &run)
{
    if (run.isEmpty())
        return;
    const QString chunk = std::exchange(run, {});
    if (m_overwriteFrom >= 0) {
        // After a lone CR: replace the line from that column on.
        const QTextBlock block = document()->lastBlock();
        const int lineLength = block.length() - 1; // without the block separator
        cursor.setPosition(block.position() + std::min(m_overwriteFrom, lineLength));
        const int end = std::min(block.position() + lineLength, cursor.position() + int(chunk.size()));
        cursor.setPosition(end, QTextCursor::KeepAnchor);
        cursor.insertText(chunk);
        m_overwriteFrom += int(chunk.size());
        // The line is overwritten to its end: what follows is appended.
        if (m_overwriteFrom >= document()->lastBlock().length() - 1)
            m_overwriteFrom = -1;
    } else {
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(chunk);
    }
}

void TerminalView::changeEvent(QEvent *event)
{
    QPlainTextEdit::changeEvent(event);
    if (event->type() == QEvent::FontChange || event->type() == QEvent::StyleChange)
        applyTerminalTabStops(this);
}

// --- TerminalLineEdit -----------------------------------------------------------------

TerminalLineEdit::TerminalLineEdit(QWidget *parent, const std::optional<TerminalStyle> &style)
    : QLineEdit(parent)
{
    applyTerminalStyle(this, style);
}

} // namespace duifx

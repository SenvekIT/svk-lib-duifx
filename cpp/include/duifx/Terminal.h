#pragma once

// A text console that looks and lines up like a real terminal (PuTTY). Port of
// duifx/gui/terminal.py.
//
// Why widgets of their own: the application stylesheet sets font-family and
// font-size on "*", and a stylesheet beats QWidget::setFont(). A
// QPlainTextEdit given Consolas therefore still renders in the proportional UI
// font, and column-aligned output comes out ragged. These widgets carry a
// style sheet of their own, which Qt prefers to the inherited application one,
// so they keep a fixed-pitch font whatever the application stylesheet says.
//
// What makes it a terminal and not a text box:
//  - PuTTY's look by default: Courier New 10 pt, light grey on black, a
//    selection that inverts the colours (TerminalStyle);
//  - tab stops every 8 columns;
//  - output is written as it arrives, an unfinished line (a prompt such as
//    "$ ") included; CR LF and LF end a line, a lone CR returns to the start
//    of the line and overwrites it, BS erases the character before;
//  - lines wrap at the window edge anywhere, not at word boundaries -- or not
//    at all (wrap = false) with a horizontal scroll bar;
//  - it follows the end of the output while the view is at the bottom, and
//    stays where it is while the user scrolls back.
//
//     auto *view = new duifx::TerminalView;
//     auto *entry = new duifx::TerminalLineEdit;
//     connect(port, &Port::textReceived, view, &duifx::TerminalView::appendText);

#include <QFont>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QString>
#include <QStringList>
#include <QTextCursor>

#include <optional>

namespace duifx {

/// Columns between two tab stops, as in a VT100 / PuTTY.
inline constexpr int kTabColumns = 8;

/// Font and colours of the terminal widgets. The defaults are PuTTY's.
struct TerminalStyle
{
    /// In order of preference; "monospace" stands for the system's fixed-pitch font.
    QStringList fontFamilies{QStringLiteral("Courier New"), QStringLiteral("Consolas"),
                             QStringLiteral("DejaVu Sans Mono"), QStringLiteral("monospace")};
    int pointSize = 10;
    QString foreground = QStringLiteral("#bbbbbb");
    QString background = QStringLiteral("#000000");
    QString selectionForeground = QStringLiteral("#000000");
    QString selectionBackground = QStringLiteral("#bbbbbb");
    QString border = QStringLiteral("#3c3c3c");
    int paddingPx = 3;

    bool operator==(const TerminalStyle &) const = default;
};

/// The style new terminal widgets take.
TerminalStyle defaultTerminalStyle();

/// Sets the style for terminal widgets created from now on and returns it.
/// To change single fields (Python's set_default_terminal_style(point_size=11)):
///     auto style = duifx::defaultTerminalStyle(); style.pointSize = 11;
///     duifx::setDefaultTerminalStyle(style);
TerminalStyle setDefaultTerminalStyle(const TerminalStyle &style);

/// A fixed-pitch QFont for style (default: defaultTerminalStyle()): the first
/// of its families the system has, otherwise whatever monospace font Qt finds.
QFont terminalFont(const std::optional<TerminalStyle> &style = std::nullopt);

/// The style sheet for one widget class ("QPlainTextEdit", ...).
QString terminalStylesheet(const QString &widgetType, const std::optional<TerminalStyle> &style = std::nullopt);

/// Gives any text widget (a QPlainTextEdit, QTextEdit or QLineEdit made
/// elsewhere) the terminal font and colours.
void applyTerminalStyle(QWidget *widget, const std::optional<TerminalStyle> &style = std::nullopt);

/// Only the terminal's fixed-pitch font, not its colours: for a widget that
/// keeps the application look but must line up -- values in columns, a
/// readout. pointSize 0 takes the style's size. Replaces the widget's own
/// style sheet.
void applyFixedPitch(QWidget *widget, int pointSize = 0, bool bold = false,
                     const std::optional<TerminalStyle> &style = std::nullopt);

/// Tab stops every kTabColumns characters of the widget's font, for a
/// QPlainTextEdit or QTextEdit (other widgets are left alone). The two
/// functions above already do this.
void applyTerminalTabStops(QWidget *widget);

/// Read-only terminal output; see the header comment.
class TerminalView : public QPlainTextEdit
{
    Q_OBJECT
    Q_PROPERTY(bool autoscroll READ autoscroll WRITE setAutoscroll)

public:
    struct Options
    {
        /// Lines kept; the oldest are dropped beyond it (0 = no limit).
        int maxLines = 5000;
        /// Wrap at the window edge (anywhere, like a terminal) or not at all.
        bool wrap = true;
        /// Font and colours; defaultTerminalStyle() when not set.
        std::optional<TerminalStyle> style;
    };

    explicit TerminalView(QWidget *parent = nullptr);
    explicit TerminalView(const Options &options, QWidget *parent = nullptr);

    /// Follow the end of the output (only while the view is at the bottom:
    /// scrolling back keeps the view where the user put it). Switching it on
    /// scrolls to the end.
    bool autoscroll() const { return m_autoscroll; }
    void setAutoscroll(bool on);

    /// Wrap anywhere at the window edge, or not at all.
    void setWrapping(bool wrap);

    void scrollToEnd();

    /// A whole line of its own (a note of the application): starts on a new
    /// line if the output stopped in the middle of one.
    void appendLine(const QString &line);

public slots:
    /// Writes text at the end as a terminal would: unfinished lines show at
    /// once, CR LF / LF end a line, a lone CR returns to the line start, BS
    /// erases the character before.
    void appendText(const QString &text);

    /// Clears the output and the pending carriage return. Hides
    /// QPlainTextEdit::clear(); call it on a TerminalView.
    void clear();

protected:
    void changeEvent(QEvent *event) override;

private:
    void flush(QTextCursor &cursor, QString &run);

    bool m_autoscroll = true;
    // Column the next character overwrites after a lone CR, -1 = append.
    int m_overwriteFrom = -1;
};

/// A one-line input in the terminal's font and colours.
class TerminalLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit TerminalLineEdit(QWidget *parent = nullptr, const std::optional<TerminalStyle> &style = std::nullopt);
};

} // namespace duifx

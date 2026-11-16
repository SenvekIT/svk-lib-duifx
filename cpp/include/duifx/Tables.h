#pragma once

// Column widths the user can drag, sized to their content until they do.
// Port of duifx/gui/tables.py.
//
// ResizeToContents and Stretch give good widths but lock the columns;
// Interactive can be dragged but ignores the content. ColumnSizer combines
// the two:
//  - every section is Interactive, so every column can be dragged;
//  - each column is sized to its content (header included) whenever the
//    content changes -- throttled -- until the user drags that column;
//  - one column (the last by default) fills the width left over, like
//    Stretch, but stays draggable; once dragged it keeps its width too.
//
//     duifx::makeColumnsInteractive(table);                 // last column fills
//     duifx::makeColumnsInteractive(tree, {.fill = 1});     // the second one fills
//     duifx::makeColumnsInteractive(table, {.fill = std::nullopt}); // nothing fills
//
// Works for QTableView / QTableWidget and QTreeView / QTreeWidget.

#include <QObject>
#include <QPointer>
#include <QSet>

#include <optional>

class QAbstractItemModel;
class QAbstractItemView;
class QHeaderView;
class QTimer;

namespace duifx {

/// Content changes are measured at most this often.
inline constexpr int kDefaultFitIntervalMs = 250;

class ColumnSizer : public QObject
{
    Q_OBJECT

public:
    struct Options
    {
        /// Column taking the left-over width: an index, -1 for the last, nullopt for none.
        std::optional<int> fill = -1;
        /// Fit again whenever the model changes; without it call fit() yourself.
        bool autoFit = true;
        /// Upper limit of a width sized to content; nullopt for none. The user
        /// can still drag wider.
        std::optional<int> maxWidth = 400;
        /// Throttle of the automatic fit.
        int intervalMs = kDefaultFitIntervalMs;
    };

    /// Parented to view, so it lives exactly as long as the view.
    explicit ColumnSizer(QAbstractItemView *view, const Options &options);
    explicit ColumnSizer(QAbstractItemView *view);

    /// The columns the user has dragged; they keep their width.
    QSet<int> userSized() const { return m_user; }

    /// The column taking the left-over width, or nullopt.
    std::optional<int> fillColumn() const;

    /// Sizes every column the user has not dragged to its content now, then
    /// lets the fill column take what is left.
    void fit();

    /// Forgets the widths the user dragged and sizes everything afresh.
    void reset();

    /// Fits within the throttle interval (several calls, one fit).
    void scheduleFit();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void watchModel();
    void fillRest();
    void onSectionResized(int column);

    QPointer<QAbstractItemView> m_view;
    QHeaderView *m_header = nullptr;
    QObject *m_viewport = nullptr;
    Options m_options;
    QSet<int> m_user;
    bool m_busy = false;
    QPointer<QAbstractItemModel> m_model;
    QTimer *m_timer = nullptr;
};

/// Makes view's columns draggable, sized to their content until the user drags
/// them, with the fill column (default the last) taking the left-over width.
/// Returns the ColumnSizer (a child of view; keeping the pointer is optional).
ColumnSizer *makeColumnsInteractive(QAbstractItemView *view, const ColumnSizer::Options &options);
ColumnSizer *makeColumnsInteractive(QAbstractItemView *view);

} // namespace duifx

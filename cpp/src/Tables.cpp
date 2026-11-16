#include "duifx/Tables.h"

#include <algorithm>
#include <QAbstractItemModel>
#include <QApplication>
#include <QEvent>
#include <QHeaderView>
#include <QTableView>
#include <QTimer>
#include <QTreeView>

#include <stdexcept>

namespace duifx {

namespace {

QHeaderView *headerOf(QAbstractItemView *view)
{
    if (auto *table = qobject_cast<QTableView *>(view))
        return table->horizontalHeader();
    if (auto *tree = qobject_cast<QTreeView *>(view))
        return tree->header();
    throw std::invalid_argument("ColumnSizer: no column header on this view");
}

} // namespace

ColumnSizer::ColumnSizer(QAbstractItemView *view)
    : ColumnSizer(view, Options{})
{
}

ColumnSizer::ColumnSizer(QAbstractItemView *view, const Options &options)
    : QObject(view)
    , m_view(view)
    , m_header(headerOf(view))
    , m_viewport(view->viewport())
    , m_options(options)
{
    m_header->setStretchLastSection(false);
    m_header->setSectionResizeMode(QHeaderView::Interactive);
    connect(m_header, &QHeaderView::sectionResized, this, [this](int column, int, int) { onSectionResized(column); });
    m_viewport->installEventFilter(this);

    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    m_timer->setInterval(std::max(0, options.intervalMs));
    connect(m_timer, &QTimer::timeout, this, &ColumnSizer::fit);
    if (options.autoFit) {
        watchModel();
        // A view replacing its model (setModel) gets the new one watched.
        connect(m_header, &QHeaderView::sectionCountChanged, this, [this] { watchModel(); });
    }
    fit();
}

std::optional<int> ColumnSizer::fillColumn() const
{
    const int count = m_header->count();
    if (!m_options.fill || count == 0)
        return std::nullopt;
    const int column = *m_options.fill < 0 ? count - 1 : *m_options.fill;
    if (column >= 0 && column < count)
        return column;
    return std::nullopt;
}

void ColumnSizer::fit()
{
    m_timer->stop();
    if (!m_view)
        return;
    const std::optional<int> fill = fillColumn();
    m_busy = true;
    for (int column = 0; column < m_header->count(); ++column) {
        if (m_user.contains(column) || m_header->isSectionHidden(column))
            continue;
        int width = std::max(m_view->sizeHintForColumn(column), m_header->sectionSizeHint(column));
        if (m_options.maxWidth && column != fill)
            width = std::min(width, *m_options.maxWidth);
        width = std::max(width, m_header->minimumSectionSize());
        if (m_header->sectionSize(column) != width)
            m_header->resizeSection(column, width);
    }
    m_busy = false;
    fillRest();
}

void ColumnSizer::reset()
{
    m_user.clear();
    fit();
}

void ColumnSizer::scheduleFit()
{
    if (!m_timer->isActive())
        m_timer->start();
}

void ColumnSizer::watchModel()
{
    QAbstractItemModel *model = m_view ? m_view->model() : nullptr;
    if (!model || model == m_model)
        return;
    if (m_model)
        disconnect(m_model, nullptr, this, nullptr);
    m_model = model;
    connect(model, &QAbstractItemModel::modelReset, this, &ColumnSizer::scheduleFit);
    connect(model, &QAbstractItemModel::layoutChanged, this, &ColumnSizer::scheduleFit);
    connect(model, &QAbstractItemModel::rowsInserted, this, &ColumnSizer::scheduleFit);
    connect(model, &QAbstractItemModel::rowsRemoved, this, &ColumnSizer::scheduleFit);
    connect(model, &QAbstractItemModel::dataChanged, this, &ColumnSizer::scheduleFit);
    connect(model, &QAbstractItemModel::headerDataChanged, this, &ColumnSizer::scheduleFit);
}

void ColumnSizer::fillRest()
{
    // The fill column gets the width the others leave, at least its own
    // content width; a horizontal scroll bar takes over below that.
    const std::optional<int> column = fillColumn();
    if (!m_view || !column || m_user.contains(*column))
        return;
    int others = 0;
    for (int c = 0; c < m_header->count(); ++c) {
        if (c != *column && !m_header->isSectionHidden(c))
            others += m_header->sectionSize(c);
    }
    const int content = std::max({m_view->sizeHintForColumn(*column), m_header->sectionSizeHint(*column),
                                  m_header->minimumSectionSize()});
    const int width = std::max(content, m_view->viewport()->width() - others);
    if (m_header->sectionSize(*column) != width) {
        m_busy = true;
        m_header->resizeSection(*column, width);
        m_busy = false;
    }
}

void ColumnSizer::onSectionResized(int column)
{
    if (m_busy)
        return;
    // Only a drag counts as the user's choice -- not the view resizing a
    // section itself, nor a programmatic resize between two fits.
    if (QApplication::mouseButtons() & Qt::LeftButton) {
        m_user.insert(column);
        if (column != fillColumn())
            fillRest();
    }
}

bool ColumnSizer::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::Resize && watched == m_viewport && m_view)
        fillRest();
    return false;
}

ColumnSizer *makeColumnsInteractive(QAbstractItemView *view, const ColumnSizer::Options &options)
{
    return new ColumnSizer(view, options);
}

ColumnSizer *makeColumnsInteractive(QAbstractItemView *view)
{
    return new ColumnSizer(view);
}

} // namespace duifx

/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qsortfilterproxymodel.h"

#ifndef QT_NO_SORTFILTERPROXYMODEL

#include "qitemselectionmodel.h"
#include <qsize.h>
#include <qdebug.h>
#include <qdatetime.h>
#include <qpair.h>
#include <qstringlist.h>
#include <private/qabstractitemmodel_p.h>
#include <private/qabstractproxymodel_p.h>

class QSortFilterProxyModelLessThan
{
public:
    inline QSortFilterProxyModelLessThan(int column, const QModelIndex &parent,
                                       const QAbstractItemModel *source,
                                       const QSortFilterProxyModel *proxy)
        : sort_column(column), source_parent(parent), source_model(source), proxy_model(proxy) {}

    inline bool operator()(int r1, int r2) const
    {
        QModelIndex i1 = source_model->index(r1, sort_column, source_parent);
        QModelIndex i2 = source_model->index(r2, sort_column, source_parent);
        return proxy_model->lessThan(i1, i2);
    }

private:
    int sort_column;
    QModelIndex source_parent;
    const QAbstractItemModel *source_model;
    const QSortFilterProxyModel *proxy_model;
};

class QSortFilterProxyModelGreaterThan
{
public:
    inline QSortFilterProxyModelGreaterThan(int column, const QModelIndex &parent,
                                          const QAbstractItemModel *source,
                                          const QSortFilterProxyModel *proxy)
        : sort_column(column), source_parent(parent), source_model(source), proxy_model(proxy) {}

    inline bool operator()(int r1, int r2) const
    {
        QModelIndex i1 = source_model->index(r1, sort_column, source_parent);
        QModelIndex i2 = source_model->index(r2, sort_column, source_parent);
        return proxy_model->lessThan(i2, i1);
    }

private:
    int sort_column;
    QModelIndex source_parent;
    const QAbstractItemModel *source_model;
    const QSortFilterProxyModel *proxy_model;
};


class QSortFilterProxyModelPrivate : public QAbstractProxyModelPrivate
{
    Q_DECLARE_PUBLIC(QSortFilterProxyModel)

public:
    struct Mapping {
        QVector<int> source_rows;
        QVector<int> source_columns;
        QVector<int> proxy_rows;
        QVector<int> proxy_columns;
        QVector<QModelIndex> mapped_children;
    };

    mutable QMap<QModelIndex, Mapping*> source_index_mapping;

    int sort_column;
    Qt::SortOrder sort_order;
    Qt::CaseSensitivity sort_casesensitivity;
    int sort_role;

    int filter_column;
    QRegExp filter_regexp;
    int filter_role;

    bool dynamic_sortfilter;

    QMap<QModelIndex, Mapping *>::const_iterator create_mapping(
        const QModelIndex &source_parent) const;
    QModelIndex proxy_to_source(const QModelIndex &proxyIndex) const;
    QModelIndex source_to_proxy(const QModelIndex &sourceIndex) const;

    void remove_from_mapping(const QModelIndex &source_parent);

    inline QMap<QModelIndex, Mapping *>::const_iterator index_to_iterator(
        const QModelIndex &proxy_index) const
    {
        Q_ASSERT(proxy_index.isValid());
        const void *p = proxy_index.internalPointer();
        Q_ASSERT(p);
        QMap<QModelIndex, Mapping *>::const_iterator it =
            reinterpret_cast<QMap<QModelIndex, Mapping *>::const_iterator & >(p);
        Q_ASSERT(it != source_index_mapping.end());
        Q_ASSERT(it.value());
        return it;
    }

    inline QModelIndex create_index(int row, int column,
                                    QMap<QModelIndex, Mapping*>::const_iterator it) const
    {
        const void *p = static_cast<const void *>(it);
        return q_func()->createIndex(row, column, const_cast<void *>(p));
    }

    void _q_sourceDataChanged(const QModelIndex &source_top_left,
                           const QModelIndex &source_bottom_right);
    void _q_sourceHeaderDataChanged(Qt::Orientation orientation, int start, int end);

    void _q_sourceReset();

    void _q_sourceRowsAboutToBeInserted(const QModelIndex &source_parent, int start, int end);
    void _q_sourceRowsInserted(const QModelIndex &source_parent, int start, int end);
    void _q_sourceRowsAboutToBeRemoved(const QModelIndex &source_parent, int start, int end);
    void _q_sourceRowsRemoved(const QModelIndex &source_parent, int start, int end);
    void _q_sourceColumnsAboutToBeInserted(const QModelIndex &source_parent, int start, int end);
    void _q_sourceColumnsInserted(const QModelIndex &source_parent, int start, int end);
    void _q_sourceColumnsAboutToBeRemoved(const QModelIndex &source_parent, int start, int end);
    void _q_sourceColumnsRemoved(const QModelIndex &source_parent, int start, int end);

    void clear_mapping();

    void sort_source_rows(QVector<int> &source_rows, const QModelIndex &source_parent) const;
    QVector<QPair<int, QVector<int > > > proxy_intervals_for_source_items_to_add(
        const QVector<int> &proxy_to_source, const QVector<int> &source_items,
        const QModelIndex &source_parent, Qt::Orientation orient) const;
    QVector<QPair<int, int > > proxy_intervals_for_source_items(
        const QVector<int> &source_to_proxy, const QVector<int> &source_items) const;
    void insert_source_items(
        QVector<int> &source_to_proxy, QVector<int> &proxy_to_source, const QVector<int> &source_items,
        const QModelIndex &source_parent, Qt::Orientation orient, bool emit_signal = true);
    void remove_source_items(
        QVector<int> &source_to_proxy, QVector<int> &proxy_to_source, const QVector<int> &source_items,
        const QModelIndex &source_parent, Qt::Orientation orient, bool emit_signal = true);
    void remove_proxy_interval(
        QVector<int> &source_to_proxy, QVector<int> &proxy_to_source, int proxy_start, int proxy_end,
        const QModelIndex &proxy_parent, Qt::Orientation orient, bool emit_signal = true);
    void build_source_to_proxy_mapping(
        const QVector<int> &proxy_to_source, QVector<int> &source_to_proxy) const;
    void source_items_inserted(
        const QModelIndex &source_parent, int start, int end, Qt::Orientation orient);
    void source_items_removed(
        const QModelIndex &source_parent, int start, int end, Qt::Orientation orient);
    void proxy_item_range(
        const QVector<int> &source_to_proxy, const QVector<int> &source_items,
        int &proxy_low, int &proxy_high) const;

    QModelIndexList store_persistent_indexes();
    void update_persistent_indexes(const QModelIndexList &source_indexes);

    void filter_changed();
    void handle_filter_changed(
        QVector<int> &source_to_proxy, QVector<int> &proxy_to_source,
        const QModelIndex &source_parent, Qt::Orientation orient);
};

typedef QMap<QModelIndex, QSortFilterProxyModelPrivate::Mapping *> IndexMap;

void QSortFilterProxyModelPrivate::remove_from_mapping(const QModelIndex &source_parent)
{
    if (Mapping *m = source_index_mapping.take(source_parent)) {
        for (int i = 0; i < m->mapped_children.size(); ++i)
            remove_from_mapping(m->mapped_children.at(i));
        delete m;
    }
}

void QSortFilterProxyModelPrivate::clear_mapping()
{
    // store the persistent indexes
    QModelIndexList source_indexes;
    int persistent_count = persistent.indexes.count();
    for (int i = 0; i < persistent_count; ++i) {
        QModelIndex proxy_index = persistent.indexes.at(i)->index;
        QModelIndex source_index = proxy_to_source(proxy_index);
        source_indexes.append(source_index);
    }

    qDeleteAll(source_index_mapping);
    source_index_mapping.clear();

    // update the persistent indexes
    for (int i = 0; i < persistent_count; ++i) {
        QModelIndex source_index = source_indexes.at(i);
        create_mapping(source_index.parent());
        QModelIndex proxy_index = source_to_proxy(source_index);
        persistent.indexes[i]->index = proxy_index;
    }
}

IndexMap::const_iterator QSortFilterProxyModelPrivate::create_mapping(
    const QModelIndex &source_parent) const
{
    Q_Q(const QSortFilterProxyModel);

    IndexMap::const_iterator it = source_index_mapping.find(source_parent);
    if (it != source_index_mapping.end()) // was mapped already
        return it;

    Mapping *m = new Mapping;

    int source_rows = model->rowCount(source_parent);
    for (int i = 0; i < source_rows; ++i) {
        if (q->filterAcceptsRow(i, source_parent))
            m->source_rows.append(i);
    }
    int source_cols = model->columnCount(source_parent);    
    for (int i = 0; i < source_cols; ++i) {
        if (q->filterAcceptsColumn(i, source_parent))
            m->source_columns.append(i);
    }

    sort_source_rows(m->source_rows, source_parent);
    m->proxy_rows.resize(source_rows);
    build_source_to_proxy_mapping(m->source_rows, m->proxy_rows);
    m->proxy_columns.resize(source_cols);
    build_source_to_proxy_mapping(m->source_columns, m->proxy_columns);

    it = source_index_mapping.insert(source_parent, m);

    if (source_parent.isValid()) {
        QModelIndex source_grand_parent = source_parent.parent();
        IndexMap::const_iterator it2 = create_mapping(source_grand_parent);
        Q_ASSERT(it2 != source_index_mapping.end());
        it2.value()->mapped_children.append(source_parent);
    }

    Q_ASSERT(it != source_index_mapping.end());
    Q_ASSERT(it.value());

    return it;
}

QModelIndex QSortFilterProxyModelPrivate::proxy_to_source(const QModelIndex &proxy_index) const
{
    if (!proxy_index.isValid())
        return QModelIndex(); // for now; we may want to be able to set a root index later
    IndexMap::const_iterator it = index_to_iterator(proxy_index);
    Mapping *m = it.value();
    if (m->source_rows.isEmpty() || m->source_columns.isEmpty())
        return QModelIndex();
    int source_row = m->source_rows.at(proxy_index.row());
    int source_col = m->source_columns.at(proxy_index.column());
    return model->index(source_row, source_col, it.key());
}

QModelIndex QSortFilterProxyModelPrivate::source_to_proxy(const QModelIndex &source_index) const
{
    if (!source_index.isValid())
        return QModelIndex(); // for now; we may want to be able to set a root index later
    QModelIndex source_parent = source_index.parent();
    IndexMap::const_iterator it = create_mapping(source_parent);
    Mapping *m = it.value();
    if (m->proxy_rows.isEmpty() || m->proxy_columns.isEmpty())
        return QModelIndex();
    int proxy_row = m->proxy_rows.at(source_index.row());
    int proxy_column = m->proxy_columns.at(source_index.column());
    return create_index(proxy_row, proxy_column, it);
}

/*!
  \internal

  Sorts the given \a source_rows according to current sort column and order.
*/
void QSortFilterProxyModelPrivate::sort_source_rows(
    QVector<int> &source_rows, const QModelIndex &source_parent) const
{
    Q_Q(const QSortFilterProxyModel);
    if (sort_column >= 0) {
        if (sort_order == Qt::AscendingOrder) {
            QSortFilterProxyModelLessThan lt(sort_column, source_parent, model, q);
            qStableSort(source_rows.begin(), source_rows.end(), lt);
        } else {
            QSortFilterProxyModelGreaterThan gt(sort_column, source_parent, model, q);
            qStableSort(source_rows.begin(), source_rows.end(), gt);
        }
    }
}

/*!
  \internal

  Given source-to-proxy mapping \a source_to_proxy and the set of
  source items \a source_items (which are part of that mapping),
  determines the corresponding proxy item intervals that should
  be removed from the proxy model.

  The result is a vector of pairs, where each pair represents a
  (start, end) tuple, sorted in ascending order.
*/
QVector<QPair<int, int > > QSortFilterProxyModelPrivate::proxy_intervals_for_source_items(
    const QVector<int> &source_to_proxy, const QVector<int> &source_items) const
{
    QVector<QPair<int, int> > proxy_intervals;
    if (source_items.isEmpty())
        return proxy_intervals;

    int source_items_index = 0;
    while (source_items_index < source_items.size()) {
        int first_proxy_item = source_to_proxy.at(source_items.at(source_items_index));
        Q_ASSERT(first_proxy_item != -1);
        int last_proxy_item = first_proxy_item;
        ++source_items_index;
        // Find end of interval
        while ((source_items_index < source_items.size())
               && (source_to_proxy.at(source_items.at(source_items_index)) == last_proxy_item + 1)) {
            ++last_proxy_item;
            ++source_items_index;
        }
        // Add interval to result
        proxy_intervals.append(QPair<int, int>(first_proxy_item, last_proxy_item));
    }
    qStableSort(proxy_intervals.begin(), proxy_intervals.end());
    return proxy_intervals;
}

/*!
  \internal

  Given source-to-proxy mapping \a src_to_proxy and proxy-to-source mapping
  \a proxy_to_source, removes \a source_items from this proxy model.
  The corresponding proxy items are removed in intervals, so that the proper
  rows/columnsRemoved(start, end) signals will be generated.
*/
void QSortFilterProxyModelPrivate::remove_source_items(
    QVector<int> &source_to_proxy, QVector<int> &proxy_to_source, const QVector<int> &source_items,
    const QModelIndex &source_parent, Qt::Orientation orient, bool emit_signal)
{
    QVector<QPair<int, int> > proxy_intervals;
    proxy_intervals = proxy_intervals_for_source_items(source_to_proxy, source_items);

    QModelIndex proxy_parent = QSortFilterProxyModelPrivate::source_to_proxy(source_parent);

    for (int i = proxy_intervals.size()-1; i >= 0; --i) {
        QPair<int, int> interval = proxy_intervals.at(i);
        int proxy_start = interval.first;
        int proxy_end = interval.second;
        remove_proxy_interval(source_to_proxy, proxy_to_source, proxy_start, proxy_end, proxy_parent, orient, emit_signal);
    }
}

/*!
  \internal

  Given source-to-proxy mapping \a source_to_proxy and proxy-to-source mapping
  \a proxy_to_source, removes items from \a proxy_start to \a proxy_end
  (inclusive) from this proxy model.
*/
void QSortFilterProxyModelPrivate::remove_proxy_interval(
    QVector<int> &source_to_proxy, QVector<int> &proxy_to_source, int proxy_start, int proxy_end,
    const QModelIndex &proxy_parent, Qt::Orientation orient, bool emit_signal)
{
    Q_Q(QSortFilterProxyModel);
    if (emit_signal) {
        if (orient == Qt::Vertical)
            q->beginRemoveRows(proxy_parent, proxy_start, proxy_end);
        else
            q->beginRemoveColumns(proxy_parent, proxy_start, proxy_end);
    }

    // Remove items from proxy-to-source mapping
    proxy_to_source.remove(proxy_start, proxy_end - proxy_start + 1);

    build_source_to_proxy_mapping(proxy_to_source, source_to_proxy);

    if (emit_signal) {
        if (orient == Qt::Vertical)
            q->endRemoveRows();
        else
            q->endRemoveColumns();
    }
}

/*!
  \internal
  
  Given proxy-to-source mapping \a proxy_to_source and a set of
  unmapped source items \a source_items, determines the proxy item
  intervals at which the subsets of source items should be inserted
  (but does not actually add them to the mapping).

  The result is a vector of pairs, each pair representing a tuple (start,
  items), where items is a vector containing the (sorted) source items that
  should be inserted at that proxy model location.
*/
QVector<QPair<int, QVector<int > > > QSortFilterProxyModelPrivate::proxy_intervals_for_source_items_to_add(
    const QVector<int> &proxy_to_source, const QVector<int> &source_items,
    const QModelIndex &source_parent, Qt::Orientation orient) const
{
    Q_Q(const QSortFilterProxyModel);
    QVector<QPair<int, QVector<int> > > proxy_intervals;
    if (source_items.isEmpty())
        return proxy_intervals;

    int proxy_low = 0;
    int proxy_item = 0;
    int source_items_index = 0;
    QVector<int> source_items_in_interval;
    bool compare = (orient == Qt::Vertical && sort_column >= 0);
    while (source_items_index < source_items.size()) {
        source_items_in_interval.clear();
        int first_new_source_item = source_items.at(source_items_index);
        source_items_in_interval.append(first_new_source_item);
        ++source_items_index;

        // Find proxy item at which insertion should be started
        int proxy_high = proxy_to_source.size() - 1;
        QModelIndex i1 = compare ? model->index(first_new_source_item, sort_column, source_parent) : QModelIndex();
        while (proxy_low <= proxy_high) {
            proxy_item = ((proxy_high - proxy_low) / 2) + proxy_low;
            if (compare) {
                QModelIndex i2 = model->index(proxy_to_source.at(proxy_item), sort_column, source_parent);
                if ((sort_order == Qt::AscendingOrder) ? q->lessThan(i1, i2) : q->lessThan(i2, i1))
                    proxy_high = proxy_item - 1;
                else
                    proxy_low = proxy_item + 1;
            } else {
                if (first_new_source_item < proxy_to_source.at(proxy_item))
                    proxy_high = proxy_item - 1;
                else
                    proxy_low = proxy_item + 1;
            }
        }
        proxy_item = proxy_low;

        // Find the sequence of new source items that should be inserted here
        if (proxy_item >= proxy_to_source.size()) {
            for ( ; source_items_index < source_items.size(); ++source_items_index)
                source_items_in_interval.append(source_items.at(source_items_index));
        } else {
            i1 = compare ? model->index(proxy_to_source.at(proxy_item), sort_column, source_parent) : QModelIndex();
            for ( ; source_items_index < source_items.size(); ++source_items_index) {
                int new_source_item = source_items.at(source_items_index);
                if (compare) {
                    QModelIndex i2 = model->index(new_source_item, sort_column, source_parent);
                    if ((sort_order == Qt::AscendingOrder) ? q->lessThan(i1, i2) : q->lessThan(i2, i1))
                        break;
                } else {
                    if (proxy_to_source.at(proxy_item) < new_source_item)
                        break;
                }
                source_items_in_interval.append(new_source_item);
            }
        }

        // Add interval to result
        proxy_intervals.append(QPair<int, QVector<int> >(proxy_item, source_items_in_interval));
    }
    return proxy_intervals;
}

/*!
  \internal

  Given source-to-proxy mapping \a source_to_proxy and proxy-to-source mapping
  \a proxy_to_source, inserts the given \a source_items into this proxy model.
  The source items are inserted in intervals (based on some sorted order), so
  that the proper rows/columnsInserted(start, end) signals will be generated.
*/
void QSortFilterProxyModelPrivate::insert_source_items(
    QVector<int> &source_to_proxy, QVector<int> &proxy_to_source,
    const QVector<int> &source_items, const QModelIndex &source_parent,
    Qt::Orientation orient, bool emit_signal)
{
    Q_Q(QSortFilterProxyModel);
    QVector<QPair<int, QVector<int> > > proxy_intervals;
    proxy_intervals = proxy_intervals_for_source_items_to_add(
        proxy_to_source, source_items, source_parent, orient);

    QModelIndex proxy_parent = QSortFilterProxyModelPrivate::source_to_proxy(source_parent);

    for (int i = proxy_intervals.size()-1; i >= 0; --i) {
        QPair<int, QVector<int> > interval = proxy_intervals.at(i);
        int proxy_start = interval.first;
        QVector<int> source_items = interval.second;
        int proxy_end = proxy_start + source_items.size() - 1;

        if (emit_signal) {
            if (orient == Qt::Vertical)
                q->beginInsertRows(proxy_parent, proxy_start, proxy_end);
            else
                q->beginInsertColumns(proxy_parent, proxy_start, proxy_end);
        }

        for (int i = 0; i < source_items.size(); ++i)
            proxy_to_source.insert(proxy_start + i, source_items.at(i));

        build_source_to_proxy_mapping(proxy_to_source, source_to_proxy);

        if (emit_signal) {
            if (orient == Qt::Vertical)
                q->endInsertRows();
            else
                q->endInsertColumns();
        }
    }
}

/*!
  \internal

  Handles source model items insertion (columnsInserted(), rowsInserted()).
  Determines
  1) which of the inserted items to also insert into proxy model (filtering),
  2) where to insert the items into the proxy model (sorting),
  then inserts those items.
  The items are inserted into the proxy model in intervals (based on
  sorted order), so that the proper rows/columnsInserted(start, end)
  signals will be generated.
*/
void QSortFilterProxyModelPrivate::source_items_inserted(
    const QModelIndex &source_parent, int start, int end, Qt::Orientation orient)
{
    Q_Q(QSortFilterProxyModel);
    IndexMap::const_iterator it = source_index_mapping.find(source_parent);
    if (it == source_index_mapping.end()) {
        // Don't care, since we don't have mapping for this index
        return;
    }

    Mapping *m = it.value();
    QVector<int> &source_to_proxy = (orient == Qt::Vertical) ? m->proxy_rows : m->proxy_columns;
    QVector<int> &proxy_to_source = (orient == Qt::Vertical) ? m->source_rows : m->source_columns;

    int delta_item_count = end - start + 1;
    int old_item_count = source_to_proxy.size();
    // Expand source-to-proxy mapping to account for new items
    source_to_proxy.insert(start, delta_item_count, -1);

    if (start < old_item_count) {
        // Adjust existing "stale" indexes in proxy-to-source mapping
        for (int proxy_item = 0; proxy_item < proxy_to_source.size(); ++proxy_item) {
            int source_item = proxy_to_source.at(proxy_item);
            if (source_item >= start)
                proxy_to_source.replace(proxy_item, source_item + delta_item_count);
        }
        build_source_to_proxy_mapping(proxy_to_source, source_to_proxy);
    }

    // Figure out which items to add to mapping based on filter
    QVector<int> source_items;
    for (int i = start; i <= end; ++i) {
        if ((orient == Qt::Vertical)
            ? q->filterAcceptsRow(i, source_parent)
            : q->filterAcceptsColumn(i, source_parent)) {
            source_items.append(i);
        }
    }

    // Sort and insert the items
    if (orient == Qt::Vertical) // Only sort rows
        sort_source_rows(source_items, source_parent);
    insert_source_items(source_to_proxy, proxy_to_source, source_items, source_parent, orient);
}

/*!
  \internal

  Handles source model items removal (columnsRemoved(), rowsRemoved()).
*/
void QSortFilterProxyModelPrivate::source_items_removed(
    const QModelIndex &source_parent, int start, int end, Qt::Orientation orient)
{
    IndexMap::const_iterator it = source_index_mapping.find(source_parent);
    if (it == source_index_mapping.end()) {
        // Don't care, since we don't have mapping for this index
        return;
    }

    int delta_item_count = end - start + 1;
    Mapping *m = it.value();
    QVector<int> &source_to_proxy = (orient == Qt::Vertical) ? m->proxy_rows : m->proxy_columns;
    QVector<int> &proxy_to_source = (orient == Qt::Vertical) ? m->source_rows : m->source_columns;

    // This approach is (and needs to be) a bit different than the one in
    // source_items_inserted(). We cannot remove the corresponding proxy items
    // in intervals, because then recipients of the
    // rowsRemoved()/columnsRemoved() signals can try to access proxy items
    // that happen to be in the subsequent intervals yet to be removed. We
    // don't want that, because the corresponding source items have just been
    // removed from the source model (e.g. their proxy items are no longer
    // valid). This is a matter of ensuring that the observable state of the
    // proxy model is kept consistent.  The solution is to find the minimal
    // interval that can be used to remove all affected proxy items in a single
    // removeRows()/removeColumns(), then insert back those items not affected
    // by the source items removal.

    // Figure out "super"-interval of proxy items to remove
    int proxy_start = INT_MAX;
    int proxy_end = INT_MIN;
    for (int i = start; i <= end; ++i) {
        int proxy_item = source_to_proxy.at(i);
        if (proxy_item != -1) {
            if (proxy_item < proxy_start)
                proxy_start = proxy_item;
            if (proxy_item > proxy_end)
                proxy_end = proxy_item;
        }
    }

    // Figure out which source items in the "super"-interval to keep (reinsert)
    QVector<int> source_items;
    for (int i = proxy_start+1; i <= proxy_end-1; ++i) {
        int source_item = proxy_to_source.at(i);
        if (source_item < start)
            source_items.append(source_item);
        else if (source_item > end)
            source_items.append(source_item - delta_item_count);
    }

    QModelIndexList source_indexes = store_persistent_indexes();

    // Remove the items in "super"-interval
    QModelIndex proxy_parent = QSortFilterProxyModelPrivate::source_to_proxy(source_parent);
    remove_proxy_interval(source_to_proxy, proxy_to_source, proxy_start, proxy_end, proxy_parent, orient);

    // Shrink the source-to-proxy mapping to reflect the new item count
    source_to_proxy.remove(start, delta_item_count);

    // Adjust "stale" indexes in proxy-to-source mapping
    for (int proxy_item = 0; proxy_item < proxy_to_source.size(); ++proxy_item) {
        int source_item = proxy_to_source.at(proxy_item);
        if (source_item >= start)
            proxy_to_source.replace(proxy_item, source_item - delta_item_count);
    }
    // Do the same adjustment for persistent indexes
    for (int i = 0; i < source_indexes.size(); ++i) {
        QModelIndex source_index = source_indexes.at(i);
        int source_item = (orient == Qt::Vertical) ? source_index.row() : source_index.column();
        if (source_item >= start) {
            if (source_item <= end) {
                // Invalidate
                source_indexes.replace(i, QModelIndex());
            } else {
                // Move
                int new_source_item = source_item - delta_item_count;
                int new_source_row = (orient == Qt::Vertical) ? new_source_item : source_index.row();
                int new_source_column = (orient == Qt::Vertical) ? source_index.column() : new_source_item;
                source_indexes.replace(i, model->index(new_source_row, new_source_column, source_index.parent()));
            }
        }
    }

    // Add back the source items to keep
    insert_source_items(source_to_proxy, proxy_to_source, source_items, source_parent, orient);

    update_persistent_indexes(source_indexes);
}

void QSortFilterProxyModelPrivate::proxy_item_range(
    const QVector<int> &source_to_proxy, const QVector<int> &source_items,
    int &proxy_low, int &proxy_high) const
{
    proxy_low = INT_MAX;
    proxy_high = INT_MIN;
    foreach (int source_item, source_items) {
        int proxy_item = source_to_proxy.at(source_item);
        Q_ASSERT(proxy_item != -1);
        if (proxy_item < proxy_low)
            proxy_low = proxy_item;
        if (proxy_item > proxy_high)
            proxy_high = proxy_item;
    }
}

/*!
  \internal
*/
void QSortFilterProxyModelPrivate::build_source_to_proxy_mapping(
    const QVector<int> &proxy_to_source, QVector<int> &source_to_proxy) const
{
    source_to_proxy.fill(-1);
    for (int i = 0; i < proxy_to_source.size(); ++i)
        source_to_proxy[proxy_to_source.at(i)] = i;
}

/*!
  \internal

  Maps the persistent proxy indexes to source indexes and
  returns the list of source indexes. 
*/
QModelIndexList QSortFilterProxyModelPrivate::store_persistent_indexes()
{
    QModelIndexList source_indexes;
    int persistent_count = persistent.indexes.count();
    for (int i = 0; i < persistent_count; ++i) {
        QModelIndex proxy_index = persistent.indexes.at(i)->index;
        QModelIndex source_index = proxy_to_source(proxy_index);
        source_indexes.append(source_index);
    }
    return source_indexes;
}

/*!
  \internal

  Maps \a source_indexes to proxy indexes and stores those
  as persistent indexes.
*/
void QSortFilterProxyModelPrivate::update_persistent_indexes(const QModelIndexList &source_indexes)
{
    for (int i = 0; i < source_indexes.count(); ++i) {
        QModelIndex source_index = source_indexes.at(i);
        create_mapping(source_index.parent());
        QModelIndex proxy_index = source_to_proxy(source_index);
        persistent.indexes[i]->index = proxy_index;
    }
}

/*!
  \internal

  Updates the proxy model (adds/removes rows) based on the
  new filter.
*/
void QSortFilterProxyModelPrivate::filter_changed()
{
    QMap<QModelIndex, Mapping *>::const_iterator it;
    for (it = source_index_mapping.constBegin(); it != source_index_mapping.constEnd(); ++it) {
        QModelIndex source_parent = it.key();
        Mapping *m = it.value();
        handle_filter_changed(m->proxy_rows, m->source_rows, source_parent, Qt::Vertical);
        handle_filter_changed(m->proxy_columns, m->source_columns, source_parent, Qt::Horizontal);
    }
}

/*!
  \internal
*/
void QSortFilterProxyModelPrivate::handle_filter_changed(
    QVector<int> &source_to_proxy, QVector<int> &proxy_to_source,
    const QModelIndex &source_parent, Qt::Orientation orient)
{
    Q_Q(QSortFilterProxyModel);
    // Figure out which mapped items to remove
    QVector<int> source_items_remove;
    foreach (int source_item, proxy_to_source) {
        if ((orient == Qt::Vertical)
            ? !q->filterAcceptsRow(source_item, source_parent)
            : !q->filterAcceptsColumn(source_item, source_parent)) {
            // This source item does not satisfy the filter, so it must be removed
            source_items_remove.append(source_item);
        }
    }
    // Figure out which non-mapped items to insert
    QVector<int> source_items_insert;
    for (int source_item = 0; source_item < source_to_proxy.size(); ++source_item) {
        if (source_to_proxy.at(source_item) == -1) {
            if ((orient == Qt::Vertical)
                ? q->filterAcceptsRow(source_item, source_parent)
                : q->filterAcceptsColumn(source_item, source_parent)) {
                // This source item satisfies the filter, so it must be added
                source_items_insert.append(source_item);
            }
        }
    }
    if (!source_items_remove.isEmpty() || !source_items_insert.isEmpty()) {
        // Do item removal and insertion
        remove_source_items(source_to_proxy, proxy_to_source, source_items_remove, source_parent, orient);
        if (orient == Qt::Vertical)
            sort_source_rows(source_items_insert, source_parent);
        insert_source_items(source_to_proxy, proxy_to_source, source_items_insert, source_parent, orient);
    }
}

void QSortFilterProxyModelPrivate::_q_sourceDataChanged(const QModelIndex &source_top_left,
                                                     const QModelIndex &source_bottom_right)
{
    Q_Q(QSortFilterProxyModel);
    QModelIndex source_parent = source_top_left.parent();
    IndexMap::const_iterator it = source_index_mapping.find(source_parent);
    if (it == source_index_mapping.end()) {
        // Don't care, since we don't have mapping for this index
        return;
    }
    Mapping *m = it.value();

    // Figure out how the source changes affect us
    QVector<int> source_rows_remove;
    QVector<int> source_rows_insert;
    QVector<int> source_rows_change;
    QVector<int> source_rows_resort;
    for (int source_row = source_top_left.row(); source_row <= source_bottom_right.row(); ++source_row) {
        if (dynamic_sortfilter) {
            if (m->proxy_rows.at(source_row) != -1) {
                if (!q->filterAcceptsRow(source_row, source_parent)) {
                    // This source row no longer satisfies the filter, so it must be removed
                    source_rows_remove.append(source_row);
                } else if (sort_column >= source_top_left.column() && sort_column <= source_bottom_right.column()) {
                    // This source row has changed in a way that may affect sorted order
                    source_rows_resort.append(source_row);
                } else {
                    // This row has simply changed, without affecting filtering nor sorting
                    source_rows_change.append(source_row);
                }
            } else {
                if (q->filterAcceptsRow(source_row, source_parent)) {
                    // This source row now satisfies the filter, so it must be added
                    source_rows_insert.append(source_row);
                }
            }
        } else {
            if (m->proxy_rows.at(source_row) != -1)
                source_rows_change.append(source_row);
        }
    }

    if (!source_rows_remove.isEmpty())
        remove_source_items(m->proxy_rows, m->source_rows, source_rows_remove, source_parent, Qt::Vertical);

    if (!source_rows_resort.isEmpty()) {
        // Re-sort the rows
        QModelIndexList source_indexes = store_persistent_indexes();
        remove_source_items(m->proxy_rows, m->source_rows, source_rows_resort, source_parent, Qt::Vertical, false);
        sort_source_rows(source_rows_resort, source_parent);
        insert_source_items(m->proxy_rows, m->source_rows, source_rows_resort, source_parent, Qt::Vertical, false);
        update_persistent_indexes(source_indexes);
        emit q->layoutChanged();
    } else if (!source_rows_change.isEmpty()) {
        // Find the proxy row range
        int proxy_start_row;
        int proxy_end_row;
        proxy_item_range(m->proxy_rows, source_rows_change, proxy_start_row, proxy_end_row);
        // ### Find the proxy column range also

        QModelIndex proxy_top_left = create_index(
            proxy_start_row, m->proxy_columns.at(source_top_left.column()), it);
        QModelIndex proxy_bottom_right = create_index(
            proxy_end_row, m->proxy_columns.at(source_bottom_right.column()), it);
        emit q->dataChanged(proxy_top_left, proxy_bottom_right);
    }

    if (!source_rows_insert.isEmpty()) {
        sort_source_rows(source_rows_insert, source_parent);
        insert_source_items(m->proxy_rows, m->source_rows, source_rows_insert, source_parent, Qt::Vertical);
    }
}

void QSortFilterProxyModelPrivate::_q_sourceHeaderDataChanged(Qt::Orientation orientation,
                                                           int start, int end)
{
    Q_Q(QSortFilterProxyModel);
    Mapping *m = create_mapping(QModelIndex()).value();
    int proxy_start = (orientation == Qt::Vertical
                       ? m->proxy_rows.at(start)
                       : m->proxy_columns.at(start));
    int proxy_end = (orientation == Qt::Vertical
                     ? m->proxy_rows.at(end)
                     : m->proxy_columns.at(end));
    emit q->headerDataChanged(orientation, proxy_start, proxy_end);
}

void QSortFilterProxyModelPrivate::_q_sourceReset()
{
    Q_Q(QSortFilterProxyModel);
    // All internal structures are deleted in clear()
    q->reset();
}

void QSortFilterProxyModelPrivate::_q_sourceRowsAboutToBeInserted(const QModelIndex &source_parent, int start, int end)
{
    Q_UNUSED(source_parent);
    Q_UNUSED(start);
    Q_UNUSED(end);
}

void QSortFilterProxyModelPrivate::_q_sourceRowsInserted(const QModelIndex &source_parent, int start, int end)
{
    source_items_inserted(source_parent, start, end, Qt::Vertical);
}

void QSortFilterProxyModelPrivate::_q_sourceRowsAboutToBeRemoved(const QModelIndex &source_parent, int start, int end)
{
    Q_UNUSED(source_parent);
    Q_UNUSED(start);
    Q_UNUSED(end);
}

void QSortFilterProxyModelPrivate::_q_sourceRowsRemoved(const QModelIndex &source_parent, int start, int end)
{
    source_items_removed(source_parent, start, end, Qt::Vertical);
}

void QSortFilterProxyModelPrivate::_q_sourceColumnsAboutToBeInserted(const QModelIndex &source_parent, int start, int end)
{
    Q_UNUSED(source_parent);
    Q_UNUSED(start);
    Q_UNUSED(end);
}

void QSortFilterProxyModelPrivate::_q_sourceColumnsInserted(const QModelIndex &source_parent, int start, int end)
{
    source_items_inserted(source_parent, start, end, Qt::Horizontal);
}

void QSortFilterProxyModelPrivate::_q_sourceColumnsAboutToBeRemoved(const QModelIndex &source_parent, int start, int end)
{
    Q_UNUSED(source_parent);
    Q_UNUSED(start);
    Q_UNUSED(end);
}

void QSortFilterProxyModelPrivate::_q_sourceColumnsRemoved(const QModelIndex &source_parent, int start, int end)
{
    source_items_removed(source_parent, start, end, Qt::Horizontal);
}

/*!
    \since 4.1
    \class QSortFilterProxyModel
    \brief The QSortFilterProxyModel class provides support for sorting and filtering data passed
    between another model and a view.

    \ingroup model-view

    QSortFilterProxyModel can be used for sorting items, filtering
    out items, or both. The model transforms the structure of a
    source model by mapping the model indexes it supplies to new
    indexes, corresponding to different locations, for views to use.
    This approach allows a given source model to be restructured as
    far as views are concerned without requiring any transformations
    on the underlying data, and without duplicating the data in
    memory.

    Let's assume that we want to sort and filter the items provided
    by a custom model. The code to set up the model and the view, \e
    without sorting and filtering, would look like this:

    \code
        QTreeView *treeView = new QTreeView;
        MyItemModel *model = new MyItemModel(this);

        treeView->setModel(model);
    \endcode

    To add sorting and filtering support to \c MyItemModel, we need
    to create a QSortFilterProxyModel, call setSourceModel() with the
    \c MyItemModel as argument, and install the QSortFilterProxyModel
    on the view:

    \code
        QTreeView *treeView = new QTreeView;
        MyItemModel *sourceModel = new MyItemModel(this);
        QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);

        proxyModel->setSourceModel(sourceModel);
        treeView->setModel(proxyModel);
    \endcode

    At this point, neither sorting nor filtering is enabled; the
    original data is displayed in the view. Any changes made through
    the QSortFilterProxyModel are applied to the original model.

    The QSortFilterProxyModel acts as a wrapper for the original
    model. If you need to convert source \l{QModelIndex}es to
    sorted/filtered model indexes or vice versa, use mapToSource(),
    mapFromSource(), mapSelectionToSource(), and
    mapSelectionFromSource().

    By default, sorting and filtering isn't dynamically reapplied
    whenever the data of the original model changes, as an
    optimization. To enable dynamic sorting and filtering, call
    setDynamicSortFilter(true). At any time, you can call clear() to
    resort/refilter the data.

    The \l{itemviews/basicsortfiltermodel}{Basic Sort/Filter Model}
    and \l{itemviews/customsortfiltermodel}{Custom Sort/Filter Model}
    examples illustrate how to use QSortFilterProxyModel to perform
    basic sorting and filtering and how to subclass it to implement
    custom behavior.

    \section1 Sorting

    QTableView and QTreeView have a
    \l{QTreeView::sortingEnabled}{sortingEnabled} property that
    controls whether the user can sort the view by clicking the
    view's horizontal header. For example:

    \code
        treeView->setSortingEnabled(true);
    \endcode
    
    When this feature is on (the default is off), clicking on a
    header section sorts the items according to that column. By
    clicking repeatedly, the user can alternate between ascending and
    descending order.

    \image qsortfilterproxymodel-sorting.png A sorted QTreeView

    Behind the scene, the view calls the sort() virtual function on
    the model to reorder the data in the model. To make your data
    sortable, you can either implement sort() in your model, or you
    use a QSortFilterProxyModel to wrap your model --
    QSortFilterProxyModel provides a generic sort() reimplementation
    that operates on the sortRole() (Qt::DisplayRole by default) of
    the items and that understands several data types, including \c
    int, QString, and QDateTime. For hierarchical models, sorting is
    applied recursively to all child items. String comparisons are
    case sensitive by default; this can be changed by setting the \l
    sortCaseSensitivity property.

    Custom sorting behavior is achieved by subclassing
    QSortFilterProxyModel and reimplementing lessThan(), which is
    used to compare items. For example:

    \quotefromfile itemviews/customsortfiltermodel/mysortfilterproxymodel.cpp
    \skipto ::lessThan
    \printuntil /^\}$/

    (This code snippet comes from the
    \l{itemviews/customsortfiltermodel}{Custom Sort/Filter Model}
    example.)

    An alternative approach to sorting is to disable sorting on the
    view and to impose a certain order to the user. This is done by
    explicitly calling sort() with the desired column and order as
    arguments on the QSortFilterProxyModel (or on the original model
    if it implements sort()). For example:

    \code
        proxyModel->sort(2, Qt::AscendingOrder);
    \endcode

    \section1 Filtering

    In addition to sorting, QSortFilterProxyModel can be used to hide
    items that don't match a certain filter. The filter is specified
    using a QRegExp object and is applied to the filterRole()
    (Qt::DisplayRole by default) of each item, for a given column.
    The QRegExp object can be used to match a regular expression, a
    wildcard pattern, or a fixed string. For example:

    \code
        proxyModel->setFilterRegExp(QRegExp(".png", Qt::CaseInsensitive,
                                            QRegExp::FixedString));
        proxyModel->setFilterKeyColumn(1);
    \endcode

    For hierarchical models, the filter is applied recursively to all
    children. If a parent item doesn't match the filter, none of its
    children will be shown.

    A common use case is to let the user specify the filter regexp,
    wildcard pattern, or fixed string in a QLineEdit and to connect
    the \l{QLineEdit::textChanged()}{textChanged()} signal to
    setFilterRegExp(), setFilterWildcard(), or setFilterFixedString()
    to reapply the filter.

    Custom filtering behavior can be achieved by reimplementing the
    filterAcceptsRow() and filterAcceptsColumn() functions. For
    example, the following implementation ignores the \l
    filterKeyColumn property and performs filtering on columns 0, 1,
    and 2:

    \quotefromfile itemviews/customsortfiltermodel/mysortfilterproxymodel.cpp
    \skipto ::filterAcceptsRow
    \printuntil /^\}$/

    (This code snippet comes from the
    \l{itemviews/customsortfiltermodel}{Custom Sort/Filter Model}
    example.)

    \sa QAbstractProxyModel, QAbstractItemModel, {Model/View Programming}
*/

/*!
    Constructs a sorting filter model with the given \a parent.
*/

QSortFilterProxyModel::QSortFilterProxyModel(QObject *parent)
    : QAbstractProxyModel(*new QSortFilterProxyModelPrivate, parent)
{
    Q_D(QSortFilterProxyModel);
    d->sort_column = -1;
    d->sort_order = Qt::AscendingOrder;
    d->sort_casesensitivity = Qt::CaseSensitive;
    d->sort_role = Qt::DisplayRole;
    d->filter_column = 0;
    d->filter_role = Qt::DisplayRole;
    d->dynamic_sortfilter = false;
    connect(this, SIGNAL(modelReset()), this, SLOT(clear()));
}

/*!
    Destroys the sorting filter model.
*/
QSortFilterProxyModel::~QSortFilterProxyModel()
{
    Q_D(QSortFilterProxyModel);
    qDeleteAll(d->source_index_mapping);
    d->source_index_mapping.clear();
}

/*!
  \reimp
*/
void QSortFilterProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    Q_D(QSortFilterProxyModel);

    if (d->model && d->model != &d->empty) {
        disconnect(d->model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                   this, SLOT(_q_sourceDataChanged(QModelIndex,QModelIndex)));

        disconnect(d->model, SIGNAL(headerDataChanged(Qt::Orientation,int,int)),
                   this, SLOT(_q_sourceHeaderDataChanged(Qt::Orientation,int,int)));

        disconnect(d->model, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
                   this, SLOT(_q_sourceRowsAboutToBeInserted(QModelIndex,int,int)));

        disconnect(d->model, SIGNAL(rowsInserted(QModelIndex,int,int)),
                   this, SLOT(_q_sourceRowsInserted(QModelIndex,int,int)));

        disconnect(d->model, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)),
                   this, SLOT(_q_sourceColumnsAboutToBeInserted(QModelIndex,int,int)));

        disconnect(d->model, SIGNAL(columnsInserted(QModelIndex,int,int)),
                   this, SLOT(_q_sourceColumnsInserted(QModelIndex,int,int)));

        disconnect(d->model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
                   this, SLOT(_q_sourceRowsAboutToBeRemoved(QModelIndex,int,int)));

        disconnect(d->model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
                   this, SLOT(_q_sourceRowsRemoved(QModelIndex,int,int)));

        disconnect(d->model, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
                   this, SLOT(_q_sourceColumnsAboutToBeRemoved(QModelIndex,int,int)));

        disconnect(d->model, SIGNAL(columnsRemoved(QModelIndex,int,int)),
                   this, SLOT(_q_sourceColumnsRemoved(QModelIndex,int,int)));

        disconnect(d->model, SIGNAL(modelReset()), this, SLOT(_q_sourceReset()));
        disconnect(d->model, SIGNAL(layoutChanged()), this, SLOT(clear()));
    }

    QAbstractProxyModel::setSourceModel(sourceModel);

    if (sourceModel) {
        connect(d->model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                this, SLOT(_q_sourceDataChanged(QModelIndex,QModelIndex)));

        connect(d->model, SIGNAL(headerDataChanged(Qt::Orientation,int,int)),
                this, SLOT(_q_sourceHeaderDataChanged(Qt::Orientation,int,int)));

        connect(d->model, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
                this, SLOT(_q_sourceRowsAboutToBeInserted(QModelIndex,int,int)));

        connect(d->model, SIGNAL(rowsInserted(QModelIndex,int,int)),
              this, SLOT(_q_sourceRowsInserted(QModelIndex,int,int)));

        connect(d->model, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)),
                this, SLOT(_q_sourceColumnsAboutToBeInserted(QModelIndex,int,int)));

        connect(d->model, SIGNAL(columnsInserted(QModelIndex,int,int)),
                this, SLOT(_q_sourceColumnsInserted(QModelIndex,int,int)));

        connect(d->model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
                this, SLOT(_q_sourceRowsAboutToBeRemoved(QModelIndex,int,int)));

        connect(d->model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
                this, SLOT(_q_sourceRowsRemoved(QModelIndex,int,int)));

        connect(d->model, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
                this, SLOT(_q_sourceColumnsAboutToBeRemoved(QModelIndex,int,int)));

        connect(d->model, SIGNAL(columnsRemoved(QModelIndex,int,int)),
                this, SLOT(_q_sourceColumnsRemoved(QModelIndex,int,int)));

        connect(d->model, SIGNAL(modelReset()), this, SLOT(_q_sourceReset()));
        connect(d->model, SIGNAL(layoutChanged()), this, SLOT(clear()));
    }

    d->clear_mapping();
}

/*!
    \reimp
*/
QModelIndex QSortFilterProxyModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_D(const QSortFilterProxyModel);
    if (row < 0 || column < 0)
        return QModelIndex();

    QModelIndex source_parent = d->proxy_to_source(parent); // parent is already mapped at this point
    IndexMap::const_iterator it = d->create_mapping(source_parent); // but make sure that the children are mapped
    if (it.value()->source_rows.count() <= row || it.value()->source_columns.count() <= column)
        return QModelIndex();

    return d->create_index(row, column, it);
}

/*!
  \reimp
*/
QModelIndex QSortFilterProxyModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();
    Q_D(const QSortFilterProxyModel);
    IndexMap::const_iterator it = d->index_to_iterator(child);
    Q_ASSERT(it != d->source_index_mapping.end());
    QModelIndex source_parent = it.key();
    QModelIndex proxy_parent = d->source_to_proxy(source_parent);
    return proxy_parent;
}

/*!
  \reimp
*/
int QSortFilterProxyModel::rowCount(const QModelIndex &parent) const
{
    Q_D(const QSortFilterProxyModel);
    QModelIndex source_parent = d->proxy_to_source(parent);
    IndexMap::const_iterator it = d->create_mapping(source_parent);
    return it.value()->source_rows.count();
}

/*!
  \reimp
*/
int QSortFilterProxyModel::columnCount(const QModelIndex &parent) const
{
    Q_D(const QSortFilterProxyModel);
    QModelIndex source_parent = d->proxy_to_source(parent);
    IndexMap::const_iterator it = d->create_mapping(source_parent);
    return it.value()->source_columns.count();
}

/*!
  \reimp
*/
bool QSortFilterProxyModel::hasChildren(const QModelIndex &parent) const
{
    Q_D(const QSortFilterProxyModel);
    QModelIndex source_parent = d->proxy_to_source(parent);
    if (!d->model->hasChildren(source_parent))
        return false;
    QSortFilterProxyModelPrivate::Mapping *m = d->create_mapping(source_parent).value();
    return m->source_rows.count() != 0 && m->source_columns.count() != 0;
}

/*!
  \reimp
*/
QVariant QSortFilterProxyModel::data(const QModelIndex &index, int role) const
{
    Q_D(const QSortFilterProxyModel);
    QModelIndex source_index = d->proxy_to_source(index);
    return d->model->data(source_index, role);
}

/*!
  \reimp
*/
bool QSortFilterProxyModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_D(QSortFilterProxyModel);
    QModelIndex source_index = d->proxy_to_source(index);
    return d->model->setData(source_index, value, role);
}

/*!
  \reimp
*/
QVariant QSortFilterProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_D(const QSortFilterProxyModel);
    IndexMap::const_iterator it = d->create_mapping(QModelIndex());

    int source_section;
    if (orientation == Qt::Vertical) {
        if (section < 0 || section >= it.value()->source_rows.count())
            return QVariant();
        source_section = it.value()->source_rows.at(section);
    }
    else {
        if (section < 0 || section >= it.value()->source_columns.count())
            return QVariant();
        source_section = it.value()->source_columns.at(section);
    }

    return d->model->headerData(source_section, orientation, role);
}

/*!
  \reimp
*/
bool QSortFilterProxyModel::setHeaderData(int section, Qt::Orientation orientation,
                                          const QVariant &value, int role)
{
    Q_D(QSortFilterProxyModel);
    IndexMap::const_iterator it = d->create_mapping(QModelIndex());

    int source_section;
    if (orientation == Qt::Vertical) {
        if (section < 0 || section >= it.value()->source_rows.count())
            return false;
        source_section = it.value()->source_rows.at(section);
    }
    else {
        if (section < 0 || section >= it.value()->source_columns.count())
            return false;
        source_section = it.value()->source_columns.at(section);
    }

    return d->model->setHeaderData(source_section, orientation, value, role);
}

/*!
  \reimp
*/
QMimeData *QSortFilterProxyModel::mimeData(const QModelIndexList &indexes) const
{
    Q_D(const QSortFilterProxyModel);
    QModelIndexList source_indexes;
    for (int i = 0; i < indexes.count(); ++i)
        source_indexes << d->proxy_to_source(indexes.at(i));
    return d->model->mimeData(source_indexes);
}

/*!
  \reimp
*/
QStringList QSortFilterProxyModel::mimeTypes() const
{
    Q_D(const QSortFilterProxyModel);
    return d->model->mimeTypes();
}

/*!
  \reimp
*/
Qt::DropActions QSortFilterProxyModel::supportedDropActions() const
{
    Q_D(const QSortFilterProxyModel);
    return d->model->supportedDropActions();
}

/*!
  \reimp
*/
bool QSortFilterProxyModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                                         int row, int column, const QModelIndex &parent)
{
    Q_D(QSortFilterProxyModel);
    QModelIndex proxy_index = index(row, column, parent);
    QModelIndex source_index = d->proxy_to_source(proxy_index);
    return d->model->dropMimeData(data, action, source_index.row(), source_index.column(),
                                  source_index.parent());
}

/*!
  \reimp
*/
bool QSortFilterProxyModel::insertRows(int row, int count, const QModelIndex &parent)
{
    Q_D(QSortFilterProxyModel);
    if (row < 0 || count <= 0)
        return false;
    QModelIndex source_parent = d->proxy_to_source(parent);
    QSortFilterProxyModelPrivate::Mapping *m = d->create_mapping(source_parent).value();
    if (row > m->source_rows.count())
        return false;
    int source_row = (row >= m->source_rows.count()
                      ? m->source_rows.count()
                      : m->source_rows.at(row));
    return d->model->insertRows(source_row, count, source_parent);
}

/*!
  \reimp
*/
bool QSortFilterProxyModel::insertColumns(int column, int count, const QModelIndex &parent)
{
    Q_D(QSortFilterProxyModel);
    if (column < 0|| count <= 0)
        return false;
    QModelIndex source_parent = d->proxy_to_source(parent);
    QSortFilterProxyModelPrivate::Mapping *m = d->create_mapping(source_parent).value();
    if (column > m->source_columns.count())
        return false;
    int source_column = (column >= m->source_columns.count()
                         ? m->source_columns.count()
                         : m->source_columns.at(column));
    return d->model->insertColumns(source_column, count, source_parent);
}

/*!
  \reimp
*/
bool QSortFilterProxyModel::removeRows(int row, int count, const QModelIndex &parent)
{
    Q_D(QSortFilterProxyModel);
    if (row < 0 || count <= 0)
        return false;
    QModelIndex source_parent = d->proxy_to_source(parent);
    QSortFilterProxyModelPrivate::Mapping *m = d->create_mapping(source_parent).value();
    if (row + count > m->source_rows.count())
        return false;
    int source_row = (row >= m->source_rows.count()
                      ? m->source_rows.at(m->source_rows.count()) + 1
                      : m->source_rows.at(row));
    return d->model->removeRows(source_row, count, source_parent);
}

/*!
  \reimp
*/
bool QSortFilterProxyModel::removeColumns(int column, int count, const QModelIndex &parent)
{
    Q_D(QSortFilterProxyModel);
    if (column < 0 || count <= 0)
        return false;
    QModelIndex source_parent = d->proxy_to_source(parent);
    QSortFilterProxyModelPrivate::Mapping *m = d->create_mapping(source_parent).value();
    if (column + count > m->source_columns.count())
        return false;
    int source_column = (column >= m->source_columns.count()
                         ? m->source_columns.at(m->source_columns.count()) + 1
                         : m->source_columns.at(column));
    return d->model->removeColumns(source_column, count, source_parent);
}

/*!
  \reimp
*/
void QSortFilterProxyModel::fetchMore(const QModelIndex &parent)
{
    Q_D(QSortFilterProxyModel);
    QModelIndex source_parent;
    if (parent.isValid())
        source_parent = d->proxy_to_source(parent);
    d->model->fetchMore(source_parent);
}

/*!
  \reimp
*/
bool QSortFilterProxyModel::canFetchMore(const QModelIndex &parent) const
{
    Q_D(const QSortFilterProxyModel);
    QModelIndex source_parent;
    if (parent.isValid())
        source_parent = d->proxy_to_source(parent);
    return d->model->canFetchMore(source_parent);
}

/*!
  \reimp
*/
Qt::ItemFlags QSortFilterProxyModel::flags(const QModelIndex &index) const
{
    Q_D(const QSortFilterProxyModel);
    QModelIndex source_index;
    if (index.isValid())
        source_index = d->proxy_to_source(index);
    return d->model->flags(source_index);
}

/*!
  \reimp
*/
QModelIndex QSortFilterProxyModel::buddy(const QModelIndex &index) const
{
    Q_D(const QSortFilterProxyModel);
    if (!index.isValid())
        return QModelIndex();
    QModelIndex source_index = d->proxy_to_source(index);
    QModelIndex source_buddy = d->model->buddy(source_index);
    if (source_index == source_buddy)
        return index;
    return d->source_to_proxy(source_buddy);
}

/*!
  \reimp
*/
QModelIndexList QSortFilterProxyModel::match(const QModelIndex &start, int role,
                                             const QVariant &value, int hits,
                                             Qt::MatchFlags flags) const
{
    Q_D(const QSortFilterProxyModel);
    QModelIndex source_start = d->proxy_to_source(start);
    QModelIndexList result = d->model->match(source_start, role, value, hits, flags);
    for (int i = 0; i < result.count(); ++i)
        result[i] = d->source_to_proxy(result.at(i));
    return result;
}

/*!
  \reimp
*/
QSize QSortFilterProxyModel::span(const QModelIndex &index) const
{
    Q_D(const QSortFilterProxyModel);
    QModelIndex source_index = d->proxy_to_source(index);
    return d->model->span(source_index);
}

/*!
  \reimp
*/
void QSortFilterProxyModel::sort(int column, Qt::SortOrder order)
{
    Q_D(QSortFilterProxyModel);
    d->sort_column = column;
    d->sort_order = order;
    clear();
}

/*!
    \property QSortFilterProxyModel::filterRegExp
    \brief the QRegExp used to filter the contents of the source model

    Setting this property overwrites the current \l filterCaseSensitivity.

    \sa setCaseSensitivity(), setFilterWildcard(), setFilterFixedString()
*/
QRegExp QSortFilterProxyModel::filterRegExp() const
{
    Q_D(const QSortFilterProxyModel);
    return d->filter_regexp;
}

void QSortFilterProxyModel::setFilterRegExp(const QRegExp &regExp)
{
    Q_D(QSortFilterProxyModel);
    d->filter_regexp = regExp;
    d->filter_changed();
}

/*!
  \property QSortFilterProxyModel::filterKeyColumn
  \brief the column where the key used to filter the contents
  of the source model is read from.
*/
int QSortFilterProxyModel::filterKeyColumn() const
{
    Q_D(const QSortFilterProxyModel);
    return d->filter_column;
}

void QSortFilterProxyModel::setFilterKeyColumn(int column)
{
    Q_D(QSortFilterProxyModel);
    Q_ASSERT(d->model == &d->empty || column < d->model->columnCount());
    d->filter_column = column;
    d->filter_changed();
}

/*!
    \property QSortFilterProxyModel::filterCaseSensitivity
    \brief the case sensitivity of the QRegExp pattern used to filter the
    contents of the source model

    By default, the filter is case sensistive.

    \sa filterRegExp, sortCaseSensitivity
*/
Qt::CaseSensitivity QSortFilterProxyModel::filterCaseSensitivity() const
{
    Q_D(const QSortFilterProxyModel);
    return d->filter_regexp.caseSensitivity();
}

void QSortFilterProxyModel::setFilterCaseSensitivity(Qt::CaseSensitivity cs)
{
    Q_D(QSortFilterProxyModel);
    if (cs == d->filter_regexp.caseSensitivity())
        return;
    d->filter_regexp.setCaseSensitivity(cs);
    d->filter_changed();
}

/*!
    \since 4.2
    \property QSortFilterProxyModel::sortCaseSensitivity
    \brief the case sensitivity setting used for comparing strings when sorting

    By default, sorting is case sensistive.

    \sa filterCaseSensitivity, lessThan()
*/
Qt::CaseSensitivity QSortFilterProxyModel::sortCaseSensitivity() const
{
    Q_D(const QSortFilterProxyModel);
    return d->sort_casesensitivity;
}

void QSortFilterProxyModel::setSortCaseSensitivity(Qt::CaseSensitivity cs)
{
    Q_D(QSortFilterProxyModel);
    if (d->sort_casesensitivity == cs)
        return;

    d->sort_casesensitivity = cs;
    clear();
}

/*!
    \overload

    Sets the regular expression used to filter the contents
    of the source model to \a pattern.

    \sa setFilterCaseSensitivity(), setFilterWildcard(), setFilterFixedString()
*/
void QSortFilterProxyModel::setFilterRegExp(const QString &pattern)
{
    Q_D(QSortFilterProxyModel);
    d->filter_regexp.setPatternSyntax(QRegExp::RegExp);
    d->filter_regexp.setPattern(pattern); 
    d->filter_changed();
}

/*!
    Sets the wildcard expression used to filter the contents
    of the source model to \a pattern.

    \sa setFilterCaseSensitivity(), setFilterRegExp(), setFilterFixedString()
*/
void QSortFilterProxyModel::setFilterWildcard(const QString &pattern)
{
    Q_D(QSortFilterProxyModel);
    d->filter_regexp.setPatternSyntax(QRegExp::Wildcard);
    d->filter_regexp.setPattern(pattern);
    d->filter_changed();
}

/*!
    Sets the fixed string used to filter the contents
    of the source model to \a pattern.

    \sa setFilterCaseSensitivity(), setFilterRegExp(), setFilterWildcard()
*/
void QSortFilterProxyModel::setFilterFixedString(const QString &pattern)
{
    Q_D(QSortFilterProxyModel);
    d->filter_regexp.setPatternSyntax(QRegExp::FixedString);
    d->filter_regexp.setPattern(pattern);
    d->filter_changed();
}

/*!
    \since 4.2
    \property QSortFilterProxyModel::dynamicSortFilter
    \brief whether the proxy model is dynamically sorted and filtered
    whenever the contents of the source model change

    The default value is false.
*/
bool QSortFilterProxyModel::dynamicSortFilter() const
{
    Q_D(const QSortFilterProxyModel);
    return d->dynamic_sortfilter;
}

void QSortFilterProxyModel::setDynamicSortFilter(bool enable)
{
    Q_D(QSortFilterProxyModel);
    d->dynamic_sortfilter = enable;
}

/*!
    \since 4.2
    \property QSortFilterProxyModel::sortRole
    \brief the item role that is used to query the source model's data when sorting items

    The default value is Qt::DisplayRole.

    \sa lessThan()
*/
int QSortFilterProxyModel::sortRole() const
{
    Q_D(const QSortFilterProxyModel);
    return d->sort_role;
}

void QSortFilterProxyModel::setSortRole(int role)
{
    Q_D(QSortFilterProxyModel);
    d->sort_role = role;
    clear();
}

/*!
    \since 4.2
    \property QSortFilterProxyModel::filterRole
    \brief the item role that is used to query the source model's data when filtering items

    The default value is Qt::DisplayRole.

    \sa filterAcceptsRow()
*/
int QSortFilterProxyModel::filterRole() const
{
    Q_D(const QSortFilterProxyModel);
    return d->filter_role;
}

void QSortFilterProxyModel::setFilterRole(int role)
{
    Q_D(QSortFilterProxyModel);
    d->filter_role = role;
    clear();
}

/*!
  Clears the sorting filter model, removing all mapping.
*/
void QSortFilterProxyModel::clear()
{
    Q_D(QSortFilterProxyModel);
    d->clear_mapping();
    emit layoutChanged();
}

/*!
  Updates the mapping// , to reflect the change in the filter.

   This function should be called if you are implementing
   custom filtering, and you filter parameters changed.
*/
void QSortFilterProxyModel::filterChanged()
{
    Q_D(QSortFilterProxyModel);
    d->filter_changed();
}

/*!
    Returns true if the value of the item referred to by the given
    index \a left is less than the value of the item referred to by
    the given index \a right, otherwise returns false.

    This function is used as the < operator when sorting, and handles
    the following QVariant types:

    \list
    \o QVariant::Int
    \o QVariant::UInt
    \o QVariant::LongLong
    \o QVariant::ULongLong
    \o QVariant::Double
    \o QVariant::Char
    \o QVariant::Date
    \o QVariant::Time
    \o QVariant::DateTime
    \o QVariant::String
    \endlist

    Any other type will be converted to a QString using
    QVariant::toString().

    Comparison of \l{QString}s is case sensitive by default; this can
    be changed using the \l sortCaseSensitivity property.

    By default, the Qt::DisplayRole associated with the
    \l{QModelIndex}es is used for comparisons. This can be changed by
    setting the \l sortRole property.

    \sa sortRole, sortCaseSensitivity, dynamicSortFilter
*/
bool QSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    Q_D(const QSortFilterProxyModel);
    QVariant l = (left.model() ? left.model()->data(left, d->sort_role) : QVariant());
    QVariant r = (right.model() ? right.model()->data(right, d->sort_role) : QVariant());
    switch (l.type()) {
    case QVariant::Int:
        return l.toInt() < r.toInt();
    case QVariant::UInt:
        return l.toUInt() < r.toUInt();
    case QVariant::LongLong:
        return l.toLongLong() < r.toLongLong();
    case QVariant::ULongLong:
        return l.toULongLong() < r.toULongLong();
    case QVariant::Double:
        return l.toDouble() < r.toDouble();
    case QVariant::Char:
        return l.toChar() < r.toChar();
    case QVariant::Date:
        return l.toDate() < r.toDate();
    case QVariant::Time:
        return l.toTime() < r.toTime();
    case QVariant::DateTime:
        return l.toDateTime() < r.toDateTime();
    case QVariant::String:
    default:
        return l.toString().compare(r.toString(), d->sort_casesensitivity) < 0;
    }
    return false;
}

/*!
    Returns true if the value in the item in the row indicated by the
    given \a source_row and \a source_parent should be included in
    the model.

    By default, the Qt::DisplayRole is used to determine if the row
    should be accepted or not. This can be changed by setting the \l
    filterRole property.

    \sa filterRole, filterKeyColumn, filterRegExp, filterAcceptsColumn()
*/
bool QSortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    Q_D(const QSortFilterProxyModel);
    if (d->filter_regexp.isEmpty() || d->filter_column == -1)
        return true;
    QModelIndex source_index = d->model->index(source_row, d->filter_column, source_parent);
    if (!source_index.isValid()) // the column may not exist
        return true;
    QString key = d->model->data(source_index, d->filter_role).toString();
    return key.contains(d->filter_regexp);
}

/*!
    Returns true if the value in the item in the column indicated by
    the given \a source_column and \a source_parent should be
    included in the model.

    The default implementation simply returns true.

    \sa filterAcceptsRow()
*/
bool QSortFilterProxyModel::filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const
{
    Q_UNUSED(source_column);
    Q_UNUSED(source_parent);
    return true;
}

/*!
  Returns the source model index  corresponding to the
  given \a proxyIndex from the sorting filter  model.
*/
QModelIndex QSortFilterProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
    Q_D(const QSortFilterProxyModel);
    return d->proxy_to_source(proxyIndex);
}

/*!
  Returns the model index in the QSortFilterProxyModel given
  the \a sourceIndex from the source model.
*/
QModelIndex QSortFilterProxyModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    Q_D(const QSortFilterProxyModel);
    return d->source_to_proxy(sourceIndex);
}

/*!
  \reimp
*/
QItemSelection QSortFilterProxyModel::mapSelectionToSource(const QItemSelection &proxySelection) const
{
    return QAbstractProxyModel::mapSelectionToSource(proxySelection);
}

/*!
  \reimp
*/
QItemSelection QSortFilterProxyModel::mapSelectionFromSource(const QItemSelection &sourceSelection) const
{
    return QAbstractProxyModel::mapSelectionFromSource(sourceSelection);
}

#include "moc_qsortfilterproxymodel.cpp"

#endif // QT_NO_SORTFILTERPROXYMODEL

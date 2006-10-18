/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qlistwidget.h"

#ifndef QT_NO_LISTWIDGET
#include <qitemdelegate.h>
#include <private/qlistview_p.h>
#include <private/qwidgetitemdata_p.h>
#include <private/qlistwidget_p.h>

// workaround for VC++ 6.0 linker bug (?)
typedef bool(*LessThan)(const QPair<QListWidgetItem*,int>&,const QPair<QListWidgetItem*,int>&);

class QListWidgetMimeData : public QMimeData
{
    Q_OBJECT
public:
    QList<QListWidgetItem*> items;
};

#include "qlistwidget.moc"

QListModel::QListModel(QListWidget *parent)
    : QAbstractListModel(parent)
{
}

QListModel::~QListModel()
{
    clear();
}

void QListModel::clear()
{
    for (int i = 0; i < items.count(); ++i) {
        if (items.at(i)) {
            items.at(i)->model = 0;
            items.at(i)->view = 0;
            delete items.at(i);
        }
    }
    items.clear();
    reset();
}

QListWidgetItem *QListModel::at(int row) const
{
    return items.value(row);
}

void QListModel::remove(QListWidgetItem *item)
{
    if (!item)
        return;
    int row = items.indexOf(item);
    Q_ASSERT(row != -1);
    beginRemoveRows(QModelIndex(), row, row);
    items.at(row)->model = 0;
    items.at(row)->view = 0;
    items.removeAt(row);
    endRemoveRows();
}

void QListModel::insert(int row, QListWidgetItem *item)
{
    if (!item)
        return;

    item->model = this;
    item->view = ::qobject_cast<QListWidget*>(QObject::parent());
    if (item->view && item->view->isSortingEnabled()) {
        // sorted insertion
        QList<QListWidgetItem*>::iterator it;
        it = sortedInsertionIterator(items.begin(), items.end(),
                                     item->view->sortOrder(), item);
        row = qMax(it - items.begin(), 0);
    } else {
        if (row < 0)
            row = 0;
        else if (row > items.count())
            row = items.count();
    }
    beginInsertRows(QModelIndex(), row, row);
    items.insert(row, item);
    endInsertRows();
}

void QListModel::insert(int row, const QStringList &labels)
{
    const int count = labels.count();
    if (count <= 0)
        return;
    QListWidget *view = ::qobject_cast<QListWidget*>(QObject::parent());
    if (view && view->isSortingEnabled()) {
        // sorted insertion
        for (int i = 0; i < count; ++i) {
            QListWidgetItem *item = new QListWidgetItem(labels.at(i));
            insert(row, item);
        }
    } else {
        if (row < 0)
            row = 0;
        else if (row > items.count())
            row = items.count();
        beginInsertRows(QModelIndex(), row, row + count - 1);
        for (int i = 0; i < count; ++i) {
            QListWidgetItem *item = new QListWidgetItem(labels.at(i));
            item->model = this;
            item->view = ::qobject_cast<QListWidget*>(QObject::parent());
            items.insert(row++, item);
        }
        endInsertRows();
    }
}

QListWidgetItem *QListModel::take(int row)
{
    if (row < 0 || row >= items.count())
        return 0;

    beginRemoveRows(QModelIndex(), row, row);
    items.at(row)->model = 0;
    items.at(row)->view = 0;
    QListWidgetItem *item = items.takeAt(row);
    endRemoveRows();
    return item;
}

int QListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : items.count();
}

QModelIndex QListModel::index(QListWidgetItem *item) const
{
    if (!item || item->model != this)
        return QModelIndex();
    int row = items.lastIndexOf(item);  // lastIndexOf is an optimization in favor of indexOf
    if (row != -1)
        return createIndex(row, 0, item);
    return QModelIndex();
}

QModelIndex QListModel::index(int row, int column, const QModelIndex &parent) const
{
    if (hasIndex(row, column, parent))
        return createIndex(row, column, items.at(row));
    return QModelIndex();
}

QVariant QListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= items.count())
        return QVariant();
    return items.at(index.row())->data(role);
}

bool QListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= items.count())
        return false;
    items.at(index.row())->setData(role, value);
    return true;
}

bool QListModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if (count < 1 || row < 0 || row > rowCount() || parent.isValid())
        return false;

    beginInsertRows(QModelIndex(), row, row + count - 1);
    QListWidget *view = ::qobject_cast<QListWidget*>(QObject::parent());
    QListWidgetItem *itm = 0;

    for (int r = row; r < row + count; ++r) {
        itm = new QListWidgetItem;
        itm->view = view;
        itm->model = this;
        items.insert(r, itm);
    }

    endInsertRows();
    return true;
}

bool QListModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (count < 1 || row < 0 || (row + count) > rowCount() || parent.isValid())
        return false;

    beginRemoveRows(QModelIndex(), row, row + count - 1);
    QListWidgetItem *itm = 0;
    for (int r = row; r < row + count; ++r) {
        itm = items.takeAt(row);
        itm->view = 0;
        itm->model = 0;
        delete itm;
    }
    endRemoveRows();
    return true;
}

Qt::ItemFlags QListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid() || index.row() >= items.count() || index.model() != this)
        return Qt::ItemIsDropEnabled; // we allow drops outside the items
    return items.at(index.row())->flags();
}

void QListModel::sort(int column, Qt::SortOrder order)
{
    if (column != 0)
        return;

    emit layoutAboutToBeChanged();

    QVector < QPair<QListWidgetItem*,int> > sorting(items.count());
    for (int i = 0; i < items.count(); ++i) {
        sorting[i].first = items.at(i);
        sorting[i].second = i;
    }

    LessThan compare = (order == Qt::AscendingOrder ? &itemLessThan : &itemGreaterThan);
    qSort(sorting.begin(), sorting.end(), compare);

    QModelIndexList fromIndexes;
    QModelIndexList toIndexes;
    for (int r = 0; r < sorting.count(); ++r) {
        QListWidgetItem *item = sorting.at(r).first;
        items[r] = item;
        QModelIndex from = createIndex(sorting.at(r).second, 0, item);
        QModelIndex to = createIndex(r, 0, item);
        fromIndexes.append(from);
        toIndexes.append(to);
    }
    changePersistentIndexList(fromIndexes, toIndexes);

    emit layoutChanged();
}

/**
 * This function assumes that all items in the model except the items that are between 
 * (inclusive) start and end are sorted.
 * With these assumptions, this function can ensure that the model is sorted in a
 * much more efficient way than doing a naive 'sort everything'.
 * (provided that the range is relatively small compared to the total number of items)
 */
void QListModel::ensureSorted(int column, Qt::SortOrder order, int start, int end)
{
    if (column != 0)
        return;

    int count = end - start + 1;
    QVector < QPair<QListWidgetItem*,int> > sorting(count);
    for (int i = 0; i < count; ++i) {
        sorting[i].first = items.at(start + i);
        sorting[i].second = start + i;
    }

    LessThan compare = (order == Qt::AscendingOrder ? &itemLessThan : &itemGreaterThan);
    qSort(sorting.begin(), sorting.end(), compare);

    QModelIndexList oldPersistentIndexes = persistentIndexList();
    QModelIndexList newPersistentIndexes = oldPersistentIndexes;
    QList<QListWidgetItem*> tmp = items;
    QList<QListWidgetItem*>::iterator lit = tmp.begin();
    bool changed = false;
    for (int i = 0; i < count; ++i) {
        int oldRow = sorting.at(i).second;
        QListWidgetItem *item = tmp.takeAt(oldRow);
        lit = sortedInsertionIterator(lit, tmp.end(), order, item);
        int newRow = qMax(lit - tmp.begin(), 0);
        lit = tmp.insert(lit, item);
        if (newRow != oldRow) {
            changed = true;
            for (int j = i + 1; j < count; ++j) {
                int otherRow = sorting.at(j).second;
                if (oldRow < otherRow && newRow >= otherRow)
                    --sorting[j].second;
                else if (oldRow > otherRow && newRow <= otherRow)
                    ++sorting[j].second;
            }
            for (int k = 0; k < newPersistentIndexes.count(); ++k) {
                QModelIndex pi = newPersistentIndexes.at(k);
                int oldPersistentRow = pi.row();
                int newPersistentRow = oldPersistentRow;
                if (oldPersistentRow == oldRow)
                    newPersistentRow = newRow;
                else if (oldRow < oldPersistentRow && newRow >= oldPersistentRow)
                    newPersistentRow = oldPersistentRow - 1;
                else if (oldRow > oldPersistentRow && newRow <= oldPersistentRow)
                    newPersistentRow = oldPersistentRow + 1;
                if (newPersistentRow != oldPersistentRow)
                    newPersistentIndexes[k] = createIndex(newPersistentRow,
                                                          pi.column(), pi.internalPointer());
            }
        }
    }

    if (changed) {
        emit layoutAboutToBeChanged();
        items = tmp;
        changePersistentIndexList(oldPersistentIndexes, newPersistentIndexes);
        emit layoutChanged();
    }
}

bool QListModel::itemLessThan(const QPair<QListWidgetItem*,int> &left,
                              const QPair<QListWidgetItem*,int> &right)
{
    return (*left.first) < (*right.first);
}

bool QListModel::itemGreaterThan(const QPair<QListWidgetItem*,int> &left,
                                 const QPair<QListWidgetItem*,int> &right)
{
    return !((*left.first) < (*right.first));
}

QList<QListWidgetItem*>::iterator QListModel::sortedInsertionIterator(
    const QList<QListWidgetItem*>::iterator &begin,
    const QList<QListWidgetItem*>::iterator &end,
    Qt::SortOrder order, QListWidgetItem *item)
{
    if (order == Qt::AscendingOrder)
        return qLowerBound(begin, end, item, QListModelLessThan());
    return qLowerBound(begin, end, item, QListModelGreaterThan());
}

void QListModel::itemChanged(QListWidgetItem *item)
{
    QModelIndex idx = index(item);
    emit dataChanged(idx, idx);
}

QStringList QListModel::mimeTypes() const
{
    const QListWidget *view = ::qobject_cast<const QListWidget*>(QObject::parent());
    return view->mimeTypes();
}

QMimeData *QListModel::internalMimeData()  const
{
    return QAbstractItemModel::mimeData(cachedIndexes);
}

QMimeData *QListModel::mimeData(const QModelIndexList &indexes) const
{
    QList<QListWidgetItem*> itemlist;
    for (int i = 0; i < indexes.count(); ++i)
        itemlist << at(indexes.at(i).row());
    const QListWidget *view = ::qobject_cast<const QListWidget*>(QObject::parent());

    cachedIndexes = indexes;
    QMimeData *mimeData = view->mimeData(itemlist);
    cachedIndexes.clear();
    return mimeData;
}

#ifndef QT_NO_DRAGANDDROP
bool QListModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                              int row, int column, const QModelIndex &index)
{
    Q_UNUSED(column);
    QListWidget *view = ::qobject_cast<QListWidget*>(QObject::parent());
    if (index.isValid()) 
        row = index.row();
    else if (row == -1)
        row = items.count();

    return view->dropMimeData(row, data, action);
}

Qt::DropActions QListModel::supportedDropActions() const
{
    const QListWidget *view = ::qobject_cast<const QListWidget*>(QObject::parent());
    return view->supportedDropActions();
}
#endif // QT_NO_DRAGANDDROP

/*!
    \class QListWidgetItem
    \brief The QListWidgetItem class provides an item for use with the
    QListWidget item view class.

    \ingroup model-view

    QListWidgetItem is used to represent items in a list provided by the
    QListWidget class. Each item can hold several pieces of information,
    and will display these appropriately.

    The item view convenience classes use a classic item-based interface
    rather than a pure model/view approach. For a more flexible list view
    widget, consider using the QListView class with a standard model.

    List items can be automatically inserted into a list when they are
    constructed by specifying the list widget:

    \quotefile snippets/qlistwidget-using/mainwindow.cpp
    \skipto new QListWidgetItem(tr("Hazel
    \printuntil new QListWidgetItem(tr("Hazel

    They can also be created without a parent widget, and later inserted into
    a list (see \l{QListWidget::insertItem()}).

    List items are typically used to display text() and an icon(). These are
    set with the setText() and setIcon() functions. The appearance of the text
    can be customized with setFont(), setForeground(), and setBackground().
    Text in list items can be aligned using the setTextAlignment() function.
    Tooltips, status tips and "What's This?" help can be added to list items
    with setToolTip(), setStatusTip(), and setWhatsThis().

    By default, items are enabled, selectable, checkable, and can be the source
    of a drag and drop operation.
    Each item's flags can be changed by calling setFlags() with the appropriate
    value (see \l{Qt::ItemFlags}). Checkable items can be checked, unchecked and
    partially checked with the setCheckState() function. The corresponding 
    checkState() function indicates what check state the item currently has.

    The isItemHidden() function can be used to determine whether the
    item is hidden.  Items can be hidden with setItemHidden().

    \section1 Subclassing

    When subclassing QListWidgetItem to provide custom items, it is possible to
    define new types for them so that they can be distinguished from standard
    items. The constructors for subclasses that require this feature need to
    call the base class constructor with a new type value equal to or greater
    than \l UserType.

    \sa QListWidget, {Model/View Programming}, QTreeWidgetItem, QTableWidgetItem
*/

/*!
    \enum QListWidgetItem::ItemType

    This enum describes the types that are used to describe list widget items.

    \value Type     The default type for list widget items.
    \value UserType The minimum value for custom types. Values below UserType are
                    reserved by Qt.

    You can define new user types in QListWidgetItem subclasses to ensure that
    custom items are treated specially.

    \sa type()
*/

/*!
    \fn int QListWidgetItem::type() const

    Returns the type passed to the QListWidgetItem constructor.
*/

/*!
    \fn QListWidget *QListWidgetItem::listWidget() const

    Returns the list widget that contains the item.
*/

/*!
  \fn void QListWidgetItem::setSelected(bool select)
  \since 4.2

  Sets the selected state of the item to \a select.

  \sa isSelected()
*/

/*!
  \fn bool QListWidgetItem::isSelected() const
  \since 4.2

  Returns true if the item is selected, otherwise returns false.

  \sa setSelected()
*/

/*!
  \fn void QListWidgetItem::setHidden(bool hide)
  \since 4.2

  Hides the item if \a hide is true, otherwise shows the item.

  \sa isHidden()
*/

/*!
  \fn bool QListWidgetItem::isHidden() const
  \since 4.2

  Returns true if the item is hidden, otherwise returns false.

  \sa setHidden()
*/

/*!
    \fn QListWidgetItem::QListWidgetItem(QListWidget *parent, int type)

    Constructs an empty list widget item of the specified \a type with the
    given \a parent.
    If the parent is not specified, the item will need to be inserted into a
    list widget with QListWidget::insertItem().

    \sa type()
*/
QListWidgetItem::QListWidgetItem(QListWidget *view, int type)
    : rtti(type), view(view), model(0),
      itemFlags(Qt::ItemIsSelectable
                |Qt::ItemIsUserCheckable
                |Qt::ItemIsEnabled
                |Qt::ItemIsDragEnabled)
{
    if (view)
        model = ::qobject_cast<QListModel*>(view->model());
    if (model)
        model->insert(model->rowCount(), this);
}

/*!
    \fn QListWidgetItem::QListWidgetItem(const QString &text, QListWidget *parent, int type)

    Constructs an empty list widget item of the specified \a type with the
    given \a text and \a parent.
    If the parent is not specified, the item will need to be inserted into a
    list widget with QListWidget::insertItem().

    \sa type()
*/
QListWidgetItem::QListWidgetItem(const QString &text, QListWidget *view, int type)
    : rtti(type), view(view), model(0),
      itemFlags(Qt::ItemIsSelectable
                |Qt::ItemIsUserCheckable
                |Qt::ItemIsEnabled
                |Qt::ItemIsDragEnabled)
{
    setData(Qt::DisplayRole, text);
    if (view)
        model = ::qobject_cast<QListModel*>(view->model());
    if (model)
        model->insert(model->rowCount(), this);
}

/*!
    \fn QListWidgetItem::QListWidgetItem(const QIcon &icon, const QString &text, QListWidget *parent, int type)

    Constructs an empty list widget item of the specified \a type with the
    given \a icon, \a text and \a parent.
    If the parent is not specified, the item will need to be inserted into a
    list widget with QListWidget::insertItem().

    \sa type()
*/
QListWidgetItem::QListWidgetItem(const QIcon &icon,const QString &text,
                                 QListWidget *view, int type)
    : rtti(type), view(view), model(0),
      itemFlags(Qt::ItemIsSelectable
                |Qt::ItemIsUserCheckable
                |Qt::ItemIsEnabled
                |Qt::ItemIsDragEnabled)
{
    setData(Qt::DisplayRole, text);
    setData(Qt::DecorationRole, icon);
    if (view)
        model = ::qobject_cast<QListModel*>(view->model());
    if (model)
        model->insert(model->rowCount(), this);
}

/*!
  Destroys the list item.
*/
QListWidgetItem::~QListWidgetItem()
{
    if (model)
        model->remove(this);
}

/*!
  Creates an exact copy of the item.
*/
QListWidgetItem *QListWidgetItem::clone() const
{
    return new QListWidgetItem(*this);
}

/*!
  This function sets the data for a given \a role to the given \a value (see
  \l{Qt::ItemDataRole}). Reimplement this function if you need
  extra roles or special behavior for certain roles.

  \sa Qt::ItemDataRole, data()
*/
void QListWidgetItem::setData(int role, const QVariant &value)
{
    bool found = false;
    role = (role == Qt::EditRole ? Qt::DisplayRole : role);
    for (int i = 0; i < values.count(); ++i) {
        if (values.at(i).role == role) {
            if (values.at(i).value == value)
                return;
            values[i].value = value;
            found = true;
            break;
        }
    }
    if (!found)
        values.append(QWidgetItemData(role, value));
    if (model)
        model->itemChanged(this);
}

/*!
   This function returns the item's data for a given \a role (see
   {Qt::ItemDataRole}). Reimplement this function if you need
   extra roles or special behavior for certain roles.
*/
QVariant QListWidgetItem::data(int role) const
{
    role = (role == Qt::EditRole ? Qt::DisplayRole : role);
    for (int i = 0; i < values.count(); ++i)
        if (values.at(i).role == role)
            return values.at(i).value;
    return QVariant();
}

/*!
  Returns true if this item's text is less then \a other item's text;
  otherwise returns false.
*/
bool QListWidgetItem::operator<(const QListWidgetItem &other) const
{
    return text() < other.text();
}

#ifndef QT_NO_DATASTREAM

/*!
    Reads the item from stream \a in.

    \sa write()
*/
void QListWidgetItem::read(QDataStream &in)
{
    in >> values;
}

/*!
    Writes the item to stream \a out.

    \sa read()
*/
void QListWidgetItem::write(QDataStream &out) const
{
    out << values;
}

/*!
    \since 4.1

    Constructs a copy of \a other. Note that type() and listWidget()
    are not copied.

    This function is useful when reimplementing clone().

    \sa data(), flags()
*/
QListWidgetItem::QListWidgetItem(const QListWidgetItem &other)
    : rtti(Type), values(other.values), view(0), model(0),
      itemFlags(other.itemFlags)
{
}

/*!
    Assigns \a other's data and flags to this item. Note that type()
    and listWidget() are not copied.

    This function is useful when reimplementing clone().

    \sa data(), flags()
*/
QListWidgetItem &QListWidgetItem::operator=(const QListWidgetItem &other)
{
    values = other.values;
    itemFlags = other.itemFlags;
    return *this;
}

/*!
    \relates QListWidgetItem

    Writes the list widget item \a item to stream \a out.

    This operator uses QListWidgetItem::write().

    \sa {Format of the QDataStream Operators}
*/
QDataStream &operator<<(QDataStream &out, const QListWidgetItem &item)
{
    item.write(out);
    return out;
}

/*!
    \relates QListWidgetItem

    Reads a list widget item from stream \a in into \a item.

    This operator uses QListWidgetItem::read().

    \sa {Format of the QDataStream Operators}
*/
QDataStream &operator>>(QDataStream &in, QListWidgetItem &item)
{
    item.read(in);
    return in;
}

#endif // QT_NO_DATASTREAM

/*!
  \fn Qt::ItemFlags QListWidgetItem::flags() const

  Returns the item flags for this item (see \l{Qt::ItemFlags}).
*/

/*!
    \fn QString QListWidgetItem::text() const

    Returns the list item's text.

    \sa setText()
*/

/*!
    \fn QIcon QListWidgetItem::icon() const

    Returns the list item's icon.

    \sa setIcon(), {QAbstractItemView::iconSize}{iconSize}
*/

/*!
    \fn QString QListWidgetItem::statusTip() const

    Returns the list item's status tip.

    \sa setStatusTip()
*/

/*!
    \fn QString QListWidgetItem::toolTip() const

    Returns the list item's tooltip.

    \sa setToolTip() statusTip() whatsThis()
*/

/*!
    \fn QString QListWidgetItem::whatsThis() const

    Returns the list item's "What's This?" help text.

    \sa setWhatsThis() statusTip() toolTip()
*/

/*!
  \fn QFont QListWidgetItem::font() const

  Returns the font used to display this list item's text.
*/

/*!
  \fn int QListWidgetItem::textAlignment() const

  Returns the text alignment for the list item (see \l{Qt::AlignmentFlag}).
*/

/*!
    \fn QColor QListWidgetItem::backgroundColor() const
    \obsolete

    This function is deprecated. Use background() instead.
*/

/*!
    \fn QBrush QListWidgetItem::background() const
    \since 4.2

    Returns the brush used to display the list item's background.

    \sa setBackground() foreground()
*/

/*!
    \fn QColor QListWidgetItem::textColor() const
    \obsolete

    Returns the color used to display the list item's text.

    This function is deprecated. Use foreground() instead.
*/

/*!
    \fn QBrush QListWidgetItem::foreground() const
    \since 4.2

    Returns the brush used to display the list item's foreground (e.g. text).

    \sa setForeground() background()
*/

/*!
    \fn Qt::CheckState QListWidgetItem::checkState() const

    Returns the checked state of the list item (see \l{Qt::CheckState}).

    \sa flags()
*/

/*!
  \fn QSize QListWidgetItem::sizeHint() const
  \since 4.1

  Returns the size hint set for the list item.
*/

/*!
  \fn void QListWidgetItem::setSizeHint(const QSize &size)
  \since 4.1

  Sets the size hint for the list item to be \a size.
  If no size hint is set, the item delegate will compute the
  size hint based on the item data.
*/

/*!
  \fn void QListWidgetItem::setFlags(Qt::ItemFlags flags)

  Sets the item flags for the list item to \a flags (see
  \l{Qt::ItemFlags}).
*/
void QListWidgetItem::setFlags(Qt::ItemFlags aflags) {
    itemFlags = aflags;
    if (model)
        model->itemChanged(this);
}


/*!
    \fn void QListWidgetItem::setText(const QString &text)

    Sets the text for the list widget item's to the given \a text.

    \sa text()
*/

/*!
    \fn void QListWidgetItem::setIcon(const QIcon &icon)

    Sets the icon for the list item to the given \a icon.

    \sa icon(), text(), {QAbstractItemView::iconSize}{iconSize}
*/

/*!
    \fn void QListWidgetItem::setStatusTip(const QString &statusTip)

    Sets the status tip for the list item to the text specified by
    \a statusTip.

    \sa statusTip() setToolTip() setWhatsThis()
*/

/*!
    \fn void QListWidgetItem::setToolTip(const QString &toolTip)

    Sets the tooltip for the list item to the text specified by \a toolTip.

    \sa toolTip() setStatusTip() setWhatsThis()
*/

/*!
    \fn void QListWidgetItem::setWhatsThis(const QString &whatsThis)

    Sets the "What's This?" help for the list item to the text specified
    by \a whatsThis.

    \sa whatsThis() setStatusTip() setToolTip()
*/

/*!
  \fn void QListWidgetItem::setFont(const QFont &font)

  Sets the font used when painting the item to the given \a font.
*/

/*!
  \fn void QListWidgetItem::setTextAlignment(int alignment)

  Sets the list item's text alignment to \a alignment (see
  \l{Qt::AlignmentFlag}).
*/

/*!
    \fn void QListWidgetItem::setBackgroundColor(const QColor &color)
    \obsolete

    This function is deprecated. Use setBackground() instead.
*/

/*!
    \fn void QListWidgetItem::setBackground(const QBrush &brush)
    \since 4.2

    Sets the background brush of the list item to the given \a brush.

    \sa background() setForeground()
*/

/*!
    \fn void QListWidgetItem::setTextColor(const QColor &color)
    \obsolete

    This function is deprecated. Use setForeground() instead.
*/

/*!
    \fn void QListWidgetItem::setForeground(const QBrush &brush)
    \since 4.2

    Sets the foreground brush of the list item to the given \a brush.

    \sa foreground() setBackground()
*/

/*!
    \fn void QListWidgetItem::setCheckState(Qt::CheckState state)

    Sets the check state of the list item to \a state.

    \sa checkState()
*/

void QListWidgetPrivate::setup()
{
    Q_Q(QListWidget);
    q->QListView::setModel(new QListModel(q));
    // view signals
    QObject::connect(q, SIGNAL(pressed(QModelIndex)), q, SLOT(_q_emitItemPressed(QModelIndex)));
    QObject::connect(q, SIGNAL(clicked(QModelIndex)), q, SLOT(_q_emitItemClicked(QModelIndex)));
    QObject::connect(q, SIGNAL(doubleClicked(QModelIndex)),
                     q, SLOT(_q_emitItemDoubleClicked(QModelIndex)));
    QObject::connect(q, SIGNAL(activated(QModelIndex)), 
                     q, SLOT(_q_emitItemActivated(QModelIndex)));
    QObject::connect(q, SIGNAL(entered(QModelIndex)), q, SLOT(_q_emitItemEntered(QModelIndex)));
    QObject::connect(model(), SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                     q, SLOT(_q_emitItemChanged(QModelIndex)));
    QObject::connect(q->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
                     q, SLOT(_q_emitCurrentItemChanged(QModelIndex,QModelIndex)));
    QObject::connect(q->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                     q, SIGNAL(itemSelectionChanged()));
    QObject::connect(model(), SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                     q, SLOT(_q_dataChanged(QModelIndex,QModelIndex)));
    QObject::connect(model(), SIGNAL(columnsRemoved(QModelIndex,int,int)), q, SLOT(_q_sort()));
}

void QListWidgetPrivate::_q_emitItemPressed(const QModelIndex &index)
{
    Q_Q(QListWidget);
    emit q->itemPressed(model()->at(index.row()));
}

void QListWidgetPrivate::_q_emitItemClicked(const QModelIndex &index)
{
    Q_Q(QListWidget);
    emit q->itemClicked(model()->at(index.row()));
}

void QListWidgetPrivate::_q_emitItemDoubleClicked(const QModelIndex &index)
{
    Q_Q(QListWidget);
    emit q->itemDoubleClicked(model()->at(index.row()));
}

void QListWidgetPrivate::_q_emitItemActivated(const QModelIndex &index)
{
    Q_Q(QListWidget);
    emit q->itemActivated(model()->at(index.row()));
}

void QListWidgetPrivate::_q_emitItemEntered(const QModelIndex &index)
{
    Q_Q(QListWidget);
    emit q->itemEntered(model()->at(index.row()));
}

void QListWidgetPrivate::_q_emitItemChanged(const QModelIndex &index)
{
    Q_Q(QListWidget);
    emit q->itemChanged(model()->at(index.row()));
}

void QListWidgetPrivate::_q_emitCurrentItemChanged(const QModelIndex &current,
                                                const QModelIndex &previous)
{
    Q_Q(QListWidget);
    QListWidgetItem *currentItem = model()->at(current.row());
    emit q->currentItemChanged(currentItem, model()->at(previous.row()));
    emit q->currentTextChanged(currentItem ? currentItem->text() : QString());
    emit q->currentRowChanged(current.row());
}

void QListWidgetPrivate::_q_sort()
{
    if (sortingEnabled)
        model()->sort(0, sortOrder);
}

void QListWidgetPrivate::_q_dataChanged(const QModelIndex &topLeft,
                                        const QModelIndex &bottomRight)
{
    if (sortingEnabled && topLeft.isValid() && bottomRight.isValid())
        model()->ensureSorted(topLeft.column(), sortOrder,
                              topLeft.row(), bottomRight.row());
}

/*!
    \class QListWidget
    \brief The QListWidget class provides an item-based list widget.

    \ingroup model-view
    \mainclass

    QListWidget is a convenience class that provides a list view similar to
    the one supplied by QListView, but with a classic item-based interface
    for adding and removing items. QListWidget uses an internal model to
    manage each QListWidgetItem in the list.

    For a more flexible list view widget, use the QListView class with a
    standard model.

    List widgets are constructed in the same way as other widgets:

    \quotefile snippets/qlistwidget-using/mainwindow.cpp
    \skipto = new QListWidget
    \printline = new

    The selectionMode() of a list widget determines how many of the items in
    the list can be selected at the same time, and whether complex selections
    of items can be created. This can be set with the setSelectionMode()
    function.

    There are two ways to add items to the list: they can be constructed with
    the list widget as their parent widget, or they can be constructed with
    no parent widget and added to the list later. If a list widget already
    exists when the items are constructed, the first method is easier to use:

    \skipto new QListWidgetItem
    \printuntil new QListWidgetItem(tr("Pine")

    If you need to insert a new item into the list at a particular position,
    it is more convenient to construct the item without a parent widget and
    use the insertItem() function to place it within the list:

    \skipto QListWidgetItem *newItem
    \printuntil newItem->setText
    \skipto listWidget->insertItem
    \printuntil listWidget->insertItem

    For multiple items, insertItems() can be used instead. The number of
    items in the list is found with the count() function.
    To remove items from the list, use takeItem().

    The current item in the list can be found with currentItem(), and changed
    with setCurrentItem(). The user can also change the current item by
    navigating with the keyboard or clicking on a different item. When the
    current item changes, the currentItemChanged() signal is emitted with the
    new current item and the item that was previously current.

    \table 100%
    \row \o \inlineimage windowsxp-listview.png Screenshot of a Windows XP style list widget
         \o \inlineimage macintosh-listview.png Screenshot of a Macintosh style table widget
         \o \inlineimage plastique-listview.png Screenshot of a Plastique style table widget
    \row \o A \l{Windows XP Style Widget Gallery}{Windows XP style} list widget.
         \o A \l{Macintosh Style Widget Gallery}{Macintosh style} list widget.
         \o A \l{Plastique Style Widget Gallery}{Plastique style} list widget.
    \endtable

    \sa QListWidgetItem, QListView, QTreeView, {Model/View Programming},
        {Config Dialog Example}
*/

/*!
    \fn void QListWidget::addItem(QListWidgetItem *item)

    Inserts the \a item at the the end of the list widget.

    \sa insertItem()
*/

/*!
    \fn void QListWidget::addItem(const QString &label)

    Inserts an item with the text \a label at the end of the list
    widget.
*/

/*!
    \fn void QListWidget::addItems(const QStringList &labels)

    Inserts items with the text \a labels at the end of the list widget.

    \sa insertItems()
*/

/*!
    \fn void QListWidget::itemPressed(QListWidgetItem *item)

    This signal is emitted with the specified \a item when a mouse button is pressed
    on an item in the widget.

    \sa itemClicked(), itemDoubleClicked()
*/

/*!
    \fn void QListWidget::itemClicked(QListWidgetItem *item)

    This signal is emitted with the specified \a item when a mouse button is clicked
    on an item in the widget.

    \sa itemPressed(), itemDoubleClicked()
*/

/*!
    \fn void QListWidget::itemDoubleClicked(QListWidgetItem *item)

    This signal is emitted with the specified \a item when a mouse button is double
    clicked on an item in the widget.

    \sa itemClicked(), itemPressed()
*/

/*!
    \fn void QListWidget::itemActivated(QListWidgetItem *item)

    This signal is emitted when the \a item is activated. The \a item
    is activated when the user clicks or double clicks on it,
    depending on the system configuration. It is also activated when
    the user presses the activation key (on Windows and X11 this is
    the \gui Return key, on Mac OS X it is \key{Ctrl+0}).
*/

/*!
    \fn void QListWidget::itemEntered(QListWidgetItem *item)

    This signal is emitted when the mouse cursor enters an item. The
    \a item is the item entered. This signal is only emitted when
    mouseTracking is turned on, or when a mouse button is pressed
    while moving into an item.
*/

/*!
    \fn void QListWidget::itemChanged(QListWidgetItem *item)

    This signal is emitted whenever the data of \a item has changed.
*/

/*!
    \fn void QListWidget::currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)

    This signal is emitted whenever the current item changes. The \a
    previous item is the item that previously had the focus, \a
    current is the new current item.
*/

/*!
  \fn void QListWidget::currentTextChanged(const QString &currentText)

  This signal is emitted whenever the current item changes. The \a currentText
  is the text data in the current item. If there is no current item, the \a currentText
  is invalid.
*/

/*!
  \fn void QListWidget::currentRowChanged(int currentRow)

  This signal is emitted whenever the current item changes. The \a currentRow
  is the row of the current item. If there is no current item, the \a currentRow is -1.
*/

/*!
    \fn void QListWidget::itemSelectionChanged()

    This signal is emitted whenever the selection changes.

    \sa selectedItems() isItemSelected() currentItemChanged()
*/

/*!
    Constructs an empty QListWidget with the given \a parent.
*/

QListWidget::QListWidget(QWidget *parent)
    : QListView(*new QListWidgetPrivate(), parent)
{
    Q_D(QListWidget);
    d->setup();
}

/*!
    Destroys the list widget and all its items.
*/

QListWidget::~QListWidget()
{
}

/*!
    Returns the item that occupies the given \a row in the list if one has been
    set; otherwise returns 0.

    \sa row()
*/

QListWidgetItem *QListWidget::item(int row) const
{
    Q_D(const QListWidget);
    if (row < 0 || row >= d->model()->rowCount())
        return 0;
    return d->model()->at(row);
}

/*!
    Returns the row containing the given \a item.

    \sa item()
*/

int QListWidget::row(const QListWidgetItem *item) const
{
    Q_D(const QListWidget);
    return d->model()->index(const_cast<QListWidgetItem*>(item)).row();
}


/*!
    Inserts the \a item at the position in the list given by \a row.

    \sa addItem()
*/

void QListWidget::insertItem(int row, QListWidgetItem *item)
{
    Q_D(QListWidget);
    if (item && !item->model && !item->view)
        d->model()->insert(row, item);
}

/*!
    Inserts an item with the text \a label in the list widget at the
    position given by \a row.

    \sa addItem()
*/

void QListWidget::insertItem(int row, const QString &label)
{
    Q_D(QListWidget);
    d->model()->insert(row, new QListWidgetItem(label));
}

/*!
    Inserts items from the list of \a labels into the list, starting at the
    given \a row.

    \sa insertItem(), addItem()
*/

void QListWidget::insertItems(int row, const QStringList &labels)
{
    Q_D(QListWidget);
    d->model()->insert(row, labels);
}

/*!
    Removes and returns the item from the given \a row in the list widget; otherwise
    returns 0.

    Items removed from a list widget will not be managed by Qt, and will need to be
    deleted manually.

    \sa insertItem(), addItem()
*/

QListWidgetItem *QListWidget::takeItem(int row)
{
    Q_D(QListWidget);
    if (row < 0 || row >= d->model()->rowCount())
        return 0;
    return d->model()->take(row);
}

/*!
  \property QListWidget::count
  \brief the number of items in the list including any hidden items.
*/

int QListWidget::count() const
{
    Q_D(const QListWidget);
    return d->model()->rowCount();
}

/*!
  Returns the current item.
*/
QListWidgetItem *QListWidget::currentItem() const
{
    Q_D(const QListWidget);
    return d->model()->at(currentIndex().row());
}


/*!
  Sets the current item to \a item.

  Depending on the current selection mode, the item may also be selected.
*/
void QListWidget::setCurrentItem(QListWidgetItem *item)
{
    setCurrentRow(row(item));
}

/*!
  \property QListWidget::currentRow
  \brief the row of the current item.

  Depending on the current selection mode, the row may also be selected.
*/

int QListWidget::currentRow() const
{
    return currentIndex().row();
}

void QListWidget::setCurrentRow(int row)
{
    Q_D(QListWidget);
    QModelIndex index = d->model()->index(row);
    if (d->selectionMode == SingleSelection)
        selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
    else if (d->selectionMode == NoSelection)
        selectionModel()->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
    else
        selectionModel()->setCurrentIndex(index, QItemSelectionModel::SelectCurrent);
}

/*!
    Returns a pointer to the item at the coordinates \a p.
*/
QListWidgetItem *QListWidget::itemAt(const QPoint &p) const
{
    Q_D(const QListWidget);
    return d->model()->at(indexAt(p).row());

}

/*!
    \fn QListWidgetItem *QListWidget::itemAt(int x, int y) const
    \overload

    Returns a pointer to the item at the coordinates (\a x, \a y).
*/


/*!
  Returns the rectangle on the viewport occupied by the item at \a item.
*/
QRect QListWidget::visualItemRect(const QListWidgetItem *item) const
{
    Q_D(const QListWidget);
    QModelIndex index = d->model()->index(const_cast<QListWidgetItem*>(item));
    return visualRect(index);
}

/*!
  Sorts all the items in the list widget according to the specified \a order.
*/
void QListWidget::sortItems(Qt::SortOrder order)
{
    Q_D(QListWidget);
    d->sortOrder = order;
    d->model()->sort(0, order);
}

/*!
    \since Qt 4.2
    \property QListWidget::sortingEnabled
    \brief whether sorting is enabled

    If this property is true, sorting is enabled for the list; if the
    property is false, sorting is not enabled. The default value is false.
*/
void QListWidget::setSortingEnabled(bool enable)
{
    Q_D(QListWidget);
    d->sortingEnabled = enable;
}

bool QListWidget::isSortingEnabled() const
{
    Q_D(const QListWidget);
    return d->sortingEnabled;
}

/*!
  \internal
*/
Qt::SortOrder QListWidget::sortOrder() const
{
    Q_D(const QListWidget);
    return d->sortOrder;
}

/*!
  Starts editing the \a item if it is editable.
*/

void QListWidget::editItem(QListWidgetItem *item)
{
    Q_D(QListWidget);
    edit(d->model()->index(item));
}

/*!
  Opens an editor for the given \a item. The editor remains open after editing.

  \sa closePersistentEditor()
*/
void QListWidget::openPersistentEditor(QListWidgetItem *item)
{
    Q_D(QListWidget);
    QModelIndex index = d->model()->index(item);
    QAbstractItemView::openPersistentEditor(index);
}

/*!
  Closes the persistent editor for the given \a item.

  \sa openPersistentEditor()
*/
void QListWidget::closePersistentEditor(QListWidgetItem *item)
{
    Q_D(QListWidget);
    QModelIndex index = d->model()->index(item);
    QAbstractItemView::closePersistentEditor(index);
}

/*!
    \since 4.1

    Returns the widget displayed in the given \a item.
*/
QWidget *QListWidget::itemWidget(QListWidgetItem *item) const
{
    Q_D(const QListWidget);
    QModelIndex index = d->model()->index(item);
    return QAbstractItemView::indexWidget(index);
}

/*!
    \since 4.1

    Sets the \a widget to be displayed in the give \a item.

    This function should only be used to display static content in the place of a list
    widget item. If you want to display custom dynamic content or implement a custom
    editor widget, use QListView and subclass QItemDelegate instead.

    \sa {Delegate Classes}
*/
void QListWidget::setItemWidget(QListWidgetItem *item, QWidget *widget)
{
    Q_D(QListWidget);
    QModelIndex index = d->model()->index(item);
    QAbstractItemView::setIndexWidget(index, widget);
}

/*!
  Returns true if \a item is selected; otherwise returns false.

  \obsolete

  This function is deprecated. Use \l{QListWidgetItem::isSelected()} instead.
*/
bool QListWidget::isItemSelected(const QListWidgetItem *item) const
{
    Q_D(const QListWidget);
    QModelIndex index = d->model()->index(const_cast<QListWidgetItem*>(item));
    return selectionModel()->isSelected(index);
}

/*!
  Selects or deselects the given \a item depending on whether \a select is
  true of false.

  \obsolete

  This function is deprecated. Use \l{QListWidgetItem::setSelected()} instead.
*/
void QListWidget::setItemSelected(const QListWidgetItem *item, bool select)
{
    Q_D(QListWidget);
    QModelIndex index = d->model()->index(const_cast<QListWidgetItem*>(item));
    selectionModel()->select(index, select
                             ? QItemSelectionModel::Select
                             : QItemSelectionModel::Deselect);
}

/*!
  Returns a list of all selected items in the list widget.
*/

QList<QListWidgetItem*> QListWidget::selectedItems() const
{
    Q_D(const QListWidget);
    QModelIndexList indexes = selectionModel()->selectedIndexes();
    QList<QListWidgetItem*> items;
    for (int i = 0; i < indexes.count(); ++i)
        items.append(d->model()->at(indexes.at(i).row()));
    return items;
}

/*!
  Finds items with the text that matches the string \a text using the given \a flags.
*/

QList<QListWidgetItem*> QListWidget::findItems(const QString &text, Qt::MatchFlags flags) const
{
    Q_D(const QListWidget);
    QModelIndexList indexes = d->model()->match(model()->index(0, 0, QModelIndex()),
                                                Qt::DisplayRole, text, -1, flags);
    QList<QListWidgetItem*> items;
    for (int i = 0; i < indexes.size(); ++i)
        items.append(d->model()->at(indexes.at(i).row()));
    return items;
}

/*!
  Returns true if the \a item is explicitly hidden; otherwise returns false.

  \obsolete

  This function is deprecated. Use \l{QListWidgetItem::isHidden()} instead.
*/
bool QListWidget::isItemHidden(const QListWidgetItem *item) const
{
    return isRowHidden(row(item));
}

/*!
  If \a hide is true, the \a item will be hidden; otherwise it will be shown.

  \obsolete

  This function is deprecated. Use \l{QListWidgetItem::setHidden()} instead.
*/
void QListWidget::setItemHidden(const QListWidgetItem *item, bool hide)
{
    setRowHidden(row(item), hide);
}

/*!
    Scrolls the view if necessary to ensure that the \a item is
    visible. The \a hint parameter specifies more precisely where the
    \a item should be located after the operation.
*/

void QListWidget::scrollToItem(const QListWidgetItem *item, QAbstractItemView::ScrollHint hint)
{
    Q_D(QListWidget);
    QModelIndex index = d->model()->index(const_cast<QListWidgetItem*>(item));
    QListView::scrollTo(index, hint);
}

/*!
  Removes all items and selections in the view.
*/
void QListWidget::clear()
{
    Q_D(QListWidget);
    selectionModel()->clear();
    d->model()->clear();
}

/*!
    Returns a list of MIME types that can be used to describe a list of
    listwidget items.

    \sa mimeData()
*/
QStringList QListWidget::mimeTypes() const
{
    return d_func()->model()->QAbstractListModel::mimeTypes();
}

/*!
    Returns an object that contains a serialized description of the specified
    \a items. The format used to describe the items is obtained from the
    mimeTypes() function.

    If the list of items is empty, 0 is returned rather than a serialized
    empty list.
*/
QMimeData *QListWidget::mimeData(const QList<QListWidgetItem*>) const
{
    return d_func()->model()->internalMimeData();
}

#ifndef QT_NO_DRAGANDDROP
/*!
    Handles the \a data supplied by a drag and drop operation that ended with
    the given \a action in the given \a index.

    \sa supportedDropActions()
*/
bool QListWidget::dropMimeData(int index, const QMimeData *data, Qt::DropAction action)
{
    QModelIndex idx;
    int row = index;
    int column = 0;
    if (dropIndicatorPosition() == QAbstractItemView::OnItem) {
        // QAbstractListModel::dropMimeData will overwrite on the index if row == -1 and column == -1
        idx = model()->index(row, column);
        row = -1;
        column = -1;
    }
    return d_func()->model()->QAbstractListModel::dropMimeData(data, action , row, column, idx);
}

/*! \reimp */
void QListWidget::dropEvent(QDropEvent *event) {
    Q_D(QListWidget);
    if (event->source() == this && d->movement != Static) {
        QListView::dropEvent(event);
        return;
    }

    if (event->source() == this && (event->proposedAction() == Qt::MoveAction ||
                                    dragDropMode() == QAbstractItemView::InternalMove)) {
        QModelIndex topIndex;
        int col = -1;
        int row = -1;
        if (d->dropOn(event, &row, &col, &topIndex)) {
            QList<QModelIndex> selIndexes = selectedIndexes();
            QList<QPersistentModelIndex> persIndexes;
            for(int i = 0; i < selIndexes.count(); i++)
                persIndexes.append(selIndexes.at(i));

            if (persIndexes.contains(topIndex))
                return;

            QPersistentModelIndex dropRow = model()->index(row, col, topIndex);

            QList<QListWidgetItem *> taken;
            for (int i = 0; i < persIndexes.count(); ++i)
                taken.append(takeItem(persIndexes.at(i).row()));

            // insert them back in at their new positions
            for (int i = 0; i < persIndexes.count(); ++i) {
                // Either at a specific point or appended
                if (row == -1) {
                    insertItem(count(), taken.takeFirst());
                } else {
                    int r = dropRow.row() >= 0 ? dropRow.row() : row;
                    insertItem(qMin(r, count()), taken.takeFirst());
                }
            }

            event->accept();
            // Don't want QAbstractItemView to delete it because it was "moved" we already did it
            event->setDropAction(Qt::CopyAction);
        }
    }

    QListView::dropEvent(event);
}

/*!
  Returns the drop actions supported by this view.

  \sa Qt::DropActions
*/
Qt::DropActions QListWidget::supportedDropActions() const
{
    Q_D(const QListWidget);
    return d->model()->QAbstractListModel::supportedDropActions() | Qt::MoveAction;
}
#endif // QT_NO_DRAGANDDROP

/*!
  Returns a list of pointers to the items contained in the \a data object.
  If the object was not created by a QListWidget in the same process, the list
  is empty.
*/
QList<QListWidgetItem*> QListWidget::items(const QMimeData *data) const
{
    const QListWidgetMimeData *lwd = qobject_cast<const QListWidgetMimeData*>(data);
    if (lwd)
        return lwd->items;
    return QList<QListWidgetItem*>();
}

/*!
  Returns the QModelIndex assocated with the given \a item.
*/

QModelIndex QListWidget::indexFromItem(QListWidgetItem *item) const
{
    Q_D(const QListWidget);
    return d->model()->index(item);
}

/*!
  Returns a pointer to the QListWidgetItem assocated with the given \a index.
*/

QListWidgetItem *QListWidget::itemFromIndex(const QModelIndex &index) const
{
    Q_D(const QListWidget);
    if (d->isIndexValid(index))
        return d->model()->at(index.row());
    return 0;
}

/*!
  \internal
*/
void QListWidget::setModel(QAbstractItemModel * /*model*/)
{
    qFatal("QListWidget::setModel() - Changing the model of the QListWidget is not allowed.");
}

/*!
    \reimp
*/
bool QListWidget::event(QEvent *e)
{
    return QListView::event(e);
}

#include "moc_qlistwidget.cpp"
#endif // QT_NO_LISTWIDGET

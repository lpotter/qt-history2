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

#include "qtablewidget.h"
#include <qheaderview.h>
#include <qitemdelegate.h>
#include <qpainter.h>
#include <qabstractitemmodel.h>
#include <private/qabstractitemmodel_p.h>
#include <private/qtableview_p.h>
#include <private/qwidgetitemdata_p.h>

// workaround for VC++ 6.0 linker bug (?)
typedef bool(*LessThan)(const QTableWidgetItem *left, const QTableWidgetItem *right);

class QTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    QTableModel(int rows, int columns, QTableWidget *parent);
    ~QTableModel();

    bool insertRows(int row, int count = 1, const QModelIndex &parent = QModelIndex());
    bool insertColumns(int column, int count = 1, const QModelIndex &parent = QModelIndex());

    bool removeRows(int row, int count = 1, const QModelIndex &parent = QModelIndex());
    bool removeColumns(int column, int count = 1, const QModelIndex &parent = QModelIndex());

    void setItem(int row, int column, QTableWidgetItem *item);
    void setItem(const QModelIndex &index, QTableWidgetItem *item);
    QTableWidgetItem *takeItem(int row, int column);
    QTableWidgetItem *item(int row, int column) const;
    QTableWidgetItem *item(const QModelIndex &index) const;
    void removeItem(QTableWidgetItem *item);

    void setHorizontalHeaderItem(int section, QTableWidgetItem *item);
    void setVerticalHeaderItem(int section, QTableWidgetItem *item);
    QTableWidgetItem *horizontalHeaderItem(int section);
    QTableWidgetItem *verticalHeaderItem(int section);

    QModelIndex index(const QTableWidgetItem *item) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;

    void setRowCount(int rows);
    void setColumnCount(int columns);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role = QAbstractItemModel::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role);

    QAbstractItemModel::ItemFlags flags(const QModelIndex &index) const;

    void sort(int column, Qt::SortOrder order);
    static bool itemLessThan(const QTableWidgetItem *left, const QTableWidgetItem *right);
    static bool itemGreaterThan(const QTableWidgetItem *left, const QTableWidgetItem *right);
    QList<QTableWidgetItem*> find(const QRegExp &rx) const;

    bool isValid(const QModelIndex &index) const;
    inline long tableIndex(int row, int column) const
        {  return (row * horizontal.count()) + column; }

    void clear();
    void itemChanged(QTableWidgetItem *item);

    inline QTableWidgetItem *createItem() const
        { return prototype ? prototype->clone() : new QTableWidgetItem(); }
    inline const QTableWidgetItem *itemPrototype() const
        { return prototype; }
    inline void setItemPrototype(const QTableWidgetItem *item)
        { prototype = item; }

private:
    const QTableWidgetItem *prototype;
    QVector<QTableWidgetItem*> table;
    QVector<QTableWidgetItem*> vertical;
    QVector<QTableWidgetItem*> horizontal;
    mutable QChar strbuf[65];
};

#include "qtablewidget.moc"

QTableModel::QTableModel(int rows, int columns, QTableWidget *parent)
    : QAbstractTableModel(parent),
      prototype(0),
      table(rows * columns), vertical(rows), horizontal(columns)
{}

QTableModel::~QTableModel()
{
    clear();
}

bool QTableModel::insertRows(int row, int count, const QModelIndex &)
{
    // insert rows
    int rc = vertical.count();
    int cc = horizontal.count();
    vertical.insert(row, count, 0);
    if (rc == 0)
        table.resize(cc * count);
    else
        table.insert(tableIndex(row, 0), cc * count, 0);
    // update persistent model indexes
    for (int i = 0; i < persistentIndexesCount(); ++i) {
        QModelIndex idx = persistentIndexAt(i);
        if (idx.row() >= row)
            setPersistentIndex(i, index(idx.row() + count, idx.column()));
    }
    emit rowsInserted(QModelIndex(), row, row + count - 1);
    return true;
}

bool QTableModel::insertColumns(int column, int count, const QModelIndex &)
{
    // insert columns
    int rc = vertical.count();
    int cc = horizontal.count();
    horizontal.insert(column, count, 0);
    if (cc == 0)
        table.resize(rc * count);
    else
        for (int row = 0; row < rc; ++row)
            table.insert(tableIndex(row, column), count, 0);
    // update persistent model indexes
    for (int i = 0; i < persistentIndexesCount(); ++i) {
        QModelIndex idx = persistentIndexAt(i);
        if (idx.column() >= column)
            setPersistentIndex(i, index(idx.row(), idx.column() + count));
    }
    emit columnsInserted(QModelIndex(), column, column + count - 1);
    return true;
}

bool QTableModel::removeRows(int row, int count, const QModelIndex &)
{
    if (row >= 0 && row < vertical.count()) {
        emit rowsAboutToBeRemoved(QModelIndex(), row, row + count - 1);
        // remove rows
        int i = tableIndex(row, 0);
        int n = count * columnCount();
        table.remove(qMax(i, 0), n);
        vertical.remove(row, count);
        // update persistent model indexes
        for (int j = 0; j < persistentIndexesCount(); ++j) {
            QModelIndex idx = persistentIndexAt(j);
            if (idx.row() >= vertical.count())
                setPersistentIndex(j, QModelIndex());
            else if (idx.row() >= row)
                setPersistentIndex(j, index(idx.row() - count, idx.column()));
        }
        return true;
    }
    return false;
}

bool QTableModel::removeColumns(int column, int count, const QModelIndex &)
{
    if (column >= 0 && column < horizontal.count()) {
        emit columnsAboutToBeRemoved(QModelIndex(), column, column + count - 1);
        // remove columns
        for (int row = rowCount() - 1; row >= 0; --row)
            table.remove(tableIndex(row, column), count);
        horizontal.remove(column, count);
        // update persistent model indexes
        for (int i = 0; i < persistentIndexesCount(); ++i) {
            QModelIndex idx = persistentIndexAt(i);
            if (idx.column() >= horizontal.count())
                setPersistentIndex(i, QModelIndex());
            else if (idx.column() >= column)
                setPersistentIndex(i, index(idx.row(), idx.column() - count));
        }
        return true;
    }
    return false;
}

void QTableModel::setItem(int row, int column, QTableWidgetItem *item)
{
    item->model = this;
    int i = tableIndex(row, column);
    if (i >= 0 && i < table.count())
        table[i] = item;
}

void QTableModel::setItem(const QModelIndex &index, QTableWidgetItem *item)
{
    if (!isValid(index))
        return;

    long i = tableIndex(index.row(), index.column());
    delete table.at(i);
    table[i] = item;
}

QTableWidgetItem *QTableModel::takeItem(int row, int column)
{
    long i = tableIndex(row, column);
    QTableWidgetItem *itm = table.value(i);
    if (itm) {
        itm->model = 0;
        table[i] = 0;
    }
    return itm;
}

QTableWidgetItem *QTableModel::item(int row, int column) const
{
    return table.value(tableIndex(row, column));
}

QTableWidgetItem *QTableModel::item(const QModelIndex &index) const
{
    if (!isValid(index))
        return 0;
    return table.at(tableIndex(index.row(), index.column()));
}

void QTableModel::removeItem(QTableWidgetItem *item)
{
    int i = table.indexOf(item);
    if (i != -1) {
        table[i] = 0;
        QModelIndex idx = index(item);
        emit dataChanged(idx, idx);
        return;
    }

    i = vertical.indexOf(item);
    if (i != -1) {
        vertical[i] = 0;
        emit headerDataChanged(Qt::Vertical, i, i);
        return;
    }

    i = horizontal.indexOf(item);
    if (i != -1) {
        horizontal[i] = 0;
        emit headerDataChanged(Qt::Horizontal, i, i);
        return;
    }
}

void QTableModel::setHorizontalHeaderItem(int section, QTableWidgetItem *item)
{
    Q_ASSERT(item);
    item->model = this;
    vertical[section] = item;
}


void QTableModel::setVerticalHeaderItem(int section, QTableWidgetItem *item)
{
    Q_ASSERT(item);
    item->model = this;
    horizontal[section] = item;
}

QTableWidgetItem *QTableModel::horizontalHeaderItem(int section)
{
    return horizontal.value(section);
}

QTableWidgetItem *QTableModel::verticalHeaderItem(int section)
{
    return vertical.value(section);
}

QModelIndex QTableModel::index(const QTableWidgetItem *item) const
{
    int i = table.indexOf(const_cast<QTableWidgetItem*>(item));
    int row = i / columnCount();
    int col = i % columnCount();
    return index(row, col);
}

QModelIndex QTableModel::index(int row, int column, const QModelIndex &parent) const
{
    if (hasIndex(row, column, parent)) {
        QTableWidgetItem *item = table.at(tableIndex(row, column));
        return createIndex(row, column, item);
    }
    return QModelIndex();
}

void QTableModel::setRowCount(int rows)
{
    int rc = vertical.count();
    if (rc == rows)
        return;
    if (rc < rows)
        insertRows(qMax(rc, 0), rows - rc);
    else
        removeRows(qMax(rows, 0), rc - rows);
}

void QTableModel::setColumnCount(int columns)
{
    int cc = horizontal.count();
    if (cc == columns)
        return;
    if (cc < columns)
        insertColumns(qMax(cc, 0), columns - cc);
    else
        removeColumns(qMax(columns, 0), cc - columns);
}

int QTableModel::rowCount(const QModelIndex &) const
{
    return vertical.count();
}

int QTableModel::columnCount(const QModelIndex &) const
{
    return horizontal.count();
}

QVariant QTableModel::data(const QModelIndex &index, int role) const
{
    QTableWidgetItem *itm = item(index);
    if (itm)
        return itm->data(role);
    return QVariant();
}

bool QTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    QTableWidgetItem *itm = item(index);

    if (itm) {
        itm->setData(role, value);
        return true;
    }

    QTableWidget *view = qobject_cast<QTableWidget*>(QObject::parent());
    if (!view)
        return false;

    itm = createItem();
    itm->setData(role, value);
    view->setItem(index.row(), index.column(), itm);
    return true;
}

QAbstractItemModel::ItemFlags QTableModel::flags(const QModelIndex &index) const
{
    QTableWidgetItem *itm = item(index);
    if (itm)
        return itm->flags();
    return QAbstractItemModel::ItemIsEditable
        |QAbstractItemModel::ItemIsSelectable
        |QAbstractItemModel::ItemIsUserCheckable
        |QAbstractItemModel::ItemIsEnabled;
}

void QTableModel::sort(int column, Qt::SortOrder order)
{
    QVector<QTableWidgetItem*> sorting(rowCount());
    for (int i = 0; i < sorting.count(); ++i)
        sorting[i] = item(i, column);
    LessThan compare = order == Qt::AscendingOrder ? &itemLessThan : &itemGreaterThan;
    qSort(sorting.begin(), sorting.end(), compare);
    for (int j = 0; j < sorting.count(); ++j)
        table[tableIndex(j, column)] = sorting.at(j);
}

bool QTableModel::itemLessThan(const QTableWidgetItem *left, const QTableWidgetItem *right)
{
    return *left < *right;
}

bool QTableModel::itemGreaterThan(const QTableWidgetItem *left, const QTableWidgetItem *right)
{
    return !(*left < *right);
}

QList<QTableWidgetItem*> QTableModel::find(const QRegExp &rx) const
{
    QList<QTableWidgetItem*> result;
    QVector<QTableWidgetItem*>::const_iterator it = table.begin();
    for (; it != table.end(); ++it) {
        if (*it && rx.exactMatch((*it)->text()))
            result << (*it);
    }
    return result;
}

QVariant QTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    QTableWidgetItem *itm = 0;
    if (section < 0)
        return QVariant();
    if (orientation == Qt::Horizontal && section < horizontal.count())
        itm = horizontal.at(section);
    else if (section < vertical.count())
        itm = vertical.at(section);
    if (itm)
        return itm->data(role);
    if (role == DisplayRole)
        return QVariant(section);
    if (role == TextAlignmentRole)
        return Qt::AlignVCenter;
    return QVariant();
}

bool QTableModel::setHeaderData(int section, Qt::Orientation orientation,
                                const QVariant &value, int role)
{
    QTableWidgetItem *itm = 0;
    if (orientation == Qt::Horizontal)
        itm = horizontal.at(section);
    else
        itm = vertical.at(section);
    if (itm) {
        itm->setData(role, value);
        return true;
    }
    return false;
}

bool QTableModel::isValid(const QModelIndex &index) const
{
    return index.isValid() && index.row() < vertical.count() && index.column() < horizontal.count();
}

void QTableModel::clear()
{
    for (int i = 0; i < table.count(); ++i) {
        if (table.at(i)) {
            table.at(i)->model = 0;
            delete table.at(i);
            table[i] = 0;
        }
    }
    for (int j = 0; j < vertical.count(); ++j) {
        if (vertical.at(j)) {
            vertical.at(j)->model = 0;
            delete vertical.at(j);
            vertical[j] = 0;
        }
    }
    for (int k = 0; k < horizontal.count(); ++k) {
        if (horizontal.at(k)) {
            horizontal.at(k)->model = 0;
            delete horizontal.at(k);
            horizontal[k] = 0;
        }
    }
    emit reset();
}

void QTableModel::itemChanged(QTableWidgetItem *item)
{
    QModelIndex idx = index(item);
    emit dataChanged(idx, idx);
}


/*!
  \class QTableWidgetSelectionRange

  \brief The QTableWidgetSelectionRange provides a class for storing a
  selection range in a QTableWidget.

  \ingroup model-view

  The QTableWidgetSelectionRange class stores the top left and bottom
  right rows and columns of a selection range in a table. The
  selections in the table may consist of several selection ranges.

  \sa QTableWidget
*/

/*!
  Constructs the table selection range.
*/
QTableWidgetSelectionRange::QTableWidgetSelectionRange()
    : top(-1), left(-1), bottom(-1), right(-1)
{
}

/*!
  Constructs the table selection range from the given \a top, \a left, \a bottom and \a right table rows and columns.
*/
QTableWidgetSelectionRange::QTableWidgetSelectionRange(int top, int left, int bottom, int right)
    : top(top), left(left), bottom(bottom), right(right)
{
}

/*!
  Constructs a the table selection range by copying the given \a other table selection range.
*/
QTableWidgetSelectionRange::QTableWidgetSelectionRange(const QTableWidgetSelectionRange &other)
    : top(other.top), left(other.left), bottom(other.bottom), right(other.right)
{
}

/*!
  Destroys the table selection range.
*/
QTableWidgetSelectionRange::~QTableWidgetSelectionRange()
{
}

/*!
  \fn inline int QTableWidgetSelectionRange::topRow() const

  Returns the top row of the range.
*/

/*!
  \fn inline int QTableWidgetSelectionRange::bottomRow() const

  Returns the bottom row of the range.
*/

/*!
  \fn inline int QTableWidgetSelectionRange::leftColumn() const

  Returns the left column of the range.
*/

/*!
  \fn inline int QTableWidgetSelectionRange::rightColumn() const

  Returns the right column of the range.
*/


/*!
    \class QTableWidgetItem
    \brief The QTableWidgetItem class provides an item for use with the
    QTableWidget class.

    \ingroup model-view

    Table items are used to hold pieces of information for table widgets.
    Items usually contain text, icons, or checkboxes

    The QTableWidgetItem class is a convenience class that replaces the
    \c QTableItem class in Qt 3. It provides an item for use with
    the QTableWidget class.

    Items are usually constructed with a table widget as their parent then
    inserted at a particular position specified by row and column numbers:

    \quotefile snippets/qtablewidget-using/mainwindow.cpp
    \skipto QTableWidgetItem *newItem
    \printuntil tableWidget->setItem(

    Each item can have its own background color which is set with
    the setBackgroundColor() function. The current background color can be
    found with backgroundColor().
    The text label for each item can be rendered with its own font and text
    color. These are specified with the setFont() and setTextColor() functions,
    and read with font() and textColor().

    Items can be made checkable by setting the appropriate flag value with the
    setFlags() function. The current state of the item's flags can be read
    with flags().

    \sa QTableWidget
*/

/*!
    \fn Qt::CheckState QTableWidgetItem::checkState() const

    Returns the checked state of the list item (see \l{Qt::CheckState}).

    \sa flags()
*/

/*!
    \fn void QTableWidgetItem::setCheckState(Qt::CheckState state)

    Sets the check state of the table item to be \a state.

    \sa checked()
*/

/*!
    \fn QTableWidget *QTableWidgetItem::tableWidget() const

    Returns the table widget that contains the item.
*/

/*!
    \fn QAbstractItemModel::ItemFlags QTableWidgetItem::flags() const

    Returns the flags used to describe the item. These determine whether
    the item can be checked, edited, and selected.

    \sa setFlags()
*/

/*!
    \fn void QTableWidgetItem::setFlags(QAbstractItemModel::ItemFlags flags)

    Sets the flags for the item to the given \a flags. These determine whether
    the item can be selected or modified.

    \sa flags()
*/

/*!
    \fn QString QTableWidgetItem::text() const

    Returns the item's text.

    \sa setText()
*/

/*!
    \fn void QTableWidgetItem::setText(const QString &text)

    Sets the item's text to the \a text specified.

    \sa text() setFont() setTextColor()
*/

/*!
    \fn QIcon QTableWidgetItem::icon() const

    Returns the item's icon.

    \sa setIcon()
*/

/*!
    \fn void QTableWidgetItem::setIcon(const QIcon &icon)

    Sets the item's icon to the \a icon specified.

    \sa icon() setText()
*/

/*!
    \fn QString QTableWidgetItem::statusTip() const

    Returns the item's status tip.

    \sa setStatusTip()
*/

/*!
    \fn void QTableWidgetItem::setStatusTip(const QString &statusTip)

    Sets the item's status tip to the string specified by \a statusTip.

    \sa statusTip() setToolTip() setWhatsThis()
*/

/*!
    \fn QString QTableWidgetItem::toolTip() const

    Returns the item's tooltip.

    \sa setToolTip()
*/

/*!
    \fn void QTableWidgetItem::setToolTip(const QString &toolTip)

    Sets the item's tooltip to the string specified by \a toolTip.

    \sa toolTip() setStatusTip() setWhatsThis()
*/

/*!
    \fn QString QTableWidgetItem::whatsThis() const

    Returns the item's "What's This?" help.

    \sa setWhatsThis()
*/

/*!
    \fn void QTableWidgetItem::setWhatsThis(const QString &whatsThis)

    Sets the item's "What's This?" help to the string specified by \a whatsThis.

    \sa whatsThis() setStatusTip() setToolTip()
*/

/*!
    \fn QFont QTableWidgetItem::font() const

    Returns the font used to render the item's text.

    \sa setFont()
*/

/*!
    \fn void QTableWidgetItem::setFont(const QFont &font)

    Sets the font used to display the item's text to the given \a font.

    \sa font() setText() setTextColor()
*/

/*!
    \fn QColor QTableWidgetItem::backgroundColor() const

    Returns the color used to render the item's background.

    \sa textColor() setBackgroundColor()
*/

/*!
    \fn void QTableWidgetItem::setBackgroundColor(const QColor &color)

    Sets the item's background color to the specified \a color.

    \sa backgroundColor() setTextColor()
*/

/*!
    \fn QColor QTableWidgetItem::textColor() const

    Returns the color used to render the item's text.

    \sa backgroundColor() setTextColor()
*/

/*!
    \fn void QTableWidgetItem::setTextColor(const QColor &color)

    Sets the color used to display the item's text to the given \a color.

    \sa textColor() setFont() setText()
*/

/*!
  \fn int QTableWidgetItem::textAlignment() const

  Returns the text alignment for the item's text (see \l{Qt::AlignmentFlag}).
*/

/*!
  \fn void QTableWidgetItem::setTextAlignment(int alignment)

  Sets the text alignment for the item's text to the \a alignment
  specified (see \l{Qt::AlignmentFlag}).
*/

/*!
    Constructs a table item that does not belong to any table.
*/
QTableWidgetItem::QTableWidgetItem()
    : view(0), model(0),
      itemFlags(QAbstractItemModel::ItemIsEditable
                |QAbstractItemModel::ItemIsSelectable
                |QAbstractItemModel::ItemIsUserCheckable
                |QAbstractItemModel::ItemIsEnabled)
{
}

/*!
    Constructs a table item with the given \a text.
*/
QTableWidgetItem::QTableWidgetItem(const QString &text)
    : view(0), model(0),
      itemFlags(QAbstractItemModel::ItemIsEditable
                |QAbstractItemModel::ItemIsSelectable
                |QAbstractItemModel::ItemIsUserCheckable
                |QAbstractItemModel::ItemIsEnabled)
{
    setData(QAbstractItemModel::DisplayRole, text);
}

/*!
    Destroys the table item.
*/
QTableWidgetItem::~QTableWidgetItem()
{
    if (model)
        model->removeItem(this);
}

/*!
  Creates an exact copy of the item.
*/
QTableWidgetItem *QTableWidgetItem::clone() const
{
    QTableWidgetItem *item = new QTableWidgetItem();
    *item = *this;
    return item;
}

/*!
    Sets the item's data for the given \a role to the specified \a value.
*/
void QTableWidgetItem::setData(int role, const QVariant &value)
{
    role = (role == QAbstractItemModel::EditRole ? QAbstractItemModel::DisplayRole : role);
    for (int i = 0; i < values.count(); ++i) {
        if (values.at(i).role == role) {
            values[i].value = value;
            break;
        }
    }
    values.append(QWidgetItemData(role, value));
    if (model)
        model->itemChanged(this);
}

/*!
    Returns the item's data for the given \a role.
*/
QVariant QTableWidgetItem::data(int role) const
{
    role = (role == QAbstractItemModel::EditRole ? QAbstractItemModel::DisplayRole : role);
    for (int i = 0; i < values.count(); ++i)
        if (values.at(i).role == role)
            return values.at(i).value;
    return QVariant();
}

/*!
    Returns true if the item is less than the \a other item; otherwise returns
    false.
*/
bool QTableWidgetItem::operator<(const QTableWidgetItem &other) const
{
    return text() < other.text();
}

#ifndef QT_NO_DATASTREAM

/*!
    Reads the item from stream \a in.

    \sa write()
*/
void QTableWidgetItem::read(QDataStream &in)
{
    in >> values;
}

/*!
    Writes the item to stream \a out.

    \sa read()
*/
void QTableWidgetItem::write(QDataStream &out) const
{
    out << values;
}

/*!
    \relates QTableWidgetItem

    Reads a table widget item from stream \a in into \a item.

    This operator uses QTableWidgetItem::read().

    \sa {Format of the QDataStream Operators}
*/
QDataStream &operator>>(QDataStream &in, QTableWidgetItem &item)
{
    item.read(in);
    return in;
}

/*!
    \relates QTableWidgetItem

    Writes the table widget item \a item to stream \a out.

    This operator uses QTableWidgetItem::write().

    \sa {Format of the QDataStream Operators}
*/
QDataStream &operator<<(QDataStream &out, const QTableWidgetItem &item)
{
    item.write(out);
    return out;
}

#endif // QT_NO_DATASTREAM

/*!
    \class QTableWidget
    \brief The QTableWidget class provides an item-based table view with a default model.

    \ingroup model-view
    \mainclass

    Table widgets provide standard table display facilities for applications.
    The items in a QTableWidget are provided by QTableWidgetItem.

    If you want a table that uses your own data model you should
    use QTableView rather than this class.

    Table widgets can be constructed with the required numbers of rows and
    columns:

    \quotefile snippets/qtablewidget-using/mainwindow.cpp
    \skipto tableWidget = new
    \printuntil tableWidget = new

    Alternatively, tables can be constructed without a given size and resized
    later:

    \quotefile snippets/qtablewidget-resizing/mainwindow.cpp
    \skipto tableWidget = new
    \printuntil tableWidget = new
    \skipto tableWidget->setRowCount(
    \printuntil tableWidget->setColumnCount(

    Items are created ouside the table (with no parent widget) and inserted
    into the table with setItem():

    \skipto QTableWidgetItem *newItem
    \printuntil tableWidget->setItem(

    Tables can be given both horizontal and vertical headers. The simplest way
    to create the headers is to supply a list of strings to the
    setHorizontalHeaderLabels() and setVerticalHeaderLabels() functions. These
    will provide simple textual headers for the table's columns and rows.
    More sophisticated headers can be created from existing table items
    that are usually constructed outside the table. For example, we can
    construct a table item with an icon and aligned text, and use it as the
    header for a particular column:

    \quotefile snippets/qtablewidget-using/mainwindow.cpp
    \skipto QTableWidgetItem *cubesHeaderItem
    \printuntil cubesHeaderItem->setTextAlignment

    The number of rows in the table can be found with rowCount(), and the
    number of columns with columnCount(). The table can be cleared with the
    clear() function.

    \sa QTableWidgetItem \link model-view-programming.html Model/View Programming\endlink
*/

/*!
    \property QTableWidget::rowCount
    \brief the number of rows in the table
*/

/*!
    \property QTableWidget::columnCount
    \brief the number of columns in the table
*/

/*!
    \property QTableWidget::sortingEnabled
    \brief whether the items in the table can be sorted
    by clicking on the horizontal header.
*/

class QTableWidgetPrivate : public QTableViewPrivate
{
    Q_DECLARE_PUBLIC(QTableWidget)
public:
    QTableWidgetPrivate() : QTableViewPrivate(), sortingEnabled(false) {}
    inline QTableModel *model() const { return ::qobject_cast<QTableModel*>(q_func()->model()); }
    void setup();
    // view signals
    void emitItemPressed(const QModelIndex &index);
    void emitItemClicked(const QModelIndex &index);
    void emitItemDoubleClicked(const QModelIndex &index);
    void emitItemActivated(const QModelIndex &index);
    void emitItemEntered(const QModelIndex &index);
    // model signals
    void emitItemChanged(const QModelIndex &index);
    // selection signals
    void emitCurrentItemChanged(const QModelIndex &previous, const QModelIndex &current);
    // data
    bool sortingEnabled;
};

void QTableWidgetPrivate::setup()
{
    Q_Q(QTableWidget);
    // view signals
    QObject::connect(q, SIGNAL(pressed(QModelIndex)), q, SLOT(emitItemPressed(QModelIndex)));
    QObject::connect(q, SIGNAL(clicked(QModelIndex)), q, SLOT(emitItemClicked(QModelIndex)));
    QObject::connect(q, SIGNAL(doubleClicked(QModelIndex)),
                     q, SLOT(emitItemDoubleClicked(QModelIndex)));
    QObject::connect(q, SIGNAL(activated(QModelIndex)), q, SLOT(emitItemActivated(QModelIndex)));
    QObject::connect(q, SIGNAL(entered(QModelIndex)), q, SLOT(emitItemEntered(QModelIndex)));
    // model signals
    QObject::connect(model(), SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                     q, SLOT(emitItemChanged(QModelIndex)));
    // selection signals
    QObject::connect(q->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
                     q, SLOT(emitCurrentItemChanged(QModelIndex,QModelIndex)));
    QObject::connect(q->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                     q, SIGNAL(itemSelectionChanged()));
}

void QTableWidgetPrivate::emitItemPressed(const QModelIndex &index)
{
    Q_Q(QTableWidget);
    emit q->itemPressed(model()->item(index));
}

void QTableWidgetPrivate::emitItemClicked(const QModelIndex &index)
{
    Q_Q(QTableWidget);
    emit q->itemClicked(model()->item(index));
}

void QTableWidgetPrivate::emitItemDoubleClicked(const QModelIndex &index)
{
    Q_Q(QTableWidget);
    emit q->itemDoubleClicked(model()->item(index));
}

void QTableWidgetPrivate::emitItemActivated(const QModelIndex &index)
{
    Q_Q(QTableWidget);
    emit q->itemActivated(model()->item(index));
}

void QTableWidgetPrivate::emitItemEntered(const QModelIndex &index)
{
    Q_Q(QTableWidget);
    emit q->itemEntered(model()->item(index));
}

void QTableWidgetPrivate::emitItemChanged(const QModelIndex &index)
{
    Q_Q(QTableWidget);
    emit q->itemChanged(model()->item(index));
}

void QTableWidgetPrivate::emitCurrentItemChanged(const QModelIndex &current,
                                                 const QModelIndex &previous)
{
    Q_Q(QTableWidget);
    emit q->currentItemChanged(model()->item(current), model()->item(previous));
}

/*!
    \fn void QTableWidget::itemActivated(QTableWidgetItem *item)

    This signal is emitted when the item has been activated
*/

/*!
    \fn void QTableWidget::currentItemChanged(QTableWidgetItem *current, QTableWidgetItem *previous)

    This signal is emitted whenever the current item changes. The \a
    previous item is the item that previously had the focus, \a
    current is the new current item.
*/

/*!
    \fn void QTableWidget::itemSelectionChanged()

    This signal is emitted whenever the selection changes.

    \sa selectedItems() isItemSelected()
*/

/*!
    \fn void QTableWidget::itemEntered(QTableWidgetItem *item)

    This signal is emitted when the mouse cursor enters an item. The
    \a item is the item entered.

    This signal is only emitted when mouseTracking is turned on, or when a
    mouse button is pressed while moving into an item.
*/

/*!
    \fn void QTableWidget::itemChanged(QTableWidgetItem *item)

    This signal is emitted whenever the data of \a item has changed.
*/

/*!
    Creates a new table view with the given \a parent.
*/
QTableWidget::QTableWidget(QWidget *parent)
    : QTableView(*new QTableWidgetPrivate, parent)
{
    Q_D(QTableWidget);
    setModel(new QTableModel(0, 0, this));
    d->setup();
}

/*!
    Creates a new table view with the given \a rows and \a columns, and with the given \a parent.
*/
QTableWidget::QTableWidget(int rows, int columns, QWidget *parent)
    : QTableView(*new QTableWidgetPrivate, parent)
{
    Q_D(QTableWidget);
    setModel(new QTableModel(rows, columns, this));
    d->setup();
}

/*!
    Destroys this QTableWidget.
*/
QTableWidget::~QTableWidget()
{
}

/*!
    Sets the number of rows in this table's model to \a rows. If
    this is less than rowCount(), the data in the unwanted rows
    is discarded.

    \sa setColumnCount()
*/
void QTableWidget::setRowCount(int rows)
{
    Q_D(QTableWidget);
    d->model()->setRowCount(rows);
}

/*!
  Returns the number of rows.
*/

int QTableWidget::rowCount() const
{
    Q_D(const QTableWidget);
    return d->model()->rowCount();
}

/*!
    Sets the number of columns in this table's model to \a columns. If
    this is less than columnCount(), the data in the unwanted columns
    is discarded.

    \sa setRowCount()
*/
void QTableWidget::setColumnCount(int columns)
{
    Q_D(QTableWidget);
    d->model()->setColumnCount(columns);
}

/*!
  Returns the number of columns.
*/

int QTableWidget::columnCount() const
{
    Q_D(const QTableWidget);
    return d->model()->columnCount();
}

/*!
  Returns the row for the \a item.
*/
int QTableWidget::row(const QTableWidgetItem *item) const
{
    Q_ASSERT(item);
    Q_D(const QTableWidget);
    return d->model()->index(item).row();
}

/*!
  Returns the column for the \a item.
*/
int QTableWidget::column(const QTableWidgetItem *item) const
{
    Q_ASSERT(item);
    Q_D(const QTableWidget);
    return d->model()->index(item).column();
}


/*!
    Returns the item for the given \a row and \a column.

    \sa setItem()
*/
QTableWidgetItem *QTableWidget::item(int row, int column) const
{
    Q_D(const QTableWidget);
    return d->model()->item(row, column);
}

/*!
    Sets the item for the given \a row and \a column to \a item.

    \sa item()
*/
void QTableWidget::setItem(int row, int column, QTableWidgetItem *item)
{
    Q_ASSERT(item);
    Q_D(QTableWidget);
    item->view = this;
    d->model()->setItem(row, column, item);
}

/*!
    Removes the item at \a row and \a column from the table without deleting it.
*/
QTableWidgetItem *QTableWidget::takeItem(int row, int column)
{
    Q_D(QTableWidget);
    QTableWidgetItem *item = d->model()->takeItem(row, column);
    item->view = 0;
    return item;
}

/*!
  Removes item \a item from the table.
*/
void QTableWidget::removeItem(QTableWidgetItem *item)
{
    Q_ASSERT(item);
    Q_D(QTableWidget);
    d->model()->removeItem(item);
}

/*!
  Returns the vertical header item for row \a row.
*/
QTableWidgetItem *QTableWidget::verticalHeaderItem(int row) const
{
    Q_D(const QTableWidget);
    return d->model()->verticalHeaderItem(row);
}

/*!
  Sets the vertical header item for row \a row to \a item.
*/
void QTableWidget::setVerticalHeaderItem(int row, QTableWidgetItem *item)
{
    Q_D(QTableWidget);
    item->view = this;
    d->model()->setHorizontalHeaderItem(row, item);
}

/*!
  Returns the horizontal header item for column \a column.
*/
QTableWidgetItem *QTableWidget::horizontalHeaderItem(int column) const
{
    Q_D(const QTableWidget);
    return d->model()->horizontalHeaderItem(column);
}

/*!
  Sets the horizontal header item for column \a column to \a item.
*/
void QTableWidget::setHorizontalHeaderItem(int column, QTableWidgetItem *item)
{
    Q_D(QTableWidget);
    item->view = this;
    d->model()->setVerticalHeaderItem(column, item);
}

/*!
  Sets the vertical header labels using \a labels.
*/
void QTableWidget::setVerticalHeaderLabels(const QStringList &labels)
{
    Q_D(QTableWidget);
    QTableModel *model = d->model();
    QTableWidgetItem *item = 0;
    for (int i = 0; i < model->rowCount() && i < labels.count(); ++i) {
        item = model->verticalHeaderItem(i);
        if (!item) {
            item = model->createItem();
            setVerticalHeaderItem(i, item);
        }
        item->setText(labels.at(i));
    }
}

/*!
  Sets the horizontal header labels using \a labels.
*/
void QTableWidget::setHorizontalHeaderLabels(const QStringList &labels)
{
    Q_D(QTableWidget);
    QTableModel *model = d->model();
    QTableWidgetItem *item = 0;
    for (int i = 0; i < model->columnCount() && i < labels.count(); ++i) {
        item = model->horizontalHeaderItem(i);
        if (!item) {
            item = model->createItem();
            setHorizontalHeaderItem(i, item);
        }
        item->setText(labels.at(i));
    }
}

/*!
  Returns the row of the current item.
*/
int QTableWidget::currentRow() const
{
    return currentIndex().row();
}

/*!
  Returns the column of the current item.
*/
int QTableWidget::currentColumn() const
{
    return currentIndex().column();
}

/*!
  Returns the current item.
*/
QTableWidgetItem *QTableWidget::currentItem() const
{
    Q_D(const QTableWidget);
    return d->model()->item(currentIndex());
}

/*!
  Sets the current item to \a item.
*/
void QTableWidget::setCurrentItem(QTableWidgetItem *item)
{
    Q_D(QTableWidget);
    setCurrentIndex(d->model()->index(item));
}

/*!
  Sorts all the rows in the table widget based on \a column and \a order.
*/
void QTableWidget::sortItems(int column, Qt::SortOrder order)
{
    Q_D(QTableWidget);
    d->model()->sort(column, order);
    horizontalHeader()->setSortIndicator(column, order);
}

/*!
  If \a enable is true, the items in the widget will be sorted if the user
  clicks on a horizontal header section; otherwise sorting is disabled.
*/

void QTableWidget::setSortingEnabled(bool enable)
{
    Q_D(QTableWidget);
    d->sortingEnabled = enable;
    if (!enable && horizontalHeader()->isSortIndicatorShown())
        horizontalHeader()->setSortIndicatorShown(false);
}

/*!
  Returns if sorting is enabled; otherwise returns false.
  Sorting is enabled when the user clicks on a horizontal header section.
*/

bool QTableWidget::isSortingEnabled() const
{
    Q_D(const QTableWidget);
    return d->sortingEnabled;
}

/*!
  Opens an editor for the give \a item. The editor remains open after editing.

  \sa closePersistentEditor()
*/
void QTableWidget::openPersistentEditor(QTableWidgetItem *item)
{
    Q_ASSERT(item);
    Q_D(QTableWidget);
    QModelIndex index = d->model()->index(item);
    QAbstractItemView::openPersistentEditor(index);
}

/*!
  Closes the persistent editor for \a item.

  \sa openPersistentEditor()
*/
void QTableWidget::closePersistentEditor(QTableWidgetItem *item)
{
    Q_ASSERT(item);
    Q_D(QTableWidget);
    QModelIndex index = d->model()->index(item);
    QAbstractItemView::closePersistentEditor(index);
}

/*!
  Returns true if the \a item is selected, otherwise returns false.
*/

bool QTableWidget::isItemSelected(const QTableWidgetItem *item) const
{
    Q_D(const QTableWidget);
    QModelIndex index = d->model()->index(item);
    return selectionModel()->isSelected(index) && !isIndexHidden(index);
}

/*!
  Selects or deselects \a item depending on \a select.
*/
void QTableWidget::setItemSelected(const QTableWidgetItem *item, bool select)
{
    Q_D(QTableWidget);
    QModelIndex index = d->model()->index(item);
    selectionModel()->select(index, select ? QItemSelectionModel::Select : QItemSelectionModel::Deselect);
}

/*!
  Selects or deselects the \a range depending on \a select.
*/
void QTableWidget::setRangeSelected(const QTableWidgetSelectionRange &range, bool select)
{
    if (!model()->hasIndex(range.topRow(), range.leftColumn(), rootIndex()) ||
        !model()->hasIndex(range.bottomRow(), range.rightColumn(), rootIndex()))
        return;

    QModelIndex topLeft = model()->index(range.topRow(), range.leftColumn(), rootIndex());
    QModelIndex bottomRight = model()->index(range.bottomRow(), range.rightColumn(), rootIndex());

    selectionModel()->select(QItemSelection(topLeft, bottomRight),
                             select ? QItemSelectionModel::Select : QItemSelectionModel::Deselect);
}

/*!
  Returns a list of all selected ranges.

  \sa QTableWidgetSelectionRange
*/

QList<QTableWidgetSelectionRange> QTableWidget::selectedRanges() const
{
    const QList<QItemSelectionRange> ranges = selectionModel()->selection();
    QList<QTableWidgetSelectionRange> result;
    for (int i = 0; i < ranges.count(); ++i)
        result.append(QTableWidgetSelectionRange(ranges.at(i).top(),
                                                 ranges.at(i).left(),
                                                 ranges.at(i).bottom(),
                                                 ranges.at(i).right()));
    return result;
}

/*!
  Returns a list of all selected items.
*/

QList<QTableWidgetItem*> QTableWidget::selectedItems()
{
    Q_D(QTableWidget);
    QModelIndexList indexes = selectedIndexes();
    QList<QTableWidgetItem*> items;
    for (int i = 0; i < indexes.count(); ++i) {
        QModelIndex index = indexes.at(i);
        QTableWidgetItem *item = d->model()->item(index);
        if (item)
            items.append(item);
    }
    return items;
}

/*!
  Finds items that matches the \a rx.
*/

QList<QTableWidgetItem*> QTableWidget::findItems(const QRegExp &rx) const
{
    Q_D(const QTableWidget);
    return d->model()->find(rx);
}

/*!
  Returns the visual row of the given \a logicalRow.
*/

int QTableWidget::visualRow(int logicalRow) const
{
    return verticalHeader()->visualIndex(logicalRow);
}

/*!
  Returns the visual column of the given \a logicalColumn.
*/

int QTableWidget::visualColumn(int logicalColumn) const
{
    return horizontalHeader()->visualIndex(logicalColumn);
}

/*!
  Returns a pointer to the item at the coordinates \a p.
*/

QTableWidgetItem *QTableWidget::itemAt(const QPoint &p) const
{
    Q_D(const QTableWidget);
    return d->model()->item(indexAt(p));
}

/*!
  Returns the rectangle on the viewport occupied by the item at \a item.
*/
QRect QTableWidget::visualItemRect(const QTableWidgetItem *item) const
{
    Q_ASSERT(item);
    Q_D(const QTableWidget);
    QModelIndex index = d->model()->index(const_cast<QTableWidgetItem*>(item));
    Q_ASSERT(index.isValid());
    return visualRect(index);
}

/*!
  Scrolls the view if necessary to ensure that the \a item is visible.
*/

void QTableWidget::scrollToItem(const QTableWidgetItem *item)
{
    Q_ASSERT(item);
    Q_D(QTableWidget);
    QModelIndex index = d->model()->index(const_cast<QTableWidgetItem*>(item));
    Q_ASSERT(index.isValid());
    QTableView::scrollTo(index);
}

const QTableWidgetItem *QTableWidget::itemPrototype() const
{
    Q_D(const QTableWidget);
    return d->model()->itemPrototype();
}

void QTableWidget::setItemPrototype(const QTableWidgetItem *item)
{
    Q_D(QTableWidget);
    d->model()->setItemPrototype(item);
}

/*!
  Inserts an empty row into the table at \a row.
*/
void QTableWidget::insertRow(int row)
{
    Q_D(QTableWidget);
    d->model()->insertRows(row);
}

/*!
  Inserts an empty column into the table at \a column.
*/
void QTableWidget::insertColumn(int column)
{
    Q_D(QTableWidget);
    d->model()->insertColumns(column);
}

/*!
  Removes the row \a row and all its items from the table.
*/
void QTableWidget::removeRow(int row)
{
    Q_D(QTableWidget);
    d->model()->removeRows(row);
}

/*!
  Removes the column \a column and all its items from the table.
*/
void QTableWidget::removeColumn(int column)
{
    Q_D(QTableWidget);
    d->model()->removeColumns(column);
}

/*!
  Removes all items and selections in the view.
*/

void QTableWidget::clear()
{
    Q_D(QTableWidget);
    selectionModel()->clear();
    d->model()->clear();
}

/*!
  \internal
*/
void QTableWidget::setModel(QAbstractItemModel *model)
{
    QTableView::setModel(model);
}

#define d d_func()
#include "moc_qtablewidget.cpp"
#undef d

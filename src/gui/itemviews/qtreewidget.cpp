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

#include "qtreewidget.h"
#include <qstyle.h>
#include <qpainter.h>
#include <qitemdelegate.h>
#include <qapplication.h>
#include <qheaderview.h>
#include <private/qtreeview_p.h>

class QTreeModel : public QAbstractItemModel
{
    friend class QTreeWidget;
    friend class QTreeWidgetItem;

public:
    QTreeModel(int columns = 0, QObject *parent = 0);
    ~QTreeModel();

    void setColumnCount(int columns);
    int columnCount() const;
    void setColumnData(int column, int role, const QVariant &value);
    QVariant columnData(int column, int role) const;
    
    QTreeWidgetItem *item(const QModelIndex &index) const;

    QModelIndex index(QTreeWidgetItem *item) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex::Null,
                      QModelIndex::Type type = QModelIndex::View) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;

    QVariant data(const QModelIndex &index, int role = QAbstractItemModel::DisplayRole) const;
    bool setData(const QModelIndex &index, int role, const QVariant &value);

    bool insertRows(int row, const QModelIndex &parent = QModelIndex::Null, int count = 1);
    bool removeRows(int row, const QModelIndex &parent = QModelIndex::Null, int count = 1);

    bool isSelectable(const QModelIndex &index) const;
    bool isEditable(const QModelIndex &index) const;

protected:
    void append(QTreeWidgetItem *item);
    void emitRowsInserted(QTreeWidgetItem *item);

private:
    int c;
    QList<QTreeWidgetItem*> tree;
    mutable QTreeWidgetItem topHeader;
};

/*
  \class QTreeModel qtreewidget.h

  \brief The QTreeModel class manages the items stored in a tree view.

  \ingroup model-view
    \mainclass
*/

/*!
  \internal

  Constructs a tree model with a \a parent object and the given
  number of \a columns.
*/

QTreeModel::QTreeModel(int columns, QObject *parent)
    : QAbstractItemModel(parent), c(0)
{
    setColumnCount(columns);
}

/*!
  \internal

  Destroys this tree model.
*/

QTreeModel::~QTreeModel()
{
    for (int i = 0; i < tree.count(); ++i)
        delete tree.at(i);
}

/*!
  \internal

  Sets the number of \a columns in the tree model.
*/

void QTreeModel::setColumnCount(int columns)
{
    if (c == columns)
        return;
    int _c = c;
    c = columns;
    if (c < _c)
        emit columnsRemoved(QModelIndex::Null, qMax(_c - 1, 0), qMax(c - 1, 0));
    topHeader.setColumnCount(c);
    for (int i = _c; i < c; ++i)
        topHeader.setText(i, QString::number(i));
    if (c > _c)
        emit columnsInserted(QModelIndex::Null, qMax(_c - 1, 0), qMax(c - 1, 0));
}

/*!
  \internal

  Returns the number of columns in the tree model.
*/

int QTreeModel::columnCount() const
{
    return c;
}

/*!
  \internal

  Sets the value for the \a column and \a role to the value specified by \a value.
*/

void QTreeModel::setColumnData(int column, int role, const QVariant &value)
{
    QModelIndex index = createIndex(0, column, 0, QModelIndex::HorizontalHeader);
    setData(index, role, value);
}

/*!
  \internal

  Returns the value set for the given \a column and \a role.
*/

QVariant QTreeModel::columnData(int column, int role) const
{
    QModelIndex index = createIndex(0, column, 0, QModelIndex::HorizontalHeader);
    return data(index, role);
}

/*!
  \internal

  Returns the tree view item corresponding to the \a index given.

  \sa QModelIndex
*/

QTreeWidgetItem *QTreeModel::item(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;
    if (index.type() != QModelIndex::View)
        return &topHeader;
    return static_cast<QTreeWidgetItem *>(index.data());
}

/*!
  \internal

  Returns the model index that refers to the tree view \a item.
*/

QModelIndex QTreeModel::index(QTreeWidgetItem *item) const
{
    if (!item)
        return QModelIndex::Null;
    const QTreeWidgetItem *par = item->parent();
    int row = par ? par->children.indexOf(item) : tree.indexOf(item);
    return createIndex(row, 0, item);
}

/*!
  \internal

  Returns the model index with the given \a row, \a column, \a type,
  and \a parent.
*/

QModelIndex QTreeModel::index(int row, int column, const QModelIndex &parent,
                              QModelIndex::Type type) const
{
    int r = tree.count();
    if (row < 0 || row >= r || column < 0 || column >= c)
        return QModelIndex::Null;
    if (!parent.isValid()) {// toplevel
        QTreeWidgetItem *itm = const_cast<QTreeModel*>(this)->tree.at(row);
        if (itm)
            return createIndex(row, column, itm, type);
        return QModelIndex::Null;
    }
    QTreeWidgetItem *parentItem = item(parent);
    if (parentItem && row < parentItem->childCount()) {
        QTreeWidgetItem *itm = static_cast<QTreeWidgetItem *>(parentItem->child(row));
        if (itm)
            return createIndex(row, column, itm, type);
        return QModelIndex::Null;
    }
    return QModelIndex::Null;
}

/*!
  \internal

  Returns the parent model index of the index given as the \a child.
*/

QModelIndex QTreeModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex::Null;
    const QTreeWidgetItem *itm = reinterpret_cast<const QTreeWidgetItem *>(child.data());
    if (!itm)
        return QModelIndex::Null;
    QTreeWidgetItem *parent = const_cast<QTreeWidgetItem *>(itm->parent()); // FIXME
    return index(parent);
}

/*!
  \internal

  Returns the number of rows in the \a parent model index.
*/

int QTreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        QTreeWidgetItem *parentItem = item(parent);
        if (parentItem)
            return parentItem->childCount();
    }
    return tree.count();
}

/*!
  \internal

  Returns the number of columns in the item referred to by the given
  \a index.
*/

int QTreeModel::columnCount(const QModelIndex &) const
{
    return c;
}

/*!
  \internal

  Returns the data corresponding to the given model \a index and
  \a role.
*/

QVariant QTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    QTreeWidgetItem *itm = item(index);
    if (itm)
        return itm->data(index.column(), role);
    return QVariant();
}

/*!
  \internal

  Sets the data for the item specified by the \a index and \a role
  to that referred to by the \a value.

  Returns true if successful; otherwise returns false.
*/

bool QTreeModel::setData(const QModelIndex &index, int role, const QVariant &value)
{
    if (!index.isValid())
        return false;
    QTreeWidgetItem *itm = item(index);
    if (itm) {
        itm->setData(index.column(), role, value);
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

/*!
  \internal

  Inserts a tree view item into the \a parent item at the given
  \a row. Returns true if successful; otherwise returns false.

  If no valid parent is given, the item will be inserted into this
  tree model at the row given.
*/

bool QTreeModel::insertRows(int row, const QModelIndex &parent, int)
{
    if (parent.isValid()) {
        QTreeWidgetItem *p =  item(parent);
        if (p) {
            p->children.insert(row, new QTreeWidgetItem(p));
            return true;
        }
        return false;
    }
    tree.insert(row, new QTreeWidgetItem());
    return true;
}

/*!
  \internal

  Removes the given \a row from the \a parent item, and returns true
  if successful; otherwise false is returned.
*/

bool QTreeModel::removeRows(int row, const QModelIndex &parent, int)
{
    if (parent.isValid()) {
        QTreeWidgetItem *p = item(parent);
        if (p) {
            p->children.removeAt(row);
            return true;
        }
        return false;
    }
    tree.removeAt(row);
    return true;
}

/*!
  \internal

  Returns true if the item at the \a index given is selectable;
  otherwise returns false.
*/

bool QTreeModel::isSelectable(const QModelIndex &) const
{
    return true;
}

/*!
  \internal

  Returns true if the item at the \a index given is editable;
  otherwise returns false.
*/

bool QTreeModel::isEditable(const QModelIndex &) const
{
    return true;
}

/*!
  \internal

  Appends the tree view \a item to the tree model.*/

void QTreeModel::append(QTreeWidgetItem *item)
{
    int r = tree.count();
    tree.push_back(item);
    emit rowsInserted(QModelIndex::Null, r, r);
}

/*!
\internal

Emits the rowsInserted() signal for the rows containing the given \a item.

\sa rowsInserted()*/

void QTreeModel::emitRowsInserted(QTreeWidgetItem *item)
{
    QModelIndex idx = index(item);
    QModelIndex parentIndex = parent(idx);
    emit rowsInserted(parentIndex, idx.row(), idx.row());
}

// QTreeWidgetDelegate

class QTreeItemDelegate : public QItemDelegate
{
public:
    QTreeItemDelegate(QObject *parent);
    ~QTreeItemDelegate();

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QAbstractItemModel *model, const QModelIndex &index) const;
    QSize sizeHint(const QFontMetrics &fontMetrics, const QStyleOptionViewItem &option,
                   const QAbstractItemModel *model, const QModelIndex &index) const;
};

QTreeItemDelegate::QTreeItemDelegate(QObject *parent)
    : QItemDelegate(parent)
{
}

QTreeItemDelegate::~QTreeItemDelegate()
{
}

void QTreeItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                              const QAbstractItemModel *model, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    // enabled
    QTreeWidgetItem *item = static_cast<QTreeWidgetItem*>(index.data()); // only the treemodel is used
    if (!item->isEnabled())
        opt.state &= ~QStyle::Style_Enabled;
    // set font
    QVariant value = model->data(index, QTreeWidgetItem::FontRole);
    if (value.isValid())
        opt.font = value.toFont();
    // set text color
    value = model->data(index, QTreeWidgetItem::TextColorRole);
    if (value.isValid() && value.toColor().isValid())
        opt.palette.setColor(QPalette::Text, value.toColor());
    // draw the background color
    value = model->data(index, QTreeWidgetItem::BackgroundColorRole);
    if (value.isValid() && value.toColor().isValid())
        painter->fillRect(option.rect, value.toColor());
    // draw the item
    QItemDelegate::paint(painter, opt, model, index);
}

QSize QTreeItemDelegate::sizeHint(const QFontMetrics &/*fontMetrics*/,
                                  const QStyleOptionViewItem &option,
                                  const QAbstractItemModel *model,
                                  const QModelIndex &index) const
{
    QVariant value = model->data(index, QTreeWidgetItem::FontRole);
    QFont fnt;
    if (value.isValid())
        fnt = value.toFont();
    else
        fnt = option.font;
    return QItemDelegate::sizeHint(QFontMetrics(fnt), option, model, index);
}

// QTreeWidgetItem

/*!
  \class QTreeWidgetItem qtreewidget.h

  \brief The QTreeWidgetItem class provides an item for use with the
  predefined QTreeWidget class.

  \ingroup model-view

  The QTreeWidgetItem class provides a familiar interface for items displayed
  in a QTreeWidget widget.

  \sa QTreeWidget QTreeModel
*/

/*!
    \enum QTreeWidgetItem::Role

    \value FontRole
    \value BackgroundColorRole
    \value TextColorRole
*/

/*!
    \fn const QTreeWidgetItem *QTreeWidgetItem::parent() const

    Returns this tree widget item's parent (which is 0 if this item is
    a top-level item).

    \sa childCount() child()
*/


/*!
    \fn const QTreeWidgetItem *QTreeWidgetItem::child(int index) const

    Returns this tree widget item's \a{index}-th child, or 0 if there
    is no such child.

    \sa childCount() parent()
*/


/*!
    \fn QTreeWidgetItem *QTreeWidgetItem::child(int index)

    \overload
*/


/*!
    \fn int QTreeWidgetItem::childCount() const

    Returns the number of children this tree widget item has; it may
    be 0.

    \sa parent() child()
*/


/*!
    \fn int QTreeWidgetItem::columnCount() const

    Returns the number of columns that this item occupies.

    \sa text() icon()
*/


/*!
    \fn bool QTreeWidgetItem::isEditable() const

    Returns true if this tree widget item is editable; otherwise
    returns false.

    \sa setEditable()
*/


/*!
    \fn bool QTreeWidgetItem::isSelectable() const

    Returns true if this tree widget item is selectable; otherwise
    returns false.

    \sa setSelectable()
*/


/*!
    \fn void QTreeWidgetItem::setEditable(bool editable)

    If \a editable is true, sets this tree widget item to be editable;
    otherwise sets it to be read-only.

    \sa isEditable()
*/


/*!
    \fn void QTreeWidgetItem::setSelectable(bool selectable)

    If \a selectable is true, sets this tree widget item to be
    selectable; otherwise sets it to be impossible for the user to
    select.

    \sa isSelectable()
*/


/*!
    \fn bool QTreeWidgetItem::operator==(const QTreeWidgetItem &other) const

    Returns true if this tree widget item is the same as the \a other
    tree widget item, i.e. has the same text and iconset in every
    column; otherwise returns false.
*/


/*!
    \fn bool QTreeWidgetItem::operator!=(const QTreeWidgetItem &other) const

    Returns true if this tree widget item has at least one column
    where its text or iconset is different from the \a other tree
    widget item; otherwise returns false.
*/



/*!
  Constructs a tree widget item. The item must be inserted
  into a tree view.

  \sa QTreeModel::append() QTreeWidget::append()
*/

QTreeWidgetItem::QTreeWidgetItem()
    : par(0), view(0), columns(0), editable(true), selectable(true), enabled(true)
{
}

/*!
    \fn QTreeWidgetItem::QTreeWidgetItem(QTreeWidget *view)

    Constructs a tree widget item and inserts it into the given tree
    \a view.
*/

QTreeWidgetItem::QTreeWidgetItem(QTreeWidget *v)
    : par(0), view(v), columns(0), editable(true), selectable(true), enabled(true)
{
    if (view)
        view->append(this);
}

/*!
    Constructs a tree widget item with the given \a parent.
*/

QTreeWidgetItem::QTreeWidgetItem(QTreeWidgetItem *parent)
    : par(parent), view(parent->view), columns(0), editable(true), selectable(true), enabled(true)
{
    if (parent)
        parent->children.push_back(this);
    QTreeModel *model = ::qt_cast<QTreeModel*>(view->model());
    model->emitRowsInserted(this);
}

/*!
  Destroys this tree widget item.
*/

QTreeWidgetItem::~QTreeWidgetItem()
{
    for (int i = 0; i < children.count(); ++i)
        delete children.at(i);
}

/*!
    Sets the number of columns in the tree widget item to \a count.
*/

void QTreeWidgetItem::setColumnCount(int count)
{
    columns = count;
    values.resize(count);
}

QTreeWidgetItem::CheckedState QTreeWidgetItem::checkedState() const
{
    return static_cast<CheckedState>(retrieve(0, CheckRole).toInt());
}

void QTreeWidgetItem::setCheckedState(CheckedState state)
{
    store(0, CheckRole, static_cast<int>(state));
}

/*!
    Returns the text stored in the \a column.

  \sa data() QAbstractItemModel::Role
*/

QString QTreeWidgetItem::text(int column) const
{
    return retrieve(column, QAbstractItemModel::DisplayRole).toString();
}

/*!
    Sets the text for the item specified by the \a column to the given \a text.

    \sa text() setIcon()
*/

void QTreeWidgetItem::setText(int column, const QString &text)
{
    store(column, QAbstractItemModel::DisplayRole, text);
}

/*!
    Returns the icon stored in the \a column.

  \sa data() QAbstractItemModel::Role
*/

QIconSet QTreeWidgetItem::icon(int column) const
{
    return retrieve(column, QAbstractItemModel::DecorationRole).toIcon();
}

/*!
    Sets the icon for the item specified by the \a column to the given \a icon.

    \sa icon() setText()
*/

void QTreeWidgetItem::setIcon(int column, const QIconSet &icon)
{
    store(column, QAbstractItemModel::DecorationRole, icon);
}


/*!
    Returns the status tip text for the specified \a column.

    \sa setStatusTip() whatsThis() toolTip()
*/
QString QTreeWidgetItem::statusTip(int column) const
{
    return retrieve(column, QAbstractItemModel::StatusTipRole).toString();
}

/*!
    Sets the status tip text to \a statusTip for the specified \a
    column.

    \sa statusTip() setWhatsThis() setToolTip()
*/
void QTreeWidgetItem::setStatusTip(int column, const QString &statusTip)
{
    store(column, QAbstractItemModel::StatusTipRole, statusTip);
}

/*!
    Returns the tool tip text for the specified \a column.

    \sa setToolTip() whatsThis() statusTip()
*/
QString QTreeWidgetItem::toolTip(int column) const
{
    return retrieve(column, QAbstractItemModel::ToolTipRole).toString();
}

/*!
    Sets the tool tip text to \a toolTip for the specified \a
    column.

    \sa toolTip() setWhatsThis() setStatusTip()
*/
void QTreeWidgetItem::setToolTip(int column, const QString &toolTip)
{
    store(column, QAbstractItemModel::ToolTipRole, toolTip);
}

/*!
    Returns the What's This text for the specified \a column.

    \sa setWhatsThis() toolTip() statusTip()
*/
QString QTreeWidgetItem::whatsThis(int column) const
{
    return retrieve(column, QAbstractItemModel::WhatsThisRole).toString();
}

/*!
    Sets the What's This text to \a whatsThis for the specified \a
    column.

    \sa whatsThis() setToolTip() setStatusTip()
*/
void QTreeWidgetItem::setWhatsThis(int column, const QString &whatsThis)
{
    store(column, QAbstractItemModel::WhatsThisRole, whatsThis);
}

/*!
    Returns the text font for the specified \a column.

    \sa setFont() textColor()
*/
QFont QTreeWidgetItem::font(int column) const
{
    QVariant value = retrieve(column, FontRole);
    return value.isValid() ? value.toFont() : QApplication::font();
}

/*!
    Sets the \a font for the specified \a column.

    \sa font() setTextColor()
*/
void QTreeWidgetItem::setFont(int column, const QFont &font)
{
    store(column, FontRole, font);
}

/*!
    Returns the background color for the specified \a column.

    \sa setBackgroundColor() textColor()
*/
QColor QTreeWidgetItem::backgroundColor(int column) const
{
    QVariant value = retrieve(column, BackgroundColorRole);
    return value.isValid() ? value.toColor() : QColor();
}

/*!
    Sets the background \a color for the specified \a column.

    \sa backgroundColor() setTextColor()
*/
void QTreeWidgetItem::setBackgroundColor(int column, const QColor &color)
{
    store(column, BackgroundColorRole, color);
}

/*!
    Returns the text color for the specified \a column.

    \sa setTextColor() backgroundColor()
*/
QColor QTreeWidgetItem::textColor(int column) const
{
    QVariant value = retrieve(column, TextColorRole);
    return value.isValid() ? value.toColor() : QColor();
}

/*!
    Sets the text \a color for the specified \a column.

    \sa textColor() setBackgroundColor()
*/
void QTreeWidgetItem::setTextColor(int column, const QColor &color)
{
    store(column, TextColorRole, color);
}

/*!
    Returns the data stored in the \a column with the given \a role.

  \sa QAbstractItemModel::Role
*/

QVariant QTreeWidgetItem::data(int column, int role) const
{
    if (column < 0 || column >= columns)
        return QVariant();
    role = (role == QAbstractItemModel::EditRole ? QAbstractItemModel::DisplayRole : role);
    switch (role) {
    case QAbstractItemModel::DisplayRole:
        return text(column);
    case QAbstractItemModel::DecorationRole:
        return icon(column);
    case QAbstractItemModel::StatusTipRole:
        return statusTip(column);
    case QAbstractItemModel::ToolTipRole:
        return toolTip(column);
    case QAbstractItemModel::WhatsThisRole:
        return whatsThis(column);
    case FontRole:
        return font(column);
    case BackgroundColorRole:
        return backgroundColor(column);
    case TextColorRole:
        return textColor(column);
    }
    return QVariant();
}

/*!
    Sets the data for the item specified by the \a column and \a role
    to the given \a value.
*/

void QTreeWidgetItem::setData(int column, int role, const QVariant &value)
{
    role = (role == QAbstractItemModel::EditRole ? QAbstractItemModel::DisplayRole : role);
    switch (role) {
    case QAbstractItemModel::DisplayRole:
        setText(column, value.toString());
        break;
    case QAbstractItemModel::DecorationRole:
        setIcon(column, value.toIconSet());
        break;
    case QAbstractItemModel::StatusTipRole:
        setStatusTip(column, value.toString());
        break;
    case QAbstractItemModel::ToolTipRole:
        setToolTip(column, value.toString());
        break;
    case QAbstractItemModel::WhatsThisRole:
        setWhatsThis(column, value.toString());
        break;
    case FontRole:
        setFont(column, value.toFont());
        break;
    case BackgroundColorRole:
        setBackgroundColor(column, value.toColor());
        break;
    case TextColorRole:
        setTextColor(column, value.toColor());
        break;
    }
}

/*!
    Sets the value for the item's \a column and \a role to the given
    \a value.

    \sa store()
*/
void QTreeWidgetItem::store(int column, int role, const QVariant &value)
{
    if (column >= columns)
        setColumnCount(column + 1);
    QVector<Data> column_values = values.at(column);
    for (int i = 0; i < column_values.count(); ++i) {
        if (column_values.at(i).role == role) {
            values[column][i].value = value;
            return;
        }
    }
    values[column].append(Data(role, value));
}

/*!
    Returns the value for the item's \a column and \a role.

    \sa store()
*/
QVariant QTreeWidgetItem::retrieve(int column, int role) const
{
    const QVector<Data> column_values = values.at(column);
    for (int i = 0; i < column_values.count(); ++i)
        if (column_values.at(i).role == role)
            return column_values.at(i).value;
    return QVariant();
}

class QTreeWidgetPrivate : public QTreeViewPrivate
{
    Q_DECLARE_PUBLIC(QTreeWidget)
public:
    QTreeWidgetPrivate() : QTreeViewPrivate() {}
    inline QTreeModel *model() const { return ::qt_cast<QTreeModel*>(q_func()->model()); }
};

#define d d_func()
#define q q_func()

/*!
  \class QTreeWidget qtreewidget.h

  \brief The QTreeWidget class provides a tree view that uses a predefined
  tree model.

  \ingroup model-view

  The QTreeWidget class is a convenience class that replaces the \c QListView
  class. It provides a list view widget that takes advantage of Qt's
  model-view architecture.

  This class uses a default model to organize the data represented in the
  tree view, but also uses the QTreeWidgetItem class to provide a familiar
  interface for simple list structures.

  \omit
  In its simplest form, a tree view can be constructed and populated in
  the familiar way:

  \code
    QTreeWidget *view = new QTreeWidget(parent);

  \endcode
  \endomit

  \sa \link model-view-programming.html Model/View Programming\endlink QTreeModel QTreeWidgetItem
*/

/*!
  Constructs a tree view with the given \a parent widget, using the default
  model
*/

QTreeWidget::QTreeWidget(QWidget *parent)
    : QTreeView(*new QTreeViewPrivate(), parent)
{
    setModel(new QTreeModel(0, this));
    setItemDelegate(new QTreeItemDelegate(this));
    header()->setItemDelegate(new QTreeItemDelegate(header()));
}

/*!
  Retuns the number of header columns in the view.
*/

int QTreeWidget::columnCount() const
{
    return d->model()->columnCount();
}

/*!
  Sets the number of header \a columns in the tree view.
*/

void QTreeWidget::setColumnCount(int columns)
{
    d->model()->setColumnCount(columns);
}


/*!
  Returns the text for the given header \a column in the tree view.
*/

QString QTreeWidget::columnText(int column) const
{
    return d->model()->columnData(column, QAbstractItemModel::DisplayRole).toString();
}

/*!
  Sets the text for the header \a column to the \a text given.
*/

void QTreeWidget::setColumnText(int column, const QString &text)
{
    d->model()->setColumnData(column, QAbstractItemModel::DisplayRole, text);
}

/*!
  Returns the icon set for the given header \a column in the tree view.
*/

QIconSet QTreeWidget::columnIcon(int column) const
{
    return d->model()->columnData(column, QAbstractItemModel::DecorationRole).toIcon();
}

/*!
  Sets the icon set for the header \a column to that specified by \a icon.
*/

void QTreeWidget::setColumnIcon(int column, const QIconSet &icon)
{
    d->model()->setColumnData(column, QAbstractItemModel::DecorationRole, icon);
}

/*!
  Returns the status tip for the given header \a column in the tree view.
*/

QString QTreeWidget::columnStatusTip(int column) const
{
    return d->model()->columnData(column, QAbstractItemModel::StatusTipRole).toString();
}

/*!
  Sets the status tip for the header \a column to that specified by \a statusTip.
*/

void QTreeWidget::setColumnStatusTip(int column, const QString &statusTip)
{
    d->model()->setColumnData(column, QAbstractItemModel::StatusTipRole, statusTip);
}

/*!
  Returns the tool tip for the given header \a column in the tree view.
*/

QString QTreeWidget::columnToolTip(int column) const
{
    return d->model()->columnData(column, QAbstractItemModel::ToolTipRole).toString();
}
/*!
  Sets the tool tip for the header \a column to that specified by \a toolTip.
*/

void QTreeWidget::setColumnToolTip(int column, const QString &toolTip)
{
    d->model()->setColumnData(column, QAbstractItemModel::ToolTipRole, toolTip);
}

/*!
  Returns the "what's this" for the given header \a column in the tree view.
*/

QString QTreeWidget::columnWhatsThis(int column) const
{
    return d->model()->columnData(column, QAbstractItemModel::WhatsThisRole).toString();
}

/*!
  Sets the "what's this" for the header \a column to that specified by \a whatsThis.
*/

void QTreeWidget::setColumnWhatsThis(int column, const QString &whatsThis)
{
    d->model()->setColumnData(column, QAbstractItemModel::WhatsThisRole, whatsThis);
}

/*!
  Returns the font for the given header \a column in the tree view.
*/

QFont QTreeWidget::columnFont(int column) const
{
    QVariant value = d->model()->columnData(column, QTreeWidgetItem::FontRole);
    if (value.isValid())
        return value.toFont();
    return QApplication::font();
}

/*!
  Sets the font for the header \a column to that specified by \a font.
*/

void QTreeWidget::setColumnFont(int column, const QFont &font)
{
    d->model()->setColumnData(column, QTreeWidgetItem::FontRole, font);
}

/*!
  Returns the background color set for the given header \a column in the tree view.
 */

QColor QTreeWidget::columnBackgroundColor(int column) const
{
    return d->model()->columnData(column, QTreeWidgetItem::BackgroundColorRole).toColor();
}

/*!
  Sets the background color for the header \a column to that specified by \a color.
*/

void QTreeWidget::setColumnBackgroundColor(int column, const QColor &color)
{
    d->model()->setColumnData(column, QTreeWidgetItem::BackgroundColorRole, color);
}

/*!
  Returns the text color set for the given header \a column in the tree view.
 */

QColor QTreeWidget::columnTextColor(int column) const
{
    return d->model()->columnData(column, QTreeWidgetItem::TextColorRole).toColor();
}

/*!
  Sets the text color for the header \a column to that specified by \a color.
*/

void QTreeWidget::setColumnTextColor(int column, const QColor &color)
{
    d->model()->setColumnData(column, QTreeWidgetItem::TextColorRole, color);
}

/*!
  Returns the value set for the given header \a column and \a role in the tree view.
*/

QVariant QTreeWidget::columnData(int column, int role) const
{
    return d->model()->columnData(column, role);
}

/*!
  Sets the value for the given header \a column and \a role to that specified by \a value.
*/

void QTreeWidget::setColumnData(int column, int role, const QVariant &value)
{
    d->model()->setColumnData(column, role, value);
}
    
/*!
  Appends a tree view \a item to the tree view.
*/

void QTreeWidget::append(QTreeWidgetItem *item)
{
    d->model()->append(item);
}

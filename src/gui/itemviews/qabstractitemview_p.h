#ifndef QABSTRACTITEMVIEW_P_H
#define QABSTRACTITEMVIEW_P_H

#include <qpointer.h>
#include <private/qviewport_p.h>

class QAbstractItemViewPrivate : public QViewportPrivate
{
    Q_DECLARE_PUBLIC(QAbstractItemView);
public:
    QAbstractItemViewPrivate();
    virtual ~QAbstractItemViewPrivate();

    void init();

    inline bool shouldEdit(const QModelIndex &item, QItemDelegate::StartEditAction action)
	{ return q_func()->model()->isEditable(item) && (action & startEditActions); }

    bool createEditor(const QModelIndex &item, QItemDelegate::StartEditAction action, QEvent *event);
//     bool sendItemEvent(const QModelIndex &data, QEvent *event);
//     QWidget *findPersistentEditor( const QModelIndexPtr &item ) const;
//     void insertPersistentEditor( const QModelIndexPtr &item, QWidget *editor );

    QGenericItemModel *model;
    QPointer<QWidget> currentEditor;
    QModelIndex editItem;
    mutable QItemDelegate *delegate;
    QItemSelectionModelPointer selectionModel;
 //    QVector<int> sorting;
//     int sortColumn;

    // #### this datastructur is far to inefficient. We need a faster
    // #### way to associate data with an item and look it up.
    // use QHash<QGenericModelItenPtr, QWidget*>
    //QList<QPair<QModelIndex, QWidget*> > persistentEditors;

    bool layoutLock; // FIXME: this is because the layout will trigger resize events
    QRect dragRect;
    QModelIndex pressedItem;
    Qt::ButtonState pressedState;
    QAbstractItemView::State state;
    QPoint cursorIndex;
    int startEditActions;

    QModelIndex root;
//     int leftMargin;
//     int topMargin;
//     int rightMargin;
//     int bottomMargin;
    int horizontalFactor;
    int verticalFactor;
};

#endif

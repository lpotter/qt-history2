#ifndef QGENERICTABLE_H
#define QGENERICTABLE_H

#ifndef QT_H
#include <qabstractitemview.h>
#endif

class QGenericHeader;
class QGenericTableViewPrivate;

class Q_GUI_EXPORT QGenericTableView : public QAbstractItemView
{
    Q_OBJECT

public:
    QGenericTableView(QGenericItemModel *model, QWidget *parent = 0, const char *name = 0);
    ~QGenericTableView();

    QGenericHeader *topHeader() const;
    QGenericHeader *leftHeader() const;
    void setTopHeader(QGenericHeader *header);
    void setLeftHeader(QGenericHeader *header);

    int rowPosition(int row) const;
    int rowHeight(int row) const;
    int rowAt(int position) const;
    int columnPosition(int column) const;
    int columnWidth(int column) const;
    int columnAt(int position) const;
    bool isRowHidden(int row) const;
    bool isColumnHidden(int column) const;

    void setShowGrid(bool show);
    bool showGrid() const;

public slots:
    void selectRow(int row, ButtonState state = Qt::NoButton);
    void selectColumn(int column, ButtonState state = Qt::NoButton);
    void hideRow(int row);
    void hideColumn(int column);
    void showRow(int row);
    void showColumn(int column);

protected slots:
    void rowIndexChanged(int row, int oldIndex, int newIndex);
    void columnIndexChanged(int column, int oldIndex, int newIndex);
    void rowHeightChanged(int row, int oldHeight, int newHeight);
    void columnWidthChanged(int column, int oldWidth, int newWidth);
    void rowCountChanged(int oldCount, int newCount);
    void columnCountChanged(int oldCount, int newCount);

protected:
    void drawContents(QPainter *p, int cx, int cy, int cw, int ch);
    virtual void drawGrid(QPainter *p, int x, int y, int w, int h) const;
    QModelIndex itemAt(int x, int y) const;
    QModelIndex moveCursor(QAbstractItemView::CursorAction cursorAction, ButtonState state);
    QRect itemRect(const QModelIndex &item) const;

    void setSelection(const QRect &rect, QItemSelectionModel::SelectionUpdateMode mode);
    QRect selectionRect(const QItemSelection *selection) const;

    void updateGeometries();

    int rowSizeHint(int row) const;
    int columnSizeHint(int column) const;
private:
    QGenericTableViewPrivate *d;
};

#endif

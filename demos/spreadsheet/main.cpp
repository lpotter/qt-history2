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

#include <qapplication.h>
#include <qtablewidget.h>
#include <qheaderview.h>
#include <qstatusbar.h>
#include <qmainwindow.h>
#include <qmenu.h>
#include <qcolordialog.h>
#include <qfontdialog.h>
#include <qtoolbar.h>

class SpreadSheetItem : public QTableWidgetItem
{
public:
    SpreadSheetItem();
    SpreadSheetItem(const QString &text);

    QVariant data(int role) const;
    QVariant display() const;

    inline QString formula() const
        { return QTableWidgetItem::data(QAbstractItemModel::DisplayRole).toString(); }
    inline void setFormula(const QString &text)
        { QTableWidgetItem::setData(QAbstractItemModel::DisplayRole, text); }

    QPoint convertCoords(const QString coords) const;
};

SpreadSheetItem::SpreadSheetItem()
    : QTableWidgetItem() {}

SpreadSheetItem::SpreadSheetItem(const QString &text)
    : QTableWidgetItem(text) {}

QVariant SpreadSheetItem::data(int role) const
{
    if (role == QAbstractItemModel::EditRole || role == QAbstractItemModel::StatusTipRole)
        return formula();

    if (role == QAbstractItemModel::DisplayRole)
        return display();

    QString t = display().toString();
    bool isNumber = false;
    int number = t.toInt(&isNumber);

    // text color
    if (role == QAbstractItemModel::TextColorRole) {
        if (!isNumber)
            return QColor(Qt::black);
        else if (number < 0)
            return QColor(Qt::red);
        return QColor(Qt::blue);
    }

    // text alignment
    if (role == QAbstractItemModel::TextAlignmentRole)
        if (!t.isEmpty() && (t.at(0).isNumber() || t.at(0) == '-'))
            return (int)(Qt::AlignRight | Qt::AlignVCenter);
    
    return QTableWidgetItem::data(role);
}

QVariant SpreadSheetItem::display() const
{
    // check if the string is actially a formula or not
    QString formula = this->formula();
    QStringList list = formula.split(" ");
    if (list.count() != 3)
        return formula; // its a normal string

    QString op = list.at(0).toLower();
    QPoint one = convertCoords(list.at(1));
    QPoint two = convertCoords(list.at(2));
 
    QTableWidgetItem *start = view->item(one.y(), one.x());
    QTableWidgetItem *end = view->item(two.y(), two.x());

    if (!start || !end)
        return "Error: Item does not exist!";

    if (op == "sum") {
        int sum = 0;
        for (int r = view->row(start); r <= view->row(end); ++r)
            for (int c = view->column(start); c <= view->column(end); ++c)
                if (view->item(r, c) != this)
                    sum += view->item(r, c)->text().toInt();
        return (sum);
    } else if (op == "+") {
        return (start->text().toInt() + end->text().toInt());
    } else if (op == "-") {
        return (start->text().toInt() - end->text().toInt());
    } else if (op == "*") {
        return (start->text().toInt() * end->text().toInt());
    } else if (op == "/") {
        return (start->text().toInt() / end->text().toInt());
    } else {
        return "Error: Operation does not exist!";
    }
    return QVariant::Invalid;
}

QPoint SpreadSheetItem::convertCoords(const QString coords) const
{
    int r = 0;
    int c = coords.at(0).toUpper().ascii() - 'A';
    for (int i = 1; i < coords.count(); ++i) {
        r *= 10;
        r += coords.at(i).digitValue();
    }
    return QPoint(c, --r);
}

class SpreadSheetTable : public QTableWidget
{
    Q_OBJECT
public:
    SpreadSheetTable(int rows, int columns, QWidget *parent) :
        QTableWidget(rows, columns, parent) {}

    QItemSelectionModel::SelectionFlags selectionCommand(Qt::ButtonState state,
                                                         const QModelIndex &index,
                                                         QEvent::Type type,
                                                         Qt::Key key) const;

protected:
    QTableWidgetItem *createItem() const;
};

QItemSelectionModel::SelectionFlags SpreadSheetTable::selectionCommand(Qt::ButtonState state,
                                                                       const QModelIndex &index,
                                                                       QEvent::Type type,
                                                                       Qt::Key key) const
{
    if (state & Qt::RightButton || state & Qt::MidButton)
        return QItemSelectionModel::NoUpdate;
    return QTableWidget::selectionCommand(state, index, type, key);
}

QTableWidgetItem *SpreadSheetTable::createItem() const
{
    return new SpreadSheetItem();
}

class SpreadSheet : public QMainWindow
{
    Q_OBJECT
public:
    SpreadSheet(int rows, int cols, QWidget *parent = 0);

public slots:
    void updateStatus(QTableWidgetItem *item);
    void updateColor(QTableWidgetItem *item);
    void contextActions(QMenu *menu, QTableWidgetItem *item);
    void keyPressed(QTableWidgetItem *item, Qt::Key key);

    void selectColor();
    void selectFont();

    void sum();
    void clear();

protected:
    void setupContents();

private:
    QToolBar *toolBar;

    QAction *colorAction;
    QAction *fontAction;
    QAction *sumAction;
    QAction *clearAction;

    QTableWidget *table;
    QTableWidgetItem *contextItem;
};

SpreadSheet::SpreadSheet(int rows, int cols, QWidget *parent) 
    : QMainWindow(parent)
{
    toolBar = new QToolBar(this);

    QPixmap pix(16, 16);
    pix.fill(Qt::black);
    colorAction = toolBar->addAction(pix, tr("&Color..."));
    connect(colorAction, SIGNAL(triggered()), this, SLOT(selectColor()));
    fontAction = toolBar->addAction(QPixmap(":/images/font.png"), tr("Font..."));
    connect(fontAction, SIGNAL(triggered()), this, SLOT(selectFont()));
    sumAction = toolBar->addAction(QPixmap(":/images/sum.png"), tr("Sum"));
    connect(sumAction, SIGNAL(triggered()), this, SLOT(sum()));
    clearAction = toolBar->addAction(QPixmap(":/images/sum.png"), tr("Clear"));
    connect(clearAction, SIGNAL(triggered()), this, SLOT(clear()));

     // Hide the toolbar for now...
    toolBar->hide();

    table = new SpreadSheetTable(rows, cols, this);
    for (int c = 0; c < cols; ++c) {
        QString character(QChar('A' + c));
        table->setHorizontalHeaderItem(c, new QTableWidgetItem(character));
        table->horizontalHeaderItem(c)->setTextAlignment(Qt::AlignCenter);
    }
    table->setKeyTracking(true);

    setupContents();

    setCentralWidget(table);
    statusBar();
    connect(table, SIGNAL(currentChanged(QTableWidgetItem*, QTableWidgetItem*)),
            this, SLOT(updateStatus(QTableWidgetItem*)));
    connect(table, SIGNAL(currentChanged(QTableWidgetItem*, QTableWidgetItem*)),
            this, SLOT(updateColor(QTableWidgetItem*)));
    connect(table, SIGNAL(itemChanged(QTableWidgetItem*)),
            this, SLOT(updateStatus(QTableWidgetItem*)));
    connect(table, SIGNAL(keyPressed(QTableWidgetItem*, Qt::Key, Qt::ButtonState)),
            this, SLOT(keyPressed(QTableWidgetItem*, Qt::Key)));
    connect(table, SIGNAL(aboutToShowContextMenu(QMenu*, QTableWidgetItem*)),
            this, SLOT(contextActions(QMenu*, QTableWidgetItem*)));
}

void SpreadSheet::updateStatus(QTableWidgetItem *item)
{
    if (item && item == table->currentItem())
        statusBar()->message(item->data(QAbstractItemModel::StatusTipRole).toString(), 1000);
}

void SpreadSheet::updateColor(QTableWidgetItem *item)
{
    QPixmap pix(16, 16);
    QColor col;
    if (item)
        col = item->backgroundColor();
    if (!col.isValid())
        col = palette().base();
    pix.fill(col);
    colorAction->setIcon(pix);
}

void SpreadSheet::contextActions(QMenu *menu, QTableWidgetItem *item)
{
    if (table->selectedItems().count() > 0) {
        menu->addAction(colorAction);
        menu->addAction(fontAction);
        menu->addSeparator();
        menu->addAction(sumAction);
        menu->addSeparator();
        menu->addAction(clearAction);
    }
    contextItem = item;
}

void SpreadSheet::keyPressed(QTableWidgetItem *item, Qt::Key key)
{
    if (key == Qt::Key_Delete && item)
        item->clear();
}

void SpreadSheet::selectColor()
{
    QList<QTableWidgetItem*> selected = table->selectedItems();
    if (selected.count() == 0)
        return;
    QColor col = QColorDialog::getColor(selected.last()->backgroundColor(), this);
    if (!col.isValid())
        return;
    foreach(QTableWidgetItem *i, selected)
        if (i) i->setBackgroundColor(col);
    updateColor(table->currentItem());
}

void SpreadSheet::selectFont()
{
    QList<QTableWidgetItem*> selected = table->selectedItems();
    if (selected.count() == 0)
        return;
    bool ok = false;
    QFont fnt = QFontDialog::getFont(&ok, font(), this);
    if (!ok)
        return;
    foreach(QTableWidgetItem *i, selected)
        if (i) i->setFont(fnt);
}

void SpreadSheet::sum()
{
    QList<QTableWidgetItem*> selected = table->selectedItems();
    QTableWidgetItem *first = selected.first();
    QTableWidgetItem *last = selected.last();
    if (first && last && contextItem)
        contextItem->setText(QString("sum %1%2 %3%4").
                             arg(QChar('A' + (table->column(first)))).
                             arg((table->row(first) + 1)).
                             arg(QChar('A' + (table->column(last)))).
                             arg((table->row(last) + 1)));
}

void SpreadSheet::clear()
{
    QList<QTableWidgetItem*> selected = table->selectedItems();
    foreach (QTableWidgetItem *i, selected)
        if (i) i->clear();
}

void SpreadSheet::setupContents()
{
    // column 0
    table->setItem(0, 0, new SpreadSheetItem("Item"));
    table->item(0, 0)->setBackgroundColor(Qt::yellow);
    table->item(0, 0)->setToolTip("This column shows the purchased item/service");    
    table->setItem(1, 0, new SpreadSheetItem("AirportBus"));
    table->setItem(2, 0, new SpreadSheetItem("Flight (Munich)"));
    table->setItem(3, 0, new SpreadSheetItem("Lunch"));
    table->setItem(4, 0, new SpreadSheetItem("Flight (LA)"));
    table->setItem(5, 0, new SpreadSheetItem("Taxi"));
    table->setItem(6, 0, new SpreadSheetItem("Dinner"));
    table->setItem(7, 0, new SpreadSheetItem("Hotel"));
    table->setItem(8, 0, new SpreadSheetItem("Flight (Oslo)"));
    table->setItem(9, 0, new SpreadSheetItem("Total:"));
    table->item(9,0)->setBackgroundColor(Qt::lightGray);
    // column 1
    table->setItem(0, 1, new SpreadSheetItem("Price"));
    table->item(0, 1)->setBackgroundColor(Qt::yellow);
    table->item(0, 1)->setToolTip("This column shows the price of the purchase");
    table->setItem(1, 1, new SpreadSheetItem("150"));
    table->setItem(2, 1, new SpreadSheetItem("2350"));
    table->setItem(3, 1, new SpreadSheetItem("-14"));
    table->setItem(4, 1, new SpreadSheetItem("980"));
    table->setItem(5, 1, new SpreadSheetItem("5"));
    table->setItem(6, 1, new SpreadSheetItem("120"));
    table->setItem(7, 1, new SpreadSheetItem("300"));
    table->setItem(8, 1, new SpreadSheetItem("1240"));
    table->setItem(9, 1, new SpreadSheetItem());
    table->item(9,1)->setBackgroundColor(Qt::lightGray);
    // column 2
    table->setItem(0, 2, new SpreadSheetItem("Currency"));
    table->item(0,2)->setBackgroundColor(Qt::yellow);
    table->item(0,2)->setToolTip("This column shows the currency");
    table->setItem(1, 2, new SpreadSheetItem("NOK"));
    table->setItem(2, 2, new SpreadSheetItem("NOK"));
    table->setItem(3, 2, new SpreadSheetItem("EUR"));
    table->setItem(4, 2, new SpreadSheetItem("EUR"));
    table->setItem(5, 2, new SpreadSheetItem("USD"));
    table->setItem(6, 2, new SpreadSheetItem("USD"));
    table->setItem(7, 2, new SpreadSheetItem("USD"));
    table->setItem(8, 2, new SpreadSheetItem("USD"));
    table->setItem(9, 2, new SpreadSheetItem());
    table->item(9,2)->setBackgroundColor(Qt::lightGray);
    // column 3
    table->setItem(0, 3, new SpreadSheetItem("Ex.Rate"));
    table->item(0,3)->setBackgroundColor(Qt::yellow);
    table->item(0,3)->setToolTip("This column shows the exchange rate to NOK");
    table->setItem(1, 3, new SpreadSheetItem("1"));
    table->setItem(2, 3, new SpreadSheetItem("1"));
    table->setItem(3, 3, new SpreadSheetItem("8"));
    table->setItem(4, 3, new SpreadSheetItem("8"));
    table->setItem(5, 3, new SpreadSheetItem("7"));
    table->setItem(6, 3, new SpreadSheetItem("7"));
    table->setItem(7, 3, new SpreadSheetItem("7"));
    table->setItem(8, 3, new SpreadSheetItem("7"));
    table->setItem(9, 3, new SpreadSheetItem());
    table->item(9,3)->setBackgroundColor(Qt::lightGray);
    // column 4
    table->setItem(0, 4, new SpreadSheetItem("NOK"));
    table->item(0,4)->setBackgroundColor(Qt::yellow);
    table->item(0,4)->setToolTip("This column shows the expenses in NOK");
    table->setItem(1, 4, new SpreadSheetItem("* B2 D2"));
    table->setItem(2, 4, new SpreadSheetItem("* B3 D3"));
    table->setItem(3, 4, new SpreadSheetItem("* B4 D4"));
    table->setItem(4, 4, new SpreadSheetItem("* B5 D5"));
    table->setItem(5, 4, new SpreadSheetItem("* B6 D6"));
    table->setItem(6, 4, new SpreadSheetItem("* B7 D7"));
    table->setItem(7, 4, new SpreadSheetItem("* B8 D8"));
    table->setItem(8, 4, new SpreadSheetItem("* B9 D9"));
    table->setItem(9, 4, new SpreadSheetItem("sum E2 E9"));
    table->item(9,4)->setBackgroundColor(Qt::lightGray);

}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    SpreadSheet sheet(20, 10);
    app.setMainWidget(&sheet);
    sheet.show();
    sheet.resize(600, 350);
    return app.exec();
}

#include "main.moc"

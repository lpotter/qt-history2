/****************************************************************************
**
** Copyright (C) 2004-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <QtGui>

static QString encode_pos(int row, int col) {
    return QString(col + 'A') + QString::number(row + 1);
}

static void decode_pos(const QString &pos, int *row, int *col)
{
    if (pos.isEmpty()) {
        *col = -1;
        *row = -1;
    } else {
        *col = pos.at(0).toLatin1() - 'A';
        *row = pos.right(pos.size() - 1).toInt() - 1;
    }
}

class SpreadSheetDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    SpreadSheetDelegate(QObject *parent = 0);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &,
        const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
        const QModelIndex &index) const;
private slots:
    void commitAndCloseEditor();
};

SpreadSheetDelegate::SpreadSheetDelegate(QObject *parent)
    : QItemDelegate(parent) {}

QWidget *SpreadSheetDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem&,
    const QModelIndex &index) const
{
    if(index.column() == 1){
        QDateTimeEdit *editor = new QDateTimeEdit(parent);
        editor->setDisplayFormat("dd/M/yyyy");
        editor->setCalendarPopup(true);
        return editor;
    }
    QLineEdit *editor = new QLineEdit(parent);
    //create a completer with the strings in the column as model.
    QStringList allStrings;
    for(int i = 1; i<index.model()->rowCount(); i++){
        QString strItem(index.model()->data(index.sibling(i, index.column()), Qt::EditRole).toString());
        if(!allStrings.contains(strItem))
            allStrings.append(strItem);
    }
    QCompleter *autoComplete = new QCompleter(allStrings);
    editor->setCompleter(autoComplete);
    connect(editor, SIGNAL(editingFinished()), this, SLOT(commitAndCloseEditor()));
    return editor;
}

void SpreadSheetDelegate::commitAndCloseEditor()
{
    QLineEdit *editor = qobject_cast<QLineEdit *>(sender());
    emit commitData(editor);
    emit closeEditor(editor);
}

void SpreadSheetDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QLineEdit *edit = qobject_cast<QLineEdit *>(editor);
    if (edit) {
        edit->setText(index.model()->data(index, Qt::EditRole).toString());
    }
    else {
        QDateTimeEdit *dateEditor = qobject_cast<QDateTimeEdit *>(editor);
        if (dateEditor) {
            dateEditor->setDate(QDate::fromString(index.model()->data(index, Qt::EditRole).toString(), "d/M/yyyy"));
        }
    }
}

void SpreadSheetDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
    const QModelIndex &index) const
{
    QLineEdit *edit = qobject_cast<QLineEdit *>(editor);
    if (edit) {
        model->setData(index, edit->text());
    }
    else {
        QDateTimeEdit *dateEditor = qobject_cast<QDateTimeEdit *>(editor);
        if (dateEditor) {
            model->setData(index, dateEditor->date().toString("dd/M/yyyy"));
        }
    }
}

class SpreadSheetItem : public QTableWidgetItem
{
public:
    SpreadSheetItem();
    SpreadSheetItem(const QString &text);

    QTableWidgetItem *clone() const;

    QVariant data(int role) const;
    void setData(int role, const QVariant &value);
    QVariant display() const;

    inline QString formula() const
        { return QTableWidgetItem::data(Qt::DisplayRole).toString(); }

    static QVariant computeFormula(const QString &formula,
                                   const QTableWidget *widget,
                                   const QTableWidgetItem *self = 0);

private:
    mutable bool isResolving;
};

SpreadSheetItem::SpreadSheetItem()
    : QTableWidgetItem(), isResolving(false) {}

SpreadSheetItem::SpreadSheetItem(const QString &text)
    : QTableWidgetItem(text), isResolving(false) {}

QTableWidgetItem *SpreadSheetItem::clone() const
{
    SpreadSheetItem *item = new SpreadSheetItem();
    *item = *this;
    return item;
}

QVariant SpreadSheetItem::data(int role) const
{
    if (role == Qt::EditRole || role == Qt::StatusTipRole)
        return formula();

    if (role == Qt::DisplayRole)
        return display();

    QString t = display().toString();
    bool isNumber = false;
    int number = t.toInt(&isNumber);

    if (role == Qt::TextColorRole) {
        if (!isNumber)
            return qVariantFromValue(QColor(Qt::black));
        else if (number < 0)
            return qVariantFromValue(QColor(Qt::red));
        return qVariantFromValue(QColor(Qt::blue));
    }

    if (role == Qt::TextAlignmentRole)
        if (!t.isEmpty() && (t.at(0).isNumber() || t.at(0) == '-'))
            return (int)(Qt::AlignRight | Qt::AlignVCenter);

    return QTableWidgetItem::data(role);
}

void SpreadSheetItem::setData(int role, const QVariant &value)
{
    QTableWidgetItem::setData(role, value);
    if (tableWidget())
        tableWidget()->viewport()->update();
}

QVariant SpreadSheetItem::display() const
{
    // avoid circular dependencies
    if (isResolving)
        return QVariant();
    isResolving = true;
    QVariant result = computeFormula(formula(), tableWidget(), this);
    isResolving = false;
    return result;
}

QVariant SpreadSheetItem::computeFormula(const QString &formula,
                                         const QTableWidget *widget,
                                         const QTableWidgetItem *self)
{
    // check if the string is actually a formula or not
    QStringList list = formula.split(' ');
    if (list.isEmpty() || !widget)
        return formula; // its a normal string

    QString op = list.value(0).toLower();

    int firstRow = -1;
    int firstCol = -1;
    int secondRow = -1;
    int secondCol = -1;

    if (list.count() > 1)
        decode_pos(list.value(1), &firstRow, &firstCol);
    if (list.count() > 2)
        decode_pos(list.value(2), &secondRow, &secondCol);

    const QTableWidgetItem *start = widget->item(firstRow, firstCol);
    const QTableWidgetItem *end = widget->item(secondRow, secondCol);

    int firstVal = start ? start->text().toInt() : 0;
    int secondVal = end ? end->text().toInt() : 0;

    QVariant result;
    if (op == "sum") {
        int sum = 0;
        for (int r = firstRow; r <= secondRow; ++r) {
            for (int c = firstCol; c <= secondCol; ++c) {
                const QTableWidgetItem *tableItem = widget->item(r, c);
                if (tableItem && tableItem != self)
                    sum += tableItem->text().toInt();
            }
        }
        result = sum;
    } else if (op == "+") {
        result = (firstVal + secondVal);
    } else if (op == "-") {
        result = (firstVal - secondVal);
    } else if (op == "*") {
        result = (firstVal * secondVal);
    } else if (op == "/") {
        if (secondVal == 0)
            result = QString("nan");
        else
            result = (firstVal / secondVal);
    } else if (op == "=") {
        if (start)
        result = start->text();
    } else {
        result = formula;
    }

    return result;
}

#if !defined(QT_NO_DBUS) && defined(Q_OS_UNIX)
#include <QtDBus/QtDBus>

class SpreadSheetAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.trolltech.DBus.SpreadSheetDemo")

public:
    SpreadSheetAdaptor(QTableWidget *table) : QDBusAbstractAdaptor(table), table(table)
    { QDBusConnection::sessionBus().registerObject("/SpreadSheetDemo", table); }

public slots:
    QString computeFormula(const QString &formula)
    { return SpreadSheetItem::computeFormula(formula, table).toString(); }
private:
    QTableWidget *table;
};
#endif // QT_NO_DBUS

class SpreadSheet : public QMainWindow
{
    Q_OBJECT
public:
    SpreadSheet(int rows, int cols, QWidget *parent = 0);

public slots:
    void updateStatus(QTableWidgetItem *item);
    void updateColor(QTableWidgetItem *item);
    void updateLineEdit(QTableWidgetItem *item);
    void returnPressed();
    void selectColor();
    void selectFont();
    void clear();
    void showAbout();

    void actionSum();
    void actionSubtract();
    void actionAdd();
    void actionMultiply();
    void actionDivide();

protected:
    void setupContextMenu();
    void setupContents();

    void setupMenuBar();
    void createActions();

    void actionMath_helper(const QString &title, const QString &op);
    bool runInputDialog(const QString &title,
                        const QString &c1Text,
                        const QString &c2Text,
                        const QString &opText,
                        const QString &outText,
                        QString *cell1, QString *cell2, QString *outCell);

private:
    QToolBar *toolBar;
    QAction *colorAction;
    QAction *fontAction;
    QAction *firstSeparator;
    QAction *cell_sumAction;
    QAction *cell_addAction;
    QAction *cell_subAction;
    QAction *cell_mulAction;
    QAction *cell_divAction;
    QAction *secondSeparator;
    QAction *clearAction;
    QAction *aboutSpreadSheet;
    QAction *exitAction;

    QLabel *cellLabel;
    QTableWidget *table;
    QLineEdit *formulaInput;
};

SpreadSheet::SpreadSheet(int rows, int cols, QWidget *parent)
    : QMainWindow(parent)
{
    addToolBar(toolBar = new QToolBar());
    formulaInput = new QLineEdit();

    cellLabel = new QLabel(toolBar);
    cellLabel->setMinimumSize(80, 0);

    toolBar->addWidget(cellLabel);
    toolBar->addWidget(formulaInput);

    table = new QTableWidget(rows, cols, this);
    for (int c = 0; c < cols; ++c) {
        QString character(QChar('A' + c));
        table->setHorizontalHeaderItem(c, new QTableWidgetItem(character));
    }
    table->setItemPrototype(table->item(rows - 1, cols - 1));
    table->setItemDelegate(new SpreadSheetDelegate());
#if !defined(QT_NO_DBUS) && defined(Q_OS_UNIX)
    new SpreadSheetAdaptor(table);
#endif
    createActions();

    updateColor(0);
    setupMenuBar();
    setupContextMenu();
    setupContents();
    setCentralWidget(table);

    statusBar();
    connect(table, SIGNAL(currentItemChanged(QTableWidgetItem*, QTableWidgetItem*)),
            this, SLOT(updateStatus(QTableWidgetItem*)));
    connect(table, SIGNAL(currentItemChanged(QTableWidgetItem*, QTableWidgetItem*)),
            this, SLOT(updateColor(QTableWidgetItem*)));
    connect(table, SIGNAL(currentItemChanged(QTableWidgetItem*, QTableWidgetItem*)),
            this, SLOT(updateLineEdit(QTableWidgetItem*)));
    connect(table, SIGNAL(itemChanged(QTableWidgetItem*)),
            this, SLOT(updateStatus(QTableWidgetItem*)));
    connect(formulaInput, SIGNAL(returnPressed()), this, SLOT(returnPressed()));
    connect(table, SIGNAL(itemChanged(QTableWidgetItem*)),
            this, SLOT(updateLineEdit(QTableWidgetItem*)));
    setWindowTitle(tr("Spreadsheet"));
}

void SpreadSheet::createActions()
{
    cell_sumAction = new QAction(tr("Sum"), this);
    connect(cell_sumAction, SIGNAL(triggered()), this, SLOT(actionSum()));

    cell_addAction = new QAction(tr("&Add"), this);
    cell_addAction->setShortcut(Qt::CTRL | Qt::Key_Plus);
    connect(cell_addAction, SIGNAL(triggered()), this, SLOT(actionAdd()));

    cell_subAction = new QAction(tr("&Subtract"), this);
    cell_subAction->setShortcut(Qt::CTRL | Qt::Key_Minus);
    connect(cell_subAction, SIGNAL(triggered()), this, SLOT(actionSubtract()));

    cell_mulAction = new QAction(tr("&Multiply"), this);
    cell_mulAction->setShortcut(Qt::CTRL | Qt::Key_multiply);
    connect(cell_mulAction, SIGNAL(triggered()), this, SLOT(actionMultiply()));

    cell_divAction = new QAction(tr("&Divide"), this);
    cell_divAction->setShortcut(Qt::CTRL | Qt::Key_division);
    connect(cell_divAction, SIGNAL(triggered()), this, SLOT(actionDivide()));

    fontAction = new QAction(tr("Font..."), this);
    fontAction->setShortcut(Qt::CTRL|Qt::Key_F);
    connect(fontAction, SIGNAL(triggered()), this, SLOT(selectFont()));

    colorAction = new QAction(QPixmap(16, 16), tr("Background &Color..."), this);
    connect(colorAction, SIGNAL(triggered()), this, SLOT(selectColor()));

    clearAction = new QAction(tr("Clear"), this);
    clearAction->setShortcut(Qt::Key_Delete);
    connect(clearAction, SIGNAL(triggered()), this, SLOT(clear()));

    aboutSpreadSheet = new QAction(tr("About Spreadsheet"), this);
    connect(aboutSpreadSheet, SIGNAL(triggered()), this, SLOT(showAbout()));

    exitAction = new QAction(tr("E&xit"), this);
    connect(exitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

    firstSeparator = new QAction(this);
    firstSeparator->setSeparator(true);

    secondSeparator = new QAction(this);
    secondSeparator->setSeparator(true);


}

void SpreadSheet::setupMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(exitAction);

    QMenu *cellMenu = menuBar()->addMenu(tr("&Cell"));
    cellMenu->addAction(cell_addAction);
    cellMenu->addAction(cell_subAction);
    cellMenu->addAction(cell_mulAction);
    cellMenu->addAction(cell_divAction);
    cellMenu->addAction(cell_sumAction);
    cellMenu->addSeparator();
    cellMenu->addAction(colorAction);
    cellMenu->addAction(fontAction);

    menuBar()->addSeparator();

    QMenu *aboutMenu = menuBar()->addMenu(tr("&Help"));
    aboutMenu->addAction(aboutSpreadSheet);
}

void SpreadSheet::updateStatus(QTableWidgetItem *item)
{
    if (item && item == table->currentItem()) {
        statusBar()->showMessage(item->data(Qt::StatusTipRole).toString(), 1000);
        cellLabel->setText("Cell: (" + encode_pos(table->row(item), table->column(item)) + ")");
    }
}

void SpreadSheet::updateColor(QTableWidgetItem *item)
{
    QPixmap pix(16, 16);
    QColor col;
    if (item)
        col = item->backgroundColor();
    if (!col.isValid())
        col = palette().base().color();

    QPainter pt(&pix);
    pt.fillRect(0, 0, 16, 16, col);

    QColor lighter = col.light();
    pt.setPen(lighter);
    QPoint lightFrame[] = { QPoint(0, 15), QPoint(0, 0), QPoint(15, 0) };
    pt.drawPolyline(lightFrame, 3);

    pt.setPen(col.dark());
    QPoint darkFrame[] = { QPoint(1, 15), QPoint(15, 15), QPoint(15, 1) };
    pt.drawPolyline(darkFrame, 3);

    pt.end();

    colorAction->setIcon(pix);
}

void SpreadSheet::updateLineEdit(QTableWidgetItem *item)
{
    if (item != table->currentItem())
        return;
    if (item)
        formulaInput->setText(item->data(Qt::EditRole).toString());
    else
        formulaInput->clear();
}

void SpreadSheet::returnPressed()
{
    QString text = formulaInput->text();
    int row = table->currentRow();
    int col = table->currentColumn();
    QTableWidgetItem *item = table->item(row, col);
    if (!item)
        table->setItem(row, col, new SpreadSheetItem(text));
    else
        item->setData(Qt::EditRole, text);
    table->viewport()->update();
}

void SpreadSheet::selectColor()
{
    QTableWidgetItem *item = table->currentItem();
    QColor col = item ? item->backgroundColor() : table->palette().base().color();
    col = QColorDialog::getColor(col, this);
    if (!col.isValid())
        return;

    QList<QTableWidgetItem*> selected = table->selectedItems();
    if (selected.count() == 0)
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

bool SpreadSheet::runInputDialog(const QString &title,
                                 const QString &c1Text,
                                 const QString &c2Text,
                                 const QString &opText,
                                 const QString &outText,
                                 QString *cell1, QString *cell2, QString *outCell)
{

    QStringList rows, cols;
    for (int c = 0; c < table->columnCount(); ++c)
        cols << QChar('A' + c);
    for (int r = 0; r < table->rowCount(); ++r)
        rows << QString::number(1 + r);

    QDialog addDialog(this);
    addDialog.setWindowTitle(title);

    QGroupBox group(title, &addDialog);
    group.setMinimumSize(250, 100);

    QLabel cell1Label(c1Text, &group);
    QComboBox cell1RowInput(&group);
    int c1Row, c1Col;
    decode_pos(*cell1, &c1Row, &c1Col);
    cell1RowInput.addItems(rows);
    cell1RowInput.setCurrentIndex(c1Row);
    QComboBox cell1ColInput(&group);
    cell1ColInput.addItems(cols);
    cell1ColInput.setCurrentIndex(c1Col);

    QLabel operatorLabel(opText, &group);
    operatorLabel.setAlignment(Qt::AlignHCenter);

    QLabel cell2Label(c2Text, &group);
    QComboBox cell2RowInput(&group);
    int c2Row, c2Col;
    decode_pos(*cell2, &c2Row, &c2Col);
    cell2RowInput.addItems(rows);
    cell2RowInput.setCurrentIndex(c2Row);
    QComboBox cell2ColInput(&group);
    cell2ColInput.addItems(cols);
    cell2ColInput.setCurrentIndex(c2Col);

    QLabel equalsLabel("=", &group);
    equalsLabel.setAlignment(Qt::AlignHCenter);

    QLabel outLabel(outText, &group);
    QComboBox outRowInput(&group);
    int outRow, outCol;
    decode_pos(*outCell, &outRow, &outCol);
    outRowInput.addItems(rows);
    outRowInput.setCurrentIndex(outRow);
    QComboBox outColInput(&group);
    outColInput.addItems(cols);
    outColInput.setCurrentIndex(outCol);

    QPushButton cancelButton(tr("Cancel"), &addDialog);
    connect(&cancelButton, SIGNAL(clicked()), &addDialog, SLOT(reject()));

    QPushButton okButton(tr("OK"), &addDialog);
    okButton.setDefault(true);
    connect(&okButton, SIGNAL(clicked()), &addDialog, SLOT(accept()));

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(&okButton);
    buttonsLayout->addSpacing(10);
    buttonsLayout->addWidget(&cancelButton);

    QVBoxLayout *dialogLayout = new QVBoxLayout(&addDialog);
    dialogLayout->addWidget(&group);
    dialogLayout->addStretch(1);
    dialogLayout->addItem(buttonsLayout);

    QHBoxLayout *cell1Layout = new QHBoxLayout;
    cell1Layout->addWidget(&cell1Label);
    cell1Layout->addSpacing(10);
    cell1Layout->addWidget(&cell1ColInput);
    cell1Layout->addSpacing(10);
    cell1Layout->addWidget(&cell1RowInput);

    QHBoxLayout *cell2Layout = new QHBoxLayout;
    cell2Layout->addWidget(&cell2Label);
    cell2Layout->addSpacing(10);
    cell2Layout->addWidget(&cell2ColInput);
    cell2Layout->addSpacing(10);
    cell2Layout->addWidget(&cell2RowInput);

    QHBoxLayout *outLayout = new QHBoxLayout;
    outLayout->addWidget(&outLabel);
    outLayout->addSpacing(10);
    outLayout->addWidget(&outColInput);
    outLayout->addSpacing(10);
    outLayout->addWidget(&outRowInput);

    QVBoxLayout *vLayout = new QVBoxLayout(&group);
    vLayout->addItem(cell1Layout);
    vLayout->addWidget(&operatorLabel);
    vLayout->addItem(cell2Layout);
    vLayout->addWidget(&equalsLabel);
    vLayout->addStretch(1);
    vLayout->addItem(outLayout);

    if (addDialog.exec()) {
        *cell1 = cell1ColInput.currentText() + cell1RowInput.currentText();
        *cell2 = cell2ColInput.currentText() + cell2RowInput.currentText();
        *outCell = outColInput.currentText() + outRowInput.currentText();
        return true;
    }

    return false;
}


void SpreadSheet::actionSum()
{

    int row_first = 0;
    int row_last = 0;
    int row_cur = 0;

    int col_first = 0;
    int col_last = 0;
    int col_cur = 0;

    QList<QTableWidgetItem*> selected = table->selectedItems();
    if (!selected.isEmpty()) {
        QTableWidgetItem *first = selected.first();
        QTableWidgetItem *last = selected.last();
        row_first = table->row(first);
        row_last = table->row(last);
        col_first = table->column(first);
        col_last = table->column(last);
    }

    QTableWidgetItem *current = table->currentItem();
    if (current) {
        row_cur = table->row(current);
        col_cur = table->column(current);
    }

    QString cell1 = encode_pos(row_first, col_first);
    QString cell2 = encode_pos(row_last, col_last);
    QString out = encode_pos(row_cur, col_cur);


    if (runInputDialog(tr("Sum cells"), tr("First cell:"), tr("Last cell:"),
                       QString("%1").arg(QChar(0x03a3)), tr("Output to:"),
                       &cell1, &cell2, &out)) {
        int row, col;
        decode_pos(out, &row, &col);
        table->item(row, col)->setText("sum " + cell1 + " " + cell2);
    }
}


void SpreadSheet::actionMath_helper(const QString &title, const QString &op)
{
    QString cell1 = "C1";
    QString cell2 = "C2";
    QString out = "C3";

    QTableWidgetItem *current = table->currentItem();
    if (current)
        out = encode_pos(table->currentRow(), table->currentColumn());

    if (runInputDialog(title, tr("Cell 1"), tr("Cell 2"), op, tr("Output to:"),
                       &cell1, &cell2, &out)) {
        int row, col;
        decode_pos(out, &row, &col);
        table->item(row, col)->setText(op + " " + cell1 + " " + cell2);
    }
}


void SpreadSheet::actionAdd()
{
    actionMath_helper(tr("Addition"), "+");
}

void SpreadSheet::actionSubtract()
{
    actionMath_helper(tr("Subtraction"), "-");
}

void SpreadSheet::actionMultiply()
{
    actionMath_helper(tr("Multiplication"), "*");
}

void SpreadSheet::actionDivide()
{
    actionMath_helper(tr("Division"), "/");
}

void SpreadSheet::clear()
{
    foreach (QTableWidgetItem *i, table->selectedItems())
        i->setText("");
}

void SpreadSheet::setupContextMenu()
{
    addAction(cell_addAction);
    addAction(cell_subAction);
    addAction(cell_mulAction);
    addAction(cell_divAction);
    addAction(cell_sumAction);
    addAction(firstSeparator);
    addAction(colorAction);
    addAction(fontAction);
    addAction(secondSeparator);
    addAction(clearAction);
    setContextMenuPolicy(Qt::ActionsContextMenu);
}

void SpreadSheet::setupContents()
{
    QColor titleBackground(Qt::lightGray);
    QFont titleFont = table->font();
    titleFont.setBold(true);

    // column 0
    table->setItem(0, 0, new SpreadSheetItem("Item"));
    table->item(0, 0)->setBackgroundColor(titleBackground);
    table->item(0, 0)->setToolTip("This column shows the purchased item/service");
    table->item(0, 0)->setFont(titleFont);
    table->setItem(1, 0, new SpreadSheetItem("AirportBus"));
    table->setItem(2, 0, new SpreadSheetItem("Flight (Munich)"));
    table->setItem(3, 0, new SpreadSheetItem("Lunch"));
    table->setItem(4, 0, new SpreadSheetItem("Flight (LA)"));
    table->setItem(5, 0, new SpreadSheetItem("Taxi"));
    table->setItem(6, 0, new SpreadSheetItem("Dinner"));
    table->setItem(7, 0, new SpreadSheetItem("Hotel"));
    table->setItem(8, 0, new SpreadSheetItem("Flight (Oslo)"));
    table->setItem(9, 0, new SpreadSheetItem("Total:"));
    table->item(9, 0)->setFont(titleFont);
    table->item(9,0)->setBackgroundColor(Qt::lightGray);
    // column 1
    table->setItem(0, 1, new SpreadSheetItem("Date"));
    table->item(0, 1)->setBackgroundColor(titleBackground);
    table->item(0, 1)->setToolTip("This column shows the purchase date, double click to change");
    table->item(0, 1)->setFont(titleFont);
    table->setItem(1, 1, new SpreadSheetItem("15/6/2006"));
    table->setItem(2, 1, new SpreadSheetItem("15/6/2006"));
    table->setItem(3, 1, new SpreadSheetItem("15/6/2006"));
    table->setItem(4, 1, new SpreadSheetItem("21/5/2006"));
    table->setItem(5, 1, new SpreadSheetItem("16/6/2006"));
    table->setItem(6, 1, new SpreadSheetItem("16/6/2006"));
    table->setItem(7, 1, new SpreadSheetItem("16/6/2006"));
    table->setItem(8, 1, new SpreadSheetItem("18/6/2006"));
    table->setItem(9, 1, new SpreadSheetItem());
    table->item(9,1)->setBackgroundColor(Qt::lightGray);
    // column 2
    table->setItem(0, 2, new SpreadSheetItem("Price"));
    table->item(0, 2)->setBackgroundColor(titleBackground);
    table->item(0, 2)->setToolTip("This column shows the price of the purchase");
    table->item(0, 2)->setFont(titleFont);
    table->setItem(1, 2, new SpreadSheetItem("150"));
    table->setItem(2, 2, new SpreadSheetItem("2350"));
    table->setItem(3, 2, new SpreadSheetItem("-14"));
    table->setItem(4, 2, new SpreadSheetItem("980"));
    table->setItem(5, 2, new SpreadSheetItem("5"));
    table->setItem(6, 2, new SpreadSheetItem("120"));
    table->setItem(7, 2, new SpreadSheetItem("300"));
    table->setItem(8, 2, new SpreadSheetItem("1240"));
    table->setItem(9, 2, new SpreadSheetItem());
    table->item(9,2)->setBackgroundColor(Qt::lightGray);
    // column 3
    table->setItem(0, 3, new SpreadSheetItem("Currency"));
    table->item(0, 3)->setBackgroundColor(titleBackground);
    table->item(0, 3)->setToolTip("This column shows the currency");
    table->item(0, 3)->setFont(titleFont);
    table->setItem(1, 3, new SpreadSheetItem("NOK"));
    table->setItem(2, 3, new SpreadSheetItem("NOK"));
    table->setItem(3, 3, new SpreadSheetItem("EUR"));
    table->setItem(4, 3, new SpreadSheetItem("EUR"));
    table->setItem(5, 3, new SpreadSheetItem("USD"));
    table->setItem(6, 3, new SpreadSheetItem("USD"));
    table->setItem(7, 3, new SpreadSheetItem("USD"));
    table->setItem(8, 3, new SpreadSheetItem("USD"));
    table->setItem(9, 3, new SpreadSheetItem());
    table->item(9,3)->setBackgroundColor(Qt::lightGray);
    // column 4
    table->setItem(0, 4, new SpreadSheetItem("Ex.Rate"));
    table->item(0, 4)->setBackgroundColor(titleBackground);
    table->item(0, 4)->setToolTip("This column shows the exchange rate to NOK");
    table->item(0, 4)->setFont(titleFont);
    table->setItem(1, 4, new SpreadSheetItem("1"));
    table->setItem(2, 4, new SpreadSheetItem("1"));
    table->setItem(3, 4, new SpreadSheetItem("8"));
    table->setItem(4, 4, new SpreadSheetItem("8"));
    table->setItem(5, 4, new SpreadSheetItem("7"));
    table->setItem(6, 4, new SpreadSheetItem("7"));
    table->setItem(7, 4, new SpreadSheetItem("7"));
    table->setItem(8, 4, new SpreadSheetItem("7"));
    table->setItem(9, 4, new SpreadSheetItem());
    table->item(9,4)->setBackgroundColor(Qt::lightGray);
    // column 5
    table->setItem(0, 5, new SpreadSheetItem("NOK"));
    table->item(0, 5)->setBackgroundColor(titleBackground);
    table->item(0, 5)->setToolTip("This column shows the expenses in NOK");
    table->item(0, 5)->setFont(titleFont);
    table->setItem(1, 5, new SpreadSheetItem("* C2 E2"));
    table->setItem(2, 5, new SpreadSheetItem("* C3 E3"));
    table->setItem(3, 5, new SpreadSheetItem("* C4 E4"));
    table->setItem(4, 5, new SpreadSheetItem("* C5 E5"));
    table->setItem(5, 5, new SpreadSheetItem("* C6 E6"));
    table->setItem(6, 5, new SpreadSheetItem("* C7 E7"));
    table->setItem(7, 5, new SpreadSheetItem("* C8 E8"));
    table->setItem(8, 5, new SpreadSheetItem("* C9 E9"));
    table->setItem(9, 5, new SpreadSheetItem("sum F2 F9"));
    table->item(9,5)->setBackgroundColor(Qt::lightGray);

}

const char *htmlText =
"<HTML>"
"<p><b>This demo shows use of <c>QTableWidget</c> with custom handling for"
" individual cells.</b></p>"
"<p>Using a customized table item we make it possible to have dynamic"
" output in different cells. The content that is implemented for this"
" particular demo is:"
"<ul>"
"<li>Adding two cells.</li>"
"<li>Subtracting one cell from another.</li>"
"<li>Multiplying two cells.</li>"
"<li>Dividing one cell with another.</li>"
"<li>Summing the contents of an arbitrary number of cells.</li>"
"</HTML>";


void SpreadSheet::showAbout()
{
    QMessageBox::about(this, "About Spreadsheet", htmlText);
}

int main(int argc, char** argv) {
    Q_INIT_RESOURCE(spreadsheet);
    QApplication app(argc, argv);
    SpreadSheet sheet(10, 6);
    sheet.setWindowIcon(QPixmap(":/images/interview.png"));
    sheet.resize(640, 420);
    sheet.show();
    return app.exec();
}

#include "main.moc"

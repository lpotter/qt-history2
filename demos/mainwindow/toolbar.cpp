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

#include "toolbar.h"

#include <qmainwindow.h>
#include <qmenu.h>
#include <qpainter.h>
#include <qpainterpath.h>
#include <qspinbox.h>

#include <stdlib.h>

static QPixmap genIcon(const QSize &iconSize, const QString &, const QColor &color, const QColor &bg)
{
    int w = iconSize.width();
    int h = iconSize.height();

    QImage image(w, h, QImage::Format_RGB32);
    image.fill(bg.rgb());

    QPainter p(&image);

    extern void render_qt_text(QPainter *, int, int, const QColor &);
    render_qt_text(&p, w, h, color);

    return QPixmap::fromImage(image, Qt::DiffuseDither | Qt::DiffuseAlphaDither);
}

static QPixmap genIcon(const QSize &iconSize, int number, const QColor &color, const QColor &bg)
{ return genIcon(iconSize, QString::number(number), color, bg); }

ToolBar::ToolBar(QWidget *parent)
    : QToolBar(parent), spinbox(0), spinboxAction(0)
{
    setWindowTitle(tr("Main Tool Bar"));

    setIconSize(QSize(32, 32));

    QColor bg(palette().background().color());
    menu = new QMenu("One", this);
    menu->setIcon(genIcon(iconSize(), 1, Qt::black, bg));
    menu->addAction(genIcon(iconSize(), "A", Qt::blue, bg), "A");
    menu->addAction(genIcon(iconSize(), "B", Qt::blue, bg), "B");
    menu->addAction(genIcon(iconSize(), "C", Qt::blue, bg), "C");
    addAction(menu->menuAction());

    QAction *two = addAction(genIcon(iconSize(), 2, Qt::white, bg), "Two");
    QFont boldFont;
    boldFont.setBold(true);
    two->setFont(boldFont);

    addAction(genIcon(iconSize(), 3, Qt::red, bg), "Three");
    addAction(genIcon(iconSize(), 4, Qt::green, bg), "Four");
    addAction(genIcon(iconSize(), 5, Qt::blue, bg), "Five");
    QAction *six = addAction(genIcon(iconSize(), 6, Qt::yellow, bg), "Six");
    orderAction = new QAction(this);
    orderAction->setText(tr("Order Items in Tool Bar"));
    connect(orderAction, SIGNAL(triggered()), SLOT(order()));

    randomizeAction = new QAction(this);
    randomizeAction->setText(tr("Randomize Items in Tool Bar"));
    connect(randomizeAction, SIGNAL(triggered()), SLOT(randomize()));

    addSpinBoxAction = new QAction(this);
    addSpinBoxAction->setText(tr("Add Spin Box"));
    connect(addSpinBoxAction, SIGNAL(triggered()), SLOT(addSpinBox()));

    removeSpinBoxAction = new QAction(this);
    removeSpinBoxAction->setText(tr("Remove Spin Box"));
    removeSpinBoxAction->setEnabled(false);
    connect(removeSpinBoxAction, SIGNAL(triggered()), SLOT(removeSpinBox()));

    movableAction = new QAction(tr("Movable"), this);
    movableAction->setCheckable(true);
    connect(movableAction, SIGNAL(triggered(bool)), SLOT(changeMovable(bool)));

    allowedAreasActions = new QActionGroup(this);
    allowedAreasActions->setExclusive(false);

    allowLeftAction = new QAction(tr("Allow on Left"), this);
    allowLeftAction->setCheckable(true);
    connect(allowLeftAction, SIGNAL(triggered(bool)), SLOT(allowLeft(bool)));

    allowRightAction = new QAction(tr("Allow on Right"), this);
    allowRightAction->setCheckable(true);
    connect(allowRightAction, SIGNAL(triggered(bool)), SLOT(allowRight(bool)));

    allowTopAction = new QAction(tr("Allow on Top"), this);
    allowTopAction->setCheckable(true);
    connect(allowTopAction, SIGNAL(triggered(bool)), SLOT(allowTop(bool)));

    allowBottomAction = new QAction(tr("Allow on Bottom"), this);
    allowBottomAction->setCheckable(true);
    connect(allowBottomAction, SIGNAL(triggered(bool)), SLOT(allowBottom(bool)));

    allowedAreasActions->addAction(allowLeftAction);
    allowedAreasActions->addAction(allowRightAction);
    allowedAreasActions->addAction(allowTopAction);
    allowedAreasActions->addAction(allowBottomAction);

    areaActions = new QActionGroup(this);
    areaActions->setExclusive(true);

    leftAction = new QAction(tr("Place on Left") , this);
    leftAction->setCheckable(true);
    connect(leftAction, SIGNAL(triggered(bool)), SLOT(placeLeft(bool)));

    rightAction = new QAction(tr("Place on Right") , this);
    rightAction->setCheckable(true);
    connect(rightAction, SIGNAL(triggered(bool)), SLOT(placeRight(bool)));

    topAction = new QAction(tr("Place on Top") , this);
    topAction->setCheckable(true);
    connect(topAction, SIGNAL(triggered(bool)), SLOT(placeTop(bool)));

    bottomAction = new QAction(tr("Place on Bottom") , this);
    bottomAction->setCheckable(true);
    connect(bottomAction, SIGNAL(triggered(bool)), SLOT(placeBottom(bool)));

    areaActions->addAction(leftAction);
    areaActions->addAction(rightAction);
    areaActions->addAction(topAction);
    areaActions->addAction(bottomAction);

    connect(movableAction, SIGNAL(triggered(bool)), areaActions, SLOT(setEnabled(bool)));

    connect(movableAction, SIGNAL(triggered(bool)), allowedAreasActions, SLOT(setEnabled(bool)));

    menu = new QMenu(tr("&Toolbar"), this);
    menu->addAction(toggleViewAction());
    menu->addSeparator();
    menu->addAction(orderAction);
    menu->addAction(randomizeAction);
    menu->addSeparator();
    menu->addAction(addSpinBoxAction);
    menu->addAction(removeSpinBoxAction);
    menu->addSeparator();
    menu->addAction(movableAction);
    menu->addSeparator();
    menu->addActions(allowedAreasActions->actions());
    menu->addSeparator();
    menu->addActions(areaActions->actions());

    randomize();
}


void ToolBar::polishEvent(QEvent *)
{
    QMainWindow *mainWindow = qobject_cast<QMainWindow *>(parentWidget());
    Q_ASSERT(mainWindow != 0);

    const Qt::ToolBarArea area = mainWindow->toolBarArea(this);
    const Qt::ToolBarAreas areas = allowedAreas();

    movableAction->setChecked(isMovable());

    allowLeftAction->setChecked(isAreaAllowed(Qt::LeftToolBarArea));
    allowRightAction->setChecked(isAreaAllowed(Qt::RightToolBarArea));
    allowTopAction->setChecked(isAreaAllowed(Qt::TopToolBarArea));
    allowBottomAction->setChecked(isAreaAllowed(Qt::BottomToolBarArea));

    if (allowedAreasActions->isEnabled()) {
        allowLeftAction->setEnabled(area != Qt::LeftToolBarArea);
        allowRightAction->setEnabled(area != Qt::RightToolBarArea);
        allowTopAction->setEnabled(area != Qt::TopToolBarArea);
        allowBottomAction->setEnabled(area != Qt::BottomToolBarArea);
    }

    leftAction->setChecked(area == Qt::LeftToolBarArea);
    rightAction->setChecked(area == Qt::RightToolBarArea);
    topAction->setChecked(area == Qt::TopToolBarArea);
    bottomAction->setChecked(area == Qt::BottomToolBarArea);

    if (areaActions->isEnabled()) {
        leftAction->setEnabled(areas & Qt::LeftToolBarArea);
        rightAction->setEnabled(areas & Qt::RightToolBarArea);
        topAction->setEnabled(areas & Qt::TopToolBarArea);
        bottomAction->setEnabled(areas & Qt::BottomToolBarArea);
    }
}

void ToolBar::order()
{
    QList<QAction *> ordered, actions1 = actions(),
                              actions2 = qFindChildren<QAction *>(this);
    while (!actions2.isEmpty()) {
        QAction *action = actions2.takeFirst();
        if (!actions1.contains(action))
            continue;
        actions1.removeAll(action);
        ordered.append(action);
    }

    clear();
    addActions(ordered);

    orderAction->setEnabled(false);
}

void ToolBar::randomize()
{
    QList<QAction *> randomized, actions = this->actions();
    while (!actions.isEmpty()) {
        QAction *action = actions.takeAt(rand() % actions.size());
        randomized.append(action);
    }
    clear();
    addActions(randomized);

    orderAction->setEnabled(true);
}

void ToolBar::addSpinBox()
{
    if (!spinbox) {
        spinbox = new QSpinBox(this);
    }
    if (!spinboxAction)
        spinboxAction = addWidget(spinbox);
    else
        addAction(spinboxAction);

    addSpinBoxAction->setEnabled(false);
    removeSpinBoxAction->setEnabled(true);
}

void ToolBar::removeSpinBox()
{
    if (spinboxAction)
        removeAction(spinboxAction);

    addSpinBoxAction->setEnabled(true);
    removeSpinBoxAction->setEnabled(false);
}

void ToolBar::allow(Qt::ToolBarArea area, bool a)
{
    Qt::ToolBarAreas areas = allowedAreas();
    areas = a ? areas | area : areas & ~area;
    setAllowedAreas(areas);

    if (areaActions->isEnabled()) {
        leftAction->setEnabled(areas & Qt::LeftToolBarArea);
        rightAction->setEnabled(areas & Qt::RightToolBarArea);
        topAction->setEnabled(areas & Qt::TopToolBarArea);
        bottomAction->setEnabled(areas & Qt::BottomToolBarArea);
    }
}

void ToolBar::place(Qt::ToolBarArea area, bool p)
{
    if (!p)
        return;

    QMainWindow *mainWindow = qobject_cast<QMainWindow *>(parentWidget());
    Q_ASSERT(mainWindow != 0);

    mainWindow->addToolBar(area, this);

    if (allowedAreasActions->isEnabled()) {
        allowLeftAction->setEnabled(area != Qt::LeftToolBarArea);
        allowRightAction->setEnabled(area != Qt::RightToolBarArea);
        allowTopAction->setEnabled(area != Qt::TopToolBarArea);
        allowBottomAction->setEnabled(area != Qt::BottomToolBarArea);
    }
}

void ToolBar::changeMovable(bool movable)
{ setMovable(movable); }

void ToolBar::allowLeft(bool a)
{ allow(Qt::LeftToolBarArea, a); }

void ToolBar::allowRight(bool a)
{ allow(Qt::RightToolBarArea, a); }

void ToolBar::allowTop(bool a)
{ allow(Qt::TopToolBarArea, a); }

void ToolBar::allowBottom(bool a)
{ allow(Qt::BottomToolBarArea, a); }

void ToolBar::placeLeft(bool p)
{ place(Qt::LeftToolBarArea, p); }

void ToolBar::placeRight(bool p)
{ place(Qt::RightToolBarArea, p); }

void ToolBar::placeTop(bool p)
{ place(Qt::TopToolBarArea, p); }

void ToolBar::placeBottom(bool p)
{ place(Qt::BottomToolBarArea, p); }

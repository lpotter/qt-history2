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

#include "qdesigner_menu_p.h"
#include "qdesigner_menubar_p.h"
#include "qdesigner_toolbar_p.h"
#include "qdesigner_command_p.h"
#include "actionrepository_p.h"
#include "actionprovider_p.h"

#include <QtDesigner/QtDesigner>

#include <QtCore/QTimer>
#include <QtCore/qdebug.h>

#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QLineEdit>
#include <QtGui/QPainter>
#include <QtGui/QRubberBand>
#include <QtGui/qevent.h>

Q_DECLARE_METATYPE(QAction*)
Q_DECLARE_METATYPE(QListWidgetItem*)

using namespace qdesigner_internal;

QDesignerMenu::QDesignerMenu(QWidget *parent)
    : QMenu(parent)
{
    m_interactive = true;
    m_dragging = false;
    m_currentIndex = 0;

    setContextMenuPolicy(Qt::DefaultContextMenu);
    setAcceptDrops(true); // ### fake

    m_addItem = new SpecialMenuAction(this);
    m_addItem->setText(tr("new action"));
    addAction(m_addItem);

    m_addSeparator = new SpecialMenuAction(this);
    m_addSeparator->setText(tr("new separator"));
    addAction(m_addSeparator);

    m_showSubMenuTimer = new QTimer(this);
    connect(m_showSubMenuTimer, SIGNAL(timeout()), this, SLOT(slotShowSubMenuNow()));

    m_deactivateWindowTimer = new QTimer(this);
    connect(m_deactivateWindowTimer, SIGNAL(timeout()), this, SLOT(slotDeactivateNow()));

    m_editor = new QLineEdit(this);
    m_editor->setObjectName("__qt__passive_editor");
    m_editor->hide();
    qApp->installEventFilter(this);

}

QDesignerMenu::~QDesignerMenu()
{
}

bool QDesignerMenu::handleEvent(QWidget *widget, QEvent *event)
{
    if (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut) {
        update();
    }

    switch (event->type()) {
        default: break;

        case QEvent::MouseButtonPress:
            return handleMousePressEvent(widget, static_cast<QMouseEvent*>(event));
        case QEvent::MouseButtonRelease:
            return handleMouseReleaseEvent(widget, static_cast<QMouseEvent*>(event));
        case QEvent::MouseMove:
            return handleMouseMoveEvent(widget, static_cast<QMouseEvent*>(event));
        case QEvent::ContextMenu:
            return handleContextMenuEvent(widget, static_cast<QContextMenuEvent*>(event));
        case QEvent::KeyPress:
            return handleKeyPressEvent(widget, static_cast<QKeyEvent*>(event));
    }

    return true;
}

void QDesignerMenu::startDrag(const QPoint &pos)
{
    int index = findAction(pos);
    if (index >= realActionCount())
        return;

    QAction *action = actions().at(index);
    removeAction(action);
    adjustSize();

    QDrag *drag = new QDrag(this);
    drag->setPixmap(action->icon().pixmap(QSize(22, 22)));

    ActionRepositoryMimeData *data = new ActionRepositoryMimeData();
    data->items.append(action);
    drag->setMimeData(data);

    if (drag->start() == Qt::IgnoreAction) {
        QAction *previous = actions().at(index);
        insertAction(previous, action);
        adjustSize();
    }
}

bool QDesignerMenu::handleKeyPressEvent(QWidget *widget, QKeyEvent *e)
{
    m_showSubMenuTimer->stop();

    if (m_editor->isHidden()) { // In navigation mode
        switch (e->key()) {

        case Qt::Key_Delete:
            if (m_currentIndex == -1 || m_currentIndex >= realActionCount())
                break;
            hideSubMenu();
            deleteAction();
            break;

        case Qt::Key_Left:
            e->accept();
            moveLeft();
            return true;

        case Qt::Key_Right:
            e->accept();
            moveRight();
            return true; // no update

        case Qt::Key_Up:
            e->accept();
            moveUp(e->modifiers() & Qt::ControlModifier);
            return true;

        case Qt::Key_Down:
            e->accept();
            moveDown(e->modifiers() & Qt::ControlModifier);
            return true;

        case Qt::Key_PageUp:
            m_currentIndex = 0;
            break;

        case Qt::Key_PageDown:
            m_currentIndex = actions().count() - 1;
            break;

        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_F2:
            e->accept();
            enterEditMode();
            return true; // no update

        case Qt::Key_Escape:
            e->ignore();
            setFocus();
            hide();
            closeMenuChain();
            return true;

        case Qt::Key_Alt:
        case Qt::Key_Shift:
        case Qt::Key_Control:
            e->ignore();
            setFocus(); // FIXME: this is because some other widget get the focus when CTRL is pressed
            return true; // no update

        default:
            if (!e->text().isEmpty() && e->text().at(0).toLatin1() >= 32) {
                showLineEdit();
                QApplication::sendEvent(m_editor, e);
                e->accept();
            } else {
                e->ignore();
            }
            return true;
        }
    } else { // In edit mode
        switch (e->key()) {
        default:
            e->ignore();
            return false;

        case Qt::Key_Enter:
        case Qt::Key_Return:
            leaveEditMode();
            m_editor->hide();
            setFocus();
            break;

        case Qt::Key_Escape:
            m_editor->hide();
            setFocus();
            break;
        }
    }

    e->accept();
    update();

    return true;
}

bool QDesignerMenu::handleMousePressEvent(QWidget *, QMouseEvent *event)
{
    m_showSubMenuTimer->stop();
    m_startPosition = QPoint();
    event->accept();

    if (event->button() != Qt::LeftButton)
        return true;

    m_startPosition = mapFromGlobal(event->globalPos());

    int index = findAction(m_startPosition);
    if (index >= actions().count() - 1)
        return true;

    hideSubMenu();
    m_currentIndex = index;
    updateCurrentAction();

    return true;
}

bool QDesignerMenu::handleMouseReleaseEvent(QWidget *, QMouseEvent *event)
{
    event->accept();

    m_startPosition = QPoint();

    return true;
}

bool QDesignerMenu::handleMouseMoveEvent(QWidget *, QMouseEvent *event)
{
    event->accept();

    if (m_startPosition.isNull())
        return true;

    QPoint pos = mapFromGlobal(event->globalPos());

    if ((pos - m_startPosition).manhattanLength() < qApp->startDragDistance())
        return true;

    startDrag(pos);
    m_startPosition = QPoint();

    return true;
}

bool QDesignerMenu::handleContextMenuEvent(QWidget *, QContextMenuEvent *event)
{
    event->accept();

    int index = findAction(mapFromGlobal(event->globalPos()));
    QAction *action = actions().at(index);
    if (action == actions().last())
        return true;

    QMenu menu(0);
    QAction *a = menu.addAction(tr("Remove action '%1'").arg(action->objectName()));
    QVariant itemData;
    qVariantSetValue(itemData, action);
    a->setData(itemData);

    connect(&menu, SIGNAL(triggered(QAction*)), this, SLOT(slotRemoveSelectedAction(QAction*)));
    menu.exec(event->globalPos());

    return true;
}

void QDesignerMenu::slotRemoveSelectedAction(QAction *action)
{
    QAction *a = qvariant_cast<QAction*>(action->data());
    Q_ASSERT(a != 0);
    removeAction(a);
    adjustSize();
}

void QDesignerMenu::paintEvent(QPaintEvent *event)
{
    QMenu::paintEvent(event);

    if (!hasFocus() || m_dragging)
        return;

    if (QAction *a = currentAction()) {
        QPainter p(this);
        QRect g = actionGeometry(a);
        drawSelection(&p, g.adjusted(1, 1, -3, -3));
    }
}

QDesignerMenu *QDesignerMenu::findRootMenu() const
{
    if (parentMenu())
        return parentMenu()->findRootMenu();

    return const_cast<QDesignerMenu*>(this);
}

QDesignerMenu *QDesignerMenu::findActivatedMenu() const
{
    QList<QDesignerMenu*> candidates;
    candidates.append(const_cast<QDesignerMenu*>(this));
    candidates += qFindChildren<QDesignerMenu*>(this);

    foreach (QDesignerMenu *m, candidates) {
        if (m->isActiveWindow())
            return m;
    }

    return 0;
}

bool QDesignerMenu::eventFilter(QObject *object, QEvent *event)
{
    if (object != this && object != m_editor)
        return false;

    if (object == m_editor && event->type() == QEvent::FocusOut) {
        leaveEditMode();
        m_editor->hide();
        update();
        return false;
    }

    switch (event->type()) {
        default: break;

        case QEvent::WindowDeactivate:
            deactivateMenu();
            break;

        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        case QEvent::ContextMenu:
        case QEvent::MouseMove:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::Enter:
        case QEvent::Leave:
        case QEvent::FocusIn:
        case QEvent::FocusOut:
        {
            QWidget *widget = qobject_cast<QWidget*>(object);

            if (widget && (widget == this || isAncestorOf(widget)))
                return handleEvent(widget, event);
        } break;
    }

    return false;
};

int QDesignerMenu::findAction(const QPoint &pos) const
{
    for (int i = 0; i<actions().size() - 1; ++i) {
        QRect g = actionGeometry(actions().at(i));
        g.setTopLeft(QPoint(0, 0));

        if (g.contains(pos)) {
            if (pos.x() > g.right() - 10) // ### 10px
                return i + 1;

            return i;
        }
    }

    return actions().size() - 2; // the fake actions
}

void QDesignerMenu::adjustIndicator(const QPoint &pos)
{
    if (QDesignerActionProviderExtension *a = actionProvider()) {
        a->adjustIndicator(pos);
    }
}

void QDesignerMenu::dragEnterEvent(QDragEnterEvent *event)
{
    m_dragging = true;

    if (const ActionRepositoryMimeData *d = qobject_cast<const ActionRepositoryMimeData*>(event->mimeData())) {
        Q_ASSERT(!d->items.isEmpty());

        QAction *action = d->items.first();
        if (action && !actions().contains(action)) {
            event->acceptProposedAction();
            adjustIndicator(event->pos());
            update();
        }
    }
}

void QDesignerMenu::dragMoveEvent(QDragMoveEvent *event)
{
    if (const ActionRepositoryMimeData *d = qobject_cast<const ActionRepositoryMimeData*>(event->mimeData())) {
        Q_ASSERT(!d->items.isEmpty());

        QAction *action = d->items.first();
        if (action && !actions().contains(action)) {
            event->acceptProposedAction();
            adjustIndicator(event->pos());
        }
    }
}

void QDesignerMenu::dragLeaveEvent(QDragLeaveEvent *)
{
    m_dragging = false;
    adjustIndicator(QPoint(-1, -1));
}

void QDesignerMenu::dropEvent(QDropEvent *event)
{
    if (const ActionRepositoryMimeData *d = qobject_cast<const ActionRepositoryMimeData*>(event->mimeData())) {
        event->acceptProposedAction();

        QAction *action = d->items.first();
        if (action && !actions().contains(action)) {
            int index = findAction(event->pos());
            index = qMin(index, actions().count() - 1);
            insertAction(actions().at(index), action);
            adjustSize();
        }
    }

    adjustIndicator(QPoint(-1, -1));
}

void QDesignerMenu::actionEvent(QActionEvent *event)
{
    QMenu::actionEvent(event);
}

QDesignerFormWindowInterface *QDesignerMenu::formWindow() const
{
    if (parentMenu())
        return parentMenu()->formWindow();

    return QDesignerFormWindowInterface::findFormWindow(parentWidget());
}

QDesignerActionProviderExtension *QDesignerMenu::actionProvider()
{
    if (QDesignerFormWindowInterface *fw = formWindow()) {
        QDesignerFormEditorInterface *core = fw->core();
        return qt_extension<QDesignerActionProviderExtension*>(core->extensionManager(), this);
    }

    return 0;
}

void QDesignerMenu::closeMenuChain()
{
    m_showSubMenuTimer->stop();

    QWidget *w = this;
    while (w && qobject_cast<QMenu*>(w))
        w = w->parentWidget();

    if (w) {
        foreach (QMenu *subMenu, qFindChildren<QMenu*>(w)) {
            subMenu->hide();
        }
    }
}

void QDesignerMenu::moveLeft()
{
    if (parentMenu()) {
        hide();
    } else if (QDesignerMenuBar *mb = parentMenuBar()) {
        hide();
        mb->moveLeft();
    }
    updateCurrentAction();
}

void QDesignerMenu::moveRight()
{
    QAction *action = currentAction();

    if (qobject_cast<SpecialMenuAction*>(action) || action->isSeparator()) {
        closeMenuChain();
        if (QDesignerMenuBar *mb = parentMenuBar()) {
            mb->moveRight();
        }
    } else {
        slotShowSubMenuNow();
    }
}

void QDesignerMenu::moveUp(bool ctrl)
{
    if (m_currentIndex == 0) {
        hide();
        return;
    }

    if (ctrl)
        (void) swap(m_currentIndex, m_currentIndex - 1);

    m_currentIndex = qMax(0, --m_currentIndex);
    updateCurrentAction();
}

void QDesignerMenu::moveDown(bool ctrl)
{
    if (m_currentIndex == actions().count() - 1) {
        hide();
        return;
    }

    if (ctrl)
        (void) swap(m_currentIndex + 1, m_currentIndex);

    m_currentIndex = qMin(actions().count() - 1, ++m_currentIndex);
    updateCurrentAction();
}

QAction *QDesignerMenu::currentAction() const
{
    if (m_currentIndex < 0 || m_currentIndex >= actions().count())
        return 0;

    return actions().at(m_currentIndex);
}

int QDesignerMenu::realActionCount()
{
    return actions().count() - 2; // 2 fake actions
}

void QDesignerMenu::updateCurrentAction()
{
    update();
}

void QDesignerMenu::createRealMenuAction(QAction *action) // ### undo/redo
{
    if (action->menu())
        return; // nothing to do

    QDesignerMenu *tempMenu = findOrCreateSubMenu(action);
    m_subMenus.remove(action);

    action->setMenu(tempMenu);

    Q_ASSERT(formWindow() != 0);
    QDesignerFormWindowInterface *fw = formWindow();
    fw->core()->metaDataBase()->add(action->menu());
}

QDesignerMenu *QDesignerMenu::findOrCreateSubMenu(QAction *action)
{
    if (action->menu())
        return qobject_cast<QDesignerMenu*>(action->menu());

    QDesignerMenu *menu = m_subMenus.value(action);
    if (!menu) {
        menu = new QDesignerMenu(this);
        m_subMenus.insert(action, menu);
    }

    return menu;
}

void QDesignerMenu::slotShowSubMenuNow()
{
    m_showSubMenuTimer->stop();
    QAction *action = currentAction();

    if (QMenu *menu = findOrCreateSubMenu(action)) {
        menu->setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
        menu->adjustSize();
        QRect g = actionGeometry(action);
        menu->move(mapToGlobal(g.topRight()));
        menu->show();
        menu->setFocus();
    }
}

void QDesignerMenu::showSubMenu(QAction *action)
{
    m_showSubMenuTimer->stop();

    if (!action || qobject_cast<SpecialMenuAction*>(action)
            || action->isSeparator() || !isVisible())
        return;

    m_showSubMenuTimer->start(1000);
}

QDesignerMenu *QDesignerMenu::parentMenu() const
{
    return qobject_cast<QDesignerMenu*>(parentWidget());
}

QDesignerMenuBar *QDesignerMenu::parentMenuBar() const
{
    if (QDesignerMenuBar *mb = qobject_cast<QDesignerMenuBar*>(parentWidget())) {
        return mb;
    } else if (QDesignerMenu *m = parentMenu()) {
        return m->parentMenuBar();
    }

    return 0;
}

void QDesignerMenu::setVisible(bool visible)
{
    if (visible)
        m_currentIndex = 0;

    QMenu::setVisible(visible);

}

void QDesignerMenu::adjustSpecialActions()
{
    removeAction(m_addItem);
    removeAction(m_addSeparator);
    addAction(m_addItem);
    addAction(m_addSeparator);
}

bool QDesignerMenu::interactive(bool i)
{
    bool old = m_interactive;
    m_interactive = i;
    return old;
}

void QDesignerMenu::enterEditMode()
{
    if (m_currentIndex <= realActionCount()) {
        showLineEdit();
    } else {
        QAction *sep = createAction();
        sep->setSeparator(true);
        insertAction(safeActionAt(realActionCount()), sep);
        m_currentIndex = realActionCount() - 1;
        update();
    }
}

void QDesignerMenu::leaveEditMode()
{
    QAction *action = 0;

    if (m_currentIndex < realActionCount()) {
        action = actions().at(m_currentIndex);
    } else {
        Q_ASSERT(formWindow() != 0);   // ### undo/redo
        action = createAction();
        insertAction(currentAction(), action);
    }

    action->setText(m_editor->text()); // ### undo/redo
    adjustSize();

    if (parentMenu()) { // ### undo/redo
        parentMenu()->createRealMenuAction(parentMenu()->currentAction());
    }
    updateCurrentAction();
}

QAction *QDesignerMenu::safeMenuAction(QDesignerMenu *menu) const
{
    QAction *action = menu->menuAction();

    if (!action)
        action = m_subMenus.key(menu);

    return action;
}

void QDesignerMenu::showLineEdit()
{
    QAction *action = 0;

    if (m_currentIndex < realActionCount())
        action = actions().at(m_currentIndex);
    else
        action = m_addItem;

    if (action->isSeparator())
        return;

    // open edit field for item name
    m_editor->setText(action->text());
    m_editor->selectAll();
    m_editor->setGeometry(actionGeometry(action));
    m_editor->show();
    m_editor->setFocus();
}

QAction *QDesignerMenu::createAction() // ### undo/redo
{
    Q_ASSERT(formWindow() != 0);
    QDesignerFormWindowInterface *fw = formWindow();

    QAction *action = new QAction(fw);
    fw->core()->widgetFactory()->initialize(action);

    action->setObjectName("action");
    fw->core()->metaDataBase()->add(action);
    fw->ensureUniqueObjectName(action);

    AddActionCommand *cmd = new AddActionCommand(fw);
    cmd->init(action);
    fw->commandHistory()->push(cmd);

    fw->core()->actionEditor()->setFormWindow(fw);

    return action;
}

// ### share with QDesignerMenu::swap
bool QDesignerMenu::swap(int a, int b) // ### undo/redo
{
    int left = qMin(a, b);
    int right = qMax(a, b);

    QAction *action_a = safeActionAt(left);
    QAction *action_b = safeActionAt(right);

    if (action_a == action_b
            || !action_a
            || !action_b
            || qobject_cast<SpecialMenuAction*>(action_a)
            || qobject_cast<SpecialMenuAction*>(action_b))
        return false; // nothing to do

    right = qMin(right, realActionCount());
    if (right < 0)
        return false; // nothing to do

    formWindow()->beginCommand(QLatin1String("Move action"));

    QAction *action_b_before = safeActionAt(right + 1);

    RemoveActionFromCommand *cmd1 = new RemoveActionFromCommand(formWindow());
    cmd1->init(this, action_b, action_b_before);
    formWindow()->commandHistory()->push(cmd1);

    QAction *action_a_before = safeActionAt(left + 1);

    InsertActionIntoCommand *cmd2 = new InsertActionIntoCommand(formWindow());
    cmd2->init(this, action_b, action_a_before);
    formWindow()->commandHistory()->push(cmd2);

    RemoveActionFromCommand *cmd3 = new RemoveActionFromCommand(formWindow());
    cmd3->init(this, action_a, action_b);
    formWindow()->commandHistory()->push(cmd3);

    InsertActionIntoCommand *cmd4 = new InsertActionIntoCommand(formWindow());
    cmd4->init(this, action_a, action_b_before);
    formWindow()->commandHistory()->push(cmd4);

    formWindow()->endCommand();

    return true;
}

QAction *QDesignerMenu::safeActionAt(int index) const
{
    if (index < 0 || index >= actions().count())
        return 0;

    return actions().at(index);
}

void QDesignerMenu::hideSubMenu()
{
    foreach (QMenu *subMenu, qFindChildren<QMenu*>(this)) {
        subMenu->hide();
    }
}

void QDesignerMenu::deleteAction() // ### undo/redo
{
    removeAction(currentAction());
    adjustSize();
    update();
}

void QDesignerMenu::deactivateMenu()
{
    m_deactivateWindowTimer->start(10);
}

void QDesignerMenu::slotDeactivateNow()
{
    m_deactivateWindowTimer->stop();

    if (m_dragging) {
        return;
    }

    QDesignerMenu *root = findRootMenu();

    if (! findRootMenu()->findActivatedMenu()) {
        root->hide();
        root->hideSubMenu();
    }
}

void QDesignerMenu::drawSelection(QPainter *p, const QRect &r)
{
    p->save();

    QColor c = Qt::blue;
    p->setPen(c);
    c.setAlpha(32);
    p->setBrush(c);
    p->drawRect(r);

    p->restore();
}


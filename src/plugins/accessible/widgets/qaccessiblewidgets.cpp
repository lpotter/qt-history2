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

#include "qaccessiblewidgets.h"
#include "qabstracttextdocumentlayout.h"
#include "qtextedit.h"
#include "qtextdocument.h"
#include "qtextobject.h"
#include "qscrollbar.h"
#include "qdebug.h"
#include <QStackedWidget>
#include <QToolBox>

#ifndef QT_NO_ACCESSIBILITY

#ifndef QT_NO_TEXTEDIT

/*!
  \class QAccessibleTextEdit qaccessiblewidget.h
  \brief The QAccessibleTextEdit class implements the QAccessibleInterface for richtext editors.
  \internal
*/

static QTextBlock qTextBlockAt(const QTextDocument *doc, int pos)
{
    Q_ASSERT(pos >= 0);

    QTextBlock block = doc->begin();
    int i = 0;
    while (block.isValid() && i < pos) {
        block = block.next();
        ++i;
    }
    return block;
}

static int qTextBlockPosition(QTextBlock block)
{
    int child = 0;
    while (block.isValid()) {
        block = block.previous();
        ++child;
    }

    return child;
}

/*!
  \fn QAccessibleTextEdit::QAccessibleTextEdit(QWidget* widget)

  Constructs a QAccessibleTextEdit object for a \a widget.
*/
QAccessibleTextEdit::QAccessibleTextEdit(QWidget *o)
: QAccessibleWidgetEx(o, EditableText)
{
    Q_ASSERT(widget()->inherits("QTextEdit"));
    childOffset = QAccessibleWidgetEx::childCount();
}

/*! Returns the text edit. */
QTextEdit *QAccessibleTextEdit::textEdit() const
{
    return static_cast<QTextEdit *>(widget());
}

QRect QAccessibleTextEdit::rect(int child) const
{
    if (child <= childOffset)
        return QAccessibleWidgetEx::rect(child);

     QTextEdit *edit = textEdit();
     QTextBlock block = qTextBlockAt(edit->document(), child - childOffset - 1);
     if (!block.isValid())
         return QRect();

     QRect rect = edit->document()->documentLayout()->blockBoundingRect(block).toRect();
     rect.translate(-edit->horizontalScrollBar()->value(), -edit->verticalScrollBar()->value());

     rect = edit->viewport()->rect().intersect(rect);
     if (rect.isEmpty())
         return QRect();

     return rect.translated(edit->viewport()->mapToGlobal(QPoint(0, 0)));
}

int QAccessibleTextEdit::childAt(int x, int y) const
{
    QTextEdit *edit = textEdit();

    QPoint point = edit->viewport()->mapFromGlobal(QPoint(x, y));
    QTextBlock block = edit->cursorForPosition(point).block();
    if (block.isValid())
        return qTextBlockPosition(block) + childOffset;

    return QAccessibleWidgetEx::childAt(x, y);
}

/*! \reimp */
QString QAccessibleTextEdit::text(Text t, int child) const
{
    if (t == Value) {
        if (child > childOffset)
            return qTextBlockAt(textEdit()->document(), child - childOffset - 1).text();
        if (!child)
            return textEdit()->toPlainText();
    }

    return QAccessibleWidgetEx::text(t, child);
}

/*! \reimp */
void QAccessibleTextEdit::setText(Text t, int child, const QString &text)
{
    if (t != Value || (child > 0 && child <= childOffset)) {
        QAccessibleWidgetEx::setText(t, child, text);
        return;
    }
    if (textEdit()->isReadOnly())
        return;

    if (!child) {
        textEdit()->setText(text);
        return;
    }
    QTextBlock block = qTextBlockAt(textEdit()->document(), child - childOffset - 1);
    if (!block.isValid())
        return;

    QTextCursor cursor(block);
    cursor.select(QTextCursor::BlockUnderCursor);
    cursor.insertText(text);
}

/*! \reimp */
QAccessible::Role QAccessibleTextEdit::role(int child) const
{
    if (child > childOffset)
        return EditableText;
    return QAccessibleWidgetEx::role(child);
}

QVariant QAccessibleTextEdit::invokeMethodEx(QAccessible::Method method, int child,
                                                     const QVariantList &params)
{
    if (child)
        return QVariant();

    switch (method) {
    case ListSupportedMethods: {
        QVariantList list;
        list << ListSupportedMethods << SetCursorPosition << GetCursorPosition;
        return list;
    }
    case SetCursorPosition: {
        QTextCursor cursor = textEdit()->textCursor();
        cursor.setPosition(params.value(0).toInt());
        textEdit()->setTextCursor(cursor);
        return true; }
    case GetCursorPosition:
        return textEdit()->textCursor().position();
    }

    return QVariant();
}

int QAccessibleTextEdit::childCount() const
{
    return childOffset + textEdit()->document()->blockCount();
}
#endif // QT_NO_TEXTEDIT

#ifndef QT_NO_STACKEDWIDGET
// ======================= QAccessibleStackedWidget ======================
QAccessibleStackedWidget::QAccessibleStackedWidget(QWidget *widget)
    : QAccessibleWidgetEx(widget, LayeredPane)
{
    Q_ASSERT(qobject_cast<QStackedWidget *>(widget));
}

QVariant QAccessibleStackedWidget::invokeMethodEx(QAccessible::Method, int, const QVariantList &)
{
    return QVariant();
}


int QAccessibleStackedWidget::childAt(int x, int y) const
{
    QWidget *currentWidget = stackedWidget()->currentWidget();
    if (!currentWidget)
        return -1;
    QPoint position = currentWidget->mapFromGlobal(QPoint(x, y));
    if (currentWidget->rect().contains(position))
        return 1;
    return -1;
}

int QAccessibleStackedWidget::childCount() const
{
    return stackedWidget()->count();
}

int QAccessibleStackedWidget::indexOfChild(const QAccessibleInterface *child) const
{
    if (!child || (stackedWidget()->currentWidget() != child->object()))
        return -1;
    return 1;
}

int QAccessibleStackedWidget::navigate(RelationFlag relation, int entry, QAccessibleInterface **target) const
{
    *target = 0;
    QObject *targetObject = 0;
    switch (relation) {
    case Child:
        if (entry != 1)
            return -1;
        targetObject = stackedWidget()->currentWidget();
        break;
    default:
        return QAccessibleWidgetEx::navigate(relation, entry, target);
    }
    *target = QAccessible::queryAccessibleInterface(targetObject);
    return *target ? 0 : -1;
}

QStackedWidget *QAccessibleStackedWidget::stackedWidget() const
{
    return static_cast<QStackedWidget *>(object());
}
#endif // QT_NO_STACKEDWIDGET

#ifndef QT_NO_TOOLBOX
// ======================= QAccessibleToolBox ======================
QAccessibleToolBox::QAccessibleToolBox(QWidget *widget)
    : QAccessibleWidgetEx(widget, LayeredPane)
{
    Q_ASSERT(qobject_cast<QToolBox *>(widget));
}

QString QAccessibleToolBox::text(Text textType, int child) const
{
    if (textType != Value || child <= 0 || child > toolBox()->count())
        return QAccessibleWidgetEx::text(textType, child);
    return toolBox()->itemText(child - 1);
}

void QAccessibleToolBox::setText(Text textType, int child, const QString &text)
{
    if (textType != Value || child <= 0 || child > toolBox()->count()) {
        QAccessibleWidgetEx::setText(textType, child, text);
        return;
    }
    toolBox()->setItemText(child - 1, text);
}

QAccessible::State QAccessibleToolBox::state(int child) const
{
    QWidget *childWidget = toolBox()->widget(child - 1);
    if (!childWidget)
        return QAccessibleWidgetEx::state(child);
    QAccessible::State childState = QAccessible::Normal;
    if (toolBox()->currentWidget() == childWidget)
        childState |= QAccessible::Expanded;
    else
        childState |= QAccessible::Collapsed;
    return childState;
}

QVariant QAccessibleToolBox::invokeMethodEx(QAccessible::Method, int, const QVariantList &)
{
    return QVariant();
}

int QAccessibleToolBox::childCount() const
{
    return toolBox()->count();
}

int QAccessibleToolBox::indexOfChild(const QAccessibleInterface *child) const
{
    if (!child)
        return -1;
    QWidget *childWidget = qobject_cast<QWidget *>(child->object());
    if (!childWidget)
        return -1;
    int index = toolBox()->indexOf(childWidget);
    if (index != -1)
        ++index;
    return index;
}

int QAccessibleToolBox::navigate(RelationFlag relation, int entry, QAccessibleInterface **target) const
{
    if (entry <= 0 || entry > toolBox()->count())
        return QAccessibleWidgetEx::navigate(relation, entry, target);
    int index = -1;
    if (relation == QAccessible::Up)
        index = entry - 2;
    else if (relation == QAccessible::Down)
        index = entry;
    *target = QAccessible::queryAccessibleInterface(toolBox()->widget(index));
    return *target ? index + 1: -1;
}

QToolBox * QAccessibleToolBox::toolBox() const
{
    return static_cast<QToolBox *>(object());
}
#endif // QT_NO_TOOLBOX

#endif // QT_NO_ACCESSIBILITY

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

#include "previewwidget.h"
#include <QtEvents>

QT_BEGIN_NAMESPACE

PreviewWidget::PreviewWidget( QWidget *parent, const char *name )
    : PreviewWidgetBase( parent, name )
{
    // install event filter on child widgets
    QObjectList l = queryList("QWidget");
    for (int i = 0; i < l.size(); ++i) {
        QObject * obj = l.at(i);
        obj->installEventFilter(this);
        ((QWidget*)obj)->setFocusPolicy(Qt::NoFocus);
    }
}


void PreviewWidget::closeEvent(QCloseEvent *e)
{
    e->ignore();
}


bool PreviewWidget::eventFilter(QObject *, QEvent *e)
{
    switch ( e->type() ) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::Enter:
    case QEvent::Leave:
        return true; // ignore;
    default:
        break;
    }
    return false;
}

QT_END_NAMESPACE

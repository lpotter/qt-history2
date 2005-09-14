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

#ifndef QTESTACCESSIBLE_H
#define QTESTACCESSIBLE_H

#ifndef QT_NO_ACCESSIBILITY

#define QTEST_ACCESSIBILITY

#define VERIFY_EVENT(object, child, event) \
    VERIFY(QTestAccessibility::verifyEvent(object, child, (int)event))

#include <QtCore/qlist.h>
#include <QtGui/qaccessible.h>
#include <QtGui/qapplication.h>

class QObject;

struct QTestAccessibilityEvent
{
    QTestAccessibilityEvent(QObject* o = 0, int c = 0, int e = 0)
        : object(o), child(c), event(e) {}

    bool operator==(const QTestAccessibilityEvent &o) const
    {
        return o.object == object && o.child == child && o.event == event;
    }

    QObject* object;
    int child;
    int event;
};

typedef QList<QTestAccessibilityEvent> EventList;

class QTestAccessibility
{
public:
    static void initialize()
    {
        if (!instance()) {
            instance() = new QTestAccessibility;
            qAddPostRoutine(cleanup);
        }
    }
    static void cleanup()
    {
        delete instance();
        instance() = 0;
    }
    static void clearEvents() { eventList().clear(); }
    static EventList events() { return eventList(); }
    static bool verifyEvent(const QTestAccessibilityEvent& ev)
    {
        if (eventList().isEmpty())
            return FALSE;
        return eventList().takeFirst() == ev;
    }

    static bool verifyEvent(QObject *o, int c, int e)
    {
        return verifyEvent(QTestAccessibilityEvent(o, c, e));
    }

private:
    QTestAccessibility()
    {
        QAccessible::installUpdateHandler(updateHandler);
        QAccessible::installRootObjectHandler(rootObjectHandler);
    }

    ~QTestAccessibility()
    {
        QAccessible::installUpdateHandler(0);
        QAccessible::installRootObjectHandler(0);
    }

    static void rootObjectHandler(QObject *object)
    {
        //    qDebug("rootObjectHandler called %p", object);
        if (object) {
            QApplication* app = qobject_cast<QApplication*>(object);
            if ( !app )
                qWarning("QTEST_ACCESSIBILITY: root Object is not a QApplication!");
        } else {
            qWarning("QTEST_ACCESSIBILITY: root Object called with 0 pointer");
        }
    }

    static void updateHandler(QObject *o, int c, QAccessible::Event e)
    {
        //    qDebug("updateHandler called: %p %d %d", o, c, (int)e);
        eventList().append(QTestAccessibilityEvent(o, c, (int)e));
    }

    static EventList &eventList()
    {
        static EventList list;
        return list;
    }

    static QTestAccessibility *&instance()
    {
        static QTestAccessibility *ta = 0;
        return ta;
    }
};

#endif

#endif


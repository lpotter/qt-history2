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

#ifndef CURSOR_H
#define CURSOR_H

#include <QtCore/QPoint>
#include <QtCore/QList>

QT_BEGIN_NAMESPACE

class Item;

class Cursor
{
public:
    Cursor();
    ~Cursor();

    void setPosition(const QPoint &pt, bool countStep = true);
    inline QPoint position() const { return m_pos; }
    inline int totalSteps() const { return m_totalSteps; }

    inline QList<const Item *> items() const { return m_items; }
    void addItem(const Item *item);

private:
    QList<const Item *> m_items;
    QPoint m_pos;
    int m_totalSteps;

};

QT_END_NAMESPACE

#endif

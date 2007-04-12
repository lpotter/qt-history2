/****************************************************************************
**
** Copyright (C) 2005-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef LETTER_ITEM_H
#define LETTER_ITEM_H

#include <QtGui>
#include "demoitem.h"

class LetterItem : public DemoItem
{
public:
    LetterItem(char letter, QGraphicsScene *scene = 0, QGraphicsItem *parent = 0);
    virtual ~LetterItem();

protected:
    QImage *createImage(const QMatrix &matrix) const;
    
private:
    char letter;
};

#endif // LETTER_ITEM_H


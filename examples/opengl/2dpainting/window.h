/****************************************************************************
**
** Copyright (C) 2006-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef WINDOW_H
#define WINDOW_H

#include <QString>
#include <QWidget>
#include "helper.h"

class QLabel;
class QWidget;

class Window : public QWidget
{
    Q_OBJECT

public:
    Window();

private:
    Helper helper;
};

#endif

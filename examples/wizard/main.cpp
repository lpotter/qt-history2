/****************************************************************************
**
** Copyright (C) 1992-2003 Trolltech AS. All rights reserved.
**
** This file is part of an example program for Qt.
** EDITIONS: NOLIMITS
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "wizard.h"
#include <qapplication.h>

int main(int argc,char **argv)
{
    QApplication a(argc,argv);

    Wizard wizard;
    wizard.setCaption("Qt Example - Wizard");
    return wizard.exec();
}

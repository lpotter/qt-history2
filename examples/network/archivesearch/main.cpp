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

#include "archivedialog.h"
#include <qapplication.h>

int main(int argc, char **argv)
{
    QApplication a( argc, argv );
    ArchiveDialog ad;
    ad.show();

    QObject::connect( &a, SIGNAL(lastWindowClosed()),
		      &a, SLOT(quit()) );
    
    return a.exec();
}

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

#include <qpixmap.h>

#include "ftpviewitem.h"


FtpViewItem::FtpViewItem( QListView *parent, Type t, const QString &name, const QString &size, const QString &lastModified )
    : QListViewItem(parent,name,size,lastModified), type(t)
{
    // the pixmaps for folders and files are in an image collection
    if ( type == Directory )
	setPixmap( 0, QPixmap::fromMimeSource( "folder.png" ) );
    else
	setPixmap( 0, QPixmap::fromMimeSource( "file.png" ) );
}

int FtpViewItem::compare( QListViewItem * i, int col, bool ascending ) const
{
    // The entry ".." is always the first one.
    if ( text(0) == ".." ) {
	if ( ascending )
	    return -1;
	else
	    return 1;
    }
    if ( i->text(0) == ".." ) {
	if ( ascending )
	    return 1;
	else
	    return -1;
    }

    // Directories are before files.
    if ( type != ((FtpViewItem*)i)->type ) {
	if ( type == Directory ) {
	    if ( ascending )
		return -1;
	    else
		return 1;
	} else {
	    if ( ascending )
		return 1;
	    else
		return -1;
	}
    }

    // Use default sorting otherwise.
    return QListViewItem::compare( i, col, ascending );
}

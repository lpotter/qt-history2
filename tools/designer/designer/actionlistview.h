/**********************************************************************
** Copyright (C) 2000 Trolltech AS.  All rights reserved.
**
** This file is part of Qt Designer.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#ifndef ACTIONLISTVIEW_H
#define ACTIONLISTVIEW_H

#include <qlistview.h>
#include <qaction.h>

class ActionItem : public QListViewItem
{
public:
    ActionItem( QListView *lv, bool group )
	: QListViewItem( lv ),
	  a( group ? new QActionGroup( 0 ) : new QAction( 0 ) ) { setDragEnabled( TRUE ); }
    ActionItem( ActionItem *parent )
	: QListViewItem( parent ),
	  a( new QAction( parent->action() ) ) { setDragEnabled( TRUE ); }
    ActionItem( QListView *lv, QAction *ac )
	: QListViewItem( lv ), a( ac ) { setDragEnabled( TRUE ); }
    ActionItem( ActionItem *parent, QAction *ac )
	: QListViewItem( parent ), a( ac ) { setDragEnabled( TRUE ); }

    QAction *action() const { return a; }

private:
    QAction *a;

};

class ActionListView : public QListView
{
public:
    ActionListView( QWidget *parent = 0, const char *name = 0 );

protected:
    QDragObject *dragObject();
    
};

#endif

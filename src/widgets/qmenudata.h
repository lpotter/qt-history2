/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qmenudata.h#54 $
**
** Definition of QMenuData class
**
** Created : 941128
**
** Copyright (C) 1992-1998 Troll Tech AS.  All rights reserved.
**
** This file is part of Troll Tech's internal development tree for Qt.
**
** This header text will be replaced by an appropriate text by the
** mkdist script which generates external distributions.
**
** If you are using the Qt Professional Edition or the Qt Free Edition,
** please notify Troll Tech at <info@troll.no> if you see this text.
**
** To Troll Tech developers: This header was generated by the script
** fixcopyright-int. It has the same number of text lines as the free
** and professional editions to avoid line number inconsistency.
**
*****************************************************************************/

#ifndef QMENUDATA_H
#define QMENUDATA_H

#ifndef QT_H
#include "qglobal.h"
#endif // QT_H


class QPopupMenu;

#if defined(INCLUDE_MENUITEM_DEF)

#include "qstring.h"
#include "qpixmap.h"
#include "qsignal.h"

class QMenuItem					// internal menu item class
{
friend class QMenuData;
public:
    QMenuItem();
   ~QMenuItem();

    int		id()		const	{ return ident; }
    QString text()		const	{ return text_data; }
    QPixmap    *pixmap()	const	{ return pixmap_data; }
    QPopupMenu *popup()		const	{ return popup_menu; }
    int		key()		const	{ return accel_key; }
    QSignal    *signal()	const	{ return signal_data; }
    bool	isSeparator()	const	{ return is_separator; }
    bool	isEnabled()	const	{ return is_enabled; }
    bool	isChecked()	const	{ return is_checked; }
    bool	isDirty()	const	{ return is_dirty; }

    void	setText( const QString &text ) { text_data = text; }
    void	setDirty( bool d )	    { is_dirty = d; }

private:
    int		ident;				// item identifier
    QString	text_data;			// item text
    QPixmap    *pixmap_data;			// item pixmap
    QPopupMenu *popup_menu;			// item popup menu
    int		accel_key;			// accelerator key
    QSignal    *signal_data;			// connection
    uint	is_separator : 1;		// separator flag
    uint	is_enabled   : 1;		// disabled flag
    uint	is_checked   : 1;		// checked flag
    uint	is_dirty     : 1;		// dirty (update) flag

private:	// Disabled copy constructor and operator=
    QMenuItem( const QMenuItem & );
    QMenuItem &operator=( const QMenuItem & );
};

#include "qlist.h"
typedef Q_DECLARE(QListM,QMenuItem)	    QMenuItemList;
typedef Q_DECLARE(QListIteratorM,QMenuItem) QMenuItemListIt;

#else

class QMenuItem;
class QMenuItemList;
class QPixmap;

#endif


class QMenuData					// menu data class
{
friend class QMenuBar;
friend class QPopupMenu;
public:
    QMenuData();
    virtual ~QMenuData();

    uint	count() const;

    int		insertItem( const QString &text,
			    const QObject *receiver, const char* member,
			    int accel=0 );
    int		insertItem( const QPixmap &pixmap,
			    const QObject *receiver, const char* member,
			    int accel=0 );
    int		insertItem( const QPixmap &pixmap, const QString &text,
			    const QObject *receiver, const char* member,
			    int accel=0 );

    int		insertItem( const QString &text,
			    const QObject *receiver, const char* member,
			    int accel, int id, int index = -1 );
    int		insertItem( const QPixmap &pixmap,
			    const QObject *receiver, const char* member,
			    int accel, int id, int index = -1 );
    int		insertItem( const QPixmap &pixmap, const QString &text,
			    const QObject *receiver, const char* member,
			    int accel, int id, int index = -1 );

    int		insertItem( const QString &text, int id=-1, int index=-1 );
    int		insertItem( const QString &text, QPopupMenu *popup,
			    int id=-1, int index=-1 );
    int		insertItem( const QPixmap &pixmap, int id=-1, int index=-1 );
    int		insertItem( const QPixmap &pixmap, QPopupMenu *popup,
			    int id=-1, int index=-1 );
    int		insertItem( const QPixmap &pixmap, const QString &text,
			    int id=-1, int index=-1 );
    int		insertItem( const QPixmap &pixmap, const QString &text,
			    QPopupMenu *popup,
			    int id=-1, int index=-1 );

    void	insertSeparator( int index=-1 );

    void	removeItem( int id )		{ removeItemAt(indexOf(id)); }
    void	removeItemAt( int index );
    void	clear();

    int		accel( int id )		const;
    virtual void	setAccel( int key, int id );

    QString text( int id )		const;
    QPixmap    *pixmap( int id )	const;
    void	changeItem( const QString &text, int id );
    void	changeItem( const QPixmap &pixmap, int id );
    void	changeItem( const QPixmap &pixmap, const QString &text, int id );

    bool	isItemEnabled( int id ) const;
    virtual void	setItemEnabled( int id, bool enable );

    bool	isItemChecked( int id ) const;
    virtual void	setItemChecked( int id, bool check );

    virtual void updateItem( int id );

    int		indexOf( int id )	const;
    int		idAt( int index )	const;
    virtual void	setId( int index, int id );

    bool	connectItem( int id,
			     const QObject *receiver, const char* member );
    bool	disconnectItem( int id,
				const QObject *receiver, const char* member );

    QMenuItem  *findItem( int id )	const;
    QMenuItem  *findItem( int id, QMenuData ** parent )	const;

protected:
    int		   actItem;
    QMenuItemList *mitems;
    QMenuData	  *parentMenu;
    uint	   isPopupMenu	: 1;
    uint	   isMenuBar	: 1;
    uint	   badSize	: 1;
    uint	   mouseBtDn	: 1;
    virtual void   menuContentsChanged();
    virtual void   menuStateChanged();
    virtual void   menuInsPopup( QPopupMenu * );
    virtual void   menuDelPopup( QPopupMenu * );

    QMenuItem * findPopup( QPopupMenu *, int *index = 0 );

private:
    int		insertAny( const QString &, const QPixmap *, QPopupMenu *,
			   int, int );
    void	removePopup( QPopupMenu * );
    virtual void	setAllDirty( bool );

private:	//Disabled copy constructor and operator=
    QMenuData( const QMenuData & );
    QMenuData &operator=( const QMenuData & );
};


#endif // QMENUDATA_H

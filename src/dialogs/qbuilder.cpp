/****************************************************************************
** $Id: //depot/qt/main/src/dialogs/qbuilder.cpp#4 $
**
** Implementation of QBuilder class
**
** Created : 980830
**
** Copyright (C) 1998 Troll Tech AS.  All rights reserved.
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

#include "qbuilder.h"
#include "qmenubar.h"
#include "qstatusbar.h"
#include "qlistview.h"
#include "qsplitter.h"
#include "qmetaobject.h"
#include "qobjectdict.h"
#include "qobjectlist.h"
#include "qlabel.h"
#include "qpushbutton.h"
#include "qvbox.h"
#include "qhbox.h"

class QBuilderClassItem : public QListViewItem {
    QBuilderPrivate* d;
    QMetaObject* meta;
public:
    QBuilderClassItem( QListView * parent, QMetaObject* mo, QBuilderPrivate* pd ) :
	QListViewItem( parent, mo->className() )
    {
	d = pd;
	meta = mo;
    }

    QBuilderClassItem( QListViewItem * parent, QMetaObject* mo, QBuilderPrivate* pd ) :
	QListViewItem( parent, mo->className() )
    {
	d = pd;
	meta = mo;
    }

    QMetaObject* at() { return meta; }

    void setup()
    {
	QListViewItem::setup();
	if ( childCount() )
	    return;
	QDictIterator<QMetaObject> it(*objectDict);
	QMetaObject* child;
	while ((child = it.current())) {
	    ++it;
	    if ( child->superClass() == meta ) {
		// Subclass of this
		(void)new QBuilderClassItem(this,child,d);
	    }
	}
    }
};

class QBuilderPrivate {
public:
    QBuilderPrivate(QWidget* parent)
    {
	splitter = new QSplitter(parent);
	classes = new QListView(splitter);
	objects = new QListView(splitter);
	classes->setFrameStyle(QFrame::Sunken|QFrame::Panel);
	objects->setFrameStyle(QFrame::Sunken|QFrame::Panel);

	classes->addColumn("Class");
	objects->addColumn("Object");
	objects->addColumn("Class");
	objects->addColumn("Address");
    }

    QBuilderObjectItem* findTopLevel( QObject* o )
    {
	QListViewItem* cursor = objects->firstChild();
	while ( cursor ) {
	    QBuilderObjectItem* boi = (QBuilderObjectItem*)cursor;
	    if ( boi->at() == o ) {
		return boi;
	    }
	    cursor = cursor->nextSibling();
	}
	return 0;
    }

    void removeTopLevel( QObject* o )
    {
	delete findTopLevel(o);
    }

    void selectClass( QMetaObject* mo )
    {
	QBuilderClassItem* bci = (QBuilderClassItem*)selectClassFrom(mo);
	classes->setSelected(bci,TRUE);
	classes->ensureItemVisible(bci);
    }

    QListViewItem* selectClassFrom( QMetaObject* mo )
    {
	if ( mo->superClass() ) {
	    QListViewItem* cursor = selectClassFrom( mo->superClass() );
	    cursor = cursor->firstChild();
	    while ( cursor ) {
		QBuilderClassItem* bci = (QBuilderClassItem*)cursor;
		if ( bci->at() == mo ) {
		    bci->setOpen(TRUE);
		    return bci;
		}
		cursor = cursor->nextSibling();
	    }
	    fatal("Huh?");
	    return 0;
	} else {
	    // QObject
	    QListViewItem* bci = classes->firstChild();
	    bci->setOpen(TRUE);
	    return bci;
	}
    }

    QSplitter* splitter;
    QListView* classes;
    QListView* objects;
};

static
QString adrtext(void* a)
{
    QString s;
    s.sprintf("0x%p", a);
    return s;
}

static
bool find( QObject* o, QListViewItem* cursor )
{
    while ( cursor ) {
	QBuilderObjectItem* boi = (QBuilderObjectItem*)cursor;
	if ( boi->at() == o ) {
	    return TRUE;
	}
	cursor = cursor->nextSibling();
    }
    return FALSE;
}

QBuilderObjectItem::QBuilderObjectItem( QListView * parent, QObject* o, QBuilderPrivate *pd ) :
    QListViewItem( parent, o->name(), o->className(), adrtext(o) )
{
    d = pd;
    object = o;
    object->installEventFilter(this);
    connect(object, SIGNAL(destroyed()), this, SLOT(objectDestroyed()));
}

QBuilderObjectItem::QBuilderObjectItem( QListViewItem * parent, QObject* o, QBuilderPrivate *pd ) :
    QListViewItem( parent, o->name(), o->className(), adrtext(o) )
{
    d = pd;
    object = o;
    object->installEventFilter(this);
    connect(object, SIGNAL(destroyed()), this, SLOT(objectDestroyed()));
}

bool QBuilderObjectItem::eventFilter(QObject* o, QEvent* e)
{
    if ( o == object ) {
	QChildEvent* ce = (QChildEvent*)e;
	switch ( e->type() ) {
	  case QEvent::ChildInserted: {
		d->removeTopLevel( ce->child() );
		if ( !find(ce->child(), firstChild()) ) {
		    QBuilderObjectItem* lvi = new QBuilderObjectItem( this, ce->child(), d );
		    lvi->fillExistingTree();
		}
		setExpandable(TRUE);
	    }
	    break;
	  case QEvent::ChildRemoved: {
		QListViewItem *cursor = firstChild();
		while ( cursor ) {
		    QBuilderObjectItem* boi = (QBuilderObjectItem*)cursor;
		    if ( boi->object == ce->child() ) {
			delete cursor;
			cursor = 0;
		    } else {
			cursor = cursor->nextSibling();
		    }
		}
	    }
	    break;
	  default:
	    break;
	}
    }
    return FALSE;
}

void QBuilderObjectItem::setup()
{
    QListViewItem::setup();
}

void QBuilderObjectItem::fillExistingTree()
{
    QObjectList *list = (QObjectList*)object->children();
    if ( !list ) return;
    QObjectListIt it(*list);
    QObject* child;
    while ((child = it.current())) {
	++it;
	// Child object of this
	if ( !find(child,firstChild()) ) {
	    QBuilderObjectItem* lvi = new QBuilderObjectItem(this,child,d);
	    lvi->fillExistingTree();
	}
    }
    setExpandable(TRUE);
}

void QBuilderObjectItem::objectDestroyed()
{
    // Me too.
    delete this;
}

QBuilder::QBuilder() :
    QMainWindow(0,0,WDestructiveClose)
{
    int id;

    QPopupMenu * file = new QPopupMenu();
    menuBar()->insertItem( "&File", file );

    QPopupMenu * edit = new QPopupMenu();
    id=menuBar()->insertItem( "&Edit", edit );
    menuBar()->setItemEnabled(id,FALSE);

    QPopupMenu * options = new QPopupMenu();
    menuBar()->insertItem( "&Options", options );

    QVBox* vbox = new QVBox(this);
    d = new QBuilderPrivate(vbox);
    QHBox* hbox = new QHBox(vbox);
    new QLabel("Class\nstuff\nhere",hbox);
    new QLabel("Object\nstuff\nhere",hbox);

    setCentralWidget(vbox);

    QString msg;
    int nclasses = QMetaObjectInit::init()+1; // +1 for QObject
    if ( nclasses ) {
	msg.sprintf("Qt Application Builder - %d classes", nclasses);
	QListViewItem *lvi = new QBuilderClassItem( d->classes, QObject::metaObject(), d );
	lvi->setOpen(TRUE); // #### WWA: Arnt, why needed?
    } else {
	msg = "Sorry, your compiler/platform is insufficient for "
		"Qt Application Builder";
    }
    statusBar()->message(msg);

    connect( d->objects, SIGNAL(selectionChanged(QListViewItem*)),
	     this, SLOT(selectObject(QListViewItem*)) );

    connect( d->classes, SIGNAL(selectionChanged(QListViewItem*)),
	     this, SLOT(selectClass(QListViewItem*)) );

    updateDetails(0,0);
}

QBuilder::~QBuilder()
{
}

void QBuilder::addTopLevelWidget(QWidget* tlw)
{
    if ( !d->findTopLevel(tlw) ) {
	QBuilderObjectItem *lvi = new QBuilderObjectItem( d->objects, tlw, d );
	lvi->fillExistingTree();
	lvi->setOpen(TRUE); // #### WWA: Arnt, why needed?
    }
}

void QBuilder::selectObject( QListViewItem* lvi )
{
    if ( lvi->isSelected() ) {
	QBuilderObjectItem* boi = (QBuilderObjectItem*)lvi;
	d->selectClass(boi->at()->metaObject());
	updateDetails(boi->at());
    }
}

void QBuilder::selectClass( QListViewItem* lvi )
{
    if ( lvi->isSelected() ) {
	QBuilderClassItem* bci = (QBuilderClassItem*)lvi;
	d->selectClass(bci->at());

	QBuilderObjectItem* boi = (QBuilderObjectItem*)
				    d->objects->currentItem();
	if ( boi && bci->at() != boi->at()->metaObject() ) {
	    d->objects->setSelected(boi,FALSE);
	    updateDetails(0,bci->at());
	}
    }
}

void QBuilder::updateDetails( QObject* object, QMetaObject* cls )
{
    if ( object ) {
	cls = object->metaObject();
	// Show detailed information
    } else {
	// Disable display
    }

    if ( cls ) {
	// Show detailed information
    } else {
	// Disable display
    }
}

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

#include <qvariant.h>  // HP-UX compiler needs this here
#include "formlist.h"
#include "formwindow.h"
#include "mainwindow.h"
#include "pixmapchooser.h"
#include "globaldefs.h"
#include "command.h"
#include "project.h"
#include "pixmapcollection.h"
#include "sourcefile.h"

#include <qheader.h>
#include <qdragobject.h>
#include <qfileinfo.h>
#include <qapplication.h>
#include <qpainter.h>
#include <qpen.h>
#include <qobjectlist.h>
#include <qworkspace.h>
#include <qpopupmenu.h>
#include <qtextstream.h>
#include "qcompletionedit.h"

static const char * folder_xpm[]={
    "16 16 6 1",
    ". c None",
    "b c #ffff00",
    "d c #000000",
    "* c #999999",
    "a c #cccccc",
    "c c #ffffff",
    "................",
    "................",
    "..*****.........",
    ".*ababa*........",
    "*abababa******..",
    "*cccccccccccc*d.",
    "*cbababababab*d.",
    "*cabababababa*d.",
    "*cbababababab*d.",
    "*cabababababa*d.",
    "*cbababababab*d.",
    "*cabababababa*d.",
    "*cbababababab*d.",
    "**************d.",
    ".dddddddddddddd.",
    "................"};

static QPixmap *folderPixmap = 0;
static bool blockNewForms = FALSE;

FormListItem::FormListItem( QListView *parent )
    : QListViewItem( parent ), formwindow( 0 ), sourcefile( 0 )
{
}

FormListItem::FormListItem( QListViewItem *parent, const QString &form, const QString &file, FormWindow *fw )
    : QListViewItem( parent, form, file, "" ), formwindow( fw ), sourcefile( 0 )
{
    setPixmap( 0, PixmapChooser::loadPixmap( "form.xpm", PixmapChooser::Mini ) );
}

FormListItem::FormListItem( QListViewItem *parent, const QString &file, SourceFile *fl )
    : QListViewItem( parent, file ), formwindow( 0 ), sourcefile( fl )
{
    t = Source;
}

void FormListItem::paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align )
{
    QColorGroup g( cg );
    g.setColor( QColorGroup::Base, backgroundColor() );
    g.setColor( QColorGroup::Foreground, Qt::black );
    g.setColor( QColorGroup::Text, Qt::black );
    p->save();

    if ( formWindow() && formWindow()->commandHistory()->isModified() ) {
	QFont f = p->font();
	f.setBold( TRUE );
	p->setFont( f );
    } else if ( sourcefile && sourcefile->isModified() ) {
	QFont f = p->font();
	f.setBold( TRUE );
	p->setFont( f );
    }

    QListViewItem::paintCell( p, g, column, width, align );
    p->setPen( QPen( cg.dark(), 1 ) );
    if ( column == 0 )
	p->drawLine( 0, 0, 0, height() - 1 );
    if ( listView()->firstChild() != this ) {
	if ( nextSibling() != itemBelow() && itemBelow()->depth() < depth() ) {
	    int d = depth() - itemBelow()->depth();
	    p->drawLine( -listView()->treeStepSize() * d, height() - 1, 0, height() - 1 );
	}
    }
    p->drawLine( 0, height() - 1, width, height() - 1 );
    p->drawLine( width - 1, 0, width - 1, height() );
    p->restore();
}

QColor FormListItem::backgroundColor()
{
    updateBackColor();
    return backColor;
}

void FormListItem::updateBackColor()
{
    if ( listView()->firstChild() == this ) {
	backColor = *backColor1;
	return;
    }

    QListViewItemIterator it( this );
    --it;
    if ( it.current() ) {
	if ( ( ( FormListItem*)it.current() )->backColor == *backColor1 )
	    backColor = *backColor2;
	else
	    backColor = *backColor1;
    } else {
	backColor == *backColor1;
    }
}

FormList::FormList( QWidget *parent, MainWindow *mw, Project *pro )
    : QListView( parent, 0, WStyle_Customize | WStyle_NormalBorder | WStyle_Title |
		 WStyle_Tool | WStyle_MinMax | WStyle_SysMenu ), mainWindow( mw ),
	project( pro )
{
    init_colors();

    bufferEdit = 0;
    header()->setMovingEnabled( FALSE );
    header()->setStretchEnabled( TRUE );
    header()->hide();
    setSorting( -1 );
    setResizePolicy( QScrollView::Manual );
    setIcon( PixmapChooser::loadPixmap( "logo" ) );
    QPalette p( palette() );
    p.setColor( QColorGroup::Base, QColor( *backColor2 ) );
    (void)*selectedBack; // hack
    setPalette( p );
    addColumn( tr( "Form" ) );
    addColumn( tr( "Filename" ) );
    setAllColumnsShowFocus( TRUE );
    connect( this, SIGNAL( mouseButtonClicked( int, QListViewItem *, const QPoint &, int ) ),
	     this, SLOT( itemClicked( int, QListViewItem * ) ) ),
    connect( this, SIGNAL( contextMenuRequested( QListViewItem *, const QPoint &, int ) ),
	     this, SLOT( rmbClicked( QListViewItem * ) ) ),
    setHScrollBarMode( AlwaysOff );
    setVScrollBarMode( AlwaysOn );
    viewport()->setAcceptDrops( TRUE );
    setAcceptDrops( TRUE );
    setColumnWidthMode( 1, Manual );
    setRootIsDecorated( TRUE );

    if ( !folderPixmap ) {
	folderPixmap = new QPixmap( folder_xpm );
    }

    LanguageInterface *iface = MetaDataBase::languageInterface( pro->language() );

    imageParent = new FormListItem( this );
    imageParent->setType( FormListItem::Parent );
    imageParent->setText( 0, tr( "Images" ) );
    imageParent->setPixmap( 0, *folderPixmap );
    imageParent->setOpen( TRUE );

    if ( iface && iface->supports( LanguageInterface::AdditionalFiles ) ) {
	sourceParent = new FormListItem( this );
	sourceParent->setType( FormListItem::Parent );
	sourceParent->setText( 0, tr( "Source Files" ) );
	sourceParent->setPixmap( 0, *folderPixmap );
	sourceParent->setOpen( TRUE );
    }

    formsParent = new FormListItem( this );
    formsParent->setType( FormListItem::Parent );
    formsParent->setText( 0, tr( "Forms" ) );
    formsParent->setPixmap( 0, *folderPixmap );
    formsParent->setOpen( TRUE );
}

void FormList::setProject( Project *pro )
{
    project = pro;
    clear();

    imageParent = new FormListItem( this );
    imageParent->setType( FormListItem::Parent );
    imageParent->setText( 0, tr( "Images" ) );
    imageParent->setPixmap( 0, *folderPixmap );
    imageParent->setOpen( TRUE );

    bufferEdit->clear();

    LanguageInterface *iface = MetaDataBase::languageInterface( pro->language() );
    QString extension = "xx";
    if ( iface )
	extension = iface->formCodeExtension();

    if ( iface && iface->supports( LanguageInterface::AdditionalFiles ) ) {
	sourceParent = new FormListItem( this );
	sourceParent->setType( FormListItem::Parent );
	sourceParent->setText( 0, tr( "Source Files" ) );
	sourceParent->setPixmap( 0, *folderPixmap );
	sourceParent->setOpen( TRUE );
    }

    formsParent = new FormListItem( this );
    formsParent->setType( FormListItem::Parent );
    formsParent->setText( 0, tr( "Forms" ) );
    formsParent->setPixmap( 0, *folderPixmap );
    formsParent->setOpen( TRUE );

    QStringList lst = project->uiFiles();
    for ( QStringList::Iterator it = lst.begin(); it != lst.end(); ++it ) {
	if ( (*it).isEmpty() )
	    continue;
	FormListItem *item = new FormListItem( formsParent, tr( "<unknown>" ), *it, 0 );
	item->setType( FormListItem::Form );
	QString className = project->formName( item->text( 1 ) );
	bufferEdit->addCompletionEntry( item->text( 1 ) );
	if ( QFile::exists( project->makeAbsolute( item->text( 1 ) + extension ) ) )
	    bufferEdit->addCompletionEntry( item->text( 1 ) + extension );
	if ( !className.isEmpty() ) {
	    item->setText( 0, className );
	    bufferEdit->addCompletionEntry( className );
	}
    }

    QObjectList *l = mainWindow->workSpace()->queryList( "FormWindow", 0, FALSE, TRUE );
    for ( QObject *o = l->first(); o; o = l->next() ) {
	if ( ( (FormWindow*)o )->project() != pro )
	    continue;
	QListViewItemIterator it( this );
	while ( it.current() ) {
	    if ( !it.current()->parent() ) {
		++it;
		continue;
	    }
	    if ( project->makeAbsolute( ( (FormListItem*)it.current() )->text( 1 ) ) ==
		 project->makeAbsolute( ( (FormWindow*)o )->fileName() ) ) {
		( (FormListItem*)it.current() )->setFormWindow( ( (FormWindow*)o ) );
		it.current()->setText( 0, o->name() );
		bufferEdit->addCompletionEntry( o->name() );
	    }
	    ++it;
	}
    }

    QPtrList<FormWindow> forms = project->unnamedForms();
    for ( FormWindow *fw = forms.first(); fw; fw = forms.next() ) {
	FormListItem *item = new FormListItem( formsParent, fw->mainContainer()->name(), "", fw );
	bufferEdit->addCompletionEntry( fw->mainContainer()->name() );
	item->setType( FormListItem::Form );
    }

    QValueList<PixmapCollection::Pixmap> pixmaps = project->pixmapCollection()->pixmaps();
    {
	for ( QValueList<PixmapCollection::Pixmap>::Iterator it = pixmaps.begin(); it != pixmaps.end(); ++it ) {
	    FormListItem *item = new FormListItem( imageParent, (*it).name, "", 0 );
	    QPixmap pix( (*it).pix );
	    QImage img = pix.convertToImage();
	    img = img.smoothScale( 20, 20 );
	    pix.convertFromImage( img );
	    item->setPixmap( 0, pix );
	    item->setType( FormListItem::Image );
	}
    }

    if ( !iface || !iface->supports( LanguageInterface::AdditionalFiles ) )
	return;

    QPtrList<SourceFile> sources = pro->sourceFiles();
    for ( SourceFile *f = sources.first(); f; f = sources.next() ) {
	(void)new FormListItem( sourceParent, pro->makeRelative( f->fileName() ), f );
	bufferEdit->addCompletionEntry( pro->makeRelative( f->fileName() ) );
    }
}

void FormList::addForm( FormWindow *fw )
{
    fw->setProject( project );
    if ( blockNewForms ) {
	if ( currentItem() ) {
	    ( (FormListItem*)currentItem() )->setFormWindow( fw );
	    ( (FormListItem*)currentItem() )->setText( 0, fw->name() );
	}
	if ( project ) {
	    project->setFormWindow( fw->fileName(), fw );
	    bufferEdit->addCompletionEntry( fw->fileName() );
	    bufferEdit->addCompletionEntry( fw->name() );
	}
	return;
    }

    QString fn = project->makeRelative( fw->fileName() );
    FormListItem *i = new FormListItem( formsParent, fw->name(), fn, 0 );
    i->setType( FormListItem::Form );
    i->setFormWindow( fw );
    bufferEdit->addCompletionEntry( fw->name() );
    bufferEdit->addCompletionEntry( fw->fileName() );
    if ( !project )
	return;
    project->addUiFile( fn, fw );
}

void FormList::removeForm( FormWindow *fw )
{
    FormListItem *i = findItem( fw );
    if ( !i )
	return;
    project->removeUiFile( ( (FormListItem*)i )->text( 1 ), ( (FormListItem*)i )->formWindow() );
    bufferEdit->removeCompletionEntry( i->text( 1 ) );
    bufferEdit->removeCompletionEntry( i->text( 0 ) );
    delete i;
}

void FormList::modificationChanged( bool, QObject *obj )
{
    if ( obj->inherits( "FormWindow" ) ) {
	FormListItem *i = findItem( (FormWindow*)obj );
	if ( i )
	    i->repaint();
    } else if ( obj->inherits( "SourceEditor" ) ) {
	FormListItem *i = findItem( (SourceFile*)( (SourceEditor*)obj )->object() );
	if ( i )
	    i->repaint();
    }
}

void FormList::fileNameChanged( const QString &fn, FormWindow *fw )
{
    QString extension = "xx";
    LanguageInterface *iface = MetaDataBase::languageInterface( project->language() );
    if ( iface )
	extension = iface->formCodeExtension();

    QString s = project->makeRelative( fn );
    FormListItem *i = findItem( fw );
    bufferEdit->removeCompletionEntry( i->text( 1 ) );
    bufferEdit->removeCompletionEntry( i->text( 1 ) + extension );
    if ( !i )
	return;
    if ( s.isEmpty() ) {
	i->setText( 1, tr( "(unnamed)" ) );
    } else {
	i->setText( 1, s );
	bufferEdit->addCompletionEntry( s );
	if ( QFile::exists( project->makeAbsolute( s + extension ) ) )
	    bufferEdit->addCompletionEntry( s + extension );
    }
    if ( project )
	project->setFormWindowFileName( fw, s );
}

void FormList::activeFormChanged( FormWindow *fw )
{
    FormListItem *i = findItem( fw );
    if ( i ) {
	setCurrentItem( i );
	setSelected( i, TRUE );
    }
}

void FormList::nameChanged( FormWindow *fw )
{
    FormListItem *i = findItem( fw );
    if ( !i )
	return;
    i->setText( 0, fw->name() );
}

FormListItem *FormList::findItem( FormWindow *fw )
{
    QListViewItemIterator it( this );
    for ( ; it.current(); ++it ) {
	if ( ( (FormListItem*)it.current() )->formWindow() == fw )
	    return (FormListItem*)it.current();
    }
    return 0;
}

FormListItem *FormList::findItem( SourceFile *sf )
{
    QListViewItemIterator it( this );
    for ( ; it.current(); ++it ) {
	if ( ( (FormListItem*)it.current() )->sourceFile() == sf )
	    return (FormListItem*)it.current();
    }
    return 0;
}


void FormList::closed( FormWindow *fw )
{
    FormListItem *i = findItem( fw );
    if ( i ) {
	i->setFormWindow( 0 );
	i->repaint();
    }
}

void FormList::closeEvent( QCloseEvent *e )
{
    emit hidden();
    e->accept();
}

void FormList::itemClicked( int button, QListViewItem *i )
{
    if ( !i || button != LeftButton )
	return;
    if ( i->rtti() == FormListItem::Form ) {
	if ( ( (FormListItem*)i )->formWindow() ) {
	    ( (FormListItem*)i )->formWindow()->setFocus();
	} else {
	    blockNewForms = TRUE;
	    mainWindow->openFile( project->makeAbsolute( ( (FormListItem*)i )->text( 1 ) ) );
	    blockNewForms = FALSE;
	}
    } else if ( i->rtti() == FormListItem::Source ) {
	mainWindow->editSource( ( (FormListItem*)i )->sourceFile() );
    }
}

void FormList::bufferChosen( const QString &buffer )
{
    bufferEdit->setText( "" );
    QListViewItemIterator it( this );
    QListViewItem *res = 0;
    QString extension = "xx";
    LanguageInterface *iface = MetaDataBase::languageInterface( project->language() );
    if ( iface )
	extension = iface->formCodeExtension();
    bool formCode = buffer.right( extension.length() + 2 ) == QString( "ui" + extension );
    while ( it.current() ) {
	if ( !formCode &&
	     ( it.current()->text( 0 ) == buffer || it.current()->text( 1 ) == buffer ) ||
	     formCode && ( it.current()->text( 1 ) + extension ) == buffer ) {
	    res = it.current();
	    break;
	}
	++it;
    }

    if ( res ) {
	setCurrentItem( res );
	itemClicked( LeftButton, res );
	if ( formCode )
	    MainWindow::self->editSource();
    }
}

void FormList::contentsDropEvent( QDropEvent *e )
{
    if ( !QUriDrag::canDecode( e ) ) {
	e->ignore();
    } else {
	QStringList files;
	QUriDrag::decodeLocalFiles( e, files );
	if ( !files.isEmpty() ) {
	    for ( QStringList::Iterator it = files.begin(); it != files.end(); ++it ) {
		QString fn = *it;
		mainWindow->fileOpen( "", "", fn );
	    }
	}
    }
}

void FormList::contentsDragEnterEvent( QDragEnterEvent *e )
{
    if ( !QUriDrag::canDecode( e ) )
	e->ignore();
    else
	e->accept();
}

void FormList::contentsDragMoveEvent( QDragMoveEvent *e )
{
    if ( !QUriDrag::canDecode( e ) )
	e->ignore();
    else
	e->accept();
}

void FormList::rmbClicked( QListViewItem *i )
{
    if ( !i )
	return;

    if ( i->rtti() == FormListItem::Form || i->text( 0 ) == tr( "Forms" ) ) {
	QPopupMenu menu( this );

	const int ADD_EXISTING_FORM = menu.insertItem( tr( "Add &existing form to project..." ) );
	const int ADD_NEW_FORM = menu.insertItem( tr( "Add &new form to project..." ) );
	int REMOVE_FORM = -2;
	if ( i->rtti() == FormListItem::Form ) {
	    menu.insertSeparator();
	    REMOVE_FORM = menu.insertItem( tr( "&Remove form from project" ) );
	}
	int id = menu.exec( QCursor::pos() );

	if ( id == -1 )
	    return;

	if ( id == REMOVE_FORM ) {
	    project->removeUiFile( ( (FormListItem*)i )->text( 1 ), ( (FormListItem*)i )->formWindow() );
	    bool doDelete = !( (FormListItem*)i )->text( 1 ).isEmpty() &&
			    ( (FormListItem*)i )->text( 1 ) != "(unnamed)";
	    if ( ( (FormListItem*)i )->formWindow() ) {
		( (FormListItem*)i )->formWindow()->setProject( 0 );
		( (FormListItem*)i )->formWindow()->commandHistory()->setModified( FALSE );
		( (FormListItem*)i )->formWindow()->close();
	    }
	    if ( doDelete )
		delete i;
	} else if ( id == ADD_EXISTING_FORM ) {
	    mainWindow->fileOpen( "Qt User-Interface Files (*.ui)", "ui" );
	} else if ( id == ADD_NEW_FORM ) {
	    mainWindow->fileNew();
	}
    } else if ( i->rtti() == FormListItem::Source || i->text( 0 ) == "Source Files" ) {
	QPopupMenu menu( this );

	const int ADD_EXISTING_SOURCE = menu.insertItem( tr( "Add existing source file to project..." ) );
	const int ADD_NEW_SOURCE = menu.insertItem( tr( "Add new source file to project..." ) );
	int REMOVE_SOURCE = -2;
	if ( i->rtti() == FormListItem::Source ) {
	    menu.insertSeparator();
	    REMOVE_SOURCE = menu.insertItem( tr( "&Remove source file from project" ) );
	}
	int id = menu.exec( QCursor::pos() );

	if ( id == -1 )
	    return;

	if ( id == REMOVE_SOURCE ) {
	    // ####
	} else if ( id == ADD_EXISTING_SOURCE ) {
	    LanguageInterface *iface = MetaDataBase::languageInterface( mainWindow->currProject()->language() );
	    QMap<QString, QString> extensionFilterMap;
	    iface->fileFilters( extensionFilterMap );
	    QMapConstIterator<QString,QString> it = extensionFilterMap.begin();
	    mainWindow->fileOpen( *it, it.key() );
	} else if ( id == ADD_NEW_SOURCE ) {
	    QMap<QString, QString> extensionFilterMap;
	    LanguageInterface *iface = MetaDataBase::languageInterface( mainWindow->currProject()->language() );
	    iface->fileFilters( extensionFilterMap );
	    QMapConstIterator<QString,QString> it = extensionFilterMap.begin();
	    QString fn = QFileDialog::getSaveFileName( QString::null, *it, mainWindow );
	    if ( !fn.isEmpty() ) {
		SourceFile *sf = new SourceFile( fn );
		MetaDataBase::addEntry( sf );
		mainWindow->currProject()->addSourceFile( sf );
		setProject( mainWindow->currProject() );
	    }
	}
    } else if ( i->rtti() == FormListItem::Image || i->text( 0 ) == "Images" && mainWindow->currProject() != mainWindow->emptyProject() ) {
	QPopupMenu menu( this );

	const int EDIT_IMAGES = menu.insertItem( tr( "&Edit pixmap collection..." ) );
	int id = menu.exec( QCursor::pos() );

	if ( id == -1 )
	    return;

	if ( id == EDIT_IMAGES ) {
	    mainWindow->editPixmapCollection();
	}
    }
}

void FormList::formNameChanged( FormWindow *fw )
{
    QListViewItemIterator it( this );
    while ( it.current() ) {
	if ( it.current()->rtti() == FormListItem::Form ) {
	    FormListItem *i = (FormListItem*)it.current();
	    if ( i->formWindow() == fw ) {
		bufferEdit->removeCompletionEntry( i->text( 0 ) );
		i->setText( 0, fw->name() );
		bufferEdit->addCompletionEntry( i->text( 0 ) );
		break;
	    }
	}
	++it;
    }
}

void FormList::setBufferEdit( QCompletionEdit *edit )
{
    bufferEdit = edit;
    connect( bufferEdit, SIGNAL( chosen( const QString & ) ),
	     this, SLOT( bufferChosen( const QString & ) ) );
}

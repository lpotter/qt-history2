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

#include "sourceeditor.h"

#include "formwindow.h"
#include "metadatabase.h"
#include "project.h"
#include "mainwindow.h"
#include "../interfaces/languageinterface.h"
#include <qregexp.h>
#include "project.h"
#include "sourcefile.h"
#include "hierarchyview.h"
#include <qmessagebox.h>
#include <qtextstream.h>

static QString make_func_pretty( const QString &s )
{
    QString res = s;
    if ( res.find( ")" ) - res.find( "(" ) == 1 )
	return res;
    res.replace( QRegExp( "[(]" ), "( " );
    res.replace( QRegExp( "[)]" ), " )" );
    res.replace( QRegExp( "&" ), " &" );
    res.replace( QRegExp( "[*]" ), " *" );
    res.replace( QRegExp( "," ), ", " );
    res.replace( QRegExp( ":" ), " : " );
    res = res.simplifyWhiteSpace();
    return res;
}

SourceEditor::SourceEditor( QWidget *parent, EditorInterface *iface, LanguageInterface *liface )
    : QVBox( parent ), iFace( iface ), lIface( liface ), obj( 0 ), pro( 0 )
{
    iFace->addRef();
    lIface->addRef();
    editor = iFace->editor( this, MainWindow::self->designerInterface() );
    iFace->onBreakPointChange( MainWindow::self, SLOT( breakPointsChanged() ) );
    resize( 600, 400 );
}

SourceEditor::~SourceEditor()
{
    saveBreakPoints();
    iFace->release();
    lIface->release();
    MainWindow::self->editorClosed( this );
    editor = 0;
    if ( obj && obj->inherits( "FormWindow" ) )
	MetaDataBase::setEdited( obj, FALSE );
    if ( obj && obj->inherits( "SourceFile" ) )
	( (SourceFile*)(QObject*)obj )->setEditor( 0 );
}

void SourceEditor::setObject( QObject *o, Project *p )
{
    if ( obj && obj->inherits( "SourceFile" ) )
	( (SourceFile*)(QObject*)obj )->setEditor( 0 );
    if ( obj && obj->inherits( "FormWindow" ) )
	MetaDataBase::setEdited( obj, FALSE );
    if ( o && o->inherits( "FormWindow" ) )
	MetaDataBase::setEdited( o, TRUE );
    save();
    bool changed = FALSE;
    if ( &(*obj) != o ) {
	saveBreakPoints();
	changed = TRUE;
    }
    obj = o;
    pro = p;
    setCaption( tr( "Edit %1" ).arg( ( obj->inherits( "FormWindow" ) ? QString( obj->name() ) : ( (SourceFile*)o )->fileName() ) ) );
    if ( obj->inherits( "SourceFile" ) )
	( (SourceFile*)(QObject*)obj )->setEditor( this );
    iFace->setText( sourceOfObject( obj, lang, iFace, lIface ) );
    if ( pro && o->inherits( "FormWindow" ) )
	iFace->setContext( pro->formList(), ( (FormWindow*)o )->mainContainer() );
    else
	iFace->setContext( pro->formList(), 0 );
    if ( changed || o->inherits( "SourceFile" ) ) // #### ?
	iFace->setBreakPoints( MetaDataBase::breakPoints( o ) );
    MainWindow::self->objectHierarchy()->showClasses( this );
    if ( o->inherits( "FormWindow" ) && lIface->supports( LanguageInterface::StoreFormCodeSeperate ) ) {
	QString fn = MetaDataBase::formSourceFile( o );
	if ( QFile::exists( fn ) )
	    lastTimeStamp = QFileInfo( fn ).lastModified();
    }
}

QString SourceEditor::sourceOfObject( QObject *o, const QString &lang,
				      EditorInterface *, LanguageInterface *lIface )
{
    QString txt;
    if ( o->inherits( "FormWindow" ) ) {
	bool createSource = TRUE;
	bool setSource = FALSE;
	if ( lIface->supports( LanguageInterface::StoreFormCodeSeperate ) ) {
	    createSource = FALSE;
	    txt = MetaDataBase::formCode( o );
	    if ( txt.isEmpty() ) {
		createSource = TRUE;
		setSource = TRUE;
	    }
	}
	if ( createSource ) {
	    QValueList<MetaDataBase::Slot> slotList = MetaDataBase::slotList( o );
	    QMap<QString, QString> bodies = MetaDataBase::functionBodies( o );
	    for ( QValueList<MetaDataBase::Slot>::Iterator it = slotList.begin(); it != slotList.end(); ++it ) {
		if ( (*it).language != lang )
		    continue;
		QString sl( (*it).slot );
		QString comments = MetaDataBase::functionComments( o, sl );
		if ( !comments.isEmpty() )
		    txt += comments + "\n";
		txt += lIface->createFunctionStart( o->name(), make_func_pretty( sl ),
						    ( (*it).returnType.isEmpty() ?
						      QString( "void" ) :
						      (*it).returnType ) );
		QMap<QString, QString>::Iterator bit = bodies.find( MetaDataBase::normalizeSlot( (*it).slot ) );
		if ( bit != bodies.end() )
		    txt += "\n" + *bit + "\n\n";
		else
		    txt += "\n" + lIface->createEmptyFunction() + "\n\n";
	    }
	    if ( setSource )
		MetaDataBase::setFormCode( o, txt );
	}
    } else if ( o->inherits( "SourceFile" ) ) {
	txt = ( (SourceFile*)o )->text();
    }
    return txt;
}

void SourceEditor::setFunction( const QString &func, const QString &clss )
{
    iFace->scrollTo( lIface->createFunctionStart( obj->name(), func, "" ), clss );
}

void SourceEditor::setClass( const QString &clss )
{
    iFace->scrollTo( clss, QString::null );
}

void SourceEditor::closeEvent( QCloseEvent *e )
{
    if ( obj->inherits( "FormWindow" ) ) {
	save();
	MainWindow::self->updateFunctionList();
	emit hidden();
	e->accept();
	if ( obj && obj->inherits( "FormWindow" ) )
	    MetaDataBase::setEdited( obj, FALSE );
    } else {
	if ( !( (SourceFile*)(QObject*)obj )->closeEvent() )
	    e->ignore();
    }
}

void SourceEditor::save()
{
    if ( !obj )
	return;
    if ( obj->inherits( "FormWindow" ) ) {
	QValueList<LanguageInterface::Function> functions;
	QValueList<MetaDataBase::Slot> newSlots, oldSlots;
	oldSlots = MetaDataBase::slotList( obj );
	lIface->functions( iFace->text(), &functions );
	QMap<QString, QString> funcs;
	for ( QValueList<LanguageInterface::Function>::Iterator it = functions.begin();
	      it != functions.end(); ++it ) {
	    bool found = FALSE;
	    for ( QValueList<MetaDataBase::Slot>::Iterator sit = oldSlots.begin(); sit != oldSlots.end(); ++sit ) {
		QString s( (*sit).slot );
		if ( MetaDataBase::normalizeSlot( s ) == MetaDataBase::normalizeSlot( (*it).name ) ) {
		    found = TRUE;
		    MetaDataBase::Slot slot;
		    slot.slot = make_func_pretty( (*it).name );
		    slot.specifier = (*sit).specifier;
		    slot.access = (*sit).access;
		    slot.language = (*sit).language;
		    slot.returnType = (*it).returnType;
		    newSlots << slot;
		    funcs.insert( (*it).name, (*it).body );
		    oldSlots.remove( sit );
		    break;
		}
	    }
	    if ( !found ) {
		MetaDataBase::Slot slot;
		slot.slot = make_func_pretty( (*it).name );
		slot.specifier = "virtual";
		slot.access = "public";
		slot.language = lang;
		slot.returnType = (*it).returnType;
		newSlots << slot;
		funcs.insert( (*it).name, (*it).body );
	    }
	    MetaDataBase::setFunctionComments( obj, (*it).name, (*it).comments );
	}

	MetaDataBase::setSlotList( obj, newSlots );
	MetaDataBase::setFunctionBodies( obj, funcs, lang, QString::null );
	if ( lIface->supports( LanguageInterface::StoreFormCodeSeperate ) )
	    MetaDataBase::setFormCode( obj, iFace->text() );
    } else if ( obj->inherits( "SourceFile" ) ) {
	( (SourceFile*)(QObject*)obj )->setText( iFace->text() );
    }
}

QString SourceEditor::language() const
{
    return lang;
}

void SourceEditor::setLanguage( const QString &l )
{
    lang = l;
}

void SourceEditor::editCut()
{
    iFace->cut();
}

void SourceEditor::editCopy()
{
    iFace->copy();
}

void SourceEditor::editPaste()
{
    iFace->paste();
}

void SourceEditor::editUndo()
{
    iFace->undo();
}

void SourceEditor::editRedo()
{
    iFace->redo();
}

void SourceEditor::editSelectAll()
{
    iFace->selectAll();
}

void SourceEditor::configChanged()
{
    iFace->readSettings();
}

void SourceEditor::setModified( bool b )
{
    iFace->setModified( b );
}

void SourceEditor::refresh( bool allowSave )
{
    if ( allowSave )
	save();
    iFace->setText( sourceOfObject( obj, lang, iFace, lIface ) );
}

void SourceEditor::resetContext()
{
    if ( pro && obj && obj->inherits( "FormWindow" ) )
	iFace->setContext( pro->formList(), ( (FormWindow*)(QObject*)obj )->mainContainer() );
    else
	iFace->setContext( pro->formList(), 0 );
}

void SourceEditor::setFocus()
{
    if ( obj && obj->inherits( "FormWindow" ) )
	MetaDataBase::setEdited( obj, TRUE );
    if ( editor )
	editor->setFocus();
}

int SourceEditor::numLines() const
{
    return iFace->numLines();
}

void SourceEditor::saveBreakPoints()
{
    if ( !obj )
	return;
    QValueList<int> l;
    iFace->breakPoints( l );
    MetaDataBase::setBreakPoints( obj, l );
}

void SourceEditor::clearStep()
{
    iFace->clearStep();
}

void SourceEditor::clearStackFrame()
{
    iFace->clearStackFrame();
}

void SourceEditor::resetBreakPoints()
{
    iFace->setBreakPoints( MetaDataBase::breakPoints( obj ) );
}

QString SourceEditor::text() const
{
    return iFace->text();
}

bool SourceEditor::isModified() const
{
    return iFace->isModified();
}

void SourceEditor::checkTimeStamp()
{
    if ( obj && obj->inherits( "FormWindow" ) &&
	 lIface->supports( LanguageInterface::StoreFormCodeSeperate ) ) {
	QString fn = MetaDataBase::formSourceFile( obj );
	if ( QFile::exists( fn ) ) {
	    if ( lastTimeStamp != QFileInfo( fn ).lastModified() ) {
		lastTimeStamp = QFileInfo( fn ).lastModified();
		if ( QMessageBox::information( this, tr( "Qt Designer" ),
					       tr( "The file %1 has been changed outside Qt Designer.\n"
						   "Do you want to reload it?" ).arg( fn ),
					       tr( "&Yes" ), tr( "&No" ) ) == 0 ) {
		    QFile f( fn );
		    if ( f.open( IO_ReadOnly ) ) {
			QTextStream ts( &f );
			iFace->setText( ts.read() );
			save();
			MainWindow::self->slotsChanged();
		    }
		}
	    }
	}
    } else if ( obj && obj->inherits( "SourceFile" ) ) {
	( (SourceFile*)(QObject*)obj )->checkTimeStamp();
    }
}

void SourceEditor::updateTimeStamp()
{
    if ( obj && obj->inherits( "FormWindow" ) &&
	 lIface->supports( LanguageInterface::StoreFormCodeSeperate ) ) {
	QString fn = MetaDataBase::formSourceFile( obj );
	lastTimeStamp = QFileInfo( fn ).lastModified();
    }
}

void SourceEditor::emitHidden()
{
    emit hidden();
}

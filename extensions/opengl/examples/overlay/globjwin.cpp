/****************************************************************************
** $Id: //depot/qt/main/extensions/opengl/examples/overlay/globjwin.cpp#1 $
**
** Implementation of GLObjectWindow widget class
**
****************************************************************************/


#include <qlayout.h>
#include <qframe.h>
#include <qmenubar.h>
#include <qpopupmenu.h>
#include <qapplication.h>
#include <qmessagebox.h>
#include "globjwin.h"
#include "glteapots.h"


GLObjectWindow::GLObjectWindow( QWidget* parent, const char* name )
    : QWidget( parent, name )
{
    // Create a menu
    QPopupMenu *file = new QPopupMenu( this );
    file->insertItem( "Exit",  qApp, SLOT(quit()), CTRL+Key_Q );

    // Create a menu bar
    QMenuBar *m = new QMenuBar( this );
    m->setSeparator( QMenuBar::InWindowsStyle );
    m->insertItem("&File", file );

    // Create a nice frame to put around the OpenGL widget
    QFrame* f = new QFrame( this, "frame" );
    f->setFrameStyle( QFrame::Sunken | QFrame::Panel );
    f->setLineWidth( 2 );

    // Create our OpenGL widget.
    GLTeapots* c = new GLTeapots( f, "glteapots" );

    // Check if we obtained an overlay
    if ( !c->format().hasOverlay() ) {
	QMessageBox::warning( 0, qApp->argv()[0], 
			      "Failed to get an OpenGL overlay",
			      "Ok" );
    }

    // Now that we have all the widgets, put them into a nice layout

    // Put the GL widget inside the frame
    QHBoxLayout* flayout = new QHBoxLayout( f, 2, 2, "flayout");
    flayout->addWidget( c, 1 );

    // Top level layout
    QVBoxLayout* hlayout = new QVBoxLayout( this, 20, 20, "hlayout");
    hlayout->setMenuBar( m );
    hlayout->addWidget( f, 1 );
}

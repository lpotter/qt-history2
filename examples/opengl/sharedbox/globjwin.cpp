/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <qpushbutton.h>
#include <qslider.h>
#include <qlayout.h>
#include <qframe.h>
#include <qmenubar.h>
#include <qpopupmenu.h>
#include <qapplication.h>
#include "globjwin.h"
#include "glbox.h"


GLObjectWindow::GLObjectWindow( QWidget* parent, const char* name )
    : QWidget( parent, name )
{
    // Create a menu
    QMenu *file = new QMenu( this );
    file->addAction( "Delete Left QGLWidget", this, SLOT(deleteFirstWidget()) );
    file->addAction( "Exit",  qApp, SLOT(quit())/*, Qt::CTRL+Qt::Key_Q*/ );

    // Create a menu bar
    QMenuBar *m = new QMenuBar( this );
    m->addMenu("&File", file );

    // Create nice frames to put around the OpenGL widgets
    QFrame* f1 = new QFrame( this, "frame1" );
    f1->setFrameStyle( QFrame::Sunken | QFrame::Panel );
    f1->setLineWidth( 2 );
    QFrame* f2 = new QFrame( this, "frame2" );
    f2->setFrameStyle( QFrame::Sunken | QFrame::Panel );
    f2->setLineWidth( 2 );

    // Create an OpenGL widget
    c1 = new GLBox( f1, "glbox1" );

    // Create another OpenGL widget that shares display lists with the first
    c2 = new GLBox( f2, "glbox2", c1 );

    // Create the three sliders; one for each rotation axis
    // Make them spin the boxes, but not in synch
    QSlider* x = new QSlider (this);
    x->setObjectName("xsl");
    x->setOrientation(Qt::Vertical);
    x->setMinimum(0);
    x->setMaximum(360);
    x->setPageStep(60);
    x->setValue(0);
    x->setTickmarks( QSlider::Left );
    connect( x, SIGNAL(valueChanged(int)), c1, SLOT(setXRotation(int)) );
    connect( x, SIGNAL(valueChanged(int)), c2, SLOT(setZRotation(int)) );

    QSlider* y = new QSlider (this);
    y->setObjectName("ysl");
    y->setOrientation(Qt::Vertical);
    y->setMinimum(0);
    y->setMaximum(360);
    y->setPageStep(60);
    y->setValue(0);
    y->setTickmarks( QSlider::Left );
    connect( y, SIGNAL(valueChanged(int)), c1, SLOT(setYRotation(int)) );
    connect( y, SIGNAL(valueChanged(int)), c2, SLOT(setXRotation(int)) );

    QSlider* z = new QSlider (this);
    z->setObjectName("zsl");
    z->setOrientation(Qt::Vertical);
    z->setMinimum(0);
    z->setMaximum(360);
    z->setPageStep(60);
    z->setValue(0);
    z->setTickmarks( QSlider::Left );
    connect( z, SIGNAL(valueChanged(int)), c1, SLOT(setZRotation(int)) );
    connect( z, SIGNAL(valueChanged(int)), c2, SLOT(setYRotation(int)) );


    // Now that we have all the widgets, put them into a nice layout

    // Put the sliders on top of each other
    QVBoxLayout* vlayout = new QVBoxLayout;
    vlayout->setMargin( 20 );
    vlayout->addWidget( x );
    vlayout->addWidget( y );
    vlayout->addWidget( z );

    // Put the GL widgets inside the frames
    QHBoxLayout* flayout1 = new QHBoxLayout( f1 );
    flayout1->setMargin( 2 );
    flayout1->setSpacing( 2 );
    flayout1->addWidget( c1, 1 );
    QHBoxLayout* flayout2 = new QHBoxLayout( f2, 2, 2, "flayout2");
    flayout2->setMargin( 2 );
    flayout2->setSpacing( 2 );
    flayout2->addWidget( c2, 1 );

    // Top level layout, puts the sliders to the left of the frame/GL widget
    QHBoxLayout* hlayout = new QHBoxLayout( this );
    hlayout->setMargin( 20 );
    hlayout->setSpacing( 20 );
    hlayout->setMenuBar( m );
    hlayout->addLayout( vlayout );
    hlayout->addWidget( f1, 1 );
    hlayout->addWidget( f2, 1 );

}


void GLObjectWindow::deleteFirstWidget()
{
    // Delete only c1; c2 will keep working and use the shared display list
    if ( c1 ) {
	delete c1;
	c1 = 0;
    }
}

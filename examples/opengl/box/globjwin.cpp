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
#include <qmenu.h>
#include <qapplication.h>
#include "globjwin.h"
#include "glbox.h"


GLObjectWindow::GLObjectWindow( QWidget* parent )
    : QWidget( parent )
{

    // Create a menu
    QMenu *file = new QMenu( this );
    file->addAction( "Exit",  qApp, SLOT(quit())/*, Qt::CTRL+Qt::Key_Q*/ );

    // Create a menu bar
    QMenuBar *m = new QMenuBar( this );
    m->addMenu("&File", file );

    // Create a nice frame to put around the OpenGL widget
    QFrame* f = new QFrame( this );
    f->setFrameStyle( QFrame::Sunken | QFrame::Panel );
    f->setLineWidth( 2 );

    // Create our OpenGL widget
    GLBox* c = new GLBox( f, "glbox");

    // Create the three sliders; one for each rotation axis
    QSlider* x = new QSlider (this);
    x->setObjectName("xsl");
    x->setOrientation(Qt::Vertical);
    x->setMinimum(0);
    x->setMaximum(360);
    x->setPageStep(60);
    x->setValue(0);
    x->setTickmarks( QSlider::Left );
    connect( x, SIGNAL(valueChanged(int)), c, SLOT(setXRotation(int)) );

    QSlider* y = new QSlider (this);
    y->setObjectName("ysl");
    y->setOrientation(Qt::Vertical);
    y->setMinimum(0);
    y->setMaximum(360);
    y->setPageStep(60);
    y->setValue(0);
    y->setTickmarks( QSlider::Left );
    connect( y, SIGNAL(valueChanged(int)), c, SLOT(setYRotation(int)) );

    QSlider* z = new QSlider (this);
    z->setObjectName("zsl");
    z->setOrientation(Qt::Vertical);
    z->setMinimum(0);
    z->setMaximum(360);
    z->setPageStep(60);
    z->setValue(0);
    z->setTickmarks( QSlider::Left );
    connect( z, SIGNAL(valueChanged(int)), c, SLOT(setZRotation(int)) );

    // Now that we have all the widgets, put them into a nice layout

    // Put the sliders on top of each other
    QVBoxLayout* vlayout = new QVBoxLayout;
    vlayout->setMargin( 20 );
    vlayout->addWidget( x );
    vlayout->addWidget( y );
    vlayout->addWidget( z );

    // Put the GL widget inside the frame
    QHBoxLayout* flayout = new QHBoxLayout( f );
    flayout->setMargin( 2 );
    flayout->setSpacing( 2 );
    flayout->addWidget( c, 1 );

    // Top level layout, puts the sliders to the left of the frame/GL widget
    QHBoxLayout* hlayout = new QHBoxLayout( this );
    hlayout->setMargin( 20 );
    hlayout->setSpacing( 20 );
    hlayout->setMenuBar( m );
    hlayout->addLayout( vlayout );
    hlayout->addWidget( f, 1 );
}

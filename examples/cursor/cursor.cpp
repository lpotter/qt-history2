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

#include <qlabel.h>
#include <qbitmap.h>
#include <qapplication.h>
#include <qlayout.h>
#include <qcursor.h>

// cb_bits and cm_bits were generated by X bitmap program.

#define cb_width  32
#define cb_height 32

static unsigned char cb_bits[] = {		// cursor bitmap
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x0f, 0x00,
   0x00, 0x06, 0x30, 0x00, 0x80, 0x01, 0xc0, 0x00, 0x40, 0x00, 0x00, 0x01,
   0x20, 0x00, 0x00, 0x02, 0x10, 0x00, 0x00, 0x04, 0x08, 0x3e, 0x3e, 0x08,
   0x08, 0x03, 0xe0, 0x08, 0xc4, 0x00, 0x00, 0x11, 0x04, 0x1e, 0x78, 0x10,
   0x02, 0x0c, 0x30, 0x20, 0x02, 0x40, 0x00, 0x20, 0x02, 0x40, 0x00, 0x20,
   0x02, 0x40, 0x00, 0x20, 0x02, 0x20, 0x04, 0x20, 0x02, 0x20, 0x04, 0x20,
   0x02, 0x10, 0x08, 0x20, 0x02, 0x08, 0x08, 0x20, 0x02, 0xf0, 0x07, 0x20,
   0x04, 0x00, 0x00, 0x10, 0x04, 0x00, 0x00, 0x10, 0x08, 0x00, 0xc0, 0x08,
   0x08, 0x3c, 0x30, 0x08, 0x10, 0xe6, 0x19, 0x04, 0x20, 0x00, 0x0f, 0x02,
   0x40, 0x00, 0x00, 0x01, 0x80, 0x01, 0xc0, 0x00, 0x00, 0x06, 0x30, 0x00,
   0x00, 0xf8, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00};

#define cm_width  32
#define cm_height 32

static unsigned char cm_bits[] = {		// cursor bitmap mask
   0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x1f, 0x00, 0x00, 0xfe, 0x3f, 0x00,
   0x80, 0x07, 0xf0, 0x00, 0xc0, 0x01, 0xc0, 0x01, 0x60, 0x00, 0x00, 0x03,
   0x30, 0x00, 0x00, 0x06, 0x18, 0x00, 0x00, 0x0c, 0x0c, 0x3e, 0x3e, 0x18,
   0x0e, 0x03, 0xe0, 0x18, 0xc6, 0x00, 0x00, 0x31, 0x07, 0x1e, 0x78, 0x30,
   0x03, 0x0c, 0x30, 0x60, 0x03, 0x40, 0x00, 0x60, 0x03, 0x40, 0x00, 0x60,
   0x03, 0x40, 0x00, 0x60, 0x03, 0x20, 0x04, 0x60, 0x03, 0x20, 0x04, 0x60,
   0x03, 0x10, 0x08, 0x60, 0x03, 0x08, 0x08, 0x60, 0x03, 0xf0, 0x07, 0x60,
   0x06, 0x00, 0x00, 0x30, 0x06, 0x00, 0x00, 0x30, 0x0c, 0x00, 0xc0, 0x18,
   0x0c, 0x3c, 0x30, 0x18, 0x18, 0xe6, 0x19, 0x0c, 0x30, 0x00, 0x0f, 0x06,
   0x60, 0x00, 0x00, 0x03, 0xc0, 0x01, 0xc0, 0x01, 0x80, 0x07, 0xf0, 0x00,
   0x00, 0xfe, 0x3f, 0x00, 0x00, 0xf8, 0x0f, 0x00};


//
// The CursorView contains many labels with different cursors.
//

class CursorView : public QWidget		// cursor view
{
public:
    CursorView();
};

//
// Constructs a cursor view.
//

CursorView::CursorView()			// construct view
{
    struct List {
	CursorShape	shape;
	const char*	name;			// cursor name
    };
    static List list[] = {
	{ ArrowCursor,		"arrowCursor" },
	{ UpArrowCursor,	"upArrowCursor" },
	{ CrossCursor,		"crossCursor" },
	{ WaitCursor,		"waitCursor" },
	{ IbeamCursor,		"ibeamCursor" },
	{ SizeVerCursor,	"sizeVerCursor" },
	{ SizeHorCursor,	"sizeHorCursor" },
	{ SizeBDiagCursor,	"sizeBDiagCursor" },
	{ SizeFDiagCursor,	"sizeFDiagCursor" },
	{ SizeAllCursor,	"sizeAllCursor" },
	{ BlankCursor,		"blankCursor" },
	{ SplitVCursor,		"splitVCursor" },
	{ SplitHCursor,		"splitHCursor" },
	{ PointingHandCursor,	"pointingHandCursor" },
	{ ForbiddenCursor,	"forbiddenCursor" },
	{ WhatsThisCursor,	"whatsThisCursor" },
	{ BusyCursor,		"busyCursor" }
    };

    setWindowTitle( "CursorView" );			// set window caption

    QGridLayout* grid = new QGridLayout( this, 5, 4, 20 );
    QLabel *label;

    int i=0;
    for ( int y=0; y<4; y++ ) {			// create the small labels
	for ( int x=0; x<4; x++ ) {
	    label = new QLabel( this );
	    label->setCursor( QCursor( list[i].shape ) );
	    label->setText( list[i].name );
	    label->setAlignment( AlignCenter );
	    label->setMargin( 10 );
	    label->setFrameStyle( QFrame::Box | QFrame::Raised );
	    grid->addWidget( label, x, y );
	    i++;
	}
    }


    label = new QLabel( this );
    label->setCursor( QCursor( list[i].shape ) );
    label->setText( list[i].name );
    label->setAlignment( AlignCenter );
    label->setMargin( 10 );
    label->setFrameStyle( QFrame::Box | QFrame::Raised );
    grid->addWidget( label, 4, 0 );


    
    QBitmap cb( cb_width, cb_height, cb_bits, TRUE );
    QBitmap cm( cm_width, cm_height, cm_bits, TRUE );
    QCursor custom( cb, cm );			// create bitmap cursor

    label = new QLabel( this );			// create the big label
    label->setCursor( custom );
    label->setText( "Custom bitmap cursor" );
    label->setAlignment( AlignCenter );
    label->setMargin( 10 );
    label->setFrameStyle( QFrame::Box | QFrame::Sunken );
    grid->addWidget(label, 4, 1, 1,  3);

}


//
// Create and display a CursorView.
//

int main( int argc, char **argv )
{
    QApplication a( argc, argv );		// application object
    CursorView   v;				// cursor view
    a.setMainWidget( &v );
    v.setWindowTitle("Qt Example - Cursors");
    v.show();
    return a.exec();
}

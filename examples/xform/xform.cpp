/****************************************************************************
** $Id: //depot/qt/main/examples/xform/xform.cpp#5 $
**
** Copyright (C) 1992-1999 Troll Tech AS.  All rights reserved.
**
** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.
**
*****************************************************************************/

#include <qapplication.h>

#include <qdialog.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qlcdnumber.h>
#include <qslider.h>
#include <qmenubar.h>
#include <qfontdialog.h>

#include <qpainter.h>
#include <qpixmap.h>
#include <qpicture.h>

#include <stdlib.h>


class ModeNames {
public:
    enum Mode { Text, Image, Picture };
};


class XFormControl : public QFrame, public ModeNames
{
    Q_OBJECT
public:
    XFormControl( QWidget *parent=0, const char *name=0 );
   ~XFormControl() {}

    QWMatrix matrix();

signals:
    void newMatrix( QWMatrix );
    void newText( const QString& );
    void newFont( const QFont & );
    void newMode( int );
private slots:
    void newMtx();
    void newTxt(const QString&);
    void selectFont();
    void fontSelected( const QFont & );
    void changeMode(int);
    void timerEvent(QTimerEvent*);
private:
    Mode mode;
    QSlider	 *rotS;		       // Rotation angle scroll bar
    QSlider	 *shearS;	       // Shear value scroll bar
    QSlider	 *magS;		       // Magnification value scroll bar
    QLCDNumber	 *rotLCD;	       // Rotation angle LCD display
    QLCDNumber	 *shearLCD;	       // Shear value LCD display
    QLCDNumber	 *magLCD;	       // Magnification value LCD display
    QCheckBox	 *mirror;	       // Checkbox for mirror image on/of
    QLineEdit	 *textEd;	       // Inp[ut field for xForm text
    QPushButton  *f;		       // Select font push button
    QRadioButton *rb_txt;	       // Radio button for text
    QRadioButton *rb_img;	       // Radio button for image
    QRadioButton *rb_pic;	       // Radio button for picture
};

/*
  ShowXForm displays a text or a pixmap (QPixmap) using a coordinate
  transformation matrix (QWMatrix)
*/

class ShowXForm : public QWidget, public ModeNames
{
    Q_OBJECT
public:
    ShowXForm( QWidget *parent=0, const char *name=0 );
   ~ShowXForm() {}
    void showIt();			// (Re)displays text or pixmap

    Mode mode() const { return m; }
public slots:
    void setText( const QString& );
    void setMatrix( QWMatrix );
    void setFont( const QFont &f );
    void setPixmap( QPixmap );
    void setPicture( const QPicture& );
    void setMode( int );
private:
    void paintEvent( QPaintEvent * );
    void resizeEvent( QResizeEvent * );
    QWMatrix  mtx;			// coordinate transform matrix
    QString   text;			// text to be displayed
    QPixmap   pix;			// pixmap to be displayed
    QPicture  picture;			// text to be displayed
    QRect     eraseRect;		// covers last displayed text/pixmap
    Mode      m;
};

XFormControl::XFormControl( QWidget *parent, const char *name )
	: QFrame( parent, name )
{
    mode = Image;

    rotLCD	= new QLCDNumber( 4, this, "rotateLCD" );
    rotS	= new QSlider( QSlider::Horizontal, this,
				  "rotateSlider" );
    shearLCD	= new QLCDNumber( 5,this, "shearLCD" );
    shearS	= new QSlider( QSlider::Horizontal, this,
				  "shearSlider" );
    mirror	= new QCheckBox( this, "mirrorCheckBox" );
    textEd	= new QLineEdit( this, "text" );
    textEd->setFocus();
    f		= new QPushButton( this, "text" );
    rb_txt = new QRadioButton( this, "text" );
    rb_img = new QRadioButton( this, "image" );
    rb_pic = new QRadioButton( this, "picture" );

    rotLCD->setGeometry( 10, 10, 100, 60 );
    rotLCD->display( "  0'" );

    rotS->setGeometry( 10, 80, 100, 15 );
    rotS->setRange( -180, 180 );
    rotS->setValue( 0 );
    connect( rotS, SIGNAL(valueChanged(int)), SLOT(newMtx()) );

    shearLCD->setGeometry( 10, 105, 100, 60 );
    shearLCD->display( "0.00" );

    shearS->setGeometry( 10, 175, 100, 15 );
    shearS->setRange( -25, 25 );
    shearS->setValue( 0 );
    connect( shearS, SIGNAL(valueChanged(int)), SLOT(newMtx()) );

    mirror->setGeometry( 10, 200, 100, 15 );
    mirror->setText( tr("Mirror") );
    connect( mirror, SIGNAL(clicked()), SLOT(newMtx()) );

    QButtonGroup *bg = new QButtonGroup(this);
    bg->hide();
    bg->insert(rb_txt,0);
    bg->insert(rb_img,1);
    bg->insert(rb_pic,2);
    rb_txt->setGeometry( 10, 220, 100, 15 );
    rb_txt->setText( tr("Text") );
    rb_img->setGeometry( 10, 235, 100, 15 );
    rb_img->setText( tr("Image") );
    rb_img->setChecked(TRUE);
    rb_pic->setGeometry( 10, 250, 100, 15 );
    rb_pic->setText( tr("Picture") );
    connect( bg, SIGNAL(clicked(int)), SLOT(changeMode(int)) );

    textEd->setGeometry( 10, 275, 100, 20 );
    textEd->setText( "Troll" );
    connect( textEd, SIGNAL(textChanged(const QString&)),
		     SLOT(newTxt(const QString&)) );

    f->setGeometry( 10, 305, 100, 20 );
    f->setText( tr("Select font...") );
    connect( f, SIGNAL(clicked()), SLOT(selectFont()) );

    magS = new QSlider( QSlider::Horizontal, this,
			   "magnifySlider" );
    magS->setGeometry( 10, 375, 100, 15 );
    magS->setRange( 0, 800 );
    connect( magS, SIGNAL(valueChanged(int)), SLOT(newMtx()) );
    magS->setValue( 0 );
    magLCD = new QLCDNumber( 4,this, "shearLCD" );
    magLCD->setGeometry( 10, 305, 100, 60 );
    magLCD->display( "100" );
    connect( magS, SIGNAL(valueChanged(int)), magLCD, SLOT(display(int)));

    changeMode(Image);

    startTimer(20); // start an initial animation
}

void XFormControl::timerEvent(QTimerEvent*)
{
    int v = magS->value();
    v = (v+2)+v/10;
    if ( v >= 200 ) {
	v = 200;
	killTimers();
    }
    magS->setValue(v);
}



/*
    Called whenever the user has changed one of the matrix parameters
    (i.e. rotate, shear or magnification)
*/
void XFormControl::newMtx()
{
    emit newMatrix( matrix() );
}

void XFormControl::newTxt(const QString& s)
{
    emit newText(s);
    changeMode(Text);
}

/*
    Calculates the matrix appropriate for the current controls,
    and updates the displays.
*/
QWMatrix XFormControl::matrix()
{
    QWMatrix m;
    if (mode != Text) {
	double magVal = 1.0*magS->value()/100;
	m.scale( magVal, magVal );
    }
    double shearVal = 1.0*shearS->value()/25;
    m.shear( shearVal, shearVal );
    m.rotate( rotS->value() );
    if ( mirror->isChecked() ) {
	m.scale( 1, -1 );
	m.rotate( 180 );
    }

    QString tmp;
    tmp.sprintf( "%1.2f", shearVal  );
    if ( shearVal >= 0 )
	tmp.insert( 0, " " );
    shearLCD->display( tmp );

    int rot = rotS->value();
    if ( rot < 0 )
	rot = rot + 360;
    tmp.sprintf( "%3i'", rot );
    rotLCD->display( tmp );
    return m;
}


void XFormControl::selectFont()
{
    bool ok;
    QFont f = QFontDialog::getFont( &ok );
    if ( ok )
	fontSelected( f );
}

void XFormControl::fontSelected( const QFont &font )
{
    emit newFont( font );
    changeMode(Text);
}

/*
    Sets the mode - Text, Image, or Picture.
*/

void XFormControl::changeMode(int m)
{
    mode = (Mode)m;

    emit newMode( m );
    newMtx();
    if ( mode == Text ) {
	magS->hide();
	magLCD->hide();
	f->show();
	rb_txt->setChecked(TRUE);
    } else {
	magS->show();
	magLCD->show();
	f->hide();
	if ( mode == Image )
	    rb_img->setChecked(TRUE);
	else
	    rb_pic->setChecked(TRUE);
    }
    qApp->flushX();
}

const int yOff = 35;

ShowXForm::ShowXForm( QWidget *parent, const char *name )
	: QWidget( parent, name, WResizeNoErase )
{
    setFont( QFont( "Charter", 48, QFont::Bold ) );
    setBackgroundColor( white );
    m = Text;
    eraseRect = QRect( 0, 0, 0, 0 );
}

void ShowXForm::paintEvent( QPaintEvent * )
{
    showIt();
}

void ShowXForm::resizeEvent( QResizeEvent * )
{
    eraseRect = QRect( width()/2, height()/2, 0, 0 );
    repaint(rect());
}

void ShowXForm::setText( const QString& s )
{
    text = s;
    showIt();
}

void ShowXForm::setMatrix( QWMatrix w )
{
    mtx = w;
    showIt();
}

void ShowXForm::setFont( const QFont &f )
{
    m = Text;
    QWidget::setFont( f );
}

void ShowXForm::setPixmap( QPixmap pm )
{
    pix	 = pm;
    m    = Image;
    showIt();
}

void ShowXForm::setPicture( const QPicture& p )
{
    picture = p;
    m = Picture;
    showIt();
}

void ShowXForm::setMode( int mode )
{
    m = (Mode)mode;
}

void ShowXForm::showIt()
{
    QPainter p;
    QRect r;	  // rectangle covering new text/pixmap in virtual coordinates
    QWMatrix m;	  // copy user specified transform
    int textYPos = 0; // distance from boundingRect y pos to baseline
    int textXPos = 0; // distance from boundingRect x pos to text start
    QRect br;
    QFontMetrics fm( fontMetrics() );	// get widget font metrics
    switch ( mode() ) {
      case Text:
	br = fm.boundingRect( text );	// rectangle covering text
	r  = br;
	textYPos = -r.y();
	textXPos = -r.x();
	br.moveTopLeft( QPoint( -br.width()/2, -br.height()/2 ) );
        break;
      case Image:
	r = pix.rect();
        break;
      case Picture:
	// ### need QPicture::boundingRect()
	r = QRect(0,0,1000,1000);
        break;
    }
    r.moveTopLeft( QPoint(-r.width()/2, -r.height()/2) );
	  // compute union of new and old rect
	  // the resulting rectangle will cover what is already displayed
	  // and have room for the new text/pixmap
    eraseRect = eraseRect.unite( mtx.map(r) );
    eraseRect.moveBy( -1, -1 ); // add border for matrix round off
    eraseRect.setSize( QSize( eraseRect.width() + 2,eraseRect.height() + 2 ) );
    int pw = QMIN(eraseRect.width(),width());
    int ph = QMIN(eraseRect.height(),height());
    QPixmap pm( pw, ph );		// off-screen drawing pixmap
    pm.fill( backgroundColor() );

    p.begin( &pm );
    m.translate( pw/2, ph/2 );	// 0,0 is center
    m = mtx * m;
    p.setWorldMatrix( m );
    switch ( mode() ) {
      case Text:
	p.setFont( font() );		// use widget font
	p.drawText( r.left() + textXPos, r.top() + textYPos, text );
#if 0
	p.setPen( red );
	p.drawRect( br );
#endif
	break;
      case Image:
	p.drawPixmap( -pix.width()/2, -pix.height()/2, pix );
	//QPixmap rotated = pix.xForm(mtx);
	//bitBlt( &pm, pm.width()/2 - rotated.width()/2,
		//pm.height()/2 - rotated.height()/2, &rotated );
	break;
      case Picture:
	// ### need QPicture::boundingRect()
	p.scale(0.25,0.25);
	p.translate(-230,-180);
	p.drawPicture( picture );
    }
    p.end();

    int xpos = width()/2  - pw/2;
    int ypos = height()/2 - ph/2;
    bitBlt( this, xpos, ypos,			// copy pixmap to widget
	    &pm, 0, 0, -1, -1 );
    eraseRect =	 mtx.map( r );
}


/*
    Grand unifying widget, putting ShowXForm and XFormControl
    together.
*/

class XFormCenter : public QWidget, public ModeNames
{
    Q_OBJECT
public:
    XFormCenter( QWidget *parent=0, const char *name=0 );
public slots:
    void setFont( const QFont &f ) { sx->setFont( f ); }
    void newMode( int );
private:
    void resizeEvent( QResizeEvent* );
    ShowXForm	*sx;
    XFormControl *xc;
};

void XFormCenter::resizeEvent( QResizeEvent* )
{
    sx->resize( width() - 120, height() );
    xc->resize( 120, height() );
}

void XFormCenter::newMode( int m )
{
    static bool first_i = TRUE;
    static bool first_p = TRUE;

    if ( sx->mode() == m )
	return;
    if ( m == Image && first_i ) {
	first_i = FALSE;
	QPixmap pm;
	if ( pm.load( "image.any" ) )
	    sx->setPixmap( pm );
	return;
    }
    if ( m == Picture && first_p ) {
	first_p = FALSE;
	QPicture p;
	if (p.load( "picture.any" ))
	    sx->setPicture( p );
	return;
    }
    sx->setMode(m);
}

XFormCenter::XFormCenter( QWidget *parent, const char *name )
    : QWidget( parent, name )
{
    sx = new ShowXForm(this);
    sx->move( 120, 0 );		    // the size is set by resizeEvent
    xc = new XFormControl(this);
    xc->move( 0, 0 );		    // the size is set by resizeEvent
    xc->setFrameStyle( QFrame::Box | QFrame::Sunken );
    xc->setLineWidth( 2 );
    connect( xc, SIGNAL(newText(const QString&)), sx,
		 SLOT(setText(const QString&)) );
    connect( xc, SIGNAL(newMatrix(QWMatrix)),
	     sx, SLOT(setMatrix(QWMatrix)) );
    connect( xc, SIGNAL(newFont(const QFont&)), sx,
		 SLOT(setFont(const QFont&)) );
    connect( xc, SIGNAL(newMode(int)), SLOT(newMode(int)) );
    sx->setText( "Troll" );
    newMode( Image );
    sx->setMatrix(xc->matrix());
}


int main( int argc, char **argv )
{
    QApplication a( argc, argv );
#ifdef _WS_WIN_
    qWarning( "QTranslator not yet implemented for Windows" );
#else    
    QTranslator mf(0);
    mf.load("xform.tr",".");
    a.installTranslator(&mf);
#endif
#if 0
    QColor x( 0xaa, 0xbe, 0xff );	// kind of blue
    QColorGroup g( black, x, x.light(), x.dark(), x.dark(120),	black, white );
    QPalette p( g, g, g );
    a.setPalette( p );
#endif

    XFormCenter *xfc = new XFormCenter;
    xfc->setGeometry( 0, 0, 500, 400 );

    a.setMainWidget( xfc );
    xfc->show();
    return a.exec();
}

#include "xform.moc"		      // include metadata generated by the moc

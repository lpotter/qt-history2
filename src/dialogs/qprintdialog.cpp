/****************************************************************************
** $Id: //depot/qt/main/src/dialogs/qprintdialog.cpp#82 $
**
** Implementation of internal print dialog (X11) used by QPrinter::select().
**
** Created : 950829
**
** Copyright (C) 1992-2000 Troll Tech AS.  All rights reserved.
**
** This file is part of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Troll Tech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** Licensees holding valid Qt Professional Edition licenses may use this
** file in accordance with the Qt Professional Edition License Agreement
** provided with the Qt Professional Edition.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
** information about the Professional Edition licensing, or see
** http://www.trolltech.com/qpl/ for QPL licensing information.
**
*****************************************************************************/

#include "qprintdialog.h"

#ifndef QT_NO_PRINTDIALOG

#include "qfiledialog.h"

#include "qfile.h"
#include "qtextstream.h"

#include "qcombobox.h"
#include "qframe.h"
#include "qlabel.h"
#include "qlineedit.h"
#include "qpushbutton.h"
#include "qprinter.h"
#include "qlistview.h"
#include "qlayout.h"
#include "qbuttongroup.h"
#include "qradiobutton.h"
#include "qspinbox.h"
#include "qapplication.h"
#include "qheader.h"
#include "qcombobox.h"

#include "qstring.h"
#include "qregexp.h"

#include <ctype.h>
#include <stdlib.h>

// REVISED: warwick

struct QPrintDialogPrivate
{
    QPrinter * printer;

    QButtonGroup * printerOrFile;

    bool outputToFile;
    QListView * printers;
    QLineEdit * fileName;
    QPushButton * browse, *ok;

    QButtonGroup * printRange;
    QLabel * firstPageLabel;
    QSpinBox * firstPage;
    QLabel * lastPageLabel;
    QSpinBox * lastPage;
    QRadioButton * printAllButton;
    QRadioButton * printRangeButton;
    QComboBox *orientationCombo, *sizeCombo;

    QPrinter::PageSize pageSize;
    QPrinter::Orientation orientation;

    QButtonGroup * pageOrder;
    QPrinter::PageOrder pageOrder2;

    QButtonGroup * colorMode;
    QPrinter::ColorMode colorMode2;

    QSpinBox * copies;
    int numCopies;

    QBoxLayout *customLayout;
};


static void perhapsAddPrinter( QListView * printers, const QString &name,
			       QString host, QString comment )
{
    const QListViewItem * i = printers->firstChild();
    while( i && i->text( 0 ) != name )
	i = i->nextSibling();
    if ( i )
	return;
    if ( host.isEmpty() )
	host = qApp->translate( "QPrintDialog", "locally connected" );
    (void)new QListViewItem( printers,
			     name.simplifyWhiteSpace(),
			     host.simplifyWhiteSpace(),
			     comment.simplifyWhiteSpace() );
}


static void parsePrintcap( QListView * printers )
{
    QFile printcap( QString::fromLatin1("/etc/printcap") );
    if ( !printcap.open( IO_ReadOnly ) )
	return;

    char * line = new char[1025];
    line[1024] = '\0';

    QString printerDesc;
    int lineLength = 0;

    while( !printcap.atEnd() &&
	   (lineLength=printcap.readLine( line, 1024 )) > 0 ) {
	if ( *line == '#' ) {
	    *line = '\0';
	    lineLength = 0;
	}
	if ( lineLength >= 2 && line[lineLength-2] == '\\' ) {
	    line[lineLength-2] = '\0';
	    printerDesc += QString::fromLocal8Bit(line);
	} else {
	    printerDesc += QString::fromLocal8Bit(line);
	    printerDesc = printerDesc.simplifyWhiteSpace();
	    int i = printerDesc.find( ':' );
	    QString printerName, printerComment, printerHost;
	    if ( i >= 0 ) {
		// have : want |
		int j = printerDesc.find( '|' );
		printerName = printerDesc.left( j > 0 ? j : i );
		if ( j > 0 ) {
		    // try extracting a comment from the aliases...
		    printerComment = qApp->translate( "QPrintDialog",
						      "Aliases: " );
		    printerComment += printerDesc.mid( j+1, i-j-1 );
		    j=printerComment.length();
		    while( j > 0 ) {
			j--;
			if ( printerComment[j] == '|' )
			    printerComment[j] = ',';
		    }
		}
		// then look for a real comment
		j = i+1;
		while( printerDesc[j].isSpace() )
		    j++;
		if ( printerDesc[j] != ':' ) {
		    printerComment = printerDesc.mid( i, j-i );
		    printerComment = printerComment.simplifyWhiteSpace();
		}
 		// look for lprng psuedo all printers entry
 		i = printerDesc.find( QRegExp( ": *all *=" ) );
 		if ( i >= 0 )
		    printerName = "";
		// look for signs of this being a remote printer
		i = printerDesc.find(
			QRegExp( QString::fromLatin1(": *rm *=") ) );
		if ( i >= 0 ) {
		    // point k at the end of remote host name
		    while( printerDesc[i] != '=' )
			i++;
		    while( printerDesc[i] == '=' || printerDesc[i].isSpace() )
			i++;
		    j = i;
		    while( j < (int)printerDesc.length() && printerDesc[j] != ':' )
			j++;

		    // and stuff that into the string
		    printerHost = printerDesc.mid( i, j-i );
		}
	    }
	    if ( printerName.length() )
		perhapsAddPrinter( printers, printerName, printerHost,
				   printerComment );
	    // chop away the line, for processing the next one
	    printerDesc = "";
	}
    }
    delete[] line;
}


// solaris, not 2.6
static void parseEtcLpPrinters( QListView * printers )
{
    QDir lp( QString::fromLatin1("/etc/lp/printers") );
    const QFileInfoList * dirs = lp.entryInfoList();
    if ( !dirs )
	return;

    QFileInfoListIterator it( *dirs );
    QFileInfo *printer;
    QString tmp;
    while ( (printer = it.current()) != 0 ) {
	++it;
	if ( printer->isDir() ) {
	    tmp.sprintf( "/etc/lp/printers/%s/configuration",
			 printer->fileName().ascii() );
	    QFile configuration( tmp );
	    char * line = new char[1025];
	    QRegExp remote( QString::fromLatin1("^Remote:") );
	    QRegExp contentType( QString::fromLatin1("^Content types:") );
	    QString printerHost;
	    bool canPrintPostscript = FALSE;
	    if ( configuration.open( IO_ReadOnly ) ) {
		while( !configuration.atEnd() &&
		       configuration.readLine( line, 1024 ) > 0 ) {
		    if ( remote.match( QString::fromLatin1(line) ) == 0 ) {
			const char * p = line;
			while( *p != ':' )
			    p++;
			p++;
			while( isspace(*p) )
			    p++;
			printerHost = QString::fromLocal8Bit(p);
			printerHost = printerHost.simplifyWhiteSpace();
		    } else if ( contentType.match( QString::fromLatin1(line) ) == 0 ) {
			char * p = line;
			while( *p != ':' )
			    p++;
			p++;
			char * e;
			while( *p ) {
			    while( isspace(*p) )
				p++;
			    if ( *p ) {
				char s;
				e = p;
				while( isalnum(*e) )
				    e++;
				s = *e;
				*e = '\0';
				if ( !qstrcmp( p, "postscript" ) ||
				     !qstrcmp( p, "any" ) )
				    canPrintPostscript = TRUE;
				*e = s;
				if ( s == ',' )
				    e++;
				p = e;
			    }
			}
		    }
		}
		if ( canPrintPostscript )
		    perhapsAddPrinter( printers, printer->fileName(),
				       printerHost, QString::fromLatin1("") );
	    }
	    delete[] line;
	}
    }
}


// solaris 2.6
static char * parsePrintersConf( QListView * printers )
{
    QFile pc( QString::fromLatin1("/etc/printers.conf") );
    if ( !pc.open( IO_ReadOnly ) )
	return 0;

    char * line = new char[1025];
    line[1024] = '\0';

    QString printerDesc;
    int lineLength = 0;

    char * defaultPrinter = 0;

    while( !pc.atEnd() &&
	   (lineLength=pc.readLine( line, 1024 )) > 0 ) {
	if ( *line == '#' ) {
	    *line = '\0';
	    lineLength = 0;
	}
	if ( lineLength >= 2 && line[lineLength-2] == '\\' ) {
	    line[lineLength-2] = '\0';
	    printerDesc += QString::fromLocal8Bit(line);
	} else {
	    printerDesc += QString::fromLocal8Bit(line);
	    printerDesc = printerDesc.simplifyWhiteSpace();
	    int i = printerDesc.find( ':' );
	    QString printerName, printerHost, printerComment;
	    if ( i >= 0 ) {
		// have : want |
		int j = printerDesc.find( '|', 0 );
		if ( j >= i )
		    j = -1;
		printerName = printerDesc.mid( 0, j < 0 ? i : j );
		if ( printerName == QString::fromLatin1("_default") ) {
		    i = printerDesc.find(
			QRegExp( QString::fromLatin1(": *use *=") ) );
		    while( printerDesc[i] != '=' )
			i++;
		    while( printerDesc[i] == '=' || printerDesc[i].isSpace() )
			i++;
		    j = i;
		    while( j < (int)printerDesc.length() &&
			   printerDesc[j] != ':' &&
			   printerDesc[j] != ',' )
			j++;
		    // that's our default printer
		    defaultPrinter = qstrdup( printerDesc.mid( i, j-i ).ascii() );
		    printerName = "";
		    printerDesc = "";
		} else if ( printerName == QString::fromLatin1("_all") ) {
		    // skip it.. any other cases we want to skip?
		    printerName = "";
		    printerDesc = "";
		}

		if ( j > 0 ) {
		    // try extracting a comment from the aliases...
		    printerComment = qApp->translate( "QPrintDialog",
						      "Aliases: " );
		    printerComment += printerDesc.mid( j+1, i-j-1 );
		    for( j=printerComment.length(); j>-1; j-- )
			if ( printerComment[j] == '|' )
			    printerComment[j] = ',';
		}
		// look for signs of this being a remote printer
		i = printerDesc.find(
		    QRegExp( QString::fromLatin1(": *bsdaddr *=") ) );
		if ( i >= 0 ) {
		    // point k at the end of remote host name
		    while( printerDesc[i] != '=' )
			i++;
		    while( printerDesc[i] == '=' || printerDesc[i].isSpace() )
			i++;
		    j = i;
		    while( j < (int)printerDesc.length() &&
			   printerDesc[j] != ':' &&
			   printerDesc[j] != ',' )
			j++;
		    // and stuff that into the string
		    printerHost = printerDesc.mid( i, j-i );
		    // maybe stick the remote printer name into the comment
		    if ( printerDesc[j] == ',' ) {
			i = ++j;
			while( printerDesc[i].isSpace() )
			    i++;
			j = i;
			while( j < (int)printerDesc.length() &&
			       printerDesc[j] != ':' &&
			       printerDesc[j] != ',' )
			    j++;
			if ( printerName != printerDesc.mid( i, j-i ) ) {
			    printerComment = QString::fromLatin1("Remote name: ");
			    printerComment += printerDesc.mid( i, j-i );
			}
		    }
		}
	    }
	    if ( printerName.length() )
		perhapsAddPrinter( printers, printerName, printerHost,
				   printerComment );
	    // chop away the line, for processing the next one
	    printerDesc = "";
	}
    }
    delete[] line;
    return defaultPrinter;
}



// HP-UX
static void parseEtcLpMember( QListView * printers )
{
    QDir lp( QString::fromLatin1("/etc/lp/member") );
    if ( !lp.exists() )
	return;
    const QFileInfoList * dirs = lp.entryInfoList();
    if ( !dirs )
	return;

    QFileInfoListIterator it( *dirs );
    QFileInfo *printer;
    QString tmp;
    while ( (printer = it.current()) != 0 ) {
	++it;
	// uglehack.
	// I haven't found any real documentation, so I'm guessing that
	// since lpstat uses /etc/lp/member rather than one of the
	// other directories, it's the one to use.  I did not find a
	// decent way to locate aliases and remote printers.
	if ( printer->isFile() )
	    perhapsAddPrinter( printers, printer->fileName(),
			       qApp->translate( "QPrintDialog","unknown"),
				QString::fromLatin1("") );
    }
}

// IRIX 6.x
static void parseSpoolInterface( QListView * printers )
{
    QDir lp( QString::fromLatin1("/usr/spool/lp/interface") );
    if ( !lp.exists() )
	return;
    const QFileInfoList * files = lp.entryInfoList();
    if( !files )
	return;

    QFileInfoListIterator it( *files );
    QFileInfo *printer;
    while ( (printer = it.current()) != 0) {
	++it;

	if ( !printer->isFile() )
	    continue;

	// parse out some information
	QFile configFile( printer->filePath() );
	if ( !configFile.open( IO_ReadOnly ) )
	    continue;

	QCString line(1025);
	QString hostName;
	QString hostPrinter;
	QString printerType;

	QRegExp typeKey(QString::fromLatin1("^TYPE="));
	QRegExp hostKey(QString::fromLatin1("^HOSTNAME="));
	QRegExp hostPrinterKey(QString::fromLatin1("^HOSTPRINTER="));
	int length;

	while( !configFile.atEnd() &&
	    (configFile.readLine( line.data(), 1024 )) > 0 ) {

	    if(typeKey.match(line, 0, &length) == 0) {
		printerType = line.mid(length, line.length()-length);
		printerType = printerType.simplifyWhiteSpace();
	    }
	    if(hostKey.match(line, 0, &length) == 0) {
		hostName = line.mid(length, line.length()-length);
		hostName = hostName.simplifyWhiteSpace();
	    }
	    if(hostPrinterKey.match(line, 0, &length) == 0) {
		hostPrinter = line.mid(length, line.length()-length);
		hostPrinter = hostPrinter.simplifyWhiteSpace();
	    }
	}
	configFile.close();

	printerType = printerType.stripWhiteSpace();
	if ( !printerType.isEmpty() && qstricmp( printerType.ascii(), "postscript" ))
	    continue;

	if(hostName.isEmpty() || hostPrinter.isEmpty())
	{
	    perhapsAddPrinter( printers, printer->fileName(),
		QString::fromLatin1(""), QString::fromLatin1(""));
	} else
	{
	    QString comment = QString::fromLatin1("Remote name: ");
	    comment += hostPrinter;
	    perhapsAddPrinter( printers, printer->fileName(),
		hostName, comment);
	}
    }
}


// Every unix must have its own.  It's a standard.  Here is AIX.
static void parseQconfig( QListView * printers )
{
    QFile qconfig( QString::fromLatin1("/etc/qconfig") );
    if ( !qconfig.open( IO_ReadOnly ) )
	return;

    QTextStream ts( &qconfig );
    QString line;

    QString stanzaName; // either a queue or a device name
    bool up = TRUE; // queue up?  default TRUE, can be FALSE
    QString remoteHost; // null if local
    QString deviceName; // null if remote

    // our basic strategy here is to process each line, detecting new
    // stanzas.  each time we see a new stanza, we check if the
    // previous stanza was a valid queue for a) a remote printer or b)
    // a local printer.  if it wasn't, we assume that what we see is
    // the start of the first stanza, or that the previous stanza was
    // a device stanza, or that there is some syntax error (we don't
    // report those).

    do {
	line = ts.readLine();
	bool indented = line[0].isSpace();
	line = line.simplifyWhiteSpace();

	if ( indented && line.contains( '=' ) ) { // line in stanza
	
	    int i = line.find( '=' );
	    QString variable = line.left( i ).simplifyWhiteSpace();
	    QString value=line.mid( i+1, line.length() ).simplifyWhiteSpace();
	    if ( variable == QString::fromLatin1("device") )
		deviceName = value;
	    else if ( variable == QString::fromLatin1("host") )
		remoteHost = value;
	    else if ( variable == QString::fromLatin1("up") )
		up = !(value.lower() == QString::fromLatin1("false"));
	} else if ( line[0] == '*' ) { // comment
	    // nothing to do
	} else if ( ts.atEnd() || // end of file, or beginning of new stanza
		    ( !indented &&
		      line.contains(
			QRegExp( QString::fromLatin1("^[0-z][0-z]*:$") ) ) ) ) {
	    if ( up && stanzaName.length() > 0 && stanzaName.length() < 21 ) {
		if ( remoteHost.length() ) // remote printer
		    perhapsAddPrinter( printers, stanzaName, remoteHost,
				       QString::null );
		else if ( deviceName.length() ) // local printer
		    perhapsAddPrinter( printers, stanzaName, QString::null,
				       QString::null );
	    }
	    line.truncate( line.length()-1 );
	    if ( line.length() >= 1 && line.length() <= 20 )
		stanzaName = line;
	    up = TRUE;
	    remoteHost = QString::null;
	    deviceName = QString::null;
	} else {
	    // syntax error?  ignore.
	}
    } while( !ts.atEnd() );
}

// And here is VMS
#if defined(_OS_VMS_)
static void parseDollarPrinters( QListView * printers )
{
    const char* dollarNames[] = { "PRINTER", "PRINTER2", 
				  "PRINTER3", "PRINTER4", 0 };
    int i = 0;
    while( dollarNames[i] ) {
	char *t = getenv( dollarNames[i] );
	if ( t )
	    perhapsAddPrinter( d->printers, QString::fromLatin1( t ), "", "");
    }
}
#endif

static QPrintDialog * globalPrintDialog = 0;

static void deleteGlobalPrintDialog()
{
    delete globalPrintDialog;
    globalPrintDialog = 0;
}

/*!
  \class QPrintDialog qprintdialog.h

  \brief The QPrintDialog class provides a dialog for specifying
  print-out details.

  \ingroup dialogs

  \warning This class is not present on all platforms; use
  QPrinter::setup() instead for portability.

  \internal
  THIS DOCUMENTATION IS Not Revised. It must be revised before
  becoming public API.

  It encompasses both the sort of details needed for doing a simple
  print-out and some print configuration setup.

  The easiest way to use the class is through the static
  function getPrinterSetup().  You can also subclass the QPrintDialog
  and add some custom buttons with addButton() to extend the
  functionality of the print dialog.

  <img src="qprintdlg-m.png"><br clear=all>
  The printer dialog, on a large screen, in Motif style.
*/


/*! Constructs a new modal printer dialog that configures \a prn and is a
  child of \a parent named \a name.
*/

QPrintDialog::QPrintDialog( QPrinter *prn, QWidget *parent, const char *name )
    : QDialog( parent, name, TRUE )
{
    d = new QPrintDialogPrivate;
    d->numCopies = 1;

    QBoxLayout * tll = new QBoxLayout( this, QBoxLayout::Down, 12, 0 );

    // destination
    QGroupBox * g;
    g = setupDestination();
    tll->addWidget( g, 1 );

    tll->addSpacing( 12 );

    // printer and paper settings
    QBoxLayout * lay = new QBoxLayout( QBoxLayout::LeftToRight );
    tll->addLayout( lay );

    g = setupPrinterSettings();
    lay->addWidget( g, 1 );

    lay->addSpacing( 12 );

    g = setupPaper();
    lay->addWidget( g );

    tll->addSpacing( 12 );

    // options
    g = setupOptions();
    tll->addWidget( g );
    tll->addSpacing( 12 );

    QBoxLayout *l = new QBoxLayout( QBoxLayout::LeftToRight, 12, 0 );
    d->customLayout = new QBoxLayout( QBoxLayout::LeftToRight );
    tll->addLayout( l );
    l->addLayout( d->customLayout );
    l->addStretch();
    tll->addSpacing( 12 );

    // buttons
    QBoxLayout *horiz = new QBoxLayout( QBoxLayout::LeftToRight );
    tll->addLayout( horiz );

    if ( style() != MotifStyle )
	horiz->addStretch( 1 );

    d->ok = new QPushButton( this, "ok" );
    d->ok->setText( tr("OK") );
    d->ok->setDefault( TRUE );
    horiz->addWidget( d->ok );
    if ( style() == MotifStyle )
	horiz->addStretch( 1 );
    horiz->addSpacing( 6 );

    QPushButton * cancel = new QPushButton( this, "cancel" );
    cancel->setText( tr("Cancel") );
    horiz->addWidget( cancel );

    QSize s1 = d->ok->sizeHint();
    QSize s2 = cancel->sizeHint();
    s1 = QSize( QMAX(s1.width(), s2.width()),
		QMAX(s1.height(), s2.height()) );

    d->ok->setFixedSize( s1 );
    cancel->setFixedSize( s1 );

    tll->activate();

    connect( d->ok, SIGNAL(clicked()), SLOT(okClicked()) );
    connect( cancel, SIGNAL(clicked()), SLOT(reject()) );

    QSize ms( minimumSize() );
    QSize ss( QApplication::desktop()->size() );
    if ( ms.height() < 512 && ss.height() >= 600 )
	ms.setHeight( 512 );
    else if ( ms.height() < 460 && ss.height() >= 480 )
	ms.setHeight( 460 );
    resize( ms );

    setPrinter( prn, TRUE );
    d->printers->setFocus();

    setFontPropagation( SameFont );
    setPalettePropagation( SamePalette );
    d->ok->setEnabled( TRUE );
}


/*! Destroys the object and frees any allocated resources.  Does not
  delete the associated QPrinter object.
*/

QPrintDialog::~QPrintDialog()
{
    if ( this == globalPrintDialog )
	globalPrintDialog = 0;
    delete d;
}


QGroupBox * QPrintDialog::setupPrinterSettings()
{
    QGroupBox * g = new QGroupBox( tr( "Printer settings"),
				   this, "settings group box" );

    d->colorMode = new QButtonGroup( this );
    d->colorMode->hide();
    connect( d->colorMode, SIGNAL(clicked(int)),
	     this, SLOT(colorModeSelected(int)) );

    QBoxLayout * tll = new QBoxLayout( g, QBoxLayout::Down, 12, 0 );
    tll->addSpacing( 8 );

    QRadioButton *rb;
    rb = new QRadioButton( tr( "Print in color if available" ),
			   g, "color" );
    tll->addWidget( rb );
    d->colorMode->insert( rb, QPrinter::Color );
    rb->setChecked( TRUE );

    rb = new QRadioButton( tr("Print in grayscale"),
			   g, "graysacle" );
    tll->addWidget( rb );
    d->colorMode->insert( rb, QPrinter::GrayScale );

    return g;
}

QGroupBox * QPrintDialog::setupDestination()
{
    QGroupBox * g = new QGroupBox( tr( "Print destination"),
				   this, "destination group box" );

    QBoxLayout * tll = new QBoxLayout( g, QBoxLayout::Down, 12, 0 );
    tll->addSpacing( 8 );

    d->printerOrFile = new QButtonGroup( this );
    d->printerOrFile->hide();
    connect( d->printerOrFile, SIGNAL(clicked(int)),
	     this, SLOT(printerOrFileSelected(int)) );

    // printer radio button, list
    QRadioButton * rb = new QRadioButton( tr( "Print to printer:" ), g,
					  "printer" );
    tll->addWidget( rb );
    d->printerOrFile->insert( rb, 0 );
    rb->setChecked( TRUE );
    d->outputToFile = FALSE;

    QBoxLayout * horiz = new QBoxLayout( QBoxLayout::LeftToRight );
    tll->addLayout( horiz, 3 );
    horiz->addSpacing( 19 );

    d->printers = new QListView( g, "list of printers" );
    d->printers->setAllColumnsShowFocus( TRUE );
    d->printers->addColumn( tr("Printer"), 125 );
    d->printers->addColumn( tr("Host"), 125 );
    d->printers->addColumn( tr("Comment"), 150 );
    d->printers->setFrameStyle( QFrame::WinPanel + QFrame::Sunken );

#if defined(UNIX)
    char * etcLpDefault = 0;

    parsePrintcap( d->printers );
    parseEtcLpMember( d->printers );
    parseSpoolInterface( d->printers );
    parseQconfig( d->printers );

    QFileInfo f;
    f.setFile( QString::fromLatin1("/etc/lp/printers") );
    if ( f.isDir() ) {
	parseEtcLpPrinters( d->printers );
	QFile def( QString::fromLatin1("/etc/lp/default") );
	if ( def.open( IO_ReadOnly ) ) {
	    etcLpDefault = new char[1025];
	    def.readLine( etcLpDefault, 1024 );
	    char * p = etcLpDefault;
	    while( p && *p ) {
		if ( !isprint(*p) || isspace(*p) )
		    *p = 0;
		else
		    p++;
	    }
	}
    }

    f.setFile( QString::fromLatin1("/etc/printers.conf") );
    if ( f.isFile() ) {
	char * def = parsePrintersConf( d->printers );
	if ( def ) {
	    if ( etcLpDefault )
		delete[] etcLpDefault;
	    etcLpDefault = def;
	}
    }
#elif defined(_OS_VMS_)
    char * etcLpDefault = 0;
    parseDollarPrinters( d->printers );
#endif

#if defined(UNIX) || defined(_OS_VMS_)
    // all printers hopefully known.  try to find a good default
    QString dollarPrinter;
    {
	char * t;
	t = getenv( "PRINTER" );
	if ( !t || !*t )
	    t = getenv( "LPDEST" );
	dollarPrinter = QString::fromLatin1(t);
    }
    int quality = 0;

    // bang the best default into the listview
    const QListViewItem * lvi = d->printers->firstChild();
    d->printers->setCurrentItem( (QListViewItem *)lvi );
    while( lvi ) {
	QRegExp ps1( QString::fromLatin1("[^a-z]ps[^a-z]") );
	QRegExp ps2( QString::fromLatin1("[^a-z]ps$") );
	QRegExp lp1( QString::fromLatin1("[^a-z]lp[^a-z]") );
	QRegExp lp2( QString::fromLatin1("[^a-z]lp$") );
	if ( quality < 4 &&
	     lvi->text( 0 ) == dollarPrinter ) {
	    d->printers->setCurrentItem( (QListViewItem *)lvi );
	    quality = 4;
	} else if ( quality < 3 && etcLpDefault &&
		    lvi->text( 0 ) == QString::fromLatin1(etcLpDefault) ) {
	    d->printers->setCurrentItem( (QListViewItem *)lvi );
	    quality = 3;
	} else if ( quality < 2 &&
		    ( lvi->text( 0 ) == QString::fromLatin1("ps") ||
		      ps1.match( lvi->text( 2 ) ) > -1 ||
		      ps2.match( lvi->text( 2 ) ) > -1 ) ) {
	    d->printers->setCurrentItem( (QListViewItem *)lvi );
	    quality = 2;
	} else if ( quality < 1 &&
		    ( lvi->text( 0 ) == QString::fromLatin1("lp") ||
		      lp1.match( lvi->text( 2 ) ) > -1 ||
		      lp2.match( lvi->text( 2 ) ) > -1 ) ) {
	    d->printers->setCurrentItem( (QListViewItem *)lvi );
	    quality = 1;
	}
	lvi = lvi->nextSibling();
    }
    if ( d->printers->currentItem() )
	d->printers->setSelected( d->printers->currentItem(), TRUE );

    if ( etcLpDefault )			// Avoid purify complaint
	delete[] etcLpDefault;
#endif

    int h = fontMetrics().height();
    if ( d->printers->firstChild() )
	h = d->printers->firstChild()->height();
    d->printers->setMinimumSize( d->printers->sizeHint().width(),
				 d->printers->header()->height() +
				  3 * h );
    horiz->addWidget( d->printers, 3 );

    tll->addSpacing( 6 );

    // file radio button, edit/browse
    rb = new QRadioButton( tr( "Print to file:" ), g, "file" );
    tll->addWidget( rb );
    d->printerOrFile->insert( rb, 1 );

    horiz = new QBoxLayout( QBoxLayout::LeftToRight );
    tll->addLayout( horiz );
    horiz->addSpacing( 19 );

    d->fileName = new QLineEdit( g, "file name" );
    connect( d->fileName, SIGNAL( textChanged( const QString & ) ),
	     this, SLOT( fileNameEditChanged( const QString & ) ) );
    horiz->addWidget( d->fileName, 1 );
    horiz->addSpacing( 6 );
    d->browse = new QPushButton( tr("Browse..."), g, "browse files" );
    d->browse->setAutoDefault( FALSE );
    connect( d->browse, SIGNAL(clicked()),
	     this, SLOT(browseClicked()) );
    horiz->addWidget( d->browse );

    d->fileName->setEnabled( FALSE );
    d->browse->setEnabled( FALSE );

    tll->activate();

    return g;
}


QGroupBox * QPrintDialog::setupOptions()
{
    QGroupBox * g = new QGroupBox( tr( "Options"),
				   this, "options group box" );

    QBoxLayout * tll = new QBoxLayout( g, QBoxLayout::Down, 12, 2 );
    tll->addSpacing( 8 );

    QBoxLayout *lay = new QBoxLayout( QBoxLayout::LeftToRight );
    tll->addLayout( lay );

    tll = new QBoxLayout( lay, QBoxLayout::Down );

    d->printRange = new QButtonGroup( this );
    d->printRange->hide();
    connect( d->printRange, SIGNAL(clicked(int)),
	     this, SLOT(printRangeSelected(int)) );

    d->pageOrder = new QButtonGroup( this );
    d->pageOrder->hide();
    connect( d->pageOrder, SIGNAL(clicked(int)),
	     this, SLOT(pageOrderSelected(int)) );

    d->printAllButton = new QRadioButton( tr("Print all"), g, "print all" );
    d->printRange->insert( d->printAllButton, 0 );
    tll->addWidget( d->printAllButton );

    d->printRangeButton = new QRadioButton( tr("Print range"),
					    g, "print range" );
    d->printRange->insert( d->printRangeButton, 1 );
    tll->addWidget( d->printRangeButton );

    QBoxLayout * horiz = new QBoxLayout( QBoxLayout::LeftToRight );
    tll->addLayout( horiz );

    d->firstPageLabel = new QLabel( tr("From page:"), g, "first page" );
    horiz->addSpacing( 19 );
    horiz->addWidget( d->firstPageLabel );

    d->firstPage = new QSpinBox( 1, 9999, 1, g, "first page" );
    d->firstPage->setValue( 1 );
    horiz->addWidget( d->firstPage, 1 );
    connect( d->firstPage, SIGNAL(valueChanged(int)),
	     this, SLOT(setFirstPage(int)) );

    horiz = new QBoxLayout( QBoxLayout::LeftToRight );
    tll->addLayout( horiz );

    d->lastPageLabel = new QLabel( tr("To page:"), g, "last page" );
    horiz->addSpacing( 19 );
    horiz->addWidget( d->lastPageLabel );

    d->lastPage = new QSpinBox( 1, 9999, 1, g, "last page" );
    d->lastPage->setValue( 9999 );
    horiz->addWidget( d->lastPage, 1 );
    connect( d->lastPage, SIGNAL(valueChanged(int)),
	     this, SLOT(setLastPage(int)) );

    lay->addSpacing( 25 );
    tll = new QBoxLayout( lay, QBoxLayout::Down );

    // print order
    QRadioButton * rb = new QRadioButton( tr("Print first page first"),
					  g, "first page first" );
    tll->addWidget( rb );
    d->pageOrder->insert( rb, QPrinter::FirstPageFirst );
    rb->setChecked( TRUE );

    rb = new QRadioButton( tr("Print last page first"),
			   g, "last page first" );
    tll->addWidget( rb );
    d->pageOrder->insert( rb, QPrinter::LastPageFirst );

    tll->addStretch();

    // copies

    horiz = new QBoxLayout( QBoxLayout::LeftToRight );
    tll->addLayout( horiz );

    QLabel * l = new QLabel( tr("Number of copies:"), g, "Number of copies" );
    horiz->addWidget( l );

    d->copies = new QSpinBox( 1, 99, 1, g, "copies" );
    d->copies->setValue( 1 );
    horiz->addWidget( d->copies, 1 );
    connect( d->copies, SIGNAL(valueChanged(int)),
	     this, SLOT(setNumCopies(int)) );

    QSize s = d->firstPageLabel->sizeHint()
	      .expandedTo( d->lastPageLabel->sizeHint() )
	      .expandedTo( l->sizeHint() );
    d->firstPageLabel->setMinimumSize( s );
    d->lastPageLabel->setMinimumSize( s );
    l->setMinimumSize( s.width() + 19, s.height() );

    tll->activate();

    return g;
}


QGroupBox * QPrintDialog::setupPaper()
{
    QGroupBox * g = new QGroupBox( tr( "Paper format"),
				   this, "Paper format" );

    QBoxLayout * tll = new QBoxLayout( g, QBoxLayout::Down, 12, 0 );
    tll->addSpacing( 12 );

    // page orientation
    d->orientationCombo = new QComboBox( FALSE, g );
    tll->addWidget( d->orientationCombo );
    d->orientationCombo->insertItem( tr( "Portrait" ), -1 );
    d->orientationCombo->insertItem( tr( "Landscape" ), -1 );

    d->orientation = QPrinter::Portrait;

    tll->addSpacing( 8 );

    connect( d->orientationCombo, SIGNAL( activated( int ) ),
	     this, SLOT( orientSelected( int ) ) );

    // paper size
    d->sizeCombo = new QComboBox( FALSE, g );
    tll->addWidget( d->sizeCombo );

    d->sizeCombo->insertItem( tr( "A4 (210x297 mm, 8.26x11.7 inches)" ), -1 );
    d->sizeCombo->insertItem( tr( "B5 (182x257 mm, 7.17x10.13 inches)" ), -1 );
    d->sizeCombo->insertItem( tr( "Letter (8.5x11 inches, 216x279 mm)" ), -1 );
    d->sizeCombo->insertItem( tr( "Legal (8.5x14 inches, 216x356 mm)" ), -1 );
    d->sizeCombo->insertItem( tr( "Executive (7.5x10 inches, 191x254 mm)" ), -1 );
    d->sizeCombo->insertItem( tr( "A0 (841 x 1189 mm)" ), -1 );
    d->sizeCombo->insertItem( tr( "A1 (594 x 841 mm)" ), -1 );
    d->sizeCombo->insertItem( tr( "A2 (420 x 594 mm)" ), -1 );
    d->sizeCombo->insertItem( tr( "A3 (297 x 420 mm)" ), -1 );
    d->sizeCombo->insertItem( tr( "A5 (148 x 210 mm)" ), -1 );
    d->sizeCombo->insertItem( tr( "A6 (105 x 148 mm)" ), -1 );
    d->sizeCombo->insertItem( tr( "A7 (74 x 105 mm)" ), -1 );
    d->sizeCombo->insertItem( tr( "A8 (52 x 74 mm)" ), -1 );
    d->sizeCombo->insertItem( tr( "A9 (37 x 52 mm)" ), -1 );
    d->sizeCombo->insertItem( tr( "B0 (1030 x 1456 mm)" ), -1 );
    d->sizeCombo->insertItem( tr( "B1 (728 x 1030 mm)" ), -1 );
    d->sizeCombo->insertItem( tr( "B10 (32 x 45 mm)" ), -1 );
    d->sizeCombo->insertItem( tr( "B2 (515 x 728 mm)" ), -1 );
    d->sizeCombo->insertItem( tr( "B3 (364 x 515 mm)" ), -1 );
    d->sizeCombo->insertItem( tr( "B4 (257 x 364 mm)" ), -1 );
    d->sizeCombo->insertItem( tr( "B6 (128 x 182 mm)" ), -1 );
    d->sizeCombo->insertItem( tr( "B7 (91 x 128 mm)" ), -1 );
    d->sizeCombo->insertItem( tr( "B8 (64 x 91 mm)" ), -1 );
    d->sizeCombo->insertItem( tr( "B9 (45 x 64 mm)" ), -1 );
    d->sizeCombo->insertItem( tr( "C5E (163 x 229 mm)" ), -1 );
    d->sizeCombo->insertItem( tr( "US Common #10 Envelope (105 x 241 mm)" ), -1 );
    d->sizeCombo->insertItem( tr( "DLE (110 x 220 mm)" ), -1 );
    d->sizeCombo->insertItem( tr( "Folio (210 x 330 mm)" ), -1 );
    d->sizeCombo->insertItem( tr( "Ledger (432 x 279 mm)" ), -1 );
    d->sizeCombo->insertItem( tr( "Tabloid (279 x 432 mm)" ), -1 );

    connect( d->sizeCombo, SIGNAL( activated( int ) ),
	     this, SLOT( paperSizeSelected( int ) ) );

    tll->activate();

    return g;
}


/*!
  Display a dialog and allow the user to configure the QPrinter \a
  p.  Returns TRUE if the user clicks OK or presses Enter, FALSE if
  the user clicks Cancel or presses Escape.

  getPrinterSetup() remembers the settings and provides the same
  settings the next time the dialog is shown.
*/

bool QPrintDialog::getPrinterSetup( QPrinter * p )
{
    if ( !globalPrintDialog ) {
	globalPrintDialog = new QPrintDialog( 0, 0, "global print dialog" );
	globalPrintDialog->setCaption( QPrintDialog::tr( "Setup Printer" ) );
	qAddPostRoutine( deleteGlobalPrintDialog );
    }

    globalPrintDialog->setPrinter( p );
    bool r = globalPrintDialog->exec() == QDialog::Accepted;
    globalPrintDialog->setPrinter( 0 );
    return r;
}


void QPrintDialog::printerOrFileSelected( int id )
{
    d->outputToFile = id ? TRUE : FALSE;
    if ( d->outputToFile ) {
	fileNameEditChanged( d->fileName->text() );
	d->browse->setEnabled( TRUE );
	d->fileName->setEnabled( TRUE );
	d->fileName->setFocus();
	d->printers->setEnabled( FALSE );
    } else {
	d->ok->setEnabled( TRUE );
	d->printers->setEnabled( TRUE );
	if ( d->fileName->hasFocus() || d->browse->hasFocus() )
	    d->printers->setFocus();
	d->browse->setEnabled( FALSE );
	d->fileName->setEnabled( FALSE );
    }
}


void QPrintDialog::landscapeSelected( int id )
{
    d->orientation = (QPrinter::Orientation)id;
}


void QPrintDialog::paperSizeSelected( int id )
{
    d->pageSize = QPrinter::PageSize(id);
}


void QPrintDialog::orientSelected( int id )
{
    d->orientation = (QPrinter::Orientation)id;
}


void QPrintDialog::pageOrderSelected( int id )
{
    d->pageOrder2 = (QPrinter::PageOrder)id;
}


void QPrintDialog::setNumCopies( int copies )
{
    d->numCopies = copies;
}


void QPrintDialog::browseClicked()
{
    QString fn = QFileDialog::getSaveFileName( QString::null, tr( "Postscript files (*.ps);;All files (*)" ), this );
    if ( !fn.isNull() )
	d->fileName->setText( fn );
}


void QPrintDialog::okClicked()
{
    if ( d->outputToFile ) {
	d->printer->setOutputToFile( TRUE );
	d->printer->setOutputFileName( d->fileName->text() );
    } else {
	d->printer->setOutputToFile( FALSE );
	QListViewItem * l = d->printers->currentItem();
	if ( l )
	    d->printer->setPrinterName( l->text( 0 ) );
    }

    d->printer->setOrientation( d->orientation );
    d->printer->setPageSize( d->pageSize );
    d->printer->setPageOrder( d->pageOrder2 );
    d->printer->setColorMode( d->colorMode2 );
    d->printer->setNumCopies( d->numCopies );
    if ( d->printAllButton->isChecked() )
	d->printer->setFromTo( d->printer->minPage(), d->printer->maxPage() );
    else
	d->printer->setFromTo( d->firstPage->value(), d->lastPage->value() );

    accept();
}


void QPrintDialog::printRangeSelected( int id )
{
    bool enable = id ? TRUE : FALSE;
    d->firstPage->setEnabled( enable );
    d->lastPage->setEnabled( enable );
}


void QPrintDialog::setFirstPage( int fp )
{
    if ( d->printer )
	d->lastPage->setRange( fp, QMAX(fp, QPrintDialog::d->printer->maxPage()) );
}


void QPrintDialog::setLastPage( int lp )
{
    if ( d->printer )
	d->firstPage->setRange( QMIN(lp, QPrintDialog::d->printer->minPage()), lp );
}


/*!  Sets this dialog to configure \a p, or no printer if \a p is
  FALSE.  If \a pickUpSettings is TRUE, the dialog reads most of its
  settings from \a printer.  If \a pickUpSettings is FALSE (the
  default) the dialog keeps its old settings. */

void QPrintDialog::setPrinter( QPrinter * p, bool pickUpSettings )
{
    d->printer = p;

    if ( p && pickUpSettings ) {
	// top to botton in the old dialog.
	// printer or file
	d->printerOrFile->setButton( p->outputToFile() );
	printerOrFileSelected( p->outputToFile() );

	// printer name
	if ( !!p->printerName() ) {
	    QListViewItem * i = d->printers->firstChild();
	    while( i && i->text( 0 ) != p->printerName() )
		i = i->nextSibling();
	    if ( i )
		d->printers->setSelected( i, TRUE );
	}

	// print command does not exist any more

	// file name
	d->fileName->setText( p->outputFileName() );

	// orientation
	d->orientationCombo->setCurrentItem( (int)p->orientation() );
	orientSelected( p->orientation() );
	
	// page size
	d->sizeCombo->setCurrentItem( (int)p->pageSize() );
	paperSizeSelected( p->pageSize() );

	// New stuff (Options)

	// page order
	d->pageOrder->setButton( (int)p->pageOrder() );
	pageOrderSelected( p->pageOrder() );

	// color mode
	d->colorMode->setButton( (int)p->colorMode() );
	colorModeSelected( p->colorMode() );

	// number of copies
	d->copies->setValue( p->numCopies() );
	setNumCopies( p->numCopies() );
    }

    if ( p && p->maxPage() ) {
	d->printRangeButton->setEnabled( TRUE );
	d->firstPage->setRange( p->minPage(), p->maxPage() );
	d->lastPage->setRange( p->minPage(), p->maxPage() );
	// page range
	int some = p->maxPage()
		&& p->fromPage() && p->toPage()
		&& (p->fromPage() != p->minPage()
		    || p->toPage() != p->maxPage());
	if ( p->fromPage() ) {
	    setFirstPage( p->fromPage() );
	    setLastPage( p->toPage() );
	    d->firstPage->setValue(p->fromPage());
	    d->lastPage->setValue(p->toPage());
	}
	d->printRange->setButton( some );
	printRangeSelected( some );
    } else {
	d->printRange->setButton( 0 );	
	d->printRangeButton->setEnabled( FALSE );
	d->firstPage->setEnabled( FALSE );
	d->lastPage->setEnabled( FALSE );
	d->firstPageLabel->setEnabled( FALSE );
	d->lastPageLabel->setEnabled( FALSE );
	d->firstPage->setValue( 1 );
	d->lastPage->setValue( 1 );
    }
}


/*!  Returns a pointer to the printer this dialog configures, or 0 if
  this dialog does not operate on any printer. */

QPrinter * QPrintDialog::printer() const
{
    return d->printer;
}


void QPrintDialog::colorModeSelected( int id )
{
    d->colorMode2 = (QPrinter::ColorMode)id;
}

/*!
  Adds the button \a but to the layout of the print dialog. The added
  buttons are arranged from the left to the right below the
  last groupbox of the printdialog.
*/

void QPrintDialog::addButton( QPushButton *but )
{
    d->customLayout->addWidget( but );
}

void QPrintDialog::fileNameEditChanged( const QString &text )
{
    d->ok->setEnabled( !text.isEmpty() );
}

#endif

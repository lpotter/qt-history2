/****************************************************************************
**
** Implementation of QTable widget class
**
** Created : 000607
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the table module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition licenses may use this
** file in accordance with the Qt Commercial License Agreement provided
** with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#include "qtable.h"

#ifndef QT_NO_TABLE

#include "qpainter.h"
#include "qkeycode.h"
#include "qlineedit.h"
#include "qcursor.h"
#include "qapplication.h"
#include "qtimer.h"
#include "qobjectlist.h"
#include "qiconset.h"
#include "qcombobox.h"
#include "qcheckbox.h"
#include "qdragobject.h"
#include <stdlib.h>
#include <limits.h>

class Q_EXPORT QTableHeader : public QHeader
{
    friend class QTable;
    Q_OBJECT

public:
    enum SectionState {
	Normal,
	Bold,
	Selected
    };

    QTableHeader( int, QTable *t, QWidget *parent=0, const char *name=0 );
    ~QTableHeader() {};
    void addLabel( const QString &s , int size );

    void setSectionState( int s, SectionState state );
    SectionState sectionState( int s ) const;

    int sectionSize( int section ) const;
    int sectionPos( int section ) const;
    int sectionAt( int section ) const;

    void setSectionStretchable( int s, bool b );
    bool isSectionStretchable( int s ) const;

signals:
    void sectionSizeChanged( int s );

protected:
    void paintEvent( QPaintEvent *e );
    void paintSection( QPainter *p, int index, const QRect& fr );
    void mousePressEvent( QMouseEvent *e );
    void mouseMoveEvent( QMouseEvent *e );
    void mouseReleaseEvent( QMouseEvent *e );
    void mouseDoubleClickEvent( QMouseEvent *e );
    void resizeEvent( QResizeEvent *e );

private slots:
    void doAutoScroll();
    void sectionWidthChanged( int col, int os, int ns );
    void indexChanged( int sec, int oldIdx, int newIdx );
    void updateStretches();
    void updateWidgetStretches();

private:
    void updateSelections();
    void saveStates();
    void setCaching( bool b );
    void swapSections( int oldIdx, int newIdx );
    bool doSelection( QMouseEvent *e );

private:
    QArray<int> states, oldStates;
    QArray<bool> stretchable;
    QArray<int> sectionSizes, sectionPoses;
    bool mousePressed;
    int pressPos, startPos, endPos;
    QTable *table;
    QTimer *autoScrollTimer;
    QWidget *line1, *line2;
    bool caching;
    int resizedSection;
    bool isResizing;
    int numStretches;
    QTimer *stretchTimer, *widgetStretchTimer;
    QTableHeaderPrivate *d;

};

struct QTablePrivate
{
};

struct QTableHeaderPrivate
{
};


/*! \class QTableSelection qtable.h

  \brief The QTableSelection class provides access to a selected area in a
  QTable.

  \module table

  The selection is a rectangular set of cells.  One of the rectangle's
  cells is called the anchor cell; this is the cell that was selected first.
  The init() function sets the anchor and the selection rectangle
  to exactly this cell; the expandTo() function expands the selection
  rectangle.

  There are various access functions to find out about the area:
  anchorRow() and anchorCol() return the anchor's position; leftCol(), rightCol(),
  topRow() and bottomRow() return the rectangle's four edges. All
  four are part of the selection.

  A newly created QTableSelection is inactive -- isActive() returns
  FALSE.  You must use init() and expandTo() to activate it.

  \sa QTable QTable::addSelection() QTable::selection().
*/

/*! Creates an inactive selection. Use init() and expandTo() to
  activate it.
*/

QTableSelection::QTableSelection()
    : active( FALSE ), inited( FALSE ), tRow( -1 ), lCol( -1 ),
      bRow( -1 ), rCol( -1 ), aRow( -1 ), aCol( -1 )
{
}

/*! Sets the selection anchor to row \a row and column \a col and
  the selection to exactly this cell.

  To activate the selection you have to call expandTo().

  \sa isActive()
*/

void QTableSelection::init( int row, int col )
{
    aCol = lCol = rCol = col;
    aRow = tRow = bRow = row;
    active = FALSE;
    inited = TRUE;
}

/*! Expands the selection to \a row, \a col. The new selection
  rectangle is the bounding rectangle of \a row, \a col and the old
  selection rectangle. After calling this function the selection is
  active.

  If you haven't called init() yet, this function does nothing.

  \sa init() isActive()
*/

void QTableSelection::expandTo( int row, int col )
{
    if ( !inited )
	return;
    active = TRUE;

    if ( row < aRow ) {
	tRow = row;
	bRow = aRow;
    } else {
	tRow = aRow;
	bRow = row;
    }

    if ( col < anchorCol() ) {
	lCol = col;
	rCol = aCol;
    } else {
	lCol = aCol;
	rCol = col;
    }
}

/*! Returns TRUE if \a s includes the same cells as \e this selection, or
  else FALSE.
*/

bool QTableSelection::operator==( const QTableSelection &s ) const
{
    return ( s.active == active &&
	     s.tRow == tRow && s.bRow == bRow &&
	     s.lCol == lCol && s.rCol == rCol );
}

/*! \fn int QTableSelection::topRow() const

  Returns the top row of the selection.

  \sa bottomRow() leftCol() rightCol()
*/

/*! \fn int QTableSelection::bottomRow() const

  Returns the bottom row of \e this selection.

  \sa topRow() leftCol() rightCol()
*/

/*! \fn int QTableSelection::leftCol() const

  Returns the left column of \e this selection.

  \sa topRow() bottomRow() rightCol()
*/

/*! \fn int QTableSelection::rightCol() const

  Returns the right column of the selection.

  \sa topRow() bottomRow() leftCol()
*/

/*! \fn int QTableSelection::anchorRow() const

  Returns the anchor row of the selection.

  \sa anchorCol() expandTo()
*/

/*! \fn int QTableSelection::anchorCol() const

  Returns the anchor column of \e this selection.

  \sa anchorRow() expandTo()
*/

/*! \fn bool QTableSelection::isActive() const

  Returns whether the selection is active or not. A selection is
  active after init() and expandTo() have been called.
*/


/*! \class QTableItem qtable.h

  \brief The QTableItem class provides the cell content in a QTable.

  \module table

  A QTableItem contains the data of a table cell and provides
  means to change them. It specifies whether
  and under which circumstances the cell might be edited by the user
  (see EditType) and the kind of editor that is used for changing its
  content.

  Items may contain a text string and one pixmap each. If a pixmap
  has been defined it is presented to the left of the text. By default
  a QLineEdit that aligns its input to the left is provided for editing.

  Furthermore, a QTableItem defines the cell size and the alignment of
  the displayed data. The item defines whether the respective cell
  data can be replaced, and whether word wrapping is desired when
  cell contents exceed the width of the cell.
  In addition the QTableItem class provides the API
  needed for sorting table items.

  QTableItems are added to a QTable using QTable::setItem(). As long
  as they haven't been attached to the table this way QTable cells are
  empty.

  To get rid of an item, simply delete it. By doing so, all required
  actions for removing it from the table are taken.

  To define another editor you have to reimplement createEditor() and
  setContentFromEditor(). By reimplementing paint() and adding
  the relevant set- and "get"-functions custom subclasses of QTableItem
  may overcome the restriction of one text string and one pixmap per cell.
  If sorting is required reimplementing the
  key() function might be neccessary.
*/

/*! \fn QTable *QTableItem::table() const

  Returns the QTable the item belongs to.

  Note that this is the parent table object of the item even if
  the item fills a cell of another table.

  \sa QTable::setItem() QTableItem()
*/

/*! \enum QTableItem::EditType

  This enum type defines whether and when the user may edit a table
  cell. Currently the following possibilities exist:

  \value Always  The cell always is and always looks editable,
                 even if QTable::isColumnReadOnly(), QTable::isRowReadOnly()
                 or QTable::isReadOnly() return TRUE.

                 This means that the editor created with createEditor()
                 (by default a QLineEdit) is always visible. This has
                 implications on the alignment of the content:
                 the default editor aligns everything (even numbers)
                 to the left whilst numerical values in the cell are by default
                 aligned to the right.

                 If a cell with the edit type \c Always looks
                 misaligned you might decide to reimplement createEditor()
                 for this particular item.

  \value WhenCurrent  The cell is editable, and looks editable
                      whenever it has keyboard focus
                      (see QTable::setCurrentCell()).

                      This setting does not overwrite QTable::isColumnReadOnly(),
                      QTable::isRowReadOnly() or QTable::isReadOnly().

  \value OnTyping  The cell is editable, but looks editable only when
                   the user types in it or double-clicks it.
                   It resembles the \c WhenCurrent functionality
                   but can look a bit cleaner.

                   Note that this setting does not overwrite
                   QTable::isColumnReadOnly(),
                   QTable::isRowReadOnly() or QTable::isReadOnly().

                   The \c OnTyping edit type is the default when QTableItem objects
                   are created by the convenient functions QTable::setText()
                   and QTable::setPixmap().

  \value Never  The cell isn't editable.
*/

/*! Creates a table item for the table \a table that contains the text
  \a text. \a et determines its \l EditType.

  To insert the item into a table use QTable::setItem():

  \walkthrough table/wineorder2/productlist.cpp
  \skipto QTableItem * discount
  \printline QTableItem * discount
  \printuntil setItem

  (Code taken from \link wineorder2-example.html
  table/wineorder2/productlist.cpp \endlink )

  Whilst the parent of the item (i.e. \a table) and the QTable it has been
  inserted into might occasionally be different, a table item can't be inserted into
  more than one tables at a time.
*/

QTableItem::QTableItem( QTable *table, EditType et, const QString &text )
    : txt( text ), pix(), t( table ), edType( et ), wordwrap( FALSE ),
      tcha( TRUE ), rw( -1 ), cl( -1 ), rowspan( 1 ), colspan( 1 )
{
    enabled = TRUE;
}

/*! Creates a child item of the table \a table with the text \a text and the
  pixmap \a p. The item has the \l EditType \a et.

  When shown in a table cell the pixmap appears to the left of \a text.

  To insert the item into \a table (or another QTable object) use QTable::setItem().

  Whilst the parent of the item (i.e. \a table) and the QTable it has been
  inserted into might occasionally be different, a table item can't be inserted into
  more than one tables at a time.
*/

QTableItem::QTableItem( QTable *table, EditType et,
			const QString &text, const QPixmap &p )
    : txt( text ), pix( p ), t( table ), edType( et ), wordwrap( FALSE ),
      tcha( TRUE ), rw( -1 ), cl( -1 ), rowspan( 1 ), colspan( 1 )
{
    enabled = TRUE;
}

/*! The destructor deletes \e this item and frees all allocated resources.

    If the item has been attached to a table it is safely removed from it.
*/

QTableItem::~QTableItem()
{
    table()->takeItem( this );
}

/*! Returns the Run Time Type Identification number of \e this item.
  All QTableItem objects of the default implementation return 0.

  Although often frowned upon by purists, Run Time Type Identification
  is very useful for QTables as it allows for an
  efficient indexed storage mechanism.

  To distingtuish between table items of various types create custom
  classes derived from QTableItem and make them return one unique rtti() value
  each. It is advisable to use values
  greater than 1000, preferably large random numbers, to allow for
  extensions to this class.

  \sa QCheckTableItem::rtti() QComboTableItem::rtti()
*/

int QTableItem::rtti() const
{
    return 0;
}

/*! Returns the item's pixmap or a null-pixmap if no pixmap has been
  set so far.

  In the default implementation no more than one pixmap per item are
  allowed.

  \sa setPixmap() text()
*/

QPixmap QTableItem::pixmap() const
{
    return pix;
}


/*! Provides the text of the item or an empty string.

  \sa setText() pixmap()
*/

QString QTableItem::text() const
{
    if ( edType == Always ) //### why only always?
	((QTableItem*)this)->setContentFromEditor(table()->cellWidget(rw,cl));
    return txt;
}

/*! Makes \a p the pixmap of \e this item.

  If text() has been defined for the item the pixmap is by
  default presented on its left hand side.

  Note that setPixmap() does not update the cell the item belongs to.
  Use QTable::updateCell() to repaint cell contents immediately.

  For \l{QComboTableItem}s and \l{QCheckTableItem}s this function
  has no visible effect.

  \sa QTable::setPixmap() pixmap() setText()
*/

void QTableItem::setPixmap( const QPixmap &p )
{
    pix = p;
}

/*! Changes the text of the item to \a str. Note that the cell is not
  repainted.

  \walkthrough table/wineorder2/productlist.cpp
  \skipto suffix
  \printline suffix
  \walkthrough table/wineorder2/spinboxitem.cpp
  \skipto setText(
  \printline setText

  (Code taken from \link wineorder2-example.html
  table/wineorder2/productlist.cpp and
  table/wineorder2/spinboxitem.cpp \endlink )

  \sa QTable::setText() text() setPixmap() QTable::updateCell()
*/

void QTableItem::setText( const QString &str )
{
    txt = str;
}

/*! This virtual function is used to paint the contents of an item
  on the painter \a p in the rectangular area \a cr using
  the color group \a cg.

  If \a selected is TRUE the cell is represented in a highlighted
  manner.

  Usually you don't need to use this function but if you want
  to draw custom content in a cell you have to reimplement it.
*/

void QTableItem::paint( QPainter *p, const QColorGroup &cg,
			const QRect &cr, bool selected )
{
    p->fillRect( 0, 0, cr.width(), cr.height(),
		 selected ? cg.brush( QColorGroup::Highlight )
		          : cg.brush( QColorGroup::Base ) );

    int w = cr.width();
    int h = cr.height();

    int x = 0;
    if ( !pix.isNull() ) {
	p->drawPixmap( 0, ( cr.height() - pix.height() ) / 2, pix );
	x = pix.width() + 2;
    }

    if ( selected )
	p->setPen( cg.highlightedText() );
    else
	p->setPen( cg.text() );
    p->drawText( x + 2, 0, w - x - 4, h,
		 wordwrap ? (alignment() | WordBreak) : alignment(), txt );
}

/*! This virtual function creates the editor with which the user can
  edit the cell. The default implementation creates a QLineEdit that
  aligns everything to the left.

  If the function returns 0, the relevant cell cannot be edited.

  The returned widget should preferably be unvisible, and it should
  have QTable::viewport() as parent.

  If you reimplement this function, you probably also need to
  reimplement setContentFromEditor().

  \walkthrough table/wineorder2/spinboxitem.cpp
  \skipto createEditor()
  \printline createEditor()
  \printuntil new QSpinBox
  \skipto return
  \printuntil }

  (Code taken from \link wineorder2-example.html
  table/wineorder2/spinboxitem.cpp \endlink )

  \sa QTable::createEditor() setContentFromEditor() QTable::viewport()
*/

QWidget *QTableItem::createEditor() const
{
    QLineEdit *e = new QLineEdit( table()->viewport() );
    e->setFrame( FALSE );
    e->setText( text() );
    QObject::connect( e, SIGNAL( returnPressed() ), table(), SLOT( doValueChanged() ) );
    return e;
}

/*! Whenever the content of a cell has been edited by the editor \a
  w, QTable calls this virtual function to copy the new values into
  the QTableItem.

  You probably \e must reimplement this function if you reimplement
  createEditor() and return something that is not a QLineEdit.

  \walkthrough table/wineorder2/spinboxitem.cpp
  \skipto setContentFromEditor
  \printline setContentFromEditor
  \printuntil }

  (Code taken from \link wineorder2-example.html
  table/wineorder2/spinboxitem.cpp \endlink )

  \sa QTable::setCellContentFromEditor()
*/

void QTableItem::setContentFromEditor( QWidget *w )
{
    if ( w && w->inherits( "QLineEdit" ) )
	setText( ( (QLineEdit*)w )->text() );
}

/*! The alignment function returns how text contents of the cell are
  drawn. The default implementation aligns numbers to the right and
  other text to the left.

  \sa Qt::AlignmentFlags
*/

// ed: For consistency reasons a setAlignment() should be provided
// as well.

int QTableItem::alignment() const
{
    bool num;
    bool ok1 = FALSE, ok2 = FALSE;
    (void)txt.toInt( &ok1 );
    if ( !ok1 )
	(void)txt.toDouble( &ok2 ); // ### should be .-aligned
    num = ok1 || ok2;

    return ( num ? AlignRight : AlignLeft ) | AlignVCenter;
}

/*! If \a b is TRUE, the cell's text is wrapped into multiple lines,
  otherwise it will be written on one line.

  \sa wordWrap()
*/

void QTableItem::setWordWrap( bool b )
{
    wordwrap = b;
}

/*! If word wrap has been turned on for the cell in question,
   wordWrap() is TRUE, otherwise it returns FALSE.

  \sa setWordWrap()
*/

bool QTableItem::wordWrap() const
{
    return wordwrap;
}

/*! \internal */

void QTableItem::updateEditor( int oldRow, int oldCol )
{
    if ( edType != Always )
	return;
    if ( oldRow != -1 && oldCol != -1 )
	table()->clearCellWidget( oldRow, oldCol );
    if ( rw != -1 && cl != -1 )
	table()->setCellWidget( rw, cl, createEditor() );
}

/*! Returns the edit type of an item.

  This is determined at creation time of the item object.
  Items automatically created by the convenient functions
  QTable::setText() and QTable::setPixmap() default to
  QTableItem::OnTyping.

  \sa EditType QTableItem()
*/

QTableItem::EditType QTableItem::editType() const
{
    return edType;
}

/*! If it shouldn't be possible to replace the contents of the relevant cell
  with those of another QTableItem, set \a b to FALSE.

  Mind the difference between this property and the \l EditType that
  defines whether the user is able to change the \e content
  of a dedicated table item.

  \sa isReplaceable()
*/

void QTableItem::setReplaceable( bool b )
{
    tcha = b;
//ed: what the heck does tcha stand for?
}

/*! This function returns whether the relevant QTableItem can be replaced
 with another item or not. Only items that cover no more than one cell
 might be replaced.

 Note that this property is about exchanging one table item with
 another and not about whether the content of \e one item
 might be changed.

 \sa setReplaceable() EditType
*/

bool QTableItem::isReplaceable() const
{
    if ( rowspan > 1 || colspan > 1 )
	return FALSE;
    return tcha;
}

/*! This virtual function returns the key that should be used for
  sorting. The default implementation returns the text() of the
  relevant item.

  \sa QTable::setSorting
*/

QString QTableItem::key() const
{
    return text();
}

/*! This virtual function returns the size a cell needs to show its
  entire content.

  Custom table items will often require reimplementation of this function.
*/

QSize QTableItem::sizeHint() const
{
    if ( edType == Always && table()->cellWidget( rw, cl ) )
	return table()->cellWidget( rw, cl )->sizeHint();

    QSize s;
    if ( !pix.isNull() ) {
	s = pix.size();
	s.setWidth( s.width() + 2 );
    }

    if ( !wordwrap )
	return QSize( s.width() + table()->fontMetrics().width( text() ) + 10, QMAX( s.height(), table()->fontMetrics().height() ) );
    QRect r = table()->fontMetrics().boundingRect( 0, 0, table()->columnWidth( col() ), 0, wordwrap ? (alignment() | WordBreak) : alignment(), txt );
    return QSize( s.width() + r.width(), QMAX( s.height(), r.height() ) );
}

/*! Creates a multi-cell QTableItem covering \a rs rows and \a cs columns.
  The top left corner of the item is at the item's former position.

  \sa rowSpan() colSpan()
*/

void QTableItem::setSpan( int rs, int cs )
{
    int rrow = rw;
    int rcol = cl;
    if ( rowspan > 1 || colspan > 1 ) {
	table()->takeItem( this );
	table()->setItem( rrow, rcol, this );
    }

    rowspan = rs;
    colspan = cs;

    for ( int r = 0; r < rowspan; ++r ) {
	for ( int c = 0; c < colspan; ++c ) {
	    if ( r == 0 && c == 0 )
		continue;
	    table()->setItem( r + rw, c + cl, this );
	    rw = rrow;
	    cl = rcol;
	}
    }

    table()->updateCell( rw, cl );
}

/*! Returns the row span of an item, usually 1.

  \sa setSpan() colSpan()
*/

int QTableItem::rowSpan() const
{
    return rowspan;
}

/*! Returns the column span of the item, usually 1.

  \sa setSpan() rowSpan()
*/

int QTableItem::colSpan() const
{
    return colspan;
}

/*! This function determines \a r as the item's row. Usually you do not need to
  call this function.

  If the cell spans multiple rows, this function sets the top row and
  retains the height.

  \sa row() setCol() rowSpan()
*/

void QTableItem::setRow( int r )
{
    rw = r;
}

/*! Makes \a c the item's column. Usually you will not need to
  call this function.

  If the cell spans multiple columns, this function sets the leftmost
  column and retains the width.

  \sa col() setRow() colSpan()
*/

void QTableItem::setCol( int c )
{
    cl = c;
}

/*! Returns the row where the item is located. If the cell spans
  multiple rows, this function returns the top most of them.

  \sa col() setRow()
*/

int QTableItem::row() const
{
    return rw;
}

/*! Returns the column where the item is located. If the cell spans
  multiple columns, this function returns the leftmost column.

  \sa row() setCol()
*/

int QTableItem::col() const
{
    return cl;
}

/*! If \a b is TRUE, the item is enabled, otherwise it is disabled.

  A disabled item doesn't react to user input.

  \sa isEnabled()
*/

void QTableItem::setEnabled( bool b )
{
    if ( b == (bool)enabled )
	return;
    enabled = b;
    table()->updateCell( row(), col() );
}

/*! Returns whether the item is enabled or disabled.

  \sa setEnabled()
*/

bool QTableItem::isEnabled() const
{
    return (bool)enabled;
}

/*! \class QComboTableItem qtable.h

  \brief The QComboTableItem class implements a convenient class to
  use combo boxes in a QTable.

  \module table

  QComboTableItems fill table cells with combo boxes of the
  edit type WhenCurrent (c.f. \l{EditType}). As long as the cell does not
  have the focus it paints itself like a combo box, but without using a real
  QComboBox widget. When the item becomes the current one, it shows a
  real combo box, so that the user can edit the value.

  This implementation saves resources: although the user always sees
  a combo box there is no need for expensive QComboBox widgets
  to be visible all the time.

  The menu entries of the combo box are taken from a
  QStringList with the first list item serving as the top most
  combo box entry. After creation the original menu list may
  be exchanged using setStringList().

  Pixmaps can neither be used in menu entries
  nor can a QComboTableItem be accompanied by icons.

  The current entry in a combo table item might be selected by the user or via
  setCurrentItem(). Whilst currentText() allows to query the
  menu text of the currently active entry, text() returns
  the menu text of any desired combo box entry.

  Whether the user is allowed to edit menu entries is determined
  at construction time of a QComboTableItem object but can be
  changed later using setEditable().

  To fill a table cell with a QComboTableItems use QTable::setItem():

  \walkthrough table/euroconversion/converter.cpp
  \skipto QStringList
  \printline QStringList
  \skipto currencies =
  \printuntil setItem

  (Code taken from \link euroconvert-example.html
  table/euroconversion/converter.cpp \endlink )

  A combo box item is deleted like ordinary \l{QTableItem}s
  using QTable::clearCell().

  QComboTableItems can be distinguished from QTableItems and
  \l{QCheckTableItem}s using their Run Time Type Identification number
  (see rtti()).
*/

/*! Creates a combo box item for the \a table spreadsheet.

  The \a list argument determines the texts of the combo box entries.
  The resulting QComboTableItem has as many menu entries as
  \a list has members whilst the first \a list entry appears
  as the top most combo box entry. The menu texts might be changed later using
  setStringList().

  \a editable specifies whether the combo box entries can be edited by the user
  or not. QComboTableItems as an entity are of the edit type QTableItem::WhenCurrent.

  To use a combo box item as content for a QTable cell use
  QTable::setItem(), e.g.:

  \walkthrough table/small-table-demo/main.cpp
  \skipto QTable
  \printline QTable
  \skipto QStringList
  \printline QStringList
  \skipto i < numRows
  \printuntil TRUE
  \skipto setItem
  \printuntil }

  (Code taken from \link small-table-demo-example.html
  table/small-table-demo/main.cpp \endlink).

  Like regular \l{QTableItem}s combo table items might serve
  as content for a QTable which is not its parent. It can
  however not be used in more than one QTables at one time.

  By default QComboTableItems cannot be replaced by other items as
  they return FALSE for isReplaceable().

  \sa QTable::clearCell() EditType
*/

QComboTableItem::QComboTableItem( QTable *table, const QStringList &list, bool editable )
    : QTableItem( table, WhenCurrent, "" ), entries( list ), current( 0 ), edit( editable )
{
    setReplaceable( FALSE );
}

/*! Sets the entries of this QComboTableItem to the strings
   in the string list \a l.

  The first list entry appears as the top most combo box entry.
*/

void QComboTableItem::setStringList( const QStringList &l )
{
    entries = l;
    current = 0;
    table()->updateCell( row(), col() );
}

/*! \reimp */

QWidget *QComboTableItem::createEditor() const
{
    // create an editor - a combobox in our case
    ( (QComboTableItem*)this )->cb = new QComboBox( edit, table()->viewport() );
    cb->insertStringList( entries );
    cb->setCurrentItem( current );
    QObject::connect( cb, SIGNAL( activated( int ) ), table(), SLOT( doValueChanged() ) );
    return cb;
}

/*! \reimp */

void QComboTableItem::setContentFromEditor( QWidget *w )
{
    if ( w->inherits( "QComboBox" ) ) {
	QComboBox *cb = (QComboBox*)w;
	entries.clear();
	for ( int i = 0; i < cb->count(); ++i )
	    entries << cb->text( i );
	current = cb->currentItem();
	setText( currentText() );
    }
}

/*! \reimp */

void QComboTableItem::paint( QPainter *p, const QColorGroup &cg,
			   const QRect &cr, bool selected )
{
    int w = cr.width();
    int h = cr.height();

    table()->style().drawComboButton( p, 0, 0, w, h, cg, FALSE, TRUE, TRUE, selected ? &cg.brush( QColorGroup::Highlight ) : 0  );
    QRect tmpR = table()->style().comboButtonRect( 0, 0, w, h );
    QRect textR( tmpR.x() + 1, tmpR.y() + 1, tmpR.width() - 2, tmpR.height() - 2 );

    if ( selected )
	p->setPen( cg.highlightedText() );
    else
	p->setPen( cg.text() );
    p->drawText( textR, wordWrap() ? ( alignment() | WordBreak ) : alignment(), currentText() );
}

/*! Makes the combo box menu entry no. \a i the current one:

  \walkthrough table/small-table-demo/main.cpp
  \skipto QComboTableItem
  \printuntil setCurrentItem

  (Code taken from \link small-table-demo-example.html
  table/small-table-demo/main.cpp \endlink )

  \sa currentItem()
*/

void QComboTableItem::setCurrentItem( int i )
{
    current = i;
    setText( currentText() );
    table()->updateCell( row(), col() );
}

/*! \overload

  Makes the combo box entry reading \a s the current one.

  \sa currentItem()
*/

void QComboTableItem::setCurrentItem( const QString &s )
{
    int i = entries.findIndex( s );
    if ( i != -1 )
	setCurrentItem( i );
}

/*! Returns the number of the current item.

  \sa setCurrentItem()
*/

int QComboTableItem::currentItem() const
{
    return current;
}

/*! Returns the text of the currently selected combo box entry.

  \sa currentItem() text()
*/

QString QComboTableItem::currentText() const
{
    return *entries.at( current );
}

/*! Returns the number of combo box entries.
*/

int QComboTableItem::count() const
{
    return entries.count();
}

/*! Returns the text of combo box entry no. \a i.

  \sa currentText()
*/

QString QComboTableItem::text( int i ) const
{
    return *entries.at( i );
}

/*! Determines whether the entries of this combo table item
  can be changed by the user (\a b is TRUE) or not (\a b
  is FALSE).

  \sa isEditable()
*/

void QComboTableItem::setEditable( bool b )
{
    edit = b;
}

/*! Returns whether combo box entries of \e this item can
  be changed by the user or not.

  \sa setEditable()
*/

bool QComboTableItem::isEditable() const
{
    return edit;
}

/*! \fn int QComboTableItem::rtti() const

  For QComboTableItems this function returns a Run Time Identification
  number of 1.

  \sa QTableItem::rtti()
*/

/*! \class QCheckTableItem qtable.h

  \brief The QCheckTableItem class implements a convenient class to
  use check boxes in QTables.

  \module table

  To fill a table cell with a check box instead of the usual
  text and pixmap one creates a QCheckTableItem
  object and uses QTable::setItem() to make it the content of
  a table cell. This results in an item that paints itself like a check
  box but is no real QCheckBox when the relevant cell does not have the focus.
  When the item becomes current, it shows a
  QCheckBox, so that the user can check or uncheck it.

  This way the user always perceives the item as a
  check box without the need of wasting resources by having a real QCheckBox
  widget present all the time.

  QCheckTableItems are of the edit type WhenCurrent.
  To change the check label string use setText().
  Check box items may however not include a pixmap.

  QCheckTableItems can be distinguished from \l{QTableItem}s and
  \l{QComboTableItem}s using their Run Time Type Identification number.

  \sa rtti() EditType
*/

/*! Creates a QCheckTableItem of the EditType WhenCurrent
  as a child of \a table. The check box
  label is set to the string \a txt, for example

  \walkthrough table/small-table-demo/main.cpp
  \skipto QTable
  \printline QTable
  \skipto new QCheckTableItem
  \printline new QCheckTableItem

  (Code taken from \link small-table-demo-example.html
  table/small-table-demo/main.cpp \endlink).

  By default QCheckTableItem objects are unchecked. To set
  their check marks use setChecked().
  \a txt can be changed later using setText().

  QTable::setItem() makes check box items the content of a
  QTable that may or may not be the item's parent. It is
  however impossible to use one item in more than one table
  at a time.
*/

QCheckTableItem::QCheckTableItem( QTable *table, const QString &txt )
    : QTableItem( table, WhenCurrent, txt ), checked( FALSE )
{
}

/*! \reimp */

QWidget *QCheckTableItem::createEditor() const
{
    // create an editor - a combobox in our case
    ( (QCheckTableItem*)this )->cb = new QCheckBox( table()->viewport() );
    cb->setChecked( checked );
    cb->setText( text() );
    cb->setBackgroundMode( table()->viewport()->backgroundMode() );
    QObject::connect( cb, SIGNAL( toggled( bool ) ), table(), SLOT( doValueChanged() ) );
    return cb;
}

/*! \reimp */

void QCheckTableItem::setContentFromEditor( QWidget *w )
{
    if ( w->inherits( "QCheckBox" ) ) {
	QCheckBox *cb = (QCheckBox*)w;
	checked = cb->isChecked();
    }
}

/*! \reimp */

void QCheckTableItem::paint( QPainter *p, const QColorGroup &cg,
				const QRect &cr, bool selected )
{
    p->fillRect( 0, 0, cr.width(), cr.height(),
		 selected ? cg.brush( QColorGroup::Highlight )
		          : cg.brush( QColorGroup::Base ) );

    int w = cr.width();
    int h = cr.height();
    QSize sz = table()->style().indicatorSize();
    table()->style().drawIndicator( p, 0, ( h - sz.height() ) / 2, sz.width(), sz.height(), cg, checked ? QButton::On : QButton::Off, FALSE, TRUE );
    int x = sz.width() + 6;
    w = w - x;
    if ( selected )
	p->setPen( cg.highlightedText() );
    else
	p->setPen( cg.text() );
    p->drawText( x, 0, w, h, wordWrap() ? ( alignment() | WordBreak ) : alignment(), text() );
}

/*! Sets the item's check mark if \a b is TRUE and unchecks the
  check box if \a b is FALSE.

  \sa isChecked()
*/

void QCheckTableItem::setChecked( bool b )
{
    checked = b;
    table()->updateCell( row(), col() );
}

/*! Returns whether the item is checked or not.

  \sa setChecked()
*/

bool QCheckTableItem::isChecked() const
{
    return checked;
}

/*! \fn int QCheckTableItem::rtti() const

  Returns the Run Time Identification number of \e this object
  which is 2 for QCheckTableItems.

  \sa QTableItem::rtti()
*/


/*! \class QTable qtable.h
  \module table

  \brief The QTable class provides a flexible and editable table widget.

  This allows programmers to easily incorporate spreadsheet functionality
  into their applications.

  A QTable widget consists of a table grid of cells created by
  interweaving
  columns and rows and framed by a horizontal header above and
  a vertical header to the left. The default headers simply show
  consecutive row and column numbers beginning with \e 1 denoting the
  first row or column. Note that this visual numeration
  differs from the internal numbering within the Qt program:
  A QTable of numRows() rows and numCols() columns consists of rows and
  columns from \e 0 to \e{numRows() - 1} or \e{numCols() - 1}, respectively.

  QTable has been designed to use no more memory than strictly
  needed. Thus, for an empty cell, no memory at all is allocated.
  The table content is by default stored in individual \l{QTableItem}s
  attached to the cells in question.

  To begin with all cells of a QTable object are empty.
  In order to add data, there are two possibilities:

  For common activities like setting cell text or pixmaps
  the convenient functions setText() and setPixmap()
  point out a first way of adding content to
  a cell. When used directly upon a table object they
  implicitly create a QTableItem that can be accessed via the
  item() function.

  The other and more general approach
  is to explicitly create a QTableItem and use
  setItem() to make it the content of the specified cell.

  \walkthrough table/wineorder/productlist.cpp
  \skipto discount
  \printline QTableItem
  \printuntil setItem

  (Code taken from \link wineorder-example.html
   table/wineorder/productlist.cpp \endlink)

  Cells can also contain combo boxes and check boxes. In this
  case QComboTableItem and QCheckTableItem objects are
  linked to the relevant cells using setItem().

  To clear a cell use clearCell().

  QTable supports various methods for selecting cells, both with
  keyboard and mouse, for example range selection or column and
  row selection via appropriate header cells. You can add and remove
  selections using addSelection() and removeSelection(), respectively, and
  gather information about current selections by means of
  numSelections(), selection(), and currentChanged().

  QTable also offers an API for sorting columns. See setSorting(),
  sortColumn() and QTableItem::key() for details.

  Cell editing can be done in two different ways: You can offer an
  edit widget where the user enters data that will afterwards replace the
  current cell content. Or he or she can directly change the data
  with an in-place editor located directly in the cell.
  If you don't want the user to replace the cell content, but still wish to
  allow for editing of the current
  data, simply set QTableItem::isReplaceable() to FALSE.

  When a user starts typing text in-place editing for the
  current cell is invoked in replace modus. Additionally, in-place editing
  (editing mode) starts as soon as he or she double-clicks a cell.

  Sometimes it is required that a cell always shows an editor, that the
  editor shows off as soon as the relevant cell receives the focus, or
  that the item shouldn't be editable at all. This \e{edit type} has to be
  specified in the constructor of a QTableItem and can be queried by
  QTableItem::editType().

  In-place editing is invoked by beginEdit(). This function creates
  the editor widget for the required cell (see createEditor() for
  detailed information) and shows it at the appropriate location.

  As soon as the user finishes editing endEdit() is called. Have a
  look at the endEdit() documentation for further information
  on how content is transferred from the editor to the item and how the
  editor is destroyed.

  In-place editing is implemented in an abstract way to make sure that
  custom edit widgets for certain cells or cell types can be written
  easily. To achieve this it is possible to place widgets in cells. See
  setCellWidget(), clearCellWidget() and cellWidget() for further
  details.

  In order to prevent a cell not containing a QTableItem from being
  edited, you have to reimplement createEditor(). This function should
  return 0 for cells that must not be edited at all.

  If you wish to draw custom content in a cell you have to implement your
  own subclass of QTableItem and reimplement the QTableItem::paint()
  method.

  It is possible to use QTable without QTableItems. In this case
  you have to reimplement createEditor() and
  setCellContentFromEditor() to allow for in-place editing without
  QTableItems.
  The documentation of these two functions explains
  details you need to know for that matter.

  If your application already stores its data in a way that allocating
  a QTableItem for each data containing cell seems inappropriate, you
  can reimplement QTable::paintCell() and draw the contents directly.
  You should also reimplement QTable::paintCell() if you wish to change
  the alignment of your items in the QTable.

  This approach will however prevent you
  from using functions like setText() etc. unless you reimplement them as
  well. In this case remember to repaint the cells using updateCell()
  after each change.
  To make sure you don't waste memory, read the
  documentation of resizeData().
*/

/*! \fn void QTable::currentChanged( int row, int col )

  This signal is emitted when the current cell has been changed to \a
  row, \a col.
*/

/*! \fn void QTable::valueChanged( int row, int col )

  This signal is emitted when the user edited the cell \a row, \a col.
*/

/*! \fn int QTable::currentRow() const

  Returns the current row.

  Note that row counting starts with zero.

  \sa currentColumn()
*/

/*! \fn int QTable::currentColumn() const

  Returns the current column.

  Note that column counting starts with zero.

  \sa currentRow()
*/

/*! \enum QTable::EditMode


  \value NotEditing  No cell is currently being edited.
  \value Editing  A cell is currently being edited. To start with the
                  user was presented with the old content.
  \value Replacing  A cell is currently being edited, and was cleared at the time
         editing started.
*/

/*! \enum QTable::SelectionMode


  \value NoSelection  No cell can be selected by the user.
  \value Single  The user may select one range of cells only.
  \value Multi  Multi-range selections are possible.
*/

/*! \fn void QTable::clicked( int row, int col, int button, const QPoint &mousePos )

  This signal is emitted as soon as a user clicks on \a row and \a col
  using mouse button \a button.  The actual mouse position is passed as
  \a mousePos.
*/

/*! \fn void QTable::doubleClicked( int row, int col, int button, const QPoint &mousePos )

  A double-click with \a button emits this signal, where \a
  row and \a col denote the position of the cell.
  The actual mouse position is passed as \a mousePos.
*/

/*! \fn void QTable::pressed( int row, int col, int button, const QPoint &mousePos )

  This signal is emitted whenever the mouse button \a button is pressed in
  the cell located at  position \a row , \a col.

  The actual mouse position is passed as \a mousePos.
*/

/*! \fn void QTable::selectionChanged()

  Whenever a selection changes, this signal is emitted.

  \sa QTableSelection
*/

/*!
  \fn void QTable::contextMenu( int row, int col, const QPoint & pos )

  This signal is emitted when the user invokes a context menu with the
  right mouse button or with special system keys, with \a row and \a
  col being the cell under the mouse cursor or the current cell,
  respectively.

  \a pos is the position for the context menu in the global coordinate
  system.
*/

/*! Creates an empty table object with the name \a name as a child widget of \a parent.

  Call \l setNumRows() and \l setNumCols() to determine the table size
  before populating the table using setItem(), setText() and setPixmap().

  Performance is boosted by modifying the table widget's window-system properties
  (widget flags) using setWFlags() so that only part
  of the QTableItem children is redrawn.  This may be unsuitable for custom
  QTableItem classes, in which case \c WNorthWestGravity and \c WRepaintNoErase
  should be cleared.

  \sa QWidget::clearWFlags() Qt::WidgetFlags
*/

QTable::QTable( QWidget *parent, const char *name )
    : QScrollView( parent, name, WRepaintNoErase | WNorthWestGravity ),
      currentSel( 0 ), lastSortCol( -1 ), sGrid( TRUE ), mRows( FALSE ), mCols( FALSE ),
      asc( TRUE ), doSort( TRUE ), readOnly( FALSE )
{
    init( 0, 0 );
}

/*! Constructs an empty table named \a name with a range of \a numRows x \a numCols cells.
  The new table object is a child of \a parent.

  To fill the table with content create QTableItem, QComboTableItem and/or
  QCheckTableItem objects and link them to the cells using setItem(). Text
  and pixmaps can be added via setText() and setPixmap().

  Performance is boosted by modifying widget flags like \c WNorthWestGravity
  and \c WRepaintNoErase so that only part of
  the QTableItem children is redrawn.  This may be unsuitable for custom
  QTableItem classes, in which case the widget flags should be reset using
  QWidget::setWFlags().

  \sa QWidget::clearWFlags() Qt::WidgetFlags
*/

QTable::QTable( int numRows, int numCols, QWidget *parent, const char *name )
    : QScrollView( parent, name, WRepaintNoErase | WNorthWestGravity ),
      currentSel( 0 ), lastSortCol( -1 ), sGrid( TRUE ), mRows( FALSE ), mCols( FALSE ),
      asc( TRUE ), doSort( TRUE ), readOnly( FALSE )
{
    init( numRows, numCols );
}

/*! \internal
*/

void QTable::init( int rows, int cols )
{
    setDragAutoScroll( FALSE );
    d = 0;
    shouldClearSelection = FALSE;
    dEnabled = FALSE;
    roRows.setAutoDelete( TRUE );
    roCols.setAutoDelete( TRUE );
    setSorting( FALSE );

    viewport()->setFocusProxy( this );
    viewport()->setFocusPolicy( WheelFocus );
    mousePressed = FALSE;

    selMode = Multi;

    contents.setAutoDelete( TRUE );
    widgets.setAutoDelete( TRUE );

    // Enable clipper and set background mode
    enableClipper( TRUE );
    viewport()->setBackgroundMode( PaletteBase );
    setResizePolicy( Manual );
    selections.setAutoDelete( TRUE );

    // Create headers
    leftHeader = new QTableHeader( rows, this, this );
    leftHeader->setOrientation( Vertical );
    leftHeader->setTracking( TRUE );
    leftHeader->setMovingEnabled( TRUE );
    topHeader = new QTableHeader( cols, this, this );
    topHeader->setOrientation( Horizontal );
    topHeader->setTracking( TRUE );
    topHeader->setMovingEnabled( TRUE );
    setMargins( 30, fontMetrics().height() + 4, 0, 0 );

    topHeader->setUpdatesEnabled( FALSE );
    leftHeader->setUpdatesEnabled( FALSE );
    // Initialize headers
    int i = 0;
    for ( i = 0; i < numCols(); ++i ) {
	topHeader->setLabel( i, QString::number( i + 1 ) );
	topHeader->resizeSection( i, 100 );
    }
    for ( i = 0; i < numRows(); ++i ) {
	leftHeader->setLabel( i, QString::number( i + 1 ) );
	leftHeader->resizeSection( i, 20 );
    }
    topHeader->setUpdatesEnabled( TRUE );
    leftHeader->setUpdatesEnabled( TRUE );

    // Prepare for contents
    contents.setAutoDelete( FALSE );

    // Connect header, table and scrollbars
    connect( horizontalScrollBar(), SIGNAL( valueChanged( int ) ),
	     topHeader, SLOT( setOffset( int ) ) );
    connect( verticalScrollBar(), SIGNAL( valueChanged( int ) ),
	     leftHeader, SLOT( setOffset( int ) ) );
    connect( topHeader, SIGNAL( sectionSizeChanged( int ) ),
	     this, SLOT( columnWidthChanged( int ) ) );
    connect( topHeader, SIGNAL( indexChange( int, int, int ) ),
	     this, SLOT( columnIndexChanged( int, int, int ) ) );
    connect( topHeader, SIGNAL( sectionClicked( int ) ),
	     this, SLOT( columnClicked( int ) ) );
    connect( leftHeader, SIGNAL( sectionSizeChanged( int ) ),
	     this, SLOT( rowHeightChanged( int ) ) );
    connect( leftHeader, SIGNAL( indexChange( int, int, int ) ),
	     this, SLOT( rowIndexChanged( int, int, int ) ) );

    // Initialize variables
    autoScrollTimer = new QTimer( this );
    connect( autoScrollTimer, SIGNAL( timeout() ),
	     this, SLOT( doAutoScroll() ) );
    curRow = curCol = 0;
    edMode = NotEditing;
    editRow = editCol = -1;

    installEventFilter( this );

    // Initial size
    resize( 640, 480 );
}

/*!  Destructor. Clears all resources used by a QTable object.
*/

QTable::~QTable()
{
    delete d;
    contents.setAutoDelete( TRUE );
    contents.clear();
    widgets.clear();
}

/*! Makes the entire table read-only (i.e. not editable) if \a b is TRUE.

  Note that if \a b is TRUE this setting overrides
  individual settings of the QComboTableItem::isEditable() property.

  EditType settings of individual \l{QTableItem}s reading \c Always
  are not affected.

  \sa isRowReadOnly() setRowReadOnly() setColumnReadOnly()
*/

void QTable::setReadOnly( bool b )
{
    readOnly = b;
}

/*! Marks the row \a row read-only if \a ro is TRUE.

  Note that you can't overwrite a isReadOnly() TRUE by
  setting \a ro to FALSE for individual rows.

  \a row being TRUE overwrites contrary QComboTableItem::isEditable()
  settings.

  A cell situated in a read-only column and an editable row
  is not writable unless its QTableItem::EditType suggests so.

  \sa isRowReadOnly() setColumnReadOnly() setReadOnly()
*/

void QTable::setRowReadOnly( int row, bool ro )
{
    if ( ro )
	roRows.replace( row, new int( 0 ) );
    else
	roRows.remove( row );
}

/*! Makes the column \a col read-only if \a ro is TRUE.

  \walkthrough table/euroconversion/converter.cpp
  \skipto setColumnReadOnly
  \printline setColumnReadOnly

  (Code taken from \link euroconvert-example.html table/euroconversion/converter.cpp
  \endlink)

  Note that a cell is non-editable if either
  of isRowReadOnly(), isColumnReadOnly() or isReadOnly()
  returns TRUE. For QComboTableItems this is the case
  no matter what the individual QComboTableItem::isEditable()
  returns. QTableItems neglect this setting if their
  QTableItem::EditType is set to \c Always.

  \sa isColumnReadOnly() setRowReadOnly() setReadOnly()
*/

void QTable::setColumnReadOnly( int col, bool ro )
{
    if ( ro )
	roCols.replace( col, new int( 0 ) );
    else
	roCols.remove( col );
}

/*! Returns whether the table is read-only (not editable) or not.

  \sa setReadOnly() isColumnReadOnly() isRowReadOnly()
*/

bool QTable::isReadOnly() const
{
    return readOnly;
}

/*! Returns whether the row \a row is read-only or not.

  Note that a combination of isRowReadOnly() FALSE and isReadOnly() TRUE
  means that the row is read-only in practice.

  \sa setRowReadOnly isColumnReadOnly()
*/

bool QTable::isRowReadOnly( int row ) const
{
    return (roRows.find( row ) != 0);
}

/*! Returns whether the column \a col is read-only or not.

  Note that a combination of isColumnReadOnly() FALSE and isReadOnly() TRUE
  means that the column is read-only in practice.

  \sa setColumnReadOnly() isRowReadOnly()
*/

bool QTable::isColumnReadOnly( int col ) const
{
    return (roCols.find( col ) != 0);
}

/*! Sets the table's selection mode to \a mode. By default
  multi-range selections (\c Multi) are allowed.

  \sa SelectionMode selectionMode()
*/

void QTable::setSelectionMode( SelectionMode mode )
{
    selMode = mode;
}

/*! Reveals the current selection mode.

  \sa SelectionMode setSelectionMode()
*/

QTable::SelectionMode QTable::selectionMode() const
{
    return selMode;
}

/*! Returns the top header of the table.

  To modify the header text use QHeader::setLabel(), e.g.:

  \walkthrough table/euroconversion/converter.cpp
  \skipto horizontalHeader()
  \printline horizontalHeader()->setLabel( 0

  (Code taken from \link euroconvert-example.html table/euroconversion/converter.cpp
  \endlink)

  \sa verticalHeader() setTopMargin() QHeader
*/

QHeader *QTable::horizontalHeader() const
{
    return (QHeader*)topHeader;
}

/*! Returns the outer left vertical table header.

  Its section titles can be modified using QHeader::setLabel().

  To hide the vertical header entirely use the following sequence:

  \walkthrough table/euroconversion/converter.cpp
  \skipto setLeftMargin
  \printline setLeftMargin
  \printline hide()

  (Code taken from \link euroconvert-example.html table/euroconversion/converter.cpp
  \endlink)

  \sa horizontalHeader()  setLeftMargin() QHeader
*/

QHeader *QTable::verticalHeader() const
{
    return (QHeader*)leftHeader;
}

/*! If \a b is TRUE, the table grid is shown, otherwise not. The
  default is TRUE.

  \sa showGrid()
*/

void QTable::setShowGrid( bool b )
{
    if ( sGrid == b )
	return;
    sGrid = b;
    viewport()->repaint( FALSE );
}

/*! Returns whether the table grid shows up or not.

  \sa setShowGrid()
*/

bool QTable::showGrid() const
{
    return sGrid;
}

/*! If \a b is set to TRUE, columns can be moved by the user.

  \sa columnMovingEnabled() setRowMovingEnabled()
*/

void QTable::setColumnMovingEnabled( bool b )
{
    mCols = b;
}

/*! Returns whether columns can be moved by the user.

  \sa setColumnMovingEnabled() rowMovingEnabled()
*/

bool QTable::columnMovingEnabled() const
{
    return mCols;
}

/*! If \a b is set to TRUE, rows can be moved by the user.

  \sa  rowMovingEnabled() setColumnMovingEnabled()
*/

void QTable::setRowMovingEnabled( bool b )
{
    mRows = b;
}

/*! Returns whether rows can be moved by the user.

  \sa  setRowMovingEnabled() columnMovingEnabled()
*/

bool QTable::rowMovingEnabled() const
{
    return mRows;
}

/*! This is called when QTable's internal array needs to be resized
  to \a len elements.

  If you don't use QTableItems you should reimplement this as an empty
  method in order to avoid waste of memory. In addition, you have to
  reimplement item(), setItem(), and clearCell() as empty functions.
  As soon as you enable sorting or allow the user to change rows or
  columns (see setRowMovingEnabled(), setColumnMovingEnabled()), you
  are strongly advised to reimplement swapColumns(), swapRows(), and
  swapCells() to work with your data.
*/

void QTable::resizeData( int len )
{
    contents.resize( len );
}

/*! Swaps data of \a row1 and \a row2.

  This is used by sorting
  mechanisms or when the user changes the order of the rows. If you
  don't use QTableItems you might wish to reimplement this function.

  \sa swapColumns() swapCells()
*/

void QTable::swapRows( int row1, int row2 )
{
    QVector<QTableItem> tmpContents;
    tmpContents.resize( numCols() );
    QVector<QWidget> tmpWidgets;
    tmpWidgets.resize( numCols() );
    int i;

    contents.setAutoDelete( FALSE );
    widgets.setAutoDelete( FALSE );
    for ( i = 0; i < numCols(); ++i ) {
	QTableItem *i1, *i2;
	i1 = item( row1, i );
	i2 = item( row2, i );
	if ( i1 || i2 ) {
	    tmpContents.insert( i, i1 );
	    contents.remove( indexOf( row1, i ) );
	    contents.insert( indexOf( row1, i ), i2 );
	    contents.remove( indexOf( row2, i ) );
	    contents.insert( indexOf( row2, i ), tmpContents[ i ] );
	    if ( contents[ indexOf( row1, i ) ] )
		contents[ indexOf( row1, i ) ]->setRow( row1 );
	    if ( contents[ indexOf( row2, i ) ] )
		contents[ indexOf( row2, i ) ]->setRow( row2 );
	}

	QWidget *w1, *w2;
	w1 = cellWidget( row1, i );
	w2 = cellWidget( row2, i );
	if ( w1 || w2 ) {
	    tmpWidgets.insert( i, w1 );
	    widgets.remove( indexOf( row1, i ) );
	    widgets.insert( indexOf( row1, i ), w2 );
	    widgets.remove( indexOf( row2, i ) );
	    widgets.insert( indexOf( row2, i ), tmpWidgets[ i ] );
	}
    }
    contents.setAutoDelete( FALSE );
    widgets.setAutoDelete( TRUE );

    updateRowWidgets( row1 );
    updateRowWidgets( row2 );
    if ( curRow == row1 )
	curRow = row2;
    else if ( curRow == row2 )
	curRow = row1;
    if ( editRow == row1 )
	editRow = row2;
    else if ( editRow == row2 )
	editRow = row1;
}

/*! Sets the left margin to \a m pixels.

  To get rid of the left header entirely, use the following code:

  \walkthrough table/euroconversion/converter.cpp
  \skipto setLeftMargin
  \printuntil verticalHeader()->hide();

  (Code taken from \link euroconvert-example.html table/euroconversion/converter.cpp
  \endlink)

  \sa leftMargin() setTopMargin() verticalHeader()
*/

void QTable::setLeftMargin( int m )
{
    setMargins( m, topMargin(), rightMargin(), bottomMargin() );
    updateGeometries();
}

/*! Sets the top margin to \a m pixels.

  To get rid of the top header entirely set the top margin to
  zero and QHeader::hide() the horizontalHeader().

  \sa topMargin() setLeftMargin()
*/

void QTable::setTopMargin( int m )
{
    setMargins( leftMargin(), m, rightMargin(), bottomMargin() );
    updateGeometries();
}

/*! Exchanges \a col1 with \a col2 and vice versa.

  This is useful for sorting, and it allows the user to rearrange
  columns in a different order.

  If you don't use QTableItems you will probably
  need to reimplement this function.

  \sa swapCells()
*/

void QTable::swapColumns( int col1, int col2 )
{
    QVector<QTableItem> tmpContents;
    tmpContents.resize( numRows() );
    QVector<QWidget> tmpWidgets;
    tmpWidgets.resize( numRows() );
    int i;

    contents.setAutoDelete( FALSE );
    widgets.setAutoDelete( FALSE );
    for ( i = 0; i < numRows(); ++i ) {
	QTableItem *i1, *i2;
	i1 = item( i, col1 );
	i2 = item( i, col2 );
	if ( i1 || i2 ) {
	    tmpContents.insert( i, i1 );
	    contents.remove( indexOf( i, col1 ) );
	    contents.insert( indexOf( i, col1 ), i2 );
	    contents.remove( indexOf( i, col2 ) );
	    contents.insert( indexOf( i, col2 ), tmpContents[ i ] );
	    if ( contents[ indexOf( i, col1 ) ] )
		contents[ indexOf( i, col1 ) ]->setCol( col1 );
	    if ( contents[ indexOf( i, col2 ) ] )
		contents[ indexOf( i, col2 ) ]->setCol( col2 );
	}

	QWidget *w1, *w2;
	w1 = cellWidget( i, col1 );
	w2 = cellWidget( i, col2 );
	if ( w1 || w2 ) {
	    tmpWidgets.insert( i, w1 );
	    widgets.remove( indexOf( i, col1 ) );
	    widgets.insert( indexOf( i, col1 ), w2 );
	    widgets.remove( indexOf( i, col2 ) );
	    widgets.insert( indexOf( i, col2 ), tmpWidgets[ i ] );
	}
    }
    contents.setAutoDelete( FALSE );
    widgets.setAutoDelete( TRUE );

    columnWidthChanged( col1 );
    columnWidthChanged( col2 );
    if ( curCol == col1 )
	curCol = col2;
    else if ( curCol == col2 )
	curCol = col1;
    if ( editCol == col1 )
	editCol = col2;
    else if ( editCol == col2 )
	editCol = col1;
}

/*! Swaps the content of the cells \a row1, \a col1 and \a row2, \a
  col2.

  This function is used for sorting cells.

  \sa swapColumns() swapRows()
*/

void QTable::swapCells( int row1, int col1, int row2, int col2 )
{
    contents.setAutoDelete( FALSE );
    widgets.setAutoDelete( FALSE );
    QTableItem *i1, *i2;
    i1 = item( row1, col1 );
    i2 = item( row2, col2 );
    if ( i1 || i2 ) {
	QTableItem *tmp = i1;
	contents.remove( indexOf( row1, col1 ) );
	contents.insert( indexOf( row1, col1 ), i2 );
	contents.remove( indexOf( row2, col2 ) );
	contents.insert( indexOf( row2, col2 ), tmp );
	if ( contents[ indexOf( row1, col1 ) ] ) {
	    contents[ indexOf( row1, col1 ) ]->setRow( row1 );
	    contents[ indexOf( row1, col1 ) ]->setCol( col1 );
	}
	if ( contents[ indexOf( row2, col2 ) ] ) {
	    contents[ indexOf( row2, col2 ) ]->setRow( row2 );
	    contents[ indexOf( row2, col2 ) ]->setCol( col2 );
	}
    }

    QWidget *w1, *w2;
    w1 = cellWidget( row1, col1 );
    w2 = cellWidget( row2, col2 );
    if ( w1 || w2 ) {
	QWidget *tmp = w1;
	widgets.remove( indexOf( row1, col1 ) );
	widgets.insert( indexOf( row1, col1 ), w2 );
	widgets.remove( indexOf( row2, col2 ) );
	widgets.insert( indexOf( row2, col2 ), tmp );
    }

    updateRowWidgets( row1 );
    updateRowWidgets( row2 );
    updateColWidgets( col1 );
    updateColWidgets( col2 );
    contents.setAutoDelete( FALSE );
    widgets.setAutoDelete( TRUE );
}

/*! Draws the table contents on the painter \a p. This function is
  optimized to exclusively draw the cells inside the \a cw pixels
  wide and \a ch pixels high clipping
  rectangle at position \a cx, \a cy.

  Additionally, drawContents() highlights the current cell.
*/

void QTable::drawContents( QPainter *p, int cx, int cy, int cw, int ch )
{
    int colfirst = columnAt( cx );
    int collast = columnAt( cx + cw );
    int rowfirst = rowAt( cy );
    int rowlast = rowAt( cy + ch );

    if ( rowfirst == -1 || colfirst == -1 ) {
	paintEmptyArea( p, cx, cy, cw, ch );
	return;
    }

    if ( rowlast == -1 )
	rowlast = numRows() - 1;
    if ( collast == -1 )
	collast = numCols() - 1;

    // Go through the rows
    for ( int r = rowfirst; r <= rowlast; ++r ) {
	// get row position and height
	int rowp = rowPos( r );
	int rowh = rowHeight( r );

	// Go through the columns in row r
	// if we know from where to where, go through [colfirst, collast],
	// else go through all of them
	for ( int c = colfirst; c <= collast; ++c ) {
	    // get position and width of column c
	    int colp, colw;
	    colp = columnPos( c );
	    colw = columnWidth( c );
	    int oldrp = rowp;
	    int oldrh = rowh;

	    QTableItem *itm = item( r, c );
	    if ( itm &&
		 ( itm->colSpan() > 1 || itm->rowSpan() > 1 ) ) {
		bool goon = r == itm->row() && c == itm->col() ||
			r == rowfirst && c == itm->col() ||
			r == itm->row() && c == colfirst;
		if ( !goon )
		    continue;
		rowp = rowPos( itm->row() );
		rowh = 0;
		int i;
		for ( i = 0; i < itm->rowSpan(); ++i )
		    rowh += rowHeight( i + itm->row() );
		colp = columnPos( itm->col() );
		colw = 0;
		for ( i = 0; i < itm->colSpan(); ++i )
		    colw += columnWidth( i + itm->col() );
	    }

	    // Translate painter and draw the cell
	    p->translate( colp, rowp );
	    paintCell( p, r, c, QRect( colp, rowp, colw, rowh ), isSelected( r, c ) );
	    p->translate( -colp, -rowp );

	    rowp = oldrp;
	    rowh = oldrh;
	}
    }

    // draw indication of current cell
    QRect focusRect = cellGeometry( curRow, curCol );
    p->translate( focusRect.x(), focusRect.y() );
    paintFocus( p, focusRect );
    p->translate( -focusRect.x(), -focusRect.y() );

    // Paint empty rects
    paintEmptyArea( p, cx, cy, cw, ch );
}

/*! Paints the cell at position \a row, \a col on the painter \a
  p. The painter has already been translated to the cell's origin. \a
  cr describes the cell coordinates in the content coordinate system.

  If \a selected is TRUE the cell is highlighted, otherwise not.

  If you want to draw custom cell content you have to reimplement
  paintCell() to do the custom drawing, or subclass QTableItem
  and reimplement QTableItem::paint().

  If you want to change the alignment of your items then you will need to
  reimplement paintCell().

  Reimplementing this function is probably better e.g. for data you
  retrieve from a database and draw at once, while using
  QTableItem::paint() has its advantages for example if you wish these data
  to be stored in a data structure in the table.
*/

void QTable::paintCell( QPainter* p, int row, int col,
			const QRect &cr, bool selected )
{
    if ( selected &&
	 row == curRow &&
	 col == curCol )
	selected = FALSE;

    int w = cr.width();
    int h = cr.height();
    int x2 = w - 1;
    int y2 = h - 1;


    QTableItem *itm = item( row, col );
    if ( itm ) {
	p->save();
	QColorGroup cg = colorGroup();
	if ( !itm->isEnabled() )
	    cg = palette().disabled();
	itm->paint( p, cg, cr, selected );
	p->restore();
    } else {
	p->fillRect( 0, 0, w, h, selected ? colorGroup().brush( QColorGroup::Highlight ) : colorGroup().brush( QColorGroup::Base ) );
    }

    if ( sGrid ) {
	// Draw our lines
	QPen pen( p->pen() );
	p->setPen( colorGroup().mid() );
	p->drawLine( x2, 0, x2, y2 );
	p->drawLine( 0, y2, x2, y2 );
	p->setPen( pen );
    }
}

/*! Draws the focus rectangle of the current cell (see currentRow(),
  currentColumn()).

  The painter \a p is already translated to the
  cell's origin, while \a cr specifies the cell's geometry in content
  coordinates.
*/

void QTable::paintFocus( QPainter *p, const QRect &cr )
{
    if ( !hasFocus() && !viewport()->hasFocus() )
	return;
    QRect focusRect( 0, 0, cr.width(), cr.height() );
    p->setPen( QPen( black, 1 ) );
    p->setBrush( NoBrush );
    bool focusEdited = FALSE;
    if ( edMode != NotEditing &&
	 curRow == editRow && curCol == editCol )
	focusEdited = TRUE;
    if ( !focusEdited ) {
	QTableItem *i = item( curRow, curCol );
	focusEdited = ( i &&
			( i->editType() == QTableItem::Always ||
			  ( i->editType() == QTableItem::WhenCurrent &&
			    curRow == i->row() && curCol == i->col() ) ) );
    }
    p->drawRect( focusRect.x(), focusRect.y(), focusRect.width() - 1, focusRect.height() - 1 );
    if ( !focusEdited ) {
	p->drawRect( focusRect.x() - 1, focusRect.y() - 1, focusRect.width() + 1, focusRect.height() + 1 );
    } else {
	p->drawRect( focusRect.x() - 1, focusRect.y() - 1, focusRect.width() + 1, focusRect.height() + 1 );
    }
}

/*! This function fills the \a cw pixels wide and \a ch pixels high
  rectangle starting at position \a cx, \a cy with the
  background color using the painter \a p.

  paintEmptyArea() is invoked by drawContents() to erase
  or fill unused areas.
*/

void QTable::paintEmptyArea( QPainter *p, int cx, int cy, int cw, int ch )
{
    // Region of the rect we should draw
    QRegion reg( QRect( cx, cy, cw, ch ) );
    // Subtract the table from it
    reg = reg.subtract( QRect( QPoint( 0, 0 ), tableSize() ) );

    // And draw the rectangles (transformed as needed)
    QArray<QRect> r = reg.rects();
    for ( int i = 0; i < (int)r.count(); ++i)
	p->fillRect( r[ i ], colorGroup().brush( QColorGroup::Base ) );
}

/*! Returns the QTableItem representing the contents of the cell \a
  row, \a col.

  If \a row or \a col are out of range or no content has
  been set for this cell so far, item() returns 0.
*/

QTableItem *QTable::item( int row, int col ) const
{
    if ( row < 0 || col < 0 || row > numRows() - 1 ||
	 col > numCols() - 1 || row * col >= (int)contents.size() )
	return 0;

    return contents[ indexOf( row, col ) ];	// contents array lookup
}

/*! Fills the cell \a row, \a col with \a item and repaints the cell.

  If a cell item already exists in that position, the old one is deleted.

  \walkthrough table/euroconversion/converter.h
  \skipto currencies
  \printline currencies
  \walkthrough table/euroconversion/converter.cpp
  \skipto new QComboTableItem(
  \printline currencies
  \printline setItem(

  (Code taken from \link euroconvert-example.html table/euroconversion/converter.cpp
  \endlink)

  Note that the first row/column is the one with \a row respectively \a col
  being 0.

  A QTableItem might serve as cell content in a table that is
  not its parent but it can't be set in two or more QTable objects
  at the same time.
  If it is unavoidable to use the same item in more than one tables remember
  to take the item out of the old table using takeItem() before setting it
  into the new one.

  \sa item()
*/

void QTable::setItem( int row, int col, QTableItem *item )
{
    if ( !item )
	return;

    if ( (int)contents.size() != numRows() * numCols() )
	resizeData( numRows() * numCols() );

    int orow = item->row();
    int ocol = item->col();
    clearCell( row, col );

    contents.insert( indexOf( row, col ), item );
    item->setRow( row );
    item->setCol( col );
    updateCell( row, col );
    item->updateEditor( orow, ocol );
}

/*! Removes the QTableItem in position \a row, \a col.

  To show an empty cell immediately you have to updateCell()
  manually.
*/

void QTable::clearCell( int row, int col )
{
    if ( (int)contents.size() != numRows() * numCols() )
	resizeData( numRows() * numCols() );
    contents.remove( indexOf( row, col ) );
}

/*! Sets the text in cell \a row, \a col to \a text.

  If no QTableItem belongs to the cell yet, an item of
  EditType QTableItem::OnTyping is created and can be queried
  using item().

  \walkthrough table/euroconversion/converter.cpp
  \skipto setText
  \printline setText

  (Code taken from \link euroconvert-example.html
  table/euroconversion/converter.cpp \endlink)

  Note that the first row/column is the one with \a row respectively
  \a col being 0. For \l{QComboTableItem}s this function has
  no visible effect.

  \sa text() setPixmap() setItem() QTableItem::setText()
*/

void QTable::setText( int row, int col, const QString &text )
{
    QTableItem *itm = item( row, col );
    if ( itm ) {
	itm->setText( text );
	updateCell( row, col );
    } else {
	QTableItem *i = new QTableItem( this, QTableItem::OnTyping,
					text, QPixmap() );
	setItem( row, col, i );
    }
}

/*! Adds \a pix to the cell \a row, \a col or exchanges
  the old pixmap of this cell with \a pix.

  If no QTableItem belongs to the cell yet, an item of EditType
  QTableItem::OnTyping is created.

  A scaled pixmap that fits into the relevant row is set like this:

  \walkthrough table/small-table-demo/main.cpp
  \skipto QImage
  \printuntil setPixmap

  (Code taken from \link small-table-demo-example.html
  table/small-table-demo/main.cpp \endlink)

  Note that \l{QComboTableItem}s and \l{QCheckTableItem}s
  don't show pixmaps.

  \sa pixmap() setText() setItem() QTableItem::setPixmap()
*/

void QTable::setPixmap( int row, int col, const QPixmap &pix )
{
    QTableItem *itm = item( row, col );
    if ( itm ) {
	itm->setPixmap( pix );
	updateCell( row, col );
    } else {
	QTableItem *i = new QTableItem( this, QTableItem::OnTyping,
					QString::null, pix );
	setItem( row, col, i );
    }
}

/*! Returns the text in cell \a row, \a col, or an empty string if
  the relevant item does not exist or has no text.

  \sa setText() setPixmap()
*/

QString QTable::text( int row, int col ) const
{
    QTableItem *itm = item( row, col );
    if ( itm )
	return itm->text();
    return QString::null;
}

/*! Returns the pixmap set for the cell \a row, \a col, or a
  null-pixmap if the cell contains no pixmap.

  \sa setPixmap()
*/

QPixmap QTable::pixmap( int row, int col ) const
{
    QTableItem *itm = item( row, col );
    if ( itm )
	return itm->pixmap();
    return QPixmap();
}

/*! Moves the focus to the cell at position \a row, \a col.

  \sa currentRow() currentColumn()
*/

void QTable::setCurrentCell( int row, int col )
{
    QTableItem *itm = item( row, col );
    QTableItem *oldIitem = item( curRow, curCol );
    if ( itm && itm->rowSpan() > 1 && oldIitem == itm && itm->row() != row ) {
	if ( row > curRow )
	    row = itm->row() + itm->rowSpan();
	else if ( row < curRow )
	    row = QMAX( 0, itm->row() - 1 );
    }
    if ( itm && itm->colSpan() > 1 && oldIitem == itm && itm->col() != col ) {
	if ( col > curCol )
	    col = itm->col() + itm->colSpan();
	else if ( col < curCol )
	    col = QMAX( 0, itm->col() - 1 );
    }
    if ( curRow != row || curCol != col ) {
	itm = oldIitem;
	if ( itm && itm->editType() != QTableItem::Always )
	    endEdit( curRow, curCol, TRUE, FALSE );
	int oldRow = curRow;
	int oldCol = curCol;
	curRow = row;
	curCol = col;
	repaintCell( oldRow, oldCol );
	repaintCell( curRow, curCol );
	ensureCellVisible( curRow, curCol );
	emit currentChanged( row, col );
	if ( !isColumnSelected( oldCol ) && !isRowSelected( oldRow ) ) {
	    topHeader->setSectionState( oldCol, QTableHeader::Normal );
	    leftHeader->setSectionState( oldRow, QTableHeader::Normal );
	}
	topHeader->setSectionState( curCol, isColumnSelected( curCol, TRUE ) ? QTableHeader::Selected : QTableHeader::Bold );
	leftHeader->setSectionState( curRow, isRowSelected( curRow, TRUE ) ? QTableHeader::Selected : QTableHeader::Bold );
	itm = item( curRow, curCol );
	
	QPoint cellPos( columnPos( curCol ) + leftMargin() - contentsX(), rowPos( curRow ) + topMargin() - contentsY() );
	setMicroFocusHint( cellPos.x(), cellPos.y(), columnWidth( curCol ), rowHeight( curRow ), ( itm && itm->editType() != QTableItem::Never ) );

	if ( cellWidget( oldRow, oldCol ) &&
	     cellWidget( oldRow, oldCol )->hasFocus() )
	    viewport()->setFocus();

	if ( itm && itm->editType() == QTableItem::WhenCurrent ) {
	    if ( beginEdit( curRow, curCol, FALSE ) )
		setEditMode( Editing, row, col );
	} else if ( itm && itm->editType() == QTableItem::Always ) {
	    if ( cellWidget( itm->row(), itm->col() ) )
		cellWidget( itm->row(), itm->col() )->setFocus();
	}
    }

#ifndef QT_NO_ACCESSIBILITY
    setAccessibilityHint( stateDescription() );
#endif
}

/*! Scrolls the table until the cell \a row, \a col becomes
  visible.
*/

void QTable::ensureCellVisible( int row, int col )
{
    int cw = columnWidth( col );
    int rh = rowHeight( row );
    ensureVisible( columnPos( col ) + cw / 2, rowPos( row ) + rh / 2, cw / 2, rh / 2 );
}

/*! Checks whether the cell at position \a row, \a col is selected.

  \sa isRowSelected() isColumnSelected()
*/

bool QTable::isSelected( int row, int col ) const
{
    QListIterator<QTableSelection> it( selections );
    QTableSelection *s;
    while ( ( s = it.current() ) != 0 ) {
	++it;
	if ( s->isActive() &&
	     row >= s->topRow() &&
	     row <= s->bottomRow() &&
	     col >= s->leftCol() &&
	     col <= s->rightCol() )
	    return TRUE;
	if ( row == currentRow() && col == currentColumn() )
	    return TRUE;
    }
    return FALSE;
}

/*! Returns TRUE if \a row is selected and FALSE otherwise.

  If \a full is TRUE, the entire row must be selected for this
  function to return TRUE. If \a full is FALSE, at least one cell in
  \a row must be selected.

  \sa isColumnSelected() isSelected()
*/

bool QTable::isRowSelected( int row, bool full ) const
{
    if ( !full ) {
	QListIterator<QTableSelection> it( selections );
	QTableSelection *s;
	while ( ( s = it.current() ) != 0 ) {
	    ++it;
	    if ( s->isActive() &&
		 row >= s->topRow() &&
		 row <= s->bottomRow() )
	    return TRUE;
	if ( row == currentRow() )
	    return TRUE;
	}
    } else {
	QListIterator<QTableSelection> it( selections );
	QTableSelection *s;
	while ( ( s = it.current() ) != 0 ) {
	    ++it;
	    if ( s->isActive() &&
		 row >= s->topRow() &&
		 row <= s->bottomRow() &&
		 s->leftCol() == 0 &&
		 s->rightCol() == numCols() - 1 )
		return TRUE;
	}
    }
    return FALSE;
}

/*! Returns TRUE if column \a col is selected and FALSE otherwise.

  If \a full is TRUE, the entire column must be selected for this
  function to return TRUE.  If \a full is FALSE, at least one cell in
  \a col must be selected.

  \sa isRowSelected() isSelected()
*/

bool QTable::isColumnSelected( int col, bool full ) const
{
    if ( !full ) {
	QListIterator<QTableSelection> it( selections );
	QTableSelection *s;
	while ( ( s = it.current() ) != 0 ) {
	    ++it;
	    if ( s->isActive() &&
		 col >= s->leftCol() &&
		 col <= s->rightCol() )
	    return TRUE;
	if ( col == currentColumn() )
	    return TRUE;
	}
    } else {
	QListIterator<QTableSelection> it( selections );
	QTableSelection *s;
	while ( ( s = it.current() ) != 0 ) {
	    ++it;
	    if ( s->isActive() &&
		 col >= s->leftCol() &&
		 col <= s->rightCol() &&
		 s->topRow() == 0 &&
		 s->bottomRow() == numRows() - 1 )
		return TRUE;
	}
    }
    return FALSE;
}

/*! Returns the number of selections.

  \sa currentSelection()
*/

int QTable::numSelections() const
{
    return selections.count();
}

/*! Returns selection number \a num, or an empty QTableSelection
  if \a num is out of range (see QTableSelection::isNull()).
*/

QTableSelection QTable::selection( int num ) const
{
    if ( num < 0 || num >= (int)selections.count() )
	return QTableSelection();

    QTableSelection *s = ( (QTable*)this )->selections.at( num );
    return *s;
}

/*! Adds a selection described by \a s to the table and returns its
 number or -1 if the selection is invalid.

 Don't forget to call QTableSelection::init() and
 QTableSelection::expandTo() to make the selection valid (see also
 QTableSelection::isActive()).

 \sa numSelections() removeSelection() clearSelection()
*/

int QTable::addSelection( const QTableSelection &s )
{
    if ( !s.isActive() )
	return -1;
    QTableSelection *sel = new QTableSelection( s );
    selections.append( sel );
    viewport()->repaint( FALSE );
    return selections.count() - 1;
}

/*! Removes the selection matching the values of \a s from the
  table.

  \sa addSelection() numSelections()
*/

void QTable::removeSelection( const QTableSelection &s )
{
    for ( QTableSelection *sel = selections.first(); sel; sel = selections.next() ) {
	if ( s == *sel ) {
	    selections.removeRef( sel );
	    viewport()->repaint( FALSE );
	}
    }
}

/*! \overload

  Removes selection number \a num.

  \sa numSelections() addSelection()
*/

void QTable::removeSelection( int num )
{
    if ( num < 0 || num >= (int)selections.count() )
	return;

    QTableSelection *s = selections.at( num );
    selections.removeRef( s );
    viewport()->repaint( FALSE );
}

/*! Returns the number of the current selection or -1 if there is
  none.

  \sa numSelection()
*/

int QTable::currentSelection() const
{
    if ( !currentSel )
	return -1;
    return ( (QTable*)this )->selections.findRef( currentSel );
}

/*! \reimp
*/

void QTable::contentsMousePressEvent( QMouseEvent* e )
{
    shouldClearSelection = FALSE;
    mousePressed = TRUE;
    if ( isEditing() )
 	endEdit( editRow, editCol, TRUE, edMode != Editing );

    int tmpRow = rowAt( e->pos().y() );
    int tmpCol = columnAt( e->pos().x() );
    pressedRow = tmpRow;
    pressedCol = tmpCol;
    fixRow( tmpRow, e->pos().y() );
    fixCol( tmpCol, e->pos().x() );
    startDragCol = -1;
    startDragRow = -1;

    if ( isSelected( tmpRow, tmpCol ) ) {
	startDragCol = tmpCol;
	startDragRow = tmpRow;
	dragStartPos = e->pos();
    }

    QTableItem *itm = item( pressedRow, pressedCol );
    if ( itm && !itm->isEnabled() )
	return;

    if ( ( e->state() & ShiftButton ) == ShiftButton ) {
	if ( selMode != NoSelection ) {
	    if ( !currentSel ) {
		currentSel = new QTableSelection();
		selections.append( currentSel );
		currentSel->init( curRow, curCol );
	    }
	    QTableSelection oldSelection = *currentSel;
	    currentSel->expandTo( tmpRow, tmpCol );
	    repaintSelections( &oldSelection, currentSel );
	    emit selectionChanged();
	}
	setCurrentCell( tmpRow, tmpCol );
    } else if ( ( e->state() & ControlButton ) == ControlButton ) {
	if ( selMode != NoSelection ) {
	    if ( selMode == Single )
		clearSelection();
	    currentSel = new QTableSelection();
	    selections.append( currentSel );
	    currentSel->init( tmpRow, tmpCol );
	    emit selectionChanged();
	}
	setCurrentCell( tmpRow, tmpCol );
    } else {
	setCurrentCell( tmpRow, tmpCol );
	if ( isSelected( tmpRow, tmpCol ) ) {
	    shouldClearSelection = TRUE;
	} else {
	    clearSelection();
	    if ( selMode != NoSelection ) {
		currentSel = new QTableSelection();
		selections.append( currentSel );
		currentSel->init( tmpRow, tmpCol );
		emit selectionChanged();
	    }
	}
    }

    emit pressed( tmpRow, tmpCol, e->button(), e->pos() );
}

/*! \reimp
*/

void QTable::contentsMouseDoubleClickEvent( QMouseEvent *e )
{
    int tmpRow = rowAt( e->pos().y() );
    int tmpCol = columnAt( e->pos().x() );
    QTableItem *itm = item( tmpRow, tmpCol );
    if ( itm && !itm->isEnabled() )
	return;
    if ( tmpRow != -1 && tmpCol != -1 ) {
	if ( beginEdit( tmpRow, tmpCol, FALSE ) )
	    setEditMode( Editing, tmpRow, tmpCol );
    }

    emit doubleClicked( tmpRow, tmpCol, e->button(), e->pos() );
}

/*! Sets the current edit mode to \a mode, the current edit row to \a
  row and the current edit column to \a col.

  \sa EditMode
*/

void QTable::setEditMode( EditMode mode, int row, int col )
{
    edMode = mode;
    editRow = row;
    editCol = col;
}


/*! \reimp
*/

void QTable::contentsMouseMoveEvent( QMouseEvent *e )
{
    if ( !mousePressed )
	return;
    int tmpRow = rowAt( e->pos().y() );
    int tmpCol = columnAt( e->pos().x() );
    fixRow( tmpRow, e->pos().y() );
    fixCol( tmpCol, e->pos().x() );

    if ( dragEnabled() && startDragRow != -1 && startDragCol != -1 ) {
	if ( QPoint( dragStartPos - e->pos() ).manhattanLength() > QApplication::startDragDistance() ) {
	    mousePressed = FALSE;
	    startDrag();
	}
	return;
    }

    if ( shouldClearSelection ) {
	clearSelection();
	if ( selMode != NoSelection ) {
	    currentSel = new QTableSelection();
	    selections.append( currentSel );
	    currentSel->init( tmpRow, tmpCol );
	    emit selectionChanged();
	}
	shouldClearSelection = FALSE;
    }

    QPoint pos = mapFromGlobal( e->globalPos() );
    pos -= QPoint( leftHeader->width(), topHeader->height() );
    autoScrollTimer->stop();
    doAutoScroll();
    if ( pos.x() < 0 || pos.x() > visibleWidth() || pos.y() < 0 || pos.y() > visibleHeight() )
	autoScrollTimer->start( 100, TRUE );
}

/*! \internal
 */

void QTable::doValueChanged()
{
    emit valueChanged( ( (QTableItem*)sender() )->row(),
		       ( (QTableItem*)sender() )->col() );
}

/*! \internal
*/

void QTable::doAutoScroll()
{
    if ( !mousePressed )
	return;
    QPoint pos = QCursor::pos();
    pos = mapFromGlobal( pos );
    pos -= QPoint( leftHeader->width(), topHeader->height() );

    int tmpRow = curRow;
    int tmpCol = curCol;
    if ( pos.y() < 0 )
	tmpRow--;
    else if ( pos.y() > visibleHeight() )
	tmpRow++;
    if ( pos.x() < 0 )
	tmpCol--;
    else if ( pos.x() > visibleWidth() )
	tmpCol++;

    pos += QPoint( contentsX(), contentsY() );
    if ( tmpRow == curRow )
	tmpRow = rowAt( pos.y() );
    if ( tmpCol == curCol )
	tmpCol = columnAt( pos.x() );
    pos -= QPoint( contentsX(), contentsY() );

    fixRow( tmpRow, pos.y() );
    fixCol( tmpCol, pos.x() );

    if ( tmpRow < 0 || tmpRow > numRows() - 1 )
	tmpRow = currentRow();
    if ( tmpCol < 0 || tmpCol > numCols() - 1 )
	tmpCol = currentColumn();

    ensureCellVisible( tmpRow, tmpCol );

    if ( currentSel && selMode != NoSelection ) {
	QTableSelection oldSelection = *currentSel;
	currentSel->expandTo( tmpRow, tmpCol );
	setCurrentCell( tmpRow, tmpCol );
	repaintSelections( &oldSelection, currentSel );
	emit selectionChanged();
    } else {
	setCurrentCell( tmpRow, tmpCol );
    }

    if ( pos.x() < 0 || pos.x() > visibleWidth() || pos.y() < 0 || pos.y() > visibleHeight() )
	autoScrollTimer->start( 100, TRUE );
}

/*! \reimp
*/

void QTable::contentsMouseReleaseEvent( QMouseEvent *e )
{
    if ( shouldClearSelection ) {
	int tmpRow = rowAt( e->pos().y() );
	int tmpCol = columnAt( e->pos().x() );
	fixRow( tmpRow, e->pos().y() );
	fixCol( tmpCol, e->pos().x() );
	clearSelection();
	if ( selMode != NoSelection ) {
	    currentSel = new QTableSelection();
	    selections.append( currentSel );
	    currentSel->init( tmpRow, tmpCol );
	    emit selectionChanged();
	}
	shouldClearSelection = FALSE;
    }
    mousePressed = FALSE;
    autoScrollTimer->stop();
    if ( pressedRow == curRow && pressedCol == curCol )
	emit clicked( curRow, curCol, e->button(), e->pos() );
}

/*!
  \reimp
*/

void QTable::contentsContextMenuEvent( QContextMenuEvent *e )
{
    if ( e->reason() == QContextMenuEvent::Keyboard ) {
	QRect r = cellGeometry( curRow, curCol );
	r.moveBy( -contentsX(), -contentsY() );
	emit contextMenu( curRow, curCol, mapToGlobal( contentsToViewport( r.center() ) ) );
    } else {
	QMouseEvent me( QEvent::MouseButtonPress, e->pos(), e->globalPos(), RightButton, e->state() );
	contentsMousePressEvent( &me );
    }
    e->accept();
}


/*! \reimp
*/

bool QTable::eventFilter( QObject *o, QEvent *e )
{
    if ( !o || !e )
	return QScrollView::eventFilter( o, e );

    QWidget *editorWidget = cellWidget( editRow, editCol );
    switch ( e->type() ) {
    case QEvent::KeyPress: {
	QTableItem *itm = item( curRow, curCol );

	if ( isEditing() && editorWidget && o == editorWidget ) {
	    itm = item( editRow, editCol );
	    QKeyEvent *ke = (QKeyEvent*)e;
	    if ( ke->key() == Key_Escape ) {
		if ( !itm || itm->editType() == QTableItem::OnTyping )
		    endEdit( editRow, editCol, FALSE, edMode != Editing );
		return TRUE;
	    }

	    if ( ke->key() == Key_Return || ke->key() == Key_Enter ) {
		if ( !itm || itm->editType() == QTableItem::OnTyping )
		    endEdit( editRow, editCol, TRUE, edMode != Editing );
		activateNextCell();
		return TRUE;
	    }

	    if ( ke->key() == Key_Tab || ke->key() == Key_BackTab ) {
		if ( !itm || itm->editType() == QTableItem::OnTyping )
		    endEdit( editRow, editCol, TRUE, edMode != Editing );
		if ( ke->key() == Key_Tab && currentColumn() >= numCols() - 1 )
		    return TRUE;
		if ( ke->key() == Key_BackTab && currentColumn() == 0 )
		    return TRUE;
		if ( ke->key() == Key_Tab ) {
		    int cc  = QMIN( numCols() - 1, currentColumn() + 1 );
		    while ( cc < numCols() ) {
			if ( !isColumnReadOnly( cc ) )
			    break;
			++cc;
		    }
		    setCurrentCell( currentRow(), cc );
		} else { // Key_BackTab
		    int cc  = QMAX( 0, currentColumn() - 1 );
		    while ( cc >= 0 ) {
			if ( !isColumnReadOnly( cc ) )
			    break;
			--cc;
		    }
		    setCurrentCell( currentRow(), QMAX( 0, currentColumn() - 1 ) );
		}
		itm = item( curRow, curCol );
		if ( beginEdit( curRow, curCol, FALSE ) )
		    setEditMode( Editing, curRow, curCol );
		return TRUE;
	    }

	    if ( ( edMode == Replacing ||
		   itm && itm->editType() == QTableItem::WhenCurrent ) &&
		 ( ke->key() == Key_Up || ke->key() == Key_Prior ||
		   ke->key() == Key_Home || ke->key() == Key_Down ||
		   ke->key() == Key_Next || ke->key() == Key_End ||
		   ke->key() == Key_Left || ke->key() == Key_Right ) ) {
		if ( !itm || itm->editType() == QTableItem::OnTyping ) {
		    endEdit( editRow, editCol, TRUE, edMode != Editing );
		}
		keyPressEvent( ke );
		return TRUE;
	    }
	} else {
	    QObjectList *l = viewport()->queryList( "QWidget" );
	    if ( l && l->find( o ) != -1 ) {
		QKeyEvent *ke = (QKeyEvent*)e;
		if ( ( ke->state() & ControlButton ) == ControlButton ||
		     ( ke->key() != Key_Left && ke->key() != Key_Right &&
		       ke->key() != Key_Up && ke->key() != Key_Down &&
		       ke->key() != Key_Prior && ke->key() != Key_Next &&
		       ke->key() != Key_Home && ke->key() != Key_End ) )
		    return FALSE;
		keyPressEvent( (QKeyEvent*)e );
		return TRUE;
	    }
	    delete l;
	}

	} break;
    case QEvent::FocusOut:
	if ( o == this || o == viewport() ) {
	    updateCell( curRow, curCol );
	    return TRUE;
	}
	if ( isEditing() && editorWidget && o == editorWidget && ( (QFocusEvent*)e )->reason() != QFocusEvent::Popup ) {
	    QTableItem *itm = item( editRow, editCol );
 	    if ( !itm || itm->editType() == QTableItem::OnTyping ) {
 		endEdit( editRow, editCol, TRUE, edMode != Editing );
		return TRUE;
	    }
	}
	break;
    case QEvent::FocusIn:
 	if ( o == this || o == viewport() ) {
	    updateCell( curRow, curCol );
	    if ( isEditing() && editorWidget )
		editorWidget->setFocus();
	    return TRUE;
	}
	break;
    default:
	break;
    }

    return QScrollView::eventFilter( o, e );
}

/*! \reimp
*/

void QTable::keyPressEvent( QKeyEvent* e )
{
    if ( isEditing() && item( editRow, editCol ) &&
	 item( editRow, editCol )->editType() == QTableItem::OnTyping )
	return;

    int tmpRow = curRow;
    int tmpCol = curCol;
    int oldRow = tmpRow;
    int oldCol = tmpCol;

    bool navigationKey = FALSE;
    int r;
    switch ( e->key() ) {
    case Key_Left:
	tmpCol = QMAX( 0, tmpCol - 1 );
	navigationKey = TRUE;
    	break;
    case Key_Right:
	tmpCol = QMIN( numCols() - 1, tmpCol + 1 );
	navigationKey = TRUE;
	break;
    case Key_Up:
	tmpRow = QMAX( 0, tmpRow - 1 );
	navigationKey = TRUE;
	break;
    case Key_Down:
	tmpRow = QMIN( numRows() - 1, tmpRow + 1 );
	navigationKey = TRUE;
	break;
    case Key_Prior:
	r = QMAX( 0, rowAt( rowPos( tmpRow ) - visibleHeight() ) );
	if ( r < tmpRow )
	    tmpRow = r;
	navigationKey = TRUE;
	break;
    case Key_Next:
	r = QMIN( numRows() - 1, rowAt( rowPos( tmpRow ) + visibleHeight() ) );
	if ( r > tmpRow )
	    tmpRow = r;
	else
	    tmpRow = numRows() - 1;
	navigationKey = TRUE;
	break;
    case Key_Home:
	tmpRow = 0;
	navigationKey = TRUE;
	break;
    case Key_End:
	tmpRow = numRows() - 1;
	navigationKey = TRUE;
	break;
    case Key_F2:
	if ( beginEdit( tmpRow, tmpCol, FALSE ) )
	    setEditMode( Editing, tmpRow, tmpCol );
	break;
    default: // ... or start in-place editing
	if ( e->text()[ 0 ].isPrint() ) {
	    QTableItem *itm = item( tmpRow, tmpCol );
	    if ( !itm || itm->editType() == QTableItem::OnTyping ) {
		QWidget *w;
		if ( ( w = beginEdit( tmpRow, tmpCol,
				      itm ? itm->isReplaceable() : TRUE ) ) ) {
		    setEditMode( ( !itm || itm && itm->isReplaceable()
				   ? Replacing : Editing ), tmpRow, tmpCol );
		    QApplication::sendEvent( w, e );
		}
	    }
	}
    }

    if ( navigationKey ) {
	if ( ( e->state() & ShiftButton ) == ShiftButton &&
	     selMode != NoSelection ) {
	    setCurrentCell( tmpRow, tmpCol );
	    bool justCreated = FALSE;
	    if ( !currentSel ) {
		justCreated = TRUE;
		currentSel = new QTableSelection();
		selections.append( currentSel );
		currentSel->init( oldRow, oldCol );
	    }
	    QTableSelection oldSelection = *currentSel;
	    currentSel->expandTo( tmpRow, tmpCol );
	    repaintSelections( justCreated ? 0 : &oldSelection, currentSel );
	    emit selectionChanged();
	} else {
	    clearSelection();
	    setCurrentCell( tmpRow, tmpCol );
	}
    } else {
	setCurrentCell( tmpRow, tmpCol );
    }
}

/*! \reimp
*/

void QTable::focusInEvent( QFocusEvent* )
{
    QPoint cellPos( columnPos( curCol ) + leftMargin() - contentsX(), rowPos( curRow ) + topMargin() - contentsY() );
    QTableItem *itm = item( curRow, curCol );
    setMicroFocusHint( cellPos.x(), cellPos.y(), columnWidth( curCol ), rowHeight( curRow ), ( itm && itm->editType() != QTableItem::Never ) );
}


/*! \reimp
*/

void QTable::focusOutEvent( QFocusEvent* )
{
}

/*! \reimp
*/

QSize QTable::sizeHint() const
{
    QSize s = tableSize();
    if ( s.width() < 500 && s.height() < 500 )
	return QSize( tableSize().width() + leftMargin() + 5,
		      tableSize().height() + topMargin() + 5 );
    return QScrollView::sizeHint();
}

/*! \reimp
*/

void QTable::resizeEvent( QResizeEvent *e )
{
    QScrollView::resizeEvent( e );
    updateGeometries();
}

/*! \reimp
*/

void QTable::showEvent( QShowEvent *e )
{
    QScrollView::showEvent( e );
    QRect r( cellGeometry( numRows() - 1, numCols() - 1 ) );
    resizeContents( r.right() + 1, r.bottom() + 1 );
    updateGeometries();
}

static bool inUpdateCell = FALSE;

/*! Repaints the cell at position \a row, \a col.

  This is for example necessary after you deleted
  cell contents using clearCell() and don't
  wish the old content to be visible any longer.
*/

void QTable::updateCell( int row, int col )
{
    if ( inUpdateCell || row == -1 || col == -1 )
	return;
    inUpdateCell = TRUE;
    QRect cg = cellGeometry( row, col );
    QRect r( contentsToViewport( QPoint( cg.x() - 2, cg.y() - 2 ) ),
	     QSize( cg.width() + 4, cg.height() + 4 ) );
    QApplication::postEvent( viewport(), new QPaintEvent( r, FALSE ) );
    inUpdateCell = FALSE;
}

void QTable::repaintCell( int row, int col )
{
    if ( row == -1 || col == -1 )
	return;
    QRect cg = cellGeometry( row, col );
    QRect r( QPoint( cg.x() - 2, cg.y() - 2 ),
	     QSize( cg.width() + 4, cg.height() + 4 ) );
    repaintContents( r, FALSE );
}

void QTable::contentsToViewport2( int x, int y, int& vx, int& vy )
{
    const QPoint v = contentsToViewport2( QPoint( x, y ) );
    vx = v.x();
    vy = v.y();
}

QPoint QTable::contentsToViewport2( const QPoint &p )
{
    return QPoint( p.x() - contentsX(),
		   p.y() - contentsY() );
}

QPoint QTable::viewportToContents2( const QPoint& vp )
{
    return QPoint( vp.x() + contentsX(),
		   vp.y() + contentsY() );
}

void QTable::viewportToContents2( int vx, int vy, int& x, int& y )
{
    const QPoint c = viewportToContents2( QPoint( vx, vy ) );
    x = c.x();
    y = c.y();
}

/*! This function should be called whenever the column width of \a col
  has been changed. It will then rearrange the content appropriately.
*/

void QTable::columnWidthChanged( int col )
{
    updateContents( columnPos( col ), 0, contentsWidth(), contentsHeight() );
    QSize s( tableSize() );
    int w = contentsWidth();
    resizeContents( s.width(), s.height() );
    if ( contentsWidth() < w )
	repaintContents( s.width(), 0,
			 w - s.width() + 1, contentsHeight(), TRUE );
    else
	repaintContents( w, 0,
			 s.width() - w + 1, contentsHeight(), FALSE );

    updateGeometries();
    qApp->processEvents();

    for ( int j = col; j < numCols(); ++j ) {
	for ( int i = 0; i < numRows(); ++i ) {
	    QWidget *w = cellWidget( i, j );
	    if ( !w )
		continue;
	    moveChild( w, columnPos( j ), rowPos( i ) );
	    w->resize( columnWidth( j ) - 1, rowHeight( i ) - 1 );
	}
 	qApp->processEvents();
    }
}

/*! Call this function whenever the height of row \a row has
  changed in order to rearrange its contents.
*/

void QTable::rowHeightChanged( int row )
{
    updateContents( 0, rowPos( row ), contentsWidth(), contentsHeight() );
    QSize s( tableSize() );
    int h = contentsHeight();
    resizeContents( s.width(), s.height() );
    if ( contentsHeight() < h )
	repaintContents( 0, contentsHeight(),
			 contentsWidth(), h - s.height() + 1, TRUE );
    else
	repaintContents( 0, h,
			 contentsWidth(), s.height() - h + 1, FALSE );

    updateGeometries();
    qApp->processEvents();

    for ( int j = row; j < numRows(); ++j ) {
	for ( int i = 0; i < numCols(); ++i ) {
	    QWidget *w = cellWidget( j, i );
	    if ( !w )
		continue;
	    moveChild( w, columnPos( i ), rowPos( j ) );
	    w->resize( columnWidth( i ) - 1, rowHeight( j ) - 1 );
	}
 	qApp->processEvents();
    }
}

/*! \internal */

void QTable::updateRowWidgets( int row )
{
    for ( int i = 0; i < numCols(); ++i ) {
	QWidget *w = cellWidget( row, i );
	if ( !w )
	    continue;
	moveChild( w, columnPos( i ), rowPos( row ) );
	w->resize( columnWidth( i ) - 1, rowHeight( row ) - 1 );
    }
}

/*! \internal */

void QTable::updateColWidgets( int col )
{
    for ( int i = 0; i < numRows(); ++i ) {
	QWidget *w = cellWidget( i, col );
	if ( !w )
	    continue;
	moveChild( w, columnPos( col ), rowPos( i ) );
	w->resize( columnWidth( col ) - 1, rowHeight( i ) - 1 );
    }
}

/*! This function is called when column order is to be changed, i.e.
  when the user moved the column header \a section
  from \a fromIndex to \a toIndex.

  If you want to change the column order programmatically, call
  QHeader::moveSection() on horizontalHeader().

  \sa QHeader::indexChange() rowIndexChanged()
*/

void QTable::columnIndexChanged( int, int, int )
{
    repaintContents( contentsX(), contentsY(),
		     visibleWidth(), visibleHeight(), FALSE );
}

/*! This function is called when the order of the rows is to be
  changed, i.e. the user moved the row header section \a section
  from \a fromIndex to \a toIndex.

  If you want to change the order programmatically, call
  QHeader::moveSection() on verticalHeader().

  \sa QHeader::indexChange() columnIndexChanged()
*/

void QTable::rowIndexChanged( int, int, int )
{
    repaintContents( contentsX(), contentsY(),
		     visibleWidth(), visibleHeight(), FALSE );
}

/*! This function is called when the column \a col has been
  clicked. The default implementation sorts this column if
  sorting() is TRUE.
*/

void QTable::columnClicked( int col )
{
    if ( !sorting() )
	return;

    if ( col == lastSortCol ) {
	asc = !asc;
    } else {
	lastSortCol = col;
	asc = TRUE;
    }

    sortColumn( lastSortCol, asc );
}

/*! If \a b is set to TRUE, a click on the header of a column sorts
  the appropriate column.

  \sa sortColumn() sorting()
*/

void QTable::setSorting( bool b )
{
    doSort = b;
}

/*! Returns whether clicking on a column header sorts the column.

 \sa setSorting()
*/

bool QTable::sorting() const
{
    return doSort;
}

static bool inUpdateGeometries = FALSE;

/*! This function updates the geometries of the left and top header.
*/

void QTable::updateGeometries()
{
    if ( inUpdateGeometries )
	return;
    inUpdateGeometries = TRUE;
    QSize ts = tableSize();
    if ( topHeader->offset() &&
	 ts.width() < topHeader->offset() + topHeader->width() )
	horizontalScrollBar()->setValue( ts.width() - topHeader->width() );
    if ( leftHeader->offset() &&
	 ts.height() < leftHeader->offset() + leftHeader->height() )
	verticalScrollBar()->setValue( ts.height() - leftHeader->height() );

    leftHeader->setGeometry( frameWidth(), topMargin() + frameWidth(),
			     leftMargin(), visibleHeight() );
    topHeader->setGeometry( leftMargin() + frameWidth(), frameWidth(),
			    visibleWidth(), topMargin() );
    inUpdateGeometries = FALSE;
}

/*! Returns the width of column \a col.

  \sa setColumnWidth rowHeight()
*/

int QTable::columnWidth( int col ) const
{
    return topHeader->sectionSize( col );
}

/*! Returns the height of the row \a row.

  \sa setRowHeight columnWidth()
*/

int QTable::rowHeight( int row ) const
{
    return leftHeader->sectionSize( row );
}

/*! Returns the x-position of the column \a col in content
  coordinates.

  \sa columnAt() rowPos()
*/

int QTable::columnPos( int col ) const
{
    return topHeader->sectionPos( col );
}

/*! Returns the y-position of the row \a row in content coordinates.

  \sa rowAt() columnPos()
*/

int QTable::rowPos( int row ) const
{
    return leftHeader->sectionPos( row );
}

/*! Returns the column at \a pos. \a pos has to be given in
  content coordinates.

  \sa columnPos() rowAt()
*/

int QTable::columnAt( int pos ) const
{
    return topHeader->sectionAt( pos );
}

/*! Returns the row at \a pos. \a pos has to be given in
  content coordinates.

  \sa rowPos() columnAt()
*/

int QTable::rowAt( int pos ) const
{
    return leftHeader->sectionAt( pos );
}

/*! Returns the bounding rectangle of the cell \a row, \a col in content
  coordinates.
*/

QRect QTable::cellGeometry( int row, int col ) const
{
    QTableItem *itm = item( row, col );
    if ( !itm || itm->rowSpan() == 1 && itm->colSpan() == 1 )
	return QRect( columnPos( col ), rowPos( row ),
		      columnWidth( col ), rowHeight( row ) );

    while ( row != itm->row() )
	row--;
    while ( col != itm->col() )
	col--;

    QRect rect( columnPos( col ), rowPos( row ),
		columnWidth( col ), rowHeight( row ) );

    for ( int r = 1; r < itm->rowSpan(); ++r )
	    rect.setHeight( rect.height() + rowHeight( r + row ) );
    for ( int c = 1; c < itm->colSpan(); ++c )
	rect.setWidth( rect.width() + columnWidth( c + col ) );

    return rect;
}

/*! Returns the size of the table.

  This is the same as the coordinates of the bottom-right edge of
  the last table cell.
*/

QSize QTable::tableSize() const
{
    return QSize( columnPos( numCols() - 1 ) + columnWidth( numCols() - 1 ),
		  rowPos( numRows() - 1 ) + rowHeight( numRows() - 1 ) );
}

/*! Returns the number of rows of the table.

  \sa setNumRows() numCols()
*/

int QTable::numRows() const
{
    return leftHeader->count();
}

/*! Returns of how many columns the table consists.

  \sa setNumCols()  numRows()
*/

int QTable::numCols() const
{
    return topHeader->count();
}

/*! Sets the number of rows to \a r.

  \sa numRows() setNumCols()
*/

void QTable::setNumRows( int r )
{
    if ( r < 0 )
	return;
    leftHeader->setUpdatesEnabled( FALSE );
    bool updateBefore = r < numRows();
    int w = leftMargin();
    if ( r > numRows() ) {
	clearSelection( FALSE );
	while ( numRows() < r ) {
	    leftHeader->addLabel( QString::number( numRows() + 1 ), 20 );
	    int tmpw = fontMetrics().width( QString::number( numRows() + 1 ) + "  " );
	    w = QMAX( w, tmpw );
	}
    } else {
	clearSelection( FALSE );
	while ( numRows() > r )
	    leftHeader->removeLabel( numRows() - 1 );
    }

    if ( w > leftMargin() )
	setLeftMargin( w );

    QVector<QTableItem> tmp;
    tmp.resize( contents.size() );
    int i;
    for ( i = 0; i < (int)tmp.size(); ++i )
	tmp.insert( i, contents[ i ] );
    contents.setAutoDelete( FALSE );
    contents.clear();
    contents.setAutoDelete( TRUE );
    resizeData( numRows() * numCols() );

    for ( i = 0; i < (int)tmp.size(); ++i ) {
	QTableItem *it = tmp [ i ];
	int idx = it ? indexOf( it->row(), it->col() ) : 0;
	if ( it && (uint)idx < contents.size() )
	    contents.insert( idx, it );
    }

    leftHeader->setUpdatesEnabled( TRUE );
    QRect r2( cellGeometry( numRows() - 1, numCols() - 1 ) );
    resizeContents( r2.right() + 1, r2.bottom() + 1 );
    updateGeometries();
    if ( updateBefore )
	repaintContents( contentsX(), contentsY(),
			 visibleWidth(), visibleHeight(), TRUE );
    else
	repaintContents( contentsX(), contentsY(),
			 visibleWidth(), visibleHeight(), FALSE );
    leftHeader->update();
}

/*! Sets the number of columns to \a c.

  \sa numCols() setNumRows()
*/

void QTable::setNumCols( int c )
{
    if ( c < 0 )
	return;
    topHeader->setUpdatesEnabled( FALSE );
    bool updateBefore = c < numCols();
    if ( c > numCols() ) {
	clearSelection( FALSE );
	while ( numCols() < c )
	    topHeader->addLabel( QString::number( numCols() + 1 ), 100 );
    } else {
	clearSelection( FALSE );
	while ( numCols() > c )
	    topHeader->removeLabel( numCols() - 1 );
    }

    QVector<QTableItem> tmp;
    tmp.resize( contents.size() );
    int i;
    for ( i = 0; i < (int)tmp.size(); ++i )
	tmp.insert( i, contents[ i ] );
    int nc = numCols();
    contents.setAutoDelete( FALSE );
    contents.clear();
    contents.setAutoDelete( TRUE );
    resizeData( numRows() * numCols() );

    for ( i = 0; i < (int)tmp.size(); ++i ) {
	QTableItem *it = tmp[ i ];
	int idx = it ? it->row() * nc + it->col() : 0;
	if ( it && (uint)idx < contents.size() && ( !it || it->col() < numCols() ) )
	    contents.insert( idx, it );
    }

    topHeader->setUpdatesEnabled( TRUE );
    QRect r( cellGeometry( numRows() - 1, numCols() - 1 ) );
    resizeContents( r.right() + 1, r.bottom() + 1 );
    updateGeometries();
    if ( updateBefore )
	repaintContents( contentsX(), contentsY(),
			 visibleWidth(), visibleHeight(), TRUE );
    else
	repaintContents( contentsX(), contentsY(),
			 visibleWidth(), visibleHeight(), FALSE );
    topHeader->update();
}

/*! This function returns the widget which should be used as editor for
  the cell \a row, \a col. Returning 0 means that the cell is not editable.

  If \a initFromCell is TRUE, the editor is
  used to edit the current content of the cell (so the editor widget
  should be initialized with this content). Otherwise the content of
  this cell is replaced with the new content which the user
  enters into the widget created by this function.

  The ownership of the editor widget is transferred to the caller.

  The default implementation looks after a QTableItem for
  the cell. If no table item exists and \a initFromCell is TRUE or
  QTableItem::isReplaceable() is FALSE, the item of
  the cell is asked to create the editor using QTableItem::createEditor().
  Otherwise a QLineEdit with left alignment is used as the editor.

  If you want to create your own editor for certain cells,
  implement a custom QTableItem subclass and reimplement
  QTableItem::createEditor().

  In most cases you do not need to reimplement this function. However if
  you decide to fill your table without the help of \l{QTableItem}s and
  choose an editor other than QLineEdit as default,
  reimplement this function and use code like

  \walkthrough table/wineorder/productlist.cpp
  \skipto item(
  \printline item(
  \printuntil createEditor
  \skipto ;
  \skipto }
  \printline }
  \printline return create

  to create the appropriate editor for the cells
  (code taken from \link wineorder-example.html table/wineorder/productlist.cpp
  \endlink).

  Additionally reimplement setCellContentFromEditor() to make sure that the
  data is stored in your custom data structure.

  \sa QTableItem::createEditor()
*/

QWidget *QTable::createEditor( int row, int col, bool initFromCell ) const
{
    if ( isReadOnly() || isRowReadOnly( row ) || isColumnReadOnly( col ) )
	return 0;

    QWidget *e = 0;

    // the current item in the cell should be edited if possible
    QTableItem *i = item( row, col );
    if ( initFromCell || i && !i->isReplaceable() ) {
	if ( i ) {
	    e = i->createEditor();
	    if ( !e || i->editType() == QTableItem::Never )
		return 0;
	}
    }

    // no contents in the cell yet, so open the default editor
    if ( !e ) {
	e = new QLineEdit( viewport() );
	( (QLineEdit*)e )->setFrame( FALSE );
    }

    return e;
}

/*! This function is called to start in-place editing of the cell \a
  row, \a col. If \a replace is TRUE the content of this cell will be
  replaced by the content of the editor later, otherwise the current
  content of the cell (if existing) will be modified by the editor.

  This function calls createEditor() to obtain the editor which should be
  used for editing the cell. After that setCellWidget() is used to make this
  editor the widget of the cell.

  \sa endEdit()
*/

QWidget *QTable::beginEdit( int row, int col, bool replace )
{
    if ( isReadOnly() || isRowReadOnly( row ) || isColumnReadOnly( col ) )
	return 0;
    QTableItem *itm = item( row, col );
    if ( itm && !itm->isEnabled() )
	return 0;
    if ( itm && cellWidget( itm->row(), itm->col() ) )
	return 0;
    ensureCellVisible( curRow, curCol );
    QWidget *e = createEditor( row, col, !replace );
    if ( !e )
	return 0;
    setCellWidget( row, col, e );
    e->setActiveWindow();
    e->setFocus();
    updateCell( row, col );
    return e;
}

/*! This function is called when in-place editing of the cell \a row,
  \a col is requested to stop.

  If \a accept is TRUE the content of the
  editor has to be transferred to the relevant cell. If \a replace
  is TRUE the current content of this cell should be replaced by the
  content of the editor (this means removing the current QTableItem of
  the cell and creating a new one for the cell). Otherwise (if possible)
  the content of the editor should just be set to the existing
  QTableItem of this cell.

  If the cell contents should be replaced or if no QTableItem
  exists for the cell yet, setCellContentFromEditor() is called.
  Otherwise QTableItem::setContentFromEditor() is called on the
  QTableItem of the cell.

  After that clearCellWidget() is called to get rid of the editor
  widget.

  \sa setCellContentFromEditor(), beginEdit()
*/

void QTable::endEdit( int row, int col, bool accept, bool replace )
{
    QWidget *editor = cellWidget( row, col );
    if ( !editor )
	return;

    if ( !accept ) {
	if ( row == editRow && col == editCol )
	    setEditMode( NotEditing, -1, -1 );
	clearCellWidget( row, col );
    	updateCell( row, col );
	viewport()->setFocus();
	updateCell( row, col );
	return;
    }

    QTableItem *i = item( row, col );
    if ( replace && i ) {
	clearCell( row, col );
	i = 0;
    }

    if ( !i )
	setCellContentFromEditor( row, col );
    else
	i->setContentFromEditor( editor );

    if ( row == editRow && col == editCol )
	setEditMode( NotEditing, -1, -1 );

    viewport()->setFocus();
    updateCell( row, col );

    clearCellWidget( row, col );
    emit valueChanged( row, col );
}

/*! This function is called to set the contents of the cell \a row,
  \a col from the editor to this cell. If
  a QTableItem already exists for this cell, it is removed first (see
  clearCell()).

  If you for example want to create different QTableItems depending on the
  contents of the editor, you might reimplement this function. Also if
  you want to work without QTableItems, you need to reimplement this
  function to save the data the user entered to your
  data structure.

  \sa QTableItem::setContentFromEditor() createEditor()
*/

void QTable::setCellContentFromEditor( int row, int col )
{
    QWidget *editor = cellWidget( row, col );
    if ( !editor )
	return;
    clearCell( row, col );
    if ( editor->inherits( "QLineEdit" ) )
	setText( row, col, ( (QLineEdit*)editor )->text() );
}

/*! Returns whether the user is editing a cell which is not in
  permanent edit mode.

  \sa QTableItem::EditType
*/

bool QTable::isEditing() const
{
    return edMode != NotEditing;
}

/*! Maps this 2D table to a 1D array and returns the index of the cell
  in column \a col at row \a row.
*/

int QTable::indexOf( int row, int col ) const
{
    return ( row * numCols() ) + col; // mapping from 2D table to 1D array
}

/*! \internal
*/

void QTable::repaintSelections( QTableSelection *oldSelection,
				QTableSelection *newSelection,
				bool updateVertical, bool updateHorizontal )
{
    if ( oldSelection && *oldSelection == *newSelection )
	return;
    if ( oldSelection && !oldSelection->isActive() )
	oldSelection = 0;

    bool optimize1 = FALSE;
    bool optimize2 = FALSE;
    QRect old;
    if ( oldSelection )
	old = rangeGeometry( oldSelection->topRow(),
			     oldSelection->leftCol(),
			     oldSelection->bottomRow(),
			     oldSelection->rightCol(),
			     optimize1 );
    else
	old = QRect( 0, 0, 0, 0 );

    QRect cur = rangeGeometry( newSelection->topRow(),
			       newSelection->leftCol(),
			       newSelection->bottomRow(),
			       newSelection->rightCol(),
			       optimize2 );
    int i;

    if ( !optimize1 || !optimize2 ||
	 old.width() > SHRT_MAX || old.height() > SHRT_MAX ||
	 cur.width() > SHRT_MAX || cur.height() > SHRT_MAX ) {
	QRect rr = cur.unite( old );
	repaintContents( rr, FALSE );
    } else {
	old = QRect( contentsToViewport2( old.topLeft() ), old.size() );
	cur = QRect( contentsToViewport2( cur.topLeft() ), cur.size() );
	QRegion r1( old );
	QRegion r2( cur );
	QRegion r3 = r1.subtract( r2 );
	QRegion r4 = r2.subtract( r1 );

	for ( i = 0; i < (int)r3.rects().count(); ++i ) {
	    QRect r( r3.rects()[ i ] );
	    r = QRect( viewportToContents2( r.topLeft() ), r.size() );
	    repaintContents( r, FALSE );
	}
	for ( i = 0; i < (int)r4.rects().count(); ++i ) {
	    QRect r( r4.rects()[ i ] );
	    r = QRect( viewportToContents2( r.topLeft() ), r.size() );
	    repaintContents( r, FALSE );
	}
    }

    topHeader->setUpdatesEnabled( FALSE );
    if ( updateHorizontal ) {
	for ( i = 0; i <= numCols(); ++i ) {
	    if ( !isColumnSelected( i ) )
		topHeader->setSectionState( i, QTableHeader::Normal );
	    else if ( isColumnSelected( i, TRUE ) )
		topHeader->setSectionState( i, QTableHeader::Selected );
	    else
		topHeader->setSectionState( i, QTableHeader::Bold );
	}
    }
    topHeader->setUpdatesEnabled( TRUE );
    topHeader->repaint( FALSE );

    leftHeader->setUpdatesEnabled( FALSE );
    if ( updateVertical ) {
	for ( i = 0; i <= numRows(); ++i ) {
	    if ( !isRowSelected( i ) )
		leftHeader->setSectionState( i, QTableHeader::Normal );
	    else if ( isRowSelected( i, TRUE ) )
		leftHeader->setSectionState( i, QTableHeader::Selected );
	    else
		leftHeader->setSectionState( i, QTableHeader::Bold );
	}
    }
    leftHeader->setUpdatesEnabled( TRUE );
    leftHeader->repaint( FALSE );
}

/*! Clears all selections and repaints the appropriate
  regions if \a repaint is TRUE.

  Repainting is the default. If this is not desired set
  \a repaint to FALSE.

  \sa removeSelection()
*/

void QTable::clearSelection( bool repaint )
{
    bool needRepaint = !selections.isEmpty();

    QRect r;
    for ( QTableSelection *s = selections.first(); s; s = selections.next() ) {
	bool b;
	r = r.unite( rangeGeometry( s->topRow(),
				    s->leftCol(),
				    s->bottomRow(),
				    s->rightCol(), b ) );
    }

    currentSel = 0;
    selections.clear();
    if ( needRepaint && repaint )
	repaintContents( r, FALSE );

    int i;
    bool b = topHeader->isUpdatesEnabled();
    topHeader->setUpdatesEnabled( FALSE );
    for ( i = 0; i <= numCols(); ++i ) {
	if ( !isColumnSelected( i ) && i != curCol )
	    topHeader->setSectionState( i, QTableHeader::Normal );
	else if ( isColumnSelected( i, TRUE ) )
	    topHeader->setSectionState( i, QTableHeader::Selected );
	else
	    topHeader->setSectionState( i, QTableHeader::Bold );
    }
    topHeader->setUpdatesEnabled( b );
    topHeader->repaint( FALSE );

    b = leftHeader->isUpdatesEnabled();
    leftHeader->setUpdatesEnabled( FALSE );
    for ( i = 0; i <= numRows(); ++i ) {
	if ( !isRowSelected( i ) && i != curRow )
	    leftHeader->setSectionState( i, QTableHeader::Normal );
	else if ( isRowSelected( i, TRUE ) )
	    leftHeader->setSectionState( i, QTableHeader::Selected );
	else
	    leftHeader->setSectionState( i, QTableHeader::Bold );
    }
    leftHeader->setUpdatesEnabled( b );
    leftHeader->repaint( FALSE );
    emit selectionChanged();
}

/*! \internal
*/

QRect QTable::rangeGeometry( int topRow, int leftCol,
			     int bottomRow, int rightCol, bool &optimize )
{
    topRow = QMAX( topRow, rowAt( contentsY() ) );
    leftCol = QMAX( leftCol, columnAt( contentsX() ) );
    int ra = rowAt( contentsY() + visibleHeight() );
    if ( ra != -1 )
	bottomRow = QMIN( bottomRow, ra );
    int ca = columnAt( contentsX() + visibleWidth() );
    if ( ca != -1 )
	rightCol = QMIN( rightCol, ca );
    optimize = TRUE;
    QRect rect;
    for ( int r = topRow; r <= bottomRow; ++r ) {
	for ( int c = leftCol; c <= rightCol; ++c ) {
	    rect = rect.unite( cellGeometry( r, c ) );
	    QTableItem *i = item( r, c );
	    if ( i && ( i->rowSpan() > 1 || i->colSpan() > 1 ) )
		optimize = FALSE;
	}
    }
    return rect;
}

/*! This function is called to activate the next cell if in-place editing was
  stopped by pressing the Return key.

  If you want a different behavior than going from top to bottom,
  reimplement this function.
*/

void QTable::activateNextCell()
{
    if ( !currentSel || !currentSel->isActive() ) {
	if ( curRow < numRows() - 1 )
	    setCurrentCell( curRow + 1, curCol );
	else if ( curCol < numCols() - 1 )
	    setCurrentCell( 0, curCol + 1 );
	else
	    setCurrentCell( 0, 0 );
    } else {
	if ( curRow < currentSel->bottomRow() )
	    setCurrentCell( curRow + 1, curCol );
	else if ( curCol < currentSel->rightCol() )
	    setCurrentCell( currentSel->topRow(), curCol + 1 );
	else
	    setCurrentCell( currentSel->topRow(), currentSel->leftCol() );
    }

}

/*! \internal
*/

void QTable::fixRow( int &row, int y )
{
    if ( row == -1 ) {
	if ( y < 0 )
	    row = 0;
	else
	    row = numRows() - 1;
    }
}

/*! \internal
*/

void QTable::fixCol( int &col, int x )
{
    if ( col == -1 ) {
	if ( x < 0 )
	    col = 0;
	else
	    col = numCols() - 1;
    }
}

struct SortableTableItem
{
    QTableItem *item;
};

#if defined(Q_C_CALLBACKS)
extern "C" {
#endif

static int cmpTableItems( const void *n1, const void *n2 )
{
    if ( !n1 || !n2 )
	return 0;

    SortableTableItem *i1 = (SortableTableItem *)n1;
    SortableTableItem *i2 = (SortableTableItem *)n2;

    return i1->item->key().compare( i2->item->key() );
}

#if defined(Q_C_CALLBACKS)
}
#endif

/*! Sorts the column \a col in ascending order if \a ascending is
  TRUE, else in descending order.

  If \a wholeRows is TRUE changing data of the cells is done
  using swapRows(). Otherwise swapCells() is called.

  \sa swapRows()
*/

void QTable::sortColumn( int col, bool ascending, bool wholeRows )
{
    int filledRows = 0, i;
    for ( i = 0; i < numRows(); ++i ) {
	QTableItem *itm = item( i, col );
	if ( itm )
	    filledRows++;
    }

    if ( !filledRows )
	return;

    SortableTableItem *items = new SortableTableItem[ filledRows ];
    int j = 0;
    for ( i = 0; i < numRows(); ++i ) {
	QTableItem *itm = item( i, col );
	if ( !itm )
	    continue;
	items[ j++ ].item = itm;
    }

    qsort( items, filledRows, sizeof( SortableTableItem ), cmpTableItems );

    setUpdatesEnabled( FALSE );
    viewport()->setUpdatesEnabled( FALSE );
    for ( i = 0; i < numRows(); ++i ) {
	if ( i < filledRows ) {
	    if ( ascending ) {
		if ( items[ i ].item->row() == i )
		    continue;
		if ( wholeRows )
		    swapRows( items[ i ].item->row(), i );
		else
		    swapCells( items[ i ].item->row(), col, i, col );
	    } else {
		if ( items[ i ].item->row() == filledRows - i - 1 )
		    continue;
		if ( wholeRows )
		    swapRows( items[ i ].item->row(), filledRows - i - 1 );
		else
		    swapCells( items[ i ].item->row(), col,
			       filledRows - i - 1, col );
	    }
	}
    }
    setUpdatesEnabled( TRUE );
    viewport()->setUpdatesEnabled( TRUE );

    if ( !wholeRows )
	repaintContents( columnPos( col ), contentsY(),
			 columnWidth( col ), visibleHeight(), FALSE );
    else
	repaintContents( contentsX(), contentsY(),
			 visibleWidth(), visibleHeight(), FALSE );

    delete [] items;
}

/*! Hides the row \a row.

  \sa showRow() hideColumn()
*/

void QTable::hideRow( int row )
{
    leftHeader->resizeSection( row, 0 );
    leftHeader->setResizeEnabled( FALSE, row );
    rowHeightChanged( row );
}

/*! Hides the column \a col.

  \sa showColumn() hideRow()
*/

void QTable::hideColumn( int col )
{
    topHeader->resizeSection( col, 0 );
    topHeader->setResizeEnabled( FALSE, col );
    columnWidthChanged( col );
}

/*! Shows the row \a row.

  \sa hideRow() showColumn()
*/

void QTable::showRow( int row )
{
    leftHeader->resizeSection( row, 100 );
    leftHeader->setResizeEnabled( TRUE, row );
    rowHeightChanged( row );
}

/*! Shows the column \a col.

  \sa hideColumn() showRow()
*/

void QTable::showColumn( int col )
{
    topHeader->resizeSection( col, 100 );
    topHeader->setResizeEnabled( TRUE, col );
    columnWidthChanged( col );
}

/*! Resizes the column \a col to \a w pixel width.

  \sa columnWidth() setRowHeight()
*/

void QTable::setColumnWidth( int col, int w )
{
    topHeader->resizeSection( col, w );
    columnWidthChanged( col );
}

/*! Resizes the row no. \a row to a height of \a h pixel.

  \sa rowHeight() setColumnWidth()
*/

void QTable::setRowHeight( int row, int h )
{
    leftHeader->resizeSection( row, h );
    rowHeightChanged( row );
}

/*! Resizes the column \a col so that its
  entire content is visible, e.g.

  \walkthrough table/euroconversion/converter.cpp
  \skipto adjustColumn
  \printline adjustColumn

  (Code taken from \link euroconvert-example.html
   table/euroconversion/converter.cpp \endlink)

  \sa adjustRow()
*/

void QTable::adjustColumn( int col )
{
    int w = topHeader->fontMetrics().width( topHeader->label( col ) ) + 10;
    w = QMAX( w, 20 );
    for ( int i = 0; i < numRows(); ++i ) {
	QTableItem *itm = item( i, col );
	if ( !itm )
	    continue;
	w = QMAX( w, itm->sizeHint().width() );
    }
    setColumnWidth( col, w );
}

/*! Resizes the row \a row to be high enough to fit
    its entire content.

  \sa adjustColumn()
*/

void QTable::adjustRow( int row )
{
    int h = 20;
    h = QMAX( h, leftHeader->fontMetrics().height() );
    for ( int i = 0; i < numCols(); ++i ) {
	QTableItem *itm = item( row, i );
	if ( !itm )
	    continue;
	h = QMAX( h, itm->sizeHint().height() );
    }
    setRowHeight( row, h );
}

/*! Makes the column \a col stretchable if \a stretch is TRUE
  and prevents \a col from being stretched if \a stretch is FALSE.

  This means that whenever the table widget becomes wider than its
  contents, stretchable columns are stretched so that the table
  contents fit exactly into the widget.

  \sa isColumnStretchable() setRowStretchable()
*/

void QTable::setColumnStretchable( int col, bool stretch )
{
    topHeader->setSectionStretchable( col, stretch );
}

/*! Makes the row \a row stretchable if \a stretch is TRUE and
  prevents it from being streched otherwise.

  This means that whenever the table widgets becomes higher than its
  contents, stretchable rows are stretched so that the table contents fit
  exactly into the widget.

  \sa isRowStretchable() setColumnStretchable()
*/

void QTable::setRowStretchable( int row, bool stretch )
{
    leftHeader->setSectionStretchable( row, stretch );
}

/*! Returns whether the column \a col is stretchable or not.

  \sa setColumnStretchable() isRowStretchable()
*/

bool QTable::isColumnStretchable( int col ) const
{
    return topHeader->isSectionStretchable( col );
}

/*! Returns whether the row \a row is stretchable or not.

  \sa setRowStretchable() isColumnStretchable()
*/

bool QTable::isRowStretchable( int row ) const
{
    return leftHeader->isSectionStretchable( row );
}

/*! Takes the item \a i out of the table. This function doesn't
  delete it.

  takeItem() is crucial if you wish to use one table item
  in several tables.
*/

void QTable::takeItem( QTableItem *i )
{
    QRect rect = cellGeometry( i->row(), i->col() );
    if ( !i )
	return;
    contents.setAutoDelete( FALSE );
    for ( int r = 0; r < i->rowSpan(); ++r ) {
	for ( int c = 0; c < i->colSpan(); ++c )
	    clearCell( i->row() + r, i->col() + c );
    }
    contents.setAutoDelete( TRUE );
    repaintContents( rect, FALSE );
    int orow = i->row();
    int ocol = i->col();
    i->setRow( -1 );
    i->setCol( -1 );
    i->updateEditor( orow, ocol );
}

/*! Places the widget \a e into the cell \a row, \a col.

  \walkthrough table/wineorder/productlist.cpp
  \skipto QSpinBox
  \printline quantities
  \skipto setCellWidget(
  \printline setCellWidget(

  (Code taken from \link wineorder-example.html
  table/wineorder/productlist.cpp \endlink)

  If the cell geometry changes setCellWidget() takes care about
  correct resizing and replacement.

  By default widgets are inserted into a vector of numRows() x
  numCols() elements. Widgets of very big tables you probably want to store
  in a datastructure that needs less memory (like a
  hash-table). To make this possible this functions calls
  insertWidget() to add the widget to the internal datastructure.

  If you want to use your own datastructure, you therefore
  have to reimplement insertWidget(),
  cellWidget() and clearCellWidget().
 */

void QTable::setCellWidget( int row, int col, QWidget *e )
{
    if ( !e )
	return;

    QWidget *w = cellWidget( row, col );
    if ( w && row == editRow && col == editCol )
 	endEdit( editRow, editCol, FALSE, edMode != Editing );

    e->installEventFilter( this );
    clearCellWidget( row, col );
    if ( e->parent() != viewport() )
	e->reparent( viewport(), QPoint( 0,0 ) );
    insertWidget( row, col, e );
    QRect cr = cellGeometry( row, col );
    e->resize( cr.size() - QSize( 1, 1 ) );
    moveChild( e, cr.x(), cr.y() );
    e->show();
    viewport()->setFocus();
}

/*! Inserts the widget \a w at the table coordinates \a row, \a col
  into the internal datastructure. See the
  documentation of setCellWidget() for further details.
*/

void QTable::insertWidget( int row, int col, QWidget *w )
{
    if ( row < 0 || col < 0 || row > numRows() - 1 || col > numCols() - 1 )
	return;

    if ( (int)widgets.size() != numRows() * numCols() )
	widgets.resize( numRows() * numCols() );

    widgets.insert( indexOf( row, col ), w );
}

/*! Returns the widget that has been set to the cell \a row, \a col,
  or 0 if there is no widget.

  \sa clearCellWidget() setCellWidget()
*/

QWidget *QTable::cellWidget( int row, int col ) const
{
    if ( row < 0 || col < 0 || row > numRows() - 1 || col > numCols() - 1 )
	return 0;

    if ( (int)widgets.size() != numRows() * numCols() )
	( (QTable*)this )->widgets.resize( numRows() * numCols() );

    return widgets[ indexOf( row, col ) ];
}

/*! Removes the widget (if there is any) set for the cell \a row, \a col.

  \sa cellWidget() setCellWidget()
*/

void QTable::clearCellWidget( int row, int col )
{
    if ( row < 0 || col < 0 || row > numRows() - 1 || col > numCols() - 1 )
	return;

    if ( (int)widgets.size() != numRows() * numCols() )
	widgets.resize( numRows() * numCols() );

    QWidget *w = cellWidget( row, col );
    if ( w )
	w->removeEventFilter( this );
    widgets.remove( indexOf( row, col ) );
}

/*! \fn void QTable::dropped ( QDropEvent * e )

  This signal is emitted when a drop event occurred onto the table.

  \a e gives you all information about the drop.
*/

/*! If \a b is TRUE, the table starts a drag procedure (see dragObject())
  when the user presses and moves the mouse on a selected cell.
*/

void QTable::setDragEnabled( bool b )
{
    dEnabled = b;
}

/*! If this function returns TRUE, the table supports dragging.

  \sa setDragEnabled();
*/

bool QTable::dragEnabled() const
{
    return dEnabled;
}

#ifndef QT_NO_DRAGANDDROP

/*! Inserts \a count empty rows at the index \a row.

  \sa insertColumns() removeRow()
*/

void QTable::insertRows( int row, int count )
{
    if ( row < 0 || count <= 0 )
	return;

    row--;
    if ( row >= numRows() )
	return;

    setNumRows( numRows() + count );

    for ( int i = numRows() - count - 1; i > row; --i )
        ( (QTableHeader*)verticalHeader() )->swapSections( i, i + count );

    repaintContents( contentsX(), contentsY(), visibleWidth(), visibleHeight() );
}

/*! Inserts \a count empty columns at the index \a col.

  \sa insertRows() removeColumn()
*/

void QTable::insertColumns( int col, int count )
{
    if ( col < 0 || count <= 0 )
	return;

    col--;
    if ( col >= numCols() )
	return;

    setNumCols( numCols() + count );

    for ( int i = numCols() - count - 1; i > col; --i )
        ( (QTableHeader*)horizontalHeader() )->swapSections( i, i + count );

    repaintContents( contentsX(), contentsY(), visibleWidth(), visibleHeight() );
}

/*! Removes the row \a row.

  \sa hideRow() insertRows() removeColumn() removeRows()
*/

void QTable::removeRow( int row )
{
    if ( row < 0 || row >= numRows() )
	return;
    if ( row < numRows() - 1 ) {
	for ( int i = row; i < numRows() - 1; ++i )
	    ( (QTableHeader*)verticalHeader() )->swapSections( i, i + 1 );
    }
    setNumRows( numRows() - 1 );
}

/*! Removes the rows listed in the array \a rows.

   \sa removeRow() insertRows() removeColumns()
*/

void QTable::removeRows( const QArray<int> &rows )
{
    if ( rows.count() == 0 )
	return;
    int i;
    for ( i = 0; i < (int)rows.count() - 1; ++i ) {
	for ( int j = rows[i] - i; j < rows[i + 1] - i - 1; j++ ) {
	    ( (QTableHeader*)verticalHeader() )->swapSections( j, j + i + 1 );
	}
    }

    for ( int j = rows[i] - i; j < numRows() - (int)rows.size(); j++)
	( (QTableHeader*)verticalHeader() )->swapSections( j, j + rows.count() );

    setNumRows( numRows() - rows.count() );
}

/*! Removes the column \a col.

  \sa removeColumns() hideColumn() insertColumns() removeRow()
*/

void QTable::removeColumn( int col )
{
    if ( col < 0 || col >= numCols() )
	return;
    if ( col < numCols() - 1 ) {
	for ( int i = col; i < numCols() - 1; ++i )
	    ( (QTableHeader*)horizontalHeader() )->swapSections( i, i + 1 );
    }
    setNumCols( numCols() - 1 );
}

/*! Removes the columns listed in the array \a cols.

   \sa removeColumn() insertColumns() removeRows()
*/

void QTable::removeColumns( const QArray<int> &cols )
{
    if ( cols.count() == 0 )
	return;
    int i;
    for ( i = 0; i < (int)cols.count() - 1; ++i ) {
	for ( int j = cols[i] - i; j < cols[i + 1] - i - 1; j++ ) {
	    ( (QTableHeader*)horizontalHeader() )->swapSections( j, j + i + 1 );
	}
    }

    for ( int j = cols[i] - i; j < numCols() - (int)cols.size(); j++)
	( (QTableHeader*)horizontalHeader() )->swapSections( j, j + cols.count() );

    setNumCols( numCols() - cols.count() );
}

/*! This event handler is called whenever a QTable object receives a
  \l QDragEnterEvent \a e, i.e. when the user pressed the mouse
  button to drag something.

  The focus is moved to the cell where the QDragEnterEvent occured.
*/

void QTable::contentsDragEnterEvent( QDragEnterEvent *e )
{
    oldCurrentRow = curRow;
    oldCurrentCol = curCol;
    int tmpRow = rowAt( e->pos().y() );
    int tmpCol = columnAt( e->pos().x() );
    fixRow( tmpRow, e->pos().y() );
    fixCol( tmpCol, e->pos().x() );
    setCurrentCell( tmpRow, tmpCol );
    e->accept();
}

/*! This event handler is called whenever a QTable object receives a
  \l QDragMoveEvent \a e, i.e. when the user actually drags the mouse.

  The focus is moved to the cell where the QDragMoveEvent occured.
*/

void QTable::contentsDragMoveEvent( QDragMoveEvent *e )
{
    int tmpRow = rowAt( e->pos().y() );
    int tmpCol = columnAt( e->pos().x() );
    fixRow( tmpRow, e->pos().y() );
    fixCol( tmpCol, e->pos().x() );
    setCurrentCell( tmpRow, tmpCol );
    e->accept();
}

/*! This event handler is called when a drag activity leaves \e this
  QTable object.
*/

void QTable::contentsDragLeaveEvent( QDragLeaveEvent * )
{
    setCurrentCell( oldCurrentRow, oldCurrentCol );
}

/*! This event handler is called when the user ends a drag-and-drop
  activity by dropping something onto \e this QTable and thus
  triggers \a e.
*/

void QTable::contentsDropEvent( QDropEvent *e )
{
    setCurrentCell( oldCurrentRow, oldCurrentCol );
    emit dropped( e );
}

/*! If the user presses the mouse on a selected cell, starts moving,
  and dragEnabled() is TRUE, this function is called to obtain a drag
  object. A drag procedure using this object is started
  immediately unless dragObject() returns 0.

  By default this function returns 0. You might reimplement it and
  create a QDragObject depending on the selected items.

  \sa dropped()
*/

QDragObject *QTable::dragObject()
{
    return 0;
}

/*! Starts a drag procedure.

  Usually you don't need to call or reimplement this function
  yourself.

  \sa dragObject();
 */

void QTable::startDrag()
{
    if ( startDragRow == -1 || startDragCol == -1 )
	return;

    startDragRow = startDragCol = -1;
    mousePressed = FALSE;

    QDragObject *drag = dragObject();
    if ( !drag )
	return;

    drag->drag();
}

#endif

#ifndef QT_NO_ACCESSIBILITY

/*! \reimp */
QString	QTable::stateDescription() const
{
    int r = currentRow();
    int c = currentColumn();
    return tr( "current cell: %1, %2" ).arg( r+1 ).arg( c+1 );
}

/*! \reimp */
QString	QTable::useDescription() const
{
    return tr( "To change cell, use cursor keys." );
}

/*! \reimp */
QString	QTable::typeDescription() const
{
    return tr( "Table" );
}

#endif


/* \class QTableHeader qtable.h
  module table

  \brief The QTableHeader class allows for creation and manipulation of spreadsheet
  headers.

  As \l QTable objects are already outfitted with a horizontal and a vertical
  header you will rarely use this class. You can access them via QTable::horizontalHeader()
  and QTable::verticalHeader().
*/

/* \enum QTableHeader::SectionState

  This enum type denotes the state of a spreadsheet header.

  \value Normal    The section title appears in roman letters.
  \value Bold      The section title appears in bold letters.
  \value Selected  The section itself appears in a sunken fashion
                   ("pressed").
*/

/*! Creates a new table header object \a name with \a i sections as a child of
  the widget \a parent and attached to the table \a t.
*/

QTableHeader::QTableHeader( int i, QTable *t,
			    QWidget *parent, const char *name )
    : QHeader( i, parent, name ),
      table( t ), caching( FALSE ), numStretches( 0 )
{
    d = 0;
    states.resize( i );
    stretchable.resize( i );
    states.fill( Normal, -1 );
    stretchable.fill( FALSE, -1 );
    mousePressed = FALSE;
    autoScrollTimer = new QTimer( this );
    connect( autoScrollTimer, SIGNAL( timeout() ),
	     this, SLOT( doAutoScroll() ) );
    line1 = new QWidget( table->viewport() );
    line1->hide();
    line1->setBackgroundMode( PaletteText );
    table->addChild( line1 );
    line2 = new QWidget( table->viewport() );
    line2->hide();
    line2->setBackgroundMode( PaletteText );
    table->addChild( line2 );

    connect( this, SIGNAL( sizeChange( int, int, int ) ),
	     this, SLOT( sectionWidthChanged( int, int, int ) ) );
    connect( this, SIGNAL( indexChange( int, int, int ) ),
	     this, SLOT( indexChanged( int, int, int ) ) );

    stretchTimer = new QTimer( this );
    widgetStretchTimer = new QTimer( this );
    connect( stretchTimer, SIGNAL( timeout() ),
	     this, SLOT( updateStretches() ) );
    connect( widgetStretchTimer, SIGNAL( timeout() ),
	     this, SLOT( updateWidgetStretches() ) );
}

/*! Adds a new section with the section title \a s to \e this
  QTableHeader.

  If \a size is non-negative this value is used
  as the section width. With a negative value the section width
  depends on the length of the string \a s.
*/

void QTableHeader::addLabel( const QString &s , int size )
{
    states.resize( states.count() + 1 );
    states[ (int)states.count() - 1 ] = Normal;
    stretchable.resize( stretchable.count() + 1 );
    stretchable[ (int)stretchable.count() - 1 ] = FALSE;
    QHeader::addLabel( s , size );
}

/*! Defines a new SectionState \a astate for section \a s.

  \sa sectionState()
*/

void QTableHeader::setSectionState( int s, SectionState astate )
{
    if ( s < 0 || s >= (int)states.count() )
	return;
    if ( states[ s ] == astate )
	return;

    states[ s ] = astate;
    if ( isUpdatesEnabled() ) {
	if ( orientation() == Horizontal )
	    repaint( FALSE );
	else
	    repaint( FALSE );
    }
}

/*! Returns the SectionState of section \a s.

  \sa setSectionState()
*/

QTableHeader::SectionState QTableHeader::sectionState( int s ) const
{
    return (QTableHeader::SectionState)states[ s ];
}

/*! \reimp
*/

void QTableHeader::paintEvent( QPaintEvent *e )
{
    QPainter p( this );
    p.setPen( colorGroup().buttonText() );
    int pos = orientation() == Horizontal
		     ? e->rect().left()
		     : e->rect().top();
    int id = mapToIndex( sectionAt( pos + offset() ) );
    if ( id < 0 )
	if ( pos > 0 )
	    return;
	else
	    id = 0;
    for ( int i = id; i < count(); i++ ) {
	QRect r = sRect( i );
	p.save();
	if ( sectionState( i ) == Bold || sectionState( i ) == Selected ) {
	    QFont f( font() );
	    f.setBold( TRUE );
	    p.setFont( f );
	}
	paintSection( &p, i, r );
	p.restore();
	if ( orientation() == Horizontal && r. right() >= e->rect().right() ||
	     orientation() == Vertical && r. bottom() >= e->rect().bottom() )
	    return;
    }
}

/*! \reimp

  Paints the header section with the index \a index into the
  rectangular region \a fr on the painter \a p.
*/

void QTableHeader::paintSection( QPainter *p, int index, const QRect& fr )
{
    int section = mapToSection( index );
    if ( section < 0 )
	return;

    if ( sectionState( index ) != Selected ) {
	QHeader::paintSection( p, index, fr );
    } else {
	style().drawBevelButton( p, fr.x(), fr.y(), fr.width(), fr.height(),
				 colorGroup(), TRUE );
	paintSectionLabel( p, index, fr );
    }
}

static int real_pos( const QPoint &p, Qt::Orientation o )
{
    if ( o == Qt::Horizontal )
	return p.x();
    return p.y();
}

/*! \reimp
*/

void QTableHeader::mousePressEvent( QMouseEvent *e )
{
    QHeader::mousePressEvent( e );
    mousePressed = TRUE;
    pressPos = real_pos( e->pos(), orientation() );
    startPos = -1;
    setCaching( TRUE );
    resizedSection = -1;
#ifdef QT_NO_CURSOR
    isResizing = FALSE;
#else
    isResizing = cursor().shape() != ArrowCursor;
    if ( !isResizing && sectionAt( pressPos ) != -1 )
	doSelection( e );
#endif
}

/*! \reimp
*/

void QTableHeader::mouseMoveEvent( QMouseEvent *e )
{
    if ( !mousePressed
#ifndef QT_NO_CURSOR
	|| cursor().shape() != ArrowCursor
#endif
	    || ( ( e->state() & ControlButton ) == ControlButton &&
	   ( orientation() == Horizontal
	     ? table->columnMovingEnabled() : table->rowMovingEnabled() ) ) ) {
	QHeader::mouseMoveEvent( e );
	return;
    }

    if ( !doSelection( e ) )
	QHeader::mouseMoveEvent( e );
}

bool QTableHeader::doSelection( QMouseEvent *e )
{
    int p = real_pos( e->pos(), orientation() ) + offset();
    if ( startPos == -1 ) {
	startPos = p;
	int secAt = sectionAt( p );
	if ( ( e->state() & ControlButton ) != ControlButton
	     || table->selectionMode() == QTable::Single ) {
	    bool upd = TRUE;
	    if ( ( orientation() == Horizontal &&
		   table->isColumnSelected( secAt, TRUE ) ||
		   orientation() == Vertical &&
		   table->isRowSelected( secAt, TRUE ) ) &&
		 table->numSelections() == 1 )
		upd = FALSE;
	    table->viewport()->setUpdatesEnabled( upd );
	    table->clearSelection();
	    table->viewport()->setUpdatesEnabled( TRUE );
	}
	saveStates();
	if ( table->selectionMode() != QTable::NoSelection ) {
	    table->currentSel = new QTableSelection();
	    table->selections.append( table->currentSel );
	    if ( orientation() == Vertical ) {
		table->currentSel->init( secAt, 0 );
	    } else {
		table->currentSel->init( 0, secAt );
	    }
	}
    }
    if ( sectionAt( p ) != -1 )
	endPos = p;
    if ( startPos != -1 ) {
	updateSelections();
	p -= offset();
	if ( orientation() == Horizontal && ( p < 0 || p > width() ) ) {
	    doAutoScroll();
	    autoScrollTimer->start( 100, TRUE );
	} else if ( orientation() == Vertical && ( p < 0 || p > height() ) ) {
	    doAutoScroll();
	    autoScrollTimer->start( 100, TRUE );
	}
	return TRUE;
    }
    return FALSE;
}

/*! \reimp
*/

void QTableHeader::mouseReleaseEvent( QMouseEvent *e )
{
    autoScrollTimer->stop();
    mousePressed = FALSE;
    bool hasCached = resizedSection != -1;
    setCaching( FALSE );
    QHeader::mouseReleaseEvent( e );
    line1->hide();
    line2->hide();
    if ( hasCached ) {
	emit sectionSizeChanged( resizedSection );
	updateStretches();
    }
}

/*! \reimp
*/

void QTableHeader::mouseDoubleClickEvent( QMouseEvent *e )
{
    if ( isResizing ) {
	int p = real_pos( e->pos(), orientation() ) + offset();
	int section = sectionAt( p );
	if ( section == -1 )
	    return;
	section--;
	if ( p >= sectionPos( count() - 1 ) + sectionSize( count() - 1 ) )
	    ++section;
	if ( orientation() == Horizontal ) {
	    table->adjustColumn( section );
	    for ( int i = 0; i < table->numCols(); ++i ) {
		if ( table->isColumnSelected( i ) )
		    table->adjustColumn( i );
	    }
	} else {
	    table->adjustRow( section );
	    for ( int i = 0; i < table->numRows(); ++i ) {
		if ( table->isRowSelected( i ) )
		    table->adjustRow( i );
	    }
	}
    }
}

/*! \reimp
*/

void QTableHeader::resizeEvent( QResizeEvent *e )
{
    stretchTimer->stop();
    widgetStretchTimer->stop();
    QHeader::resizeEvent( e );
    if ( numStretches == 0 )
	return;
    stretchTimer->start( 0, TRUE );
}

void QTableHeader::updateStretches()
{
    if ( numStretches == 0 )
	return;
    if ( orientation() == Horizontal ) {
	if ( sectionPos( count() - 1 ) + sectionSize( count() - 1 ) ==
	     width() )
	    return;
	int i;
	int pw = width() -
		 ( sectionPos( count() - 1 ) + sectionSize( count() - 1 ) ) -
		 1;
	bool block = signalsBlocked();
	blockSignals( TRUE );
	for ( i = 0; i < (int)stretchable.count(); ++i ) {
	    if ( !stretchable[ i ] )
		continue;
	    pw += sectionSize( i );
	}
	pw /= numStretches;
	for ( i = 0; i < (int)stretchable.count(); ++i ) {
	    if ( !stretchable[ i ] )
		continue;
	    if ( i == (int)stretchable.count() - 1 &&
		 sectionPos( i ) + pw < width() )
		pw = width() - sectionPos( i );
	    resizeSection( i, QMAX( 20, pw ) );
	}
	blockSignals( block );
	table->viewport()->repaint( FALSE );
	widgetStretchTimer->start( 100, TRUE );
    } else {
	if ( sectionPos( count() - 1 ) + sectionSize( count() - 1 ) == height() )
	    return;
	int i;
	int ph = height() - ( sectionPos( count() - 1 ) + sectionSize( count() - 1 ) ) - 1;
	bool block = signalsBlocked();
	blockSignals( TRUE );
	for ( i = 0; i < (int)stretchable.count(); ++i ) {
	    if ( !stretchable[ i ] )
		continue;
	    ph += sectionSize( i );
	}
	ph /= numStretches;
	for ( i = 0; i < (int)stretchable.count(); ++i ) {
	    if ( !stretchable[ i ] )
		continue;
	    if ( i == (int)stretchable.count() - 1 && sectionPos( i ) + ph < height() )
		ph = height() - sectionPos( i );
	    resizeSection( i, QMAX( 20, ph ) );
	}
	blockSignals( block );
	table->viewport()->repaint( FALSE );
	widgetStretchTimer->start( 100, TRUE );
    }
}

void QTableHeader::updateWidgetStretches()
{
    QSize s = table->tableSize();
    table->resizeContents( s.width(), s.height() );
    for ( int i = 0; i < table->numCols(); ++i )
	table->updateColWidgets( i );
}

void QTableHeader::updateSelections()
{
    if ( table->selectionMode() == QTable::NoSelection )
	return;
    int a = sectionAt( startPos );
    int b = sectionAt( endPos );
    int start = QMIN( a, b );
    int end = QMAX( a, b );
    setUpdatesEnabled( FALSE );
    for ( int i = 0; i < count(); ++i ) {
	if ( i < start || i > end )
	    setSectionState( i, (QTableHeader::SectionState)oldStates[ i ] );
	else
	    setSectionState( i, Selected );
    }
    setUpdatesEnabled( TRUE );
    repaint( FALSE );

    QTableSelection oldSelection = *table->currentSel;
    if ( orientation() == Vertical )
	table->currentSel->expandTo( b, table->horizontalHeader()->count() - 1 );
    else
	table->currentSel->expandTo( table->verticalHeader()->count() - 1, b );
    table->repaintSelections( &oldSelection, table->currentSel,
			      orientation() == Horizontal,
			      orientation() == Vertical );
}

void QTableHeader::saveStates()
{
    oldStates.resize( count() );
    for ( int i = 0; i < count(); ++i )
	oldStates[ i ] = sectionState( i );
}

void QTableHeader::doAutoScroll()
{
    QPoint pos = mapFromGlobal( QCursor::pos() );
    int p = real_pos( pos, orientation() ) + offset();
    if ( sectionAt( p ) != -1 )
	endPos = p;
    if ( orientation() == Horizontal )
	table->ensureVisible( endPos, table->contentsY() );
    else
	table->ensureVisible( table->contentsX(), endPos );
    updateSelections();
    autoScrollTimer->start( 100, TRUE );
}

void QTableHeader::sectionWidthChanged( int col, int, int )
{
    resizedSection = col;
    if ( orientation() == Horizontal ) {
	table->moveChild( line1, QHeader::sectionPos( col ) - 1,
			  table->contentsY() );
	line1->resize( 1, table->visibleHeight() );
	line1->show();
	line1->raise();
	table->moveChild( line2,
			  QHeader::sectionPos( col ) + QHeader::sectionSize( col ) - 1,
			  table->contentsY() );
	line2->resize( 1, table->visibleHeight() );
	line2->show();
	line2->raise();
    } else {
	table->moveChild( line1, table->contentsX(),
			  QHeader::sectionPos( col ) - 1 );
	line1->resize( table->visibleWidth(), 1 );
	line1->show();
	line1->raise();
	table->moveChild( line2, table->contentsX(),
			  QHeader::sectionPos( col ) + QHeader::sectionSize( col ) - 1 );
	line2->resize( table->visibleWidth(), 1 );
	line2->show();
	line2->raise();
    }
}

/*! \reimp

  Returns the size of section \a section in pixels and \e -1
  when there is no such \a section.
*/

int QTableHeader::sectionSize( int section ) const
{
    if ( count() <= 0 || section < 0 )
	return -1;
    if ( caching )
	return sectionSizes[ section ];
    return QHeader::sectionSize( section );
}

/*! \reimp

  Returns the start position of section \a section in pixels
  and \e -1 when there is no such section.

  \sa sectionAt()
*/

int QTableHeader::sectionPos( int section ) const
{
    if ( count() <= 0 || section < 0 )
	return -1;
    if ( caching )
	return sectionPoses[ section ];
    return QHeader::sectionPos( section );
}

/*! \reimp

  Returns the section that contains position \a pos
  (in pixels) and \e -1 otherwise.

  \sa sectionPos()
*/

int QTableHeader::sectionAt( int pos ) const
{
    if ( !caching )
	return QHeader::sectionAt( pos );
    if ( count() <= 0 || pos > sectionPoses[ count() - 1 ] + sectionSizes[ count() - 1 ] )
	return -1;
    int l = 0;
    int r = count() - 1;
    int i = ( (l+r+1) / 2 );
    while ( r - l ) {
	if ( sectionPoses[i] > pos )
	    r = i -1;
	else
	    l = i;
	i = ( (l+r+1) / 2 );
    }
    if ( sectionPoses[i] <= pos &&
	 pos <= sectionPoses[i] + sectionSizes[ mapToSection( i ) ] )
	return mapToSection( i );
    return -1;
}

void QTableHeader::setCaching( bool b )
{
    if ( caching == b )
	return;
    caching = b;
    sectionPoses.resize( count() );
    sectionSizes.resize( count() );
    if ( b ) {
	for ( int i = 0; i < count(); ++i ) {
	    sectionSizes[ i ] = QHeader::sectionSize( i );
	    sectionPoses[ i ] = QHeader::sectionPos( i );
	}
    }
}

/*! Makes section \a s stretcheable if \a b is TRUE
  and prevents resizing of \e this section if \a b
  is FALSE.

  \sa isSectionStretchable()
*/

void QTableHeader::setSectionStretchable( int s, bool b )
{
    if ( stretchable[ s ] == b )
	return;
    stretchable[ s ] = b;
    if ( b )
	numStretches++;
    else
	numStretches--;
}

/*! Returns whether section \a s is stretcheable or not.

  \sa setSectionStretchable()
*/

bool QTableHeader::isSectionStretchable( int s ) const
{
    return stretchable[ s ];
}

void QTableHeader::swapSections( int oldIdx, int newIdx )
{
    QIconSet is;
    bool his = FALSE;
    if ( iconSet( oldIdx ) ) {
	his = TRUE;
	is = *iconSet( oldIdx );
    }
    QString l = label( oldIdx );
    if ( iconSet( newIdx ) )
	setLabel( oldIdx, *iconSet( newIdx ), label( newIdx ) );
    else
	setLabel( oldIdx, label( newIdx ) );

    if ( his )
	setLabel( newIdx, is, l );
    else
	setLabel( newIdx, l );

    int w1 = sectionSize( oldIdx );
    int w2 = sectionSize( newIdx );
    resizeSection( oldIdx, w2 );
    resizeSection( newIdx, w1 );

    if ( orientation() == Horizontal )
	table->swapColumns( oldIdx, newIdx );
    else
	table->swapRows( oldIdx, newIdx );
}

void QTableHeader::indexChanged( int sec, int oldIdx, int newIdx )
{
    newIdx = mapToIndex( sec );
    if ( oldIdx > newIdx )
	moveSection( sec, oldIdx + 1 );
    else
	moveSection( sec, oldIdx );

    if ( oldIdx < newIdx ) {
	while ( oldIdx < newIdx ) {
	    swapSections( oldIdx, oldIdx + 1 );
	    oldIdx++;
	}
    } else {
	while ( oldIdx > newIdx ) {
	    swapSections( oldIdx - 1, oldIdx );
	    oldIdx--;
	}
    }

    table->repaintContents( table->contentsX(), table->contentsY(),
			    table->visibleWidth(), table->visibleHeight() );
}

#include "qtable.moc"

#endif // QT_NO_TABLE


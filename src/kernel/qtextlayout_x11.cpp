#include "qtextlayout.h"
#include "qtextengine_p.h"

#include <qfont.h>
#include <qapplication.h>
#include <qpainter.h>


int QTextItem::x() const
{
    return engine->items[item].x;
}

int QTextItem::y() const
{
    return engine->items[item].y;
}

int QTextItem::width() const
{
    return engine->items[item].width;
}

int QTextItem::ascent() const
{
    return engine->items[item].ascent;
}

int QTextItem::descent() const
{
    return engine->items[item].descent;
}

int QTextItem::baselineAdjustment() const
{
    return engine->items[item].baselineAdjustment;
}


void QTextItem::setWidth( int w )
{
    engine->items[item].width = w;
}

void QTextItem::setAscent( int a )
{
    engine->items[item].ascent = a;
}

void QTextItem::setDescent( int d )
{
    engine->items[item].descent = d;
}


void QTextItem::setBaselineAdjustment( int adjust )
{
    engine->items[item].baselineAdjustment = adjust;
}


void QTextItem::setFont( const QFont & f )
{
    engine->setFont( item, f.d );
}

int QTextItem::from() const
{
    return engine->items[item].position;
}

int QTextItem::length() const
{
    return engine->length(item);
}


int QTextItem::cursorToX( int *cPos, Edge edge )
{
    int pos = *cPos;
    QScriptItem &si = engine->items[item];

    const QShapedItem *shaped = si.shaped;
    if ( !shaped )
	shaped = engine->shape( item );

    int l = engine->length( item );
    if ( pos > l )
	pos = l;
    if ( pos < 0 )
	pos = 0;

    int glyph_pos = pos == l ? shaped->num_glyphs : shaped->logClusters[pos];
    if ( edge == Trailing ) {
	// trailing edge is leading edge of next cluster
	while ( glyph_pos < shaped->num_glyphs && !shaped->glyphAttributes[glyph_pos].clusterStart )
	    glyph_pos++;
    }

    int x = 0;
    bool reverse = engine->items[item].analysis.bidiLevel % 2;

    if ( reverse ) {
	for ( int i = shaped->num_glyphs-1; i >= glyph_pos; i-- )
	    x += shaped->advances[i].x;
    } else {
	for ( int i = 0; i < glyph_pos; i++ )
	    x += shaped->advances[i].x;
    }
//     qDebug("cursorToX: pos=%d, gpos=%d x=%d", pos, glyph_pos, x );
    *cPos = pos;
    return x;
}

int QTextItem::xToCursor( int x )
{
    QScriptItem &si = engine->items[item];
    const QShapedItem *shaped = si.shaped;
    if ( !shaped )
	shaped = engine->shape( item );

    int l = engine->length( item );
    bool reverse = si.analysis.bidiLevel % 2;
    if ( x < 0 )
	return reverse ? l : 0;


    if ( reverse ) {
	int width = 0;
	for ( int i = 0; i < shaped->num_glyphs; i++ ) {
	    width += shaped->advances[i].x;
	}
	x = -x + width;
    }
    int cp_before = 0;
    int cp_after = 0;
    int x_before = 0;
    int x_after = 0;

    int lastCluster = 0;
    for ( int i = 1; i <= l; i++ ) {
	int newCluster = i < l ? shaped->logClusters[i] : shaped->num_glyphs;
	if ( newCluster != lastCluster ) {
	    // calculate cluster width
	    cp_before = cp_after;
	    x_before = x_after;
	    cp_after = i;
	    for ( int j = lastCluster; j < newCluster; j++ )
		x_after += shaped->advances[j].x;
	    // 		qDebug("cluster boundary: lastCluster=%d, newCluster=%d, x_before=%d, x_after=%d",
	    // 		       lastCluster, newCluster, x_before, x_after );
	    if ( x_after > x )
		break;
	    lastCluster = newCluster;
	}
    }

    bool before = (x - x_before) < (x_after - x);
//     qDebug("got cursor position for %d: %d/%d, x_ba=%d/%d using %d",
// 	   x, cp_before,cp_after,  x_before, x_after,  before ? cp_before : cp_after );

    return before ? cp_before : cp_after;

}


bool QTextItem::isRightToLeft() const
{
    return (engine->items[item].analysis.bidiLevel % 2);
}

bool QTextItem::isObject() const
{
    return (engine->string.at(engine->items[item].position).unicode() == 0xfffc);
}




QTextLayout::QTextLayout( const QString &string, QPainter *p )
{
    QFontPrivate *f = p ? p->font().d : QApplication::font().d;
    d = new QTextEngine( string, f );
}

QTextLayout::QTextLayout( const QString &string, const QFont &fnt )
{
    d = new QTextEngine( string, fnt.d );
}

QTextLayout::~QTextLayout()
{
    delete d;
}

/* add an additional item boundary eg. for style change */
void QTextLayout::setBoundary( int strPos )
{
    if ( strPos <= 0 || strPos >= (int)d->string.length() )
	return;

    int itemToSplit = 0;
    while ( itemToSplit < d->items.size() && d->items[itemToSplit].position <= strPos )
	itemToSplit++;
    itemToSplit--;
    if ( d->items[itemToSplit].position == strPos ) {
	// already a split at the requested position
	return;
    }
    d->items.split( itemToSplit, strPos - d->items[itemToSplit].position );
}


bool QTextLayout::validCursorPosition( int strPos )
{
    return d->attributes()[strPos].charStop;
}


int QTextLayout::numItems() const
{
    return d->items.size();
}

QTextItem QTextLayout::itemAt( int i )
{
    return QTextItem( i, d );
}


void QTextLayout::beginLayout()
{
    d->items.clear();
    d->itemize();
    d->currentItem = 0;
    d->firstItemInLine = -1;
}

void QTextLayout::beginLine( int width )
{
    d->lineWidth = width;
    d->widthUsed = 0;
    d->firstItemInLine = -1;
}

bool QTextLayout::atEnd() const
{
    return d->currentItem == d->items.size();
}

QTextItem QTextLayout::nextItem()
{
    d->currentItem++;
    d->shape( d->currentItem );
    return QTextItem( d->currentItem, d );
}

QTextItem QTextLayout::currentItem()
{
    d->shape( d->currentItem );
    return QTextItem( d->currentItem, d );
}

/* ## maybe also currentItem() */
void QTextLayout::setLineWidth( int newWidth )
{
    d->lineWidth = newWidth;
}

int QTextLayout::lineWidth() const
{
    return d->lineWidth;
}

int QTextLayout::availableWidth() const
{
    return d->lineWidth - d->widthUsed;
}


/* returns true if completely added */
QTextLayout::Result QTextLayout::addCurrentItem()
{
    if ( d->firstItemInLine == -1 )
	d->firstItemInLine = d->currentItem;
    QScriptItem &current = d->items[d->currentItem];
    if ( !current.shaped )
	d->shape( d->currentItem );
    d->widthUsed += current.width;
//     qDebug("trying to add item %d with width %d, remaining %d", d->currentItem, current.width, d->lineWidth-d->widthUsed );

    d->currentItem++;

    return (d->widthUsed > d->lineWidth) ? LineFull : Ok;
}

QTextLayout::Result QTextLayout::endLine( int x, int y, Qt::AlignmentFlags alignment, int *ascent, int *descent )
{
//     qDebug("endLine x=%d, y=%d, first=%d, current=%d", x,  y, d->firstItemInLine, d->currentItem );
    if ( d->firstItemInLine == -1 )
	return LineEmpty;

    if ( d->widthUsed > d->lineWidth ) {
	// find linebreak
	const QCharAttributes *attrs = d->attributes();
	int w = 0;
	int itemWidth = 0;
	int breakItem = d->firstItemInLine;
	int breakPosition = -1;
	int breakGlyph = 0;
#if 0
	// we iterate backwards or forward depending on what we guess is closer
	if ( d->widthUsed - d->lineWidth < d->lineWidth ) {
	    // backwards search should be faster

	} else
#endif
	{
	    int tmpWidth = 0;
	    bool lastWasSpace = FALSE;
	    // forward search is probably faster
	    for ( int i = d->firstItemInLine; i < d->currentItem; i++ ) {
		const QScriptItem &si = d->items[i];
		int length = d->length( i );
		const QCharAttributes *itemAttrs = attrs + si.position;

		const QShapedItem *shaped = si.shaped;
		const offset_t *advances = shaped->advances;

		int lastGlyph = 0;
		int tmpItemWidth = 0;

// 		qDebug("looking for break in item %d", i );

		for ( int pos = 0; pos < length; pos++ ) {
// 		    qDebug("advance=%d, tmpWidth=%d, softbreak=%d, whitespace=%d",
// 			   advances->x, tmpWidth, itemAttrs->softBreak, itemAttrs->whiteSpace );
		    int glyph = shaped->logClusters[pos];
		    if ( lastGlyph != glyph ) {
			while ( lastGlyph < glyph )
			    tmpItemWidth += advances[lastGlyph++].x;
			if ( w + tmpWidth + tmpItemWidth > d->lineWidth )
			    goto found;
		    }
		    if ( lastWasSpace || itemAttrs->softBreak ) {
			if ( breakItem != i )
			    itemWidth = 0;
			breakItem = i;
			breakPosition = pos;
// 			qDebug("found possible break at item %d, position %d (absolute=%d), tmpWidth=%d, tmpItemWidth=%d", breakItem, breakPosition, d->items[breakItem].position+breakPosition, tmpWidth, tmpItemWidth );
			breakGlyph = glyph;
			w += tmpWidth + tmpItemWidth;
			itemWidth += tmpItemWidth;
			tmpWidth = 0;
			tmpItemWidth = 0;
		    }
		    lastWasSpace = itemAttrs->whiteSpace;
		    itemAttrs++;
		}
		while ( lastGlyph < shaped->num_glyphs )
		    tmpItemWidth += advances[lastGlyph++].x;
		tmpWidth += tmpItemWidth;
		if ( w + tmpWidth > d->lineWidth )
		    goto found;
	    }
	}

    found:
	// no valid break point found
	if ( breakPosition == -1 ) {
// 	    qDebug("no valid linebreak found, returning empty line");
	    d->currentItem = d->firstItemInLine;
	    return LineEmpty;
	}

//  	qDebug("linebreak at item %d, position %d, glyph %d", breakItem, breakPosition, breakGlyph );
	// split the line
	QScriptItem &endItem = d->items[breakItem];
	if ( breakPosition > 0 ) {
	    int length = d->length( breakItem );

//  	    qDebug("splitting item, itemWidth=%d", itemWidth);
	    QShapedItem *shaped = endItem.shaped;
	    // not a full item, need to break
	    d->items.split( breakItem, breakPosition );
// 	    qDebug("new items are at %d (len=%d) and %d (len=%d)", endItem.position, d->length(breakItem),
// 		   d->items[breakItem+1].position, d->length(breakItem+1) );

	    // split the shapedItem
	    QShapedItem *split = new QShapedItem;
	    split->ownGlyphs = FALSE;
	    split->num_glyphs = shaped->num_glyphs - breakGlyph;
	    split->glyphs = shaped->glyphs + breakGlyph;
	    split->offsets = shaped->offsets + breakGlyph;
	    split->advances = shaped->advances + breakGlyph;
	    split->glyphAttributes = shaped->glyphAttributes + breakGlyph;
	    split->logClusters = shaped->logClusters + breakPosition;
	    for ( int i = 0; i < length-breakPosition; i++ )
		split->logClusters[i] -= breakGlyph;

	    shaped->num_glyphs = breakGlyph;

	    d->items[breakItem + 1].shaped = split;
	    d->items[breakItem + 1].width = endItem.width - itemWidth;
	    endItem.width = itemWidth;
	    d->currentItem = breakItem+1;
	} else {
	    d->currentItem = breakItem;
	}
    }

    // position the objects in the line
    int available = d->lineWidth - d->widthUsed;

    // ### FIXME
    if ( alignment == Qt::AlignAuto )
	alignment = Qt::AlignLeft;

    int numRuns = d->currentItem - d->firstItemInLine;
    Q_UINT8 _levels[128];
    int _visual[128];
    Q_UINT8 *levels = _levels;
    int *visual = _visual;
    if ( numRuns > 127 ) {
	levels = new Q_UINT8[numRuns];
	visual = new int[numRuns];
    }
    int i;
//     qDebug("reordering %d runs:", numRuns );
    for ( i = 0; i < numRuns; i++ ) {
	levels[i] = d->items[i+d->firstItemInLine].analysis.bidiLevel;
// 	qDebug("    level = %d", d->items[i+d->firstItemInLine].analysis.bidiLevel );
    }
    d->bidiReorder( numRuns, levels, visual );

    if ( alignment & Qt::AlignJustify ) {
	// #### justify items
    }

    if ( alignment & Qt::AlignRight )
	x += available;
    else if ( alignment & Qt::AlignCenter )
	x += available/2;


    int asc = 0;
    int desc = 0;

    for ( i = 0; i < numRuns; i++ ) {
	QScriptItem &si = d->items[d->firstItemInLine+visual[i]];
	asc = QMAX( asc, si.ascent );
	desc = QMAX( desc, si.descent );
    }

    for ( i = 0; i < numRuns; i++ ) {
	QScriptItem &si = d->items[d->firstItemInLine+visual[i]];
//  	qDebug("positioning item %d with width %d (from=%d/length=%d) at %d", d->firstItemInLine+visual[i], si.width, si.position,
// 	       d->length(d->firstItemInLine+visual[i]), x );
	si.x = x;
	si.y = y + asc;
	x += si.width;
    }

    if ( ascent )
	*ascent = asc;
    if ( descent )
	*descent = desc;
    return Ok;
}

void QTextLayout::endLayout()
{
    for ( int i = 0; i < d->items.size(); i++ ) {
	QScriptItem &si = d->items[i];
	delete si.shaped;
	si.shaped = 0;
    }
}


int QTextLayout::nextCursorPosition( int oldPos, CursorMode mode ) const
{
    qDebug("looking for next cursor pos for %d", oldPos );
    const QCharAttributes *attributes = d->attributes();
    int len = d->string.length();
    if ( oldPos >= len )
	return oldPos;
    oldPos++;
    if ( mode == SkipCharacters ) {
	while ( oldPos < len - 1 && !attributes[oldPos].charStop )
	    oldPos++;
    } else {
	while ( oldPos < len - 1 && !attributes[oldPos].wordStop )
	    oldPos++;
    }
    qDebug("  -> %d",  oldPos );
    return oldPos;
}

int QTextLayout::previousCursorPosition( int oldPos, CursorMode mode ) const
{
    qDebug("looking for previous cursor pos for %d", oldPos );
    const QCharAttributes *attributes = d->attributes();
    if ( oldPos <= 0 )
	return 0;
    oldPos--;
    if ( mode == SkipCharacters ) {
	while ( oldPos && !attributes[oldPos].charStop )
	    oldPos--;
    } else {
	while ( oldPos && !attributes[oldPos].wordStop )
	    oldPos--;
    }
    qDebug("  -> %d",  oldPos );
    return oldPos;
}

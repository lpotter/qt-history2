/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qtextlayout.h"
#include "qtextengine_p.h"

#include <qfont.h>
#include <qapplication.h>
#include <qpainter.h>
#include <qvarlengtharray.h>
#include <qtextformat.h>
#include <qabstracttextdocumentlayout.h>
#include "qtextformat_p.h"
#include <limits.h>

#include "qfontengine_p.h"

/*!
  \class QTextInlineObject

  Represents an inline object in a QTextLayout.

  Only used if the text layout is used to layout parts of a
  QTextDocument.
*/

QRect QTextInlineObject::rect() const
{
    QScriptItem& si = eng->items[itm];
    return QRect(0, -si.ascent.toInt(), si.width.toInt(), (si.ascent+si.descent).toInt());
}

int QTextInlineObject::width() const
{
    return eng->items[itm].width.toInt();
}

int QTextInlineObject::ascent() const
{
    return eng->items[itm].ascent.toInt();
}

int QTextInlineObject::descent() const
{
    return eng->items[itm].descent.toInt();
}

void QTextInlineObject::setWidth(int w)
{
    eng->items[itm].width = w;
}

void QTextInlineObject::setAscent(int a)
{
    eng->items[itm].ascent = a;
}

void QTextInlineObject::setDescent(int d)
{
    eng->items[itm].descent = d;
}

/*!
  The position of the inline object within the text layout.
*/
int QTextInlineObject::at() const
{
    return eng->items[itm].position;
}

/*!
  Returns an integer describing the format of the inline object
  within the text layout.
*/
int QTextInlineObject::formatIndex() const
{
    return eng->items[itm].format;
}

/*!
  Returns format of the inline object within the text layout.
*/
QTextFormat QTextInlineObject::format() const
{
    if (!eng->formats)
        return QTextFormat();
    return eng->formats->format(eng->items[itm].format);
}

/*!
  Returns if the object should be laid out right-to-left or left-to-right.
*/
bool QTextInlineObject::isRightToLeft() const
{
    return (eng->items[itm].analysis.bidiLevel % 2);
}

/*!
  \class QTextLayout

  The QTextLayout class is a class to layout and paint a single
  paragraph of text.

  It offers most features expected from a modern text layouting engine
  as Unicode compliant rendering, line breaking and handling of cursor
  positioning. It can also produce and render device independent
  layout, something that is important for WYSIWYG applications.

  The class has a rather low level API and unless you intend to
  implement you own text rendering for some specialized widget, you
  probably won't need to use it directly.

  QTextLayout can currently deal with plain text and rich text paragrpahs that
  are part of a \a QTextDocument.

  QTextLayout can be used to create a sequence of QTextLine's with
  given width and position them independently on the screen. Once the
  layouting is done, these lines can be drawn on a paint device.


  ### Mark: an example of usage that is rather simple can be found in qpainter.cpp:qt_format_text:

  layouting phase:

        int leading = fontMetrics.leading();
        int height = 0;
        int widthUsed = 0;
        textLayout.clearLines();
        while (1) {
            QTextLine l = textLayout.createLine();
            if (!l.isValid())
                break;

            l.layout(lineWidth);
            height += leading;
            l.setPosition(QPoint(0, height));
            height += l.ascent() + l.descent();
            widthUsed = qMax(widthUsed, l.textWidth());
        }

  painting phase:

        for (int i = 0; i < textLayout.numLines(); i++) {
            QTextLine line = textLayout.lineAt(i);
            line.draw(painter, r.x() + xoff + line.x(), r.y() + yoff);
        }

*/


/*!
  constructs an empty text layout
*/
QTextLayout::QTextLayout()
{ d = new QTextEngine(); }

/*!
  constructs a text layout to layout the string \a string.
*/
QTextLayout::QTextLayout(const QString& string)
{
    d = new QTextEngine();
    d->setText(string);
}

/*!
  constructs a text layout to layout the string \a string.

  The QPainter \a p is used for all calculating metrics and lyout in device
*/
QTextLayout::QTextLayout(const QString& string, QPainter *p)
{
    QFontPrivate *f = p ? (p->font().d) : QApplication::font().d;
    d = new QTextEngine((string.isNull() ? (const QString&)QString::fromLatin1("") : string), f);
}

/*!
  constructs a text layout to layout the string \a string with the font \a fnt.
*/
QTextLayout::QTextLayout(const QString& string, const QFont& fnt)
{
    d = new QTextEngine((string.isNull() ? (const QString&)QString::fromLatin1("") : string), fnt.d);
}

/*!
  destructs the layout
*/
QTextLayout::~QTextLayout()
{
    delete d;
}

// ####### go away!
/*!
  \internal
*/
void QTextLayout::setText(const QString& string, const QFont& fnt)
{
    delete d;
    d = new QTextEngine((string.isNull() ? (const QString&)QString::fromLatin1("") : string), fnt.d);
}

/*!
  \internal
*/
void QTextLayout::setFormatCollection(const QTextFormatCollection *formats)
{
    d->setFormatCollection(formats);
}

/*!
  Sets the text of the layout to \a string. The layout is
  invalidated and you will have to redo it.
*/
void QTextLayout::setText(const QString& string)
{
    d->setText(string);
}

/*!
  returns the current text of the layout.
*/
QString QTextLayout::text() const
{
    return d->string;
}

/*!
  \internal
*/
void QTextLayout::setDocumentLayout(QAbstractTextDocumentLayout *layout)
{
    d->setDocumentLayout(layout);
}

/*!
  \internal
*/
void QTextLayout::setFormat(int from, int length, int format)
{
    if (d->items.size() == 0)
        d->itemize(QTextEngine::Full);
    d->setFormat(from, length, format);
}

/*!
  \internal
*/
void QTextLayout::setTextFlags(int textFlags)
{
    d->textFlags = textFlags;
}

/*!
  Tells the layout to use design metrics for layouting.

  The default value is false.
*/
void QTextLayout::useDesignMetrics(bool b)
{
    d->designMetrics = b;
}

bool QTextLayout::usesDesignMetrics() const
{
    return d->designMetrics;
}

/*!
  \internal
*/
void QTextLayout::setPalette(const QPalette &p, PaletteFlags f)
{
    if (!d->pal)
        d->pal = new QPalette(p);
    else
        *d->pal = p;
    d->textColorFromPalette = (f & UseTextColor);
}

/*!
  \internal
*/
void QTextLayout::beginLayout(QTextLayout::LayoutMode m, int textFlags)
{
    d->items.clear();
    QTextEngine::Mode mode = QTextEngine::Full;
    if (m == NoBidi)
        mode = QTextEngine::NoBidi;
    else if (m == SingleLine)
        mode = QTextEngine::SingleLine;
    d->itemize(mode);
    d->textFlags = textFlags;
}

/*!
  Returns the next valid cursor position after \a oldPos.
*/
int QTextLayout::nextCursorPosition(int oldPos, CursorMode mode) const
{
//     qDebug("looking for next cursor pos for %d", oldPos);
    const QCharAttributes *attributes = d->attributes();
    int len = d->string.length();
    if (oldPos >= len)
        return oldPos;
    oldPos++;
    if (mode == SkipCharacters) {
        while (oldPos < len && !attributes[oldPos].charStop)
            oldPos++;
    } else {
        while (oldPos < len && !attributes[oldPos].wordStop && !attributes[oldPos-1].whiteSpace)
            oldPos++;
    }
//     qDebug("  -> %d",  oldPos);
    return oldPos;
}

/*!
  Returns the first valid cursor position before \a oldPos.
*/
int QTextLayout::previousCursorPosition(int oldPos, CursorMode mode) const
{
//     qDebug("looking for previous cursor pos for %d", oldPos);
    const QCharAttributes *attributes = d->attributes();
    if (oldPos <= 0)
        return 0;
    oldPos--;
    if (mode == SkipCharacters) {
        while (oldPos && !attributes[oldPos].charStop)
            oldPos--;
    } else {
        while (oldPos && !attributes[oldPos].wordStop && !attributes[oldPos-1].whiteSpace)
            oldPos--;
    }
//     qDebug("  -> %d",  oldPos);
    return oldPos;
}

/*!
  returns true is the position \a pos is a valid cursor position.

  In a Unicode context some positions in the string are not valid
  cursor positions, becuase the position is inside a Unicode surrogate
  or a so called grapheme cluster.

  A Grapheme cluster is a sequence of several Unicode characters that
  form one individable entity on the screen. An example in the latin
  script is 0x41 'A' and 0x308 (Combining diaresis), that would render
  as a '�' on the screen. In indic languages every syllable forms a
  grapheme cluster.
*/
bool QTextLayout::validCursorPosition(int pos) const
{
    const QCharAttributes *attributes = d->attributes();
    if (pos < 0 || pos > (int)d->string.length())
        return false;
    return attributes[pos].charStop;
}


/*!
  clear the layout information stored in the layout, and begins a new layouting
  process.
*/
void QTextLayout::clearLines()
{
    d->lines.clear();
    // invalidate bounding rect
    d->boundingRect = QRect();
}

/*!
  Creates a new text line for layouting.
*/
QTextLine QTextLayout::createLine()
{
    int l = d->lines.size();
    int from = l > 0 ? d->lines.at(l-1).from + d->lines.at(l-1).length : 0;
    if (l && from >= d->string.length())
        return QTextLine();

    QScriptLine line;
    line.from = from;
    line.length = 0;
    line.justified = false;
    line.gridfitted = false;

    d->lines.append(line);
    return QTextLine(l, d);
}

/*!
  The number of lines contained in the layout
*/
int QTextLayout::numLines() const
{
    return d->lines.size();
}

/*!
  returns the i'th line.
*/
QTextLine QTextLayout::lineAt(int i) const
{
    return QTextLine(i, d);
}

/*!
  returns the line that contains the cursor position \a pos.
*/
QTextLine QTextLayout::findLine(int pos) const
{
    for (int i = 0; i < d->lines.size(); ++i) {
        const QScriptLine& line = d->lines[i];
        if (line.from + (int)line.length > pos)
            return QTextLine(i, d);
    }
    if (pos == d->string.length() && d->lines.size())
        return QTextLine(d->lines.size()-1, d);
    return QTextLine();
}

/*!
  The global position of the layout. Independent of the bounding rect and
  layouting process.

  This can be used to move the whole layout to a different place.
*/
QPoint QTextLayout::position() const
{
    return d->position;
}

void QTextLayout::setPosition(const QPoint &p)
{
    d->position = p;
}

/*!
  The smallest rectangle that contains all lines in the layout
*/
QRect QTextLayout::boundingRect() const
{
    if (!d->boundingRect.isValid()) {
        Q26Dot6 xmin, xmax, ymin, ymax;
        for (int i = 0; i < d->lines.size(); ++i) {
            const QScriptLine &si = d->lines[i];
            xmin = qMin(xmin, si.x);
            ymin = qMin(ymin, si.y);
            xmax = qMax(xmax, si.x+si.width);
            // ### shouldn't the ascent be used in ymin???
            ymax = qMax(ymax, si.y+si.ascent+si.descent);
        }
        d->boundingRect = QRect(xmin.toInt(), ymin.toInt(), (xmax-xmin).toInt(), (ymax-ymin).toInt());
    }
    return d->boundingRect;
}

/*!
  internal
*/
QRect QTextLayout::rect() const
{
    QRect r = boundingRect();
    r.moveBy(d->position);
    return r;
}

/*!
  The minimum width the layout would need (the width of the smallest non breakable
  substring in the layout

  This variable only contains a valid number after layouting.
*/
int QTextLayout::minimumWidth() const
{
    return d->minWidth.toInt();
}

/*!
  The maximum width the layout could expand to (basically the width of the whole text).

  This variable only contains a valid number after layouting.
*/
int QTextLayout::maximumWidth() const
{
    return d->maxWidth.toInt();
}

static void drawSelection(QPainter *p, QPalette *pal, QTextLayout::SelectionType type,
                          const QRect &rect, const QTextLine &line, const QPoint &pos, int selectionIdx)
{
    p->save();
    p->setClipRect(rect);
    QColor bg;
    QColor text;
    switch(type) {
    case QTextLayout::Highlight:
        bg = pal->highlight();
        text = pal->highlightedText();
        break;
    case QTextLayout::ImText:
        int h1, s1, v1, h2, s2, v2;
        pal->color(QPalette::Base).getHsv(&h1, &s1, &v1);
        pal->color(QPalette::Background).getHsv(&h2, &s2, &v2);
        bg.setHsv(h1, s1, (v1 + v2) / 2);
        break;
    case QTextLayout::ImSelection:
        bg = pal->text();
        text = pal->background();
        break;
    case QTextLayout::NoSelection:
        Q_ASSERT(false); // should never happen.
        return;
    }
    p->fillRect(rect, bg);
    if (text.isValid())
        p->setPen(text);
    line.draw(p, pos.x(), pos.y(), selectionIdx);
    p->restore();
    return;
}

/*!
  draw the whole layout.
*/
void QTextLayout::draw(QPainter *p, const QPoint &pos, int cursorPos, const Selection *selections, int nSelections, const QRect &cr) const
{
    Q_ASSERT(numLines() != 0);

    d->cursorPos = cursorPos;
    d->selections = selections;
    d->nSelections = nSelections;

    int clipy = INT_MIN;
    int clipe = INT_MAX;
    if (cr.isValid()) {
        clipy = cr.y() - pos.y();
        clipe = clipy + cr.height();
    }

    QPoint position = pos + d->position;

    for (int i = 0; i < d->lines.size(); i++) {
        QTextLine l(i, d);
        const QScriptLine &sl = d->lines[i];

        if (sl.y.toInt() > clipe || (sl.y + sl.ascent + sl.descent).toInt() < clipy)
            continue;

        int from = sl.from;
        int length = sl.length;

        l.draw(p, position.x(), position.y());
        if (selections) {
            for (int j = 0; j < nSelections; ++j) {
                const Selection &s = selections[j];
                if (s.type() != Highlight)
                    continue;
                if (!d->pal)
                    continue;

                if (s.from() + s.length() > from && s.from() < from+length) {
                    QRect highlight = QRect(QPoint(position.x() + l.cursorToX(qMax(s.from(), from)),
                                                   position.y() + sl.y.toInt()),
                                            QPoint(position.x() + l.cursorToX(qMin(s.from() + s.length(), from+length)) - 1,
                                                   position.y() + (sl.y + sl.ascent + sl.descent).toInt())).normalize();
                    drawSelection(p, d->pal, (QTextLayout::SelectionType)s.type(), highlight, l, position, j);
                }
            }
        }
        if (sl.from <= cursorPos && sl.from + (int)sl.length >= cursorPos) {
            int x = l.cursorToX(cursorPos);
            p->drawLine(position.x() + x, position.y() + sl.y.toInt(), position.x() + x, position.y() + (sl.y + sl.ascent + sl.descent).toInt());
        }
    }

    d->cursorPos = -1;
    d->selections = 0;
    d->nSelections = 0;
}


/*!
  \class QTextLine

  This class represents a line of text inside a QtextLayout.

  A line of text can be created using QTextLayout::createLine().

  After being created, the line can be filled using the layout(int
  width) method.
*/

/*!
  returns the bounding rectangle of the line.
*/
QRect QTextLine::rect() const
{
    const QScriptLine& si = eng->lines[i];
    return QRect(si.x.toInt(), si.y.toInt(), si.width.toInt(), (si.ascent + si.descent).toInt());
}

/*!
  the x position of the line.
*/
int QTextLine::x() const
{
    return eng->lines[i].x.toInt();
}

/*!
  the y position of the line.
*/
int QTextLine::y() const
{
    return eng->lines[i].y.toInt();
}

/*!
  the width position of the line as specified by the layout() method.
*/
int QTextLine::width() const
{
    return eng->lines[i].width.toInt();
}


/*!
  the ascent of the line.
*/
int QTextLine::ascent() const
{
    return eng->lines[i].ascent.toInt();
}

/*!
  the descent of the line.
*/
int QTextLine::descent() const
{
    return eng->lines[i].descent.toInt();
}

/*!
  the actual space of the line that is occupied by text. This is
  always smaller or equals to width().

  textWidth() equals the minimum width one could use in layout() that would result
  in the same line break position .
*/
int QTextLine::textWidth() const
{
    return eng->lines[i].textWidth.toInt();
}

/*!
  layout the line with a width of \a width. The line is filled from
  it's starting position with as many character as fit into the line.
*/
void QTextLine::layout(int width, BreakMode mode)
{
    int maxGlyphs = INT_MAX;
    if (mode == BreakGlyphs) {
        maxGlyphs = width;
        width = INT_MAX >> 6;
    }

    QScriptLine &line = eng->lines[i];
    line.width = width;
    line.length = 0;
    line.textWidth = 0;

    if (!eng->items.size()) {
        // ##### use block font
        if (eng->fnt) {
            QFontEngine *e = eng->fnt->engineForScript(QFont::Latin);
            line.ascent = e->ascent();
            line.descent = e->descent();
        }
        return;
    }

    Q_ASSERT(line.from < eng->string.length());

    bool breakany = eng->textFlags & Qt::BreakAnywhere;

    // #### binary search!
    int item;
    for (item = eng->items.size()-1; item > 0; --item) {
        if (eng->items[item].position <= line.from)
            break;
    }

    Q26Dot6 minw, spacew;
    int glyphCount = 0;

//     qDebug("from: %d:   item=%d, total %d width available %d/%d", line.from, item, eng->items.size(), line.width.value(), line.width.toInt());

    while (item < eng->items.size()) {
        eng->shape(item);
        const QCharAttributes *attributes = eng->attributes();
        const QScriptItem &current = eng->items[item];
        line.ascent = qMax(line.ascent, current.ascent);
        line.descent = qMax(line.descent, current.descent);

        if (current.isObject) {
            QTextFormat format = eng->formats->format(eng->items[item].format);
            if (eng->docLayout)
                eng->docLayout->layoutObject(QTextInlineObject(item, eng), format);
            if (line.length && !(eng->textFlags & Qt::SingleLine)) {
                if (line.textWidth + current.width > line.width || glyphCount > maxGlyphs)
                    goto found;
            }

            line.length++;
            // the width of the linesep doesn't count into the textwidth
            if (eng->string[current.position] == QChar::LineSeparator)
                goto found;
            line.textWidth += current.width;

            ++item;
            ++glyphCount;
            continue;
        }

        int length = eng->length(item);

        const QCharAttributes *itemAttrs = attributes + current.position;
        QGlyphLayout *glyphs = eng->glyphs(&current);
        unsigned short *logClusters = eng->logClusters(&current);

        int pos = qMax(0, line.from - current.position);

        do {
            int next = pos;

            Q26Dot6 tmpw;
            if (!itemAttrs[next].whiteSpace) {
                tmpw = spacew;
                spacew = 0;
                do {
                    int gp = logClusters[next];
                    do {
                        ++next;
                    } while (next < length && logClusters[next] == gp);
                    do {
                        tmpw += glyphs[gp].advance.x;
                        ++gp;
                    } while (gp < current.num_glyphs && !glyphs[gp].attributes.clusterStart);

                    Q_ASSERT((next == length && gp == current.num_glyphs) || logClusters[next] == gp);

                    ++glyphCount;
                } while (next < length && !itemAttrs[next].whiteSpace && !itemAttrs[next].softBreak && !(breakany && itemAttrs[next].charStop));
                minw = qMax(tmpw, minw);
            }

            if (itemAttrs[next].softBreak)
                breakany = false;

            while (next < length && itemAttrs[next].whiteSpace) {
                int gp = logClusters[next];
                do {
                    ++next;
                } while (next < length && logClusters[next] == gp);
                do {
                    spacew += glyphs[gp].advance.x;
                    ++gp;
                } while (gp < current.num_glyphs && !glyphs[gp].attributes.clusterStart);

                ++glyphCount;
                Q_ASSERT((next == length && gp == current.num_glyphs) || logClusters[next] == gp);
            }

//             qDebug("possible break at %d, chars (%d-%d) / glyphs (%d-%d): width %d, spacew=%d",
//                    current.position + next, pos, next, logClusters[pos], logClusters[next], tmpw.value(), spacew.value());

            if (line.length && tmpw.value() 
                && (line.textWidth + tmpw > line.width || glyphCount > maxGlyphs)
                && !(eng->textFlags & Qt::SingleLine))
                goto found;

            line.textWidth += tmpw;
            line.length += next - pos;

            pos = next;
        } while (pos < length);
        ++item;
    }
 found:
//     qDebug("line length = %d, ascent=%d, descent=%d, textWidth=%d (%d)", line.length, line.ascent.value(),
//            line.descent.value(), line.textWidth.value(), line.textWidth.toInt());
//     qDebug("        : '%s'", eng->string.mid(line.from, line.length).utf8());

    eng->minWidth = qMax(eng->minWidth, minw);
    eng->maxWidth += line.textWidth + spacew;
    // ##########################
}

/*!
  Move the line to position \a pos.
*/
void QTextLine::setPosition(const QPoint &pos)
{
    eng->lines[i].x = pos.x();
    eng->lines[i].y = pos.y();
}

/*!
  The start of the line from the beginning of the string passed to the QTextLayout.
*/
int QTextLine::from() const
{
    return eng->lines[i].from;
}

/*!
  The length of the text in the line.
*/
int QTextLine::length() const
{
    return eng->lines[i].length;
}

/*!
  Draws the line to the painter at position \a xpos /\a ypos. \a
  selection is reserved for internal use.
*/
void QTextLine::draw(QPainter *p, int xpos, int ypos, int selection) const
{
    const QScriptLine &line = eng->lines[i];

    if (!line.length)
        return;

    int lineEnd = line.from + line.length;
    // don't draw trailing spaces or take them into the layout.
    const QCharAttributes *attributes = eng->attributes();
    while (lineEnd > line.from && attributes[lineEnd-1].whiteSpace)
        --lineEnd;

    int firstItem = eng->findItem(line.from);
    int lastItem = eng->findItem(lineEnd - 1);
    int nItems = lastItem-firstItem+1;

    Q26Dot6 x(xpos);
    Q26Dot6 y(ypos);
    x += line.x;
    y += line.y + line.ascent;

    eng->justify(line);
    if (eng->textFlags & Qt::AlignRight)
        x += line.width - line.textWidth;
    else if (eng->textFlags & Qt::AlignHCenter)
        x += (line.width - line.textWidth)/2;

    QVarLengthArray<int> visualOrder(nItems);
    QVarLengthArray<uchar> levels(nItems);
    for (int i = 0; i < nItems; ++i)
        levels[i] = eng->items[i+firstItem].analysis.bidiLevel;
    QTextEngine::bidiReorder(nItems, levels.data(), visualOrder.data());

    QFont f = eng->font();
    for (int i = 0; i < nItems; ++i) {
        int item = visualOrder[i]+firstItem;
        QScriptItem &si = eng->items[item];

        if (si.isObject && eng->docLayout && eng->formats) {
            QTextFormat format = eng->formats->format(si.format);

            QTextLayout::SelectionType selType = QTextLayout::NoSelection;
            if (selection >= 0 && eng->selections && eng->nSelections > 0)
                // ###
                selType = static_cast<QTextLayout::SelectionType>(eng->selections[selection].type());

            eng->docLayout->drawObject(p, QRect(x.toInt(), (y-si.ascent).toInt(), si.width.toInt(), (si.ascent+si.descent).toInt()),
                                       QTextInlineObject(item, eng), format, selType);
        }

        if (si.isTab || si.isObject) {
            x += si.width;
            continue;
        }
        unsigned short *logClusters = eng->logClusters(&si);
        QGlyphLayout *glyphs = eng->glyphs(&si);

        int start = qMax(line.from, si.position);
        int gs = logClusters[start-si.position];
        int end;
        int ge;
        if (lineEnd < si.position + eng->length(item)) {
            end = lineEnd;
            ge = logClusters[end-si.position];
        } else {
            end = si.position + eng->length(item);
            ge = si.num_glyphs;
        }

        if (eng->formats) {
            QTextFormat fmt = eng->formats->format(si.format);
            Q_ASSERT(fmt.isCharFormat());
            QTextCharFormat chf = fmt.toCharFormat();
            QColor c = chf.color();
            if (!c.isValid() && eng->textColorFromPalette) {
                c = eng->pal->color(QPalette::Text);
            }
            p->setPen(c);
            f = chf.font();
        }
        QFontEngine *fe = f.d->engineForScript((QFont::Script)si.analysis.script);
        Q_ASSERT(fe);

        QTextItem gf;
        gf.right_to_left = (si.analysis.bidiLevel % 2);
        gf.ascent = si.ascent.toInt();
        gf.descent = si.descent.toInt();
        gf.num_glyphs = ge - gs + 1;
        gf.glyphs = glyphs + gs;
        gf.fontEngine = fe;
        gf.chars = eng->string.unicode() + si.position;
        gf.num_chars = eng->length(item);
        int textFlags = 0;
        if (f.d->underline) textFlags |= Qt::Underline;
        if (f.d->overline) textFlags |= Qt::Overline;
        if (f.d->strikeOut) textFlags |= Qt::StrikeOut;

        int *ul = eng->underlinePositions;
        if (ul)
            while (*ul != -1 && *ul < start)
                ++ul;
        do {
            int gtmp = ge;
            if (ul && *ul != -1 && *ul < end)
                gtmp = logClusters[*ul-si.position];

            gf.num_glyphs = gtmp - gs;
            gf.glyphs = glyphs + gs;
            Q26Dot6 w;
            while (gs < gtmp) {
                w += glyphs[gs].advance.x + Q26Dot6(glyphs[gs].space_18d6, F26Dot6);
                ++gs;
            }
            gf.width = w.toInt();
            p->drawTextItem(QPoint(x.toInt(), y.toInt()), gf, textFlags);
            x += w;
            if (ul && *ul != -1 && *ul < end) {
                // draw underline
                gtmp = (*ul == end-1) ? ge : logClusters[*ul+1-si.position];
                gf.num_glyphs = gtmp - gs;
                gf.glyphs = glyphs + gs;
                w = 0;
                while (gs < gtmp) {
                    w += glyphs[gs].advance.x + Q26Dot6(glyphs[gs].space_18d6, F26Dot6);
                    ++gs;
                }
                gf.width = w.toInt();
                p->drawTextItem(QPoint(x.toInt(), y.toInt()), gf, (textFlags ^ Qt::Underline));
                x += w;
                ++ul;
            }
        } while (gs < ge);

    }
}


/*!
  Converts the cursor position cPos to the corresponding x position inside the line.

  If cPos does not point to a valid cursor position it will be
  adjusted to the next valid cursor position.
*/
int QTextLine::cursorToX(int *cPos, Edge edge) const
{
    if (!i && !eng->items.size()) {
        *cPos = 0;
        return eng->lines[0].x.toInt();
    }

    int pos = *cPos;

    int itm = eng->findItem(pos);

    const QScriptLine &line = eng->lines[i];
    if (pos == line.from + (int)line.length) {
        // end of line ensure we have the last item on the line
        itm = eng->findItem(pos-1);
    }

    const QScriptItem *si = &eng->items[itm];
    pos -= si->position;

    eng->shape(itm);
    QGlyphLayout *glyphs = eng->glyphs(si);
    unsigned short *logClusters = eng->logClusters(si);

    int l = eng->length(itm);
    if (pos > l)
        pos = l;
    if (pos < 0)
        pos = 0;

    int glyph_pos = pos == l ? si->num_glyphs : logClusters[pos];
    if (edge == Trailing) {
        // trailing edge is leading edge of next cluster
        while (glyph_pos < si->num_glyphs && !glyphs[glyph_pos].attributes.clusterStart)
            glyph_pos++;
    }

    Q26Dot6 x;
    bool reverse = eng->items[itm].analysis.bidiLevel % 2;

    if (reverse) {
        for (int i = si->num_glyphs-1; i >= glyph_pos; i--)
            x += glyphs[i].advance.x + Q26Dot6(glyphs[i].space_18d6, F26Dot6);
    } else {
        for (int i = 0; i < glyph_pos; i++)
            x += glyphs[i].advance.x + Q26Dot6(glyphs[i].space_18d6, F26Dot6);
    }

    // add the items left of the cursor

    int lineEnd = line.from + line.length;
//     // don't draw trailing spaces or take them into the layout.
//     const QCharAttributes *attributes = eng->attributes();
//     while (lineEnd > line.from && attributes[lineEnd-1].whiteSpace)
//         --lineEnd;

    int firstItem = eng->findItem(line.from);
    int lastItem = eng->findItem(lineEnd - 1);
    int nItems = lastItem-firstItem+1;

    x += line.x;

    eng->justify(line);
    if (eng->textFlags & Qt::AlignRight)
        x += line.width - line.textWidth;
    else if (eng->textFlags & Qt::AlignHCenter)
        x += (line.width - line.textWidth)/2;

    QVarLengthArray<int> visualOrder(nItems);
    QVarLengthArray<uchar> levels(nItems);
    for (int i = 0; i < nItems; ++i)
        levels[i] = eng->items[i+firstItem].analysis.bidiLevel;
    QTextEngine::bidiReorder(nItems, levels.data(), visualOrder.data());

    for (int i = 0; i < nItems; ++i) {
        int item = visualOrder[i]+firstItem;
        if (item == itm)
            break;
        QScriptItem &si = eng->items[item];

        if (si.isTab || si.isObject) {
            x += si.width;
            continue;
        }
        int start = qMax(line.from, si.position);
        int end = qMin(lineEnd, si.position + eng->length(item));

        unsigned short *logClusters = eng->logClusters(&si);

        int gs = logClusters[start-si.position];
        int ge = logClusters[end-si.position-1];

        QGlyphLayout *glyphs = eng->glyphs(&si);

        while (gs <= ge) {
            x += glyphs[gs].advance.x + Q26Dot6(glyphs[gs].space_18d6, F26Dot6);
            ++gs;
        }
    }

    *cPos = pos + si->position;
    return x.toInt();
}

/*!
  Converts a x coordinate to the nearest matching cursor position.
*/
int QTextLine::xToCursor(int xpos, CursorPosition cpos) const
{
    const QScriptLine &line = eng->lines[i];

    if (!line.length)
        return line.from;

    Q26Dot6 x(xpos);

    int line_length = line.length;
    // don't draw trailing spaces or take them into the layout.
    const QCharAttributes *a = eng->attributes() + line.from;
    while (line_length && a[line_length-1].whiteSpace)
        --line_length;

    int firstItem = eng->findItem(line.from);
    int lastItem = eng->findItem(line.from + line_length - 1);
    int nItems = lastItem-firstItem+1;

    x -= line.x;

    eng->justify(line);
    if (eng->textFlags & Qt::AlignRight)
        x -= line.width - line.textWidth;
    else if (eng->textFlags & Qt::AlignHCenter)
        x -= (line.width - line.textWidth)/2;

    QVarLengthArray<int> visualOrder(nItems);
    QVarLengthArray<unsigned char> levels(nItems);
    for (int i = 0; i < nItems; ++i)
        levels[i] = eng->items[i+firstItem].analysis.bidiLevel;
    QTextEngine::bidiReorder(nItems, levels.data(), visualOrder.data());

    int gl_before = 0;
    int gl_after = 0;
    int it_before = 0;
    int it_after = 0;
    Q26Dot6 x_before(0xffffff);
    Q26Dot6 x_after(0xffffff);


    for (int i = 0; i < nItems; ++i) {
        int item = visualOrder[i]+firstItem;
        QScriptItem &si = eng->items[item];
        int item_length = eng->length(item);

        if (si.isTab || si.isObject) {
            x -= si.width;
            continue;
        }
        int start = qMax(line.from - si.position, 0);
        int end = qMin(line.from + line_length - si.position, item_length);

        unsigned short *logClusters = eng->logClusters(&si);

        int gs = logClusters[start];
        int ge = (end == item_length ? si.num_glyphs : logClusters[end]) - 1;
        QGlyphLayout *glyphs = eng->glyphs(&si);

        if (si.analysis.bidiLevel %2) {
            Q26Dot6 item_width;
            int g = gs;
            while (g <= ge) {
                item_width += glyphs[g].advance.x + Q26Dot6(glyphs[g].space_18d6, F26Dot6);
                ++g;
            }

            x -= item_width;
            if (x > 0) {
                gl_before = gs;
                it_before = item;
                x_before = x;
                continue;
            }

            while (1) {
                if (glyphs[gs].attributes.clusterStart) {
                    if (x < 0) {
                        gl_after = gs;
                        it_after = item;
                        x_after = -x;
                    } else {
                        gl_before = gs;
                        it_before = item;
                        x_before = x;
                        goto end;
                    }
                }
                if (gs > ge)
                    Q_ASSERT(false);
                x += glyphs[gs].advance.x + Q26Dot6(glyphs[gs].space_18d6, F26Dot6);
                ++gs;
            }
        } else {
            while (1) {
                if (glyphs[gs].attributes.clusterStart) {
                    if (x > 0) {
                        gl_before = gs;
                        it_before = item;
                        x_before = x;
                    } else {
                        gl_after = gs;
                        it_after = item;
                        x_after = -x;
                        goto end;
                    }
                }
                if (gs > ge)
                    break;
                x -= glyphs[gs].advance.x + Q26Dot6(glyphs[gs].space_18d6, F26Dot6);
                ++gs;
            }
        }
    }

 end:

    int item;
    int glyph;
    if (cpos == OnCharacters || x_before < x_after) {
        item = it_before;
        glyph = gl_before;
    } else {
        item = it_after;
        glyph = gl_after;
    }

    // find the corresponding cursor position
    const QScriptItem &si = eng->items[item];
    unsigned short *logClusters = eng->logClusters(&si);
    int j;
    for (j = 0; j < eng->length(item); ++j)
        if (logClusters[j] == glyph)
            break;
    return si.position + j;
}

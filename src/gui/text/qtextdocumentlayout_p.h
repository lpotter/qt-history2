#ifndef QTEXTLAYOUTER_H
#define QTEXTLAYOUTER_H

#include <qpalette.h>

#include "qtextpiecetable_p.h"
#include <qtextformat.h>
#include "qtextdocument.h"
#include "qtextglobal_p.h"
#include "qtextcursor.h"

#include <private/qtextlayout_p.h>

class QTextBlockLayouter;
class QTextListFormat;

struct QTextPaintContext
{
    QTextPaintContext(QPainter &_painter)
	: painter(_painter) {}

    QPainter &painter;
    QRect clipRect;
    QTextCursor cursor;
    QPalette palette;
    bool showCursor;
};

class QTextDocumentLayout : public QObject
{
public:
    QTextDocumentLayout(QTextPieceTable *parent);

    void draw(QTextPaintContext &context);
    void drawBlock(QTextPaintContext &context, const QTextPieceTable::BlockIterator block);

    int indent(const QTextPieceTable::BlockIterator block) const;
    QTextListFormat listFormat(const QTextPieceTable::BlockIterator block) const;
    void drawListItem(QTextPaintContext &context, const QTextPieceTable::BlockIterator block, const QTextLayout::Selection &selection);


    void layoutBlock(const QTextPieceTable::BlockIterator block, const QPoint &p);
    QTextPieceTable::BlockIterator layoutCell(QTextPieceTable::BlockIterator block, QPoint *pos);
    QTextPieceTable::BlockIterator layoutTable(QTextPieceTable::BlockIterator block, QPoint *pos);

    QTextPieceTable *pieceTable() const { return static_cast<QTextPieceTable *>(parent()); }

    void setPageSize(const QSize &size) { pgSize = size; }
    QSize pageSize() const { return pgSize; }

    QRect pageRect(int /*page*/) const
	{ return QRect(0, 0, pgSize.width(), totalHeight()); } // ####
    int numPages() const { return 1; } // #####

    // ### remove me
    int totalHeight() const;

    void recreateAllBlocks();

    int hitTest(const QPoint &point, QText::HitTestAccuracy accuracy) const;

private:
    int hitTest(QTextPieceTable::BlockIterator bl, const QPoint &point, QText::HitTestAccuracy accuracy) const;

    QSize pgSize;
};

#endif // QTEXTLAYOUTER_H

#ifndef QTEXTLAYOUTER_H
#define QTEXTLAYOUTER_H

#ifndef QT_H
#include <qabstracttextdocumentlayout.h>
#endif // QT_H

class QTextListFormat;

class QTextDocumentLayoutPrivate;

class QTextDocumentLayout : public QAbstractTextDocumentLayout
{
    Q_DECLARE_PRIVATE(QTextDocumentLayout)
public:
    QTextDocumentLayout();

    // from the abstract layout
    void draw(QPainter *painter, const PaintContext &context);
    void documentChange(int from, int oldLength, int length);
    int hitTest(const QPoint &point, QText::HitTestAccuracy accuracy) const;

    void setSize(QTextObject item, const QTextFormat &format);
    void layoutObject(QTextObject item, const QTextFormat &format);
    void drawObject(QPainter *p, const QRect &rect, QTextObject item,
                    const QTextFormat &format, QTextLayout::SelectionType selType);

    int numPages() const;

    // ### remove me
    int totalHeight() const;

    virtual void setPageSize(const QSize &size);
    virtual QSize pageSize() const;
};

#endif // QTEXTLAYOUTER_H

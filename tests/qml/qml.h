/****************************************************************************
** $Id: //depot/qt/main/tests/qml/qml.h#3 $
**
** Definition of something or other
**
** Created : 979899
**
** Copyright (C) 1997 by Troll Tech AS.  All rights reserved.
**
****************************************************************************/

#ifndef QML_H
#define QML_H


#include <qpushbutton.h>
#include <qlineedit.h>
#include <qlist.h>
#include <qdict.h>
#include <qpixmap.h>
#include <qscrollview.h>

class QMLStyle {
public:
    QMLStyle( const QString& name );

    static const QMLStyle& nullStyle();

    QString name() const;

    enum Display {display_block, display_inline, display_list_item, display_none};
    Display display() const;
    void setDisplay(Display d);

    enum FontStyle {style_undefined, style_normal, style_italic, style_oblique};
    FontStyle fontStyle() const;
    void setFontStyle(FontStyle s);

    enum FontWeight {weight_undefined = -1,
		     weight_normal = QFont::Normal,
		     weight_bold = QFont::Bold,
		     weight_bolder = QFont::Black,
		     weight_lighter = QFont::Light};

    int fontWeight() const;
    void setFontWeight(int w);

    int fontSize() const;
    void setFontSize(int s);

    int numberOfColumns() const;
    void setNumberOfColumns(int ncols);

private:
    void init();
    Display disp;
    FontStyle fontstyle;
    int fontweight;
    int fontsize;
    QMLStyle *parentstyle;
    QString stylename;
    int ncolumns;
};

class QMLContext {
public:
    QMLContext();
    void insert(QString name, const QPixmap& pm);

    QPixmap* image(const QString &name) const;

private:
    QDict<QPixmap>images;
};

class QMLNode;
class QMLContainer;
class QMLStyleSheet {
public:
    QMLStyleSheet();

    QMLStyle& defaultStyle() const;
    static QMLStyleSheet& defaultSheet();

    void insert( QMLStyle* style);
    const QMLStyle& style(const char* name) const;

    virtual QMLContainer* tag( const QMLStyle &,
			       const QDict<QString> *attr=0, const QMLContext* context = 0 ) const;

//     virtual QMLNode* emptyTag( const QMLStyle&, QMLContainer* parent,
// 			       const QDict<QString> *attr=0, const QMLContext* context = 0 ) const;

private:
    void init();
    QDict <QMLStyle> styles;
    QMLStyle* defaultstyle;

};


class QMLContainer;
class QMLBox;

class QMLNode  {
public:
    QMLNode();
    ~QMLNode();
    QMLNode* next;

    QMLNode* depthFirstSearch(QMLNode* tag, QMLContainer* &parent, bool down = TRUE);
    QMLNode* nextLayout(QMLNode* tag, QMLContainer* &parent);
    QMLNode* nextLeaf(QMLNode* tag, QMLContainer* &parent);

    QChar c;

    bool isSpace() const {return c.isSpace();}

    QMLContainer* parent() const;
    QMLBox* box() const;
    QMLNode* previous() const;
    QMLNode* lastSibling() const;
    QMLNode* nextSibling() const;

    uint isSimpleNode: 1;
    uint isLastSibling:1;
    uint isContainer:1;
    uint isBox:1;
};

class QMLRow {
public:
    QMLRow();
    QMLRow(QMLContainer* box, QPainter* p, QMLNode* &t, QMLContainer* &par, int w);
    ~QMLRow();
    int x;
    int y;
    int width;
    int height;
    int base;
    bool intersects(int xr, int yr, int wr, int hr);
    void draw(QMLContainer* box, QPainter* p, int obx, int oby, int ox, int oy, int cx, int cy, int cw, int ch, 
	      QRegion& backgroundRegion, bool onlyDirty = FALSE);
    QMLNode* hitTest(QMLContainer* box, QPainter* p, int obx, int oby, int xarg, int yarg);


    bool locate(QMLContainer* box, QPainter* p, QMLNode* node, int &lx, int &ly, int &lh);

    bool dirty;

    QMLNode* start;
    QMLNode* end;
    QMLContainer* parent;

};


class QMLContainer : public QMLNode {
public:
    QMLContainer( const QMLStyle &stl);
    ~QMLContainer();
    QFont font() const;
    QMLContainer* parent;
    const QMLStyle* style;
    QMLNode* child;

    QMLBox* box() const;
    QMLBox* parentBox() const;

    QMLNode* lastChild() const;

private:
    int fontWeight() const;
    QMLStyle::FontStyle fontStyle() const;
    int fontSize() const;

    QFont* fnt;
};

class QMLBox : public QMLContainer {
public:
    QMLBox( const QMLStyle &stl);
    ~QMLBox();

    void draw(QPainter* p, int obx, int oby, int ox, int oy, int cx, int cy, int cw, int ch, 
	      QRegion& backgroundRegion, bool onlyDirty = FALSE);
    void resize (QPainter* p, int newWidth);

    void update(QPainter* p, QMLRow* r = 0);


    QList<QMLRow> rows;

    int width;
    int height;

    //    QMLNode* locate(int x, int y);
    QMLRow*  locate(QPainter* p, QMLNode* node, int &lx, int &ly, int &lh, int&lry, int &lrh);

    QMLNode* hitTest(QPainter* p, int obx, int oby, int xarg, int yarg);

};



class QMLCursor;

class QMLDocument : public QMLBox{
public:
    QMLDocument( const QString &doc,  const QMLContext* context = 0, const QMLStyleSheet* sheet = 0);
    ~QMLDocument();

    const QMLStyleSheet& styleSheet() const;

    bool isValid() const;

    void dump();

    QMLCursor* cursor;

private:
    const QMLStyleSheet* sheet_;
    void parse (QMLContainer* current, QMLNode* lastChild, const QString& doc, int& pos);
    bool eatSpace(const QString& doc, int& pos);
    bool eat(const QString& doc, int& pos, const QChar& c);
    QString parseOpenTag(const QString& doc, int& pos);
    bool eatCloseTag(const QString& doc, int& pos, const QString& open);
    QString parseWord(const QString& doc, int& pos);
    QString parsePlainText(const QString& doc, int& pos);
    bool hasPrefix(const QString& doc, int pos, const QChar& c);
    bool valid;
    QChar* openChar;
    QChar* closeChar;
    QChar* slashChar;
    const QMLContext* context_;

};

class QMLCursor{
public:
    QMLCursor(QMLDocument* doc);
    ~QMLCursor();
    void draw(QPainter* p,  int ox, int oy, int cx, int cy, int cw, int ch);

    QMLDocument* document;

    int x;
    int y;
    int height;

    QMLRow* row;
    int rowY;
    int rowHeight;

    int width() { return 2; }

    QMLNode *node;
    QMLContainer *nodeParent;

    void insert(QPainter* p, const QChar& c);

    void right(QPainter* p);
    void left(QPainter* p);
    void up(QPainter* p);
    void down(QPainter* p);
    void goTo(QPainter* p, int xarg, int yarg);


    void goTo(QMLNode* n, QMLContainer* par);
    void calculatePosition(QPainter* p);

    int xline;

};


class QMLView : public QScrollView {
    Q_OBJECT
public:
    QMLView();

protected:
    void drawContentsOffset(QPainter*, int ox, int oy,
			    int cx, int cy, int cw, int ch);
    void viewportMousePressEvent( QMouseEvent* );
    void keyPressEvent( QKeyEvent * );
    void  resizeEvent(QResizeEvent*);

    void showCursor();
    void hideCursor();

private:
    QMLDocument* doc;
    bool cursor_hidden;

};


#endif

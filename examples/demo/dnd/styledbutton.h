/****************************************************************************
**
** Copyright (C) 1992-2003 Trolltech AS. All rights reserved.
**
** This file is part of Qt Designer.
** EDITIONS: FREE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef STYLEDBUTTON_H
#define STYLEDBUTTON_H

#include <qbutton.h>
#include <qpixmap.h>

class QColor;
class QBrush;

class StyledButton : public QButton
{
    Q_OBJECT

    Q_PROPERTY( QColor color READ color WRITE setColor )
    Q_PROPERTY( QPixmap pixmap READ pixmap WRITE setPixmap )
    Q_PROPERTY( EditorType editor READ editor WRITE setEditor )
    Q_PROPERTY( bool scale READ scale WRITE setScale )

    Q_ENUMS( EditorType )

public:
    enum EditorType { ColorEditor, PixmapEditor };

    StyledButton( QWidget* parent = 0, const char* name = 0 );
    StyledButton( const QBrush& b, QWidget* parent = 0, const char* name = 0, WFlags f = 0 );
    ~StyledButton();

    void setEditor( EditorType );
    EditorType editor() const;

    void setColor( const QColor& );
    void setPixmap( const QPixmap& );

    QPixmap* pixmap() const;
    QColor color() const;

    void setScale( bool );
    bool scale() const;

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

public slots:
    virtual void onEditor();

signals:
    void changed();

protected:
    void mousePressEvent(QMouseEvent*);
    void mouseMoveEvent(QMouseEvent*);
#ifndef QT_NO_DRAGANDDROP
    void dragEnterEvent ( QDragEnterEvent * );
    void dragMoveEvent ( QDragMoveEvent * );
    void dragLeaveEvent ( QDragLeaveEvent * );
    void dropEvent ( QDropEvent * );
#endif // QT_NO_DRAGANDDROP
    void drawButton( QPainter* );
    void drawButtonLabel( QPainter* );
    void resizeEvent( QResizeEvent* );
    void scalePixmap();

private:
    QPixmap* pix;
    QPixmap* spix;  // the pixmap scaled down to fit into the button
    QColor col;
    EditorType edit;
    bool s;
    QPoint pressPos;
    bool mousePressed;
};

#endif //STYLEDBUTTON_H

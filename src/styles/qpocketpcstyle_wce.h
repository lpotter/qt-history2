/****************************************************************************
**
** Implementation of PocketPC-like style class.
**
** Copyright (C) 1992-2003 Trolltech AS. All rights reserved.
**
** This file is part of the styles module of the Qt GUI Toolkit.
** EDITIONS: PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QPOCKETPCSTYLE_WCE_H
#define QPOCKETPCSTYLE_WCE_H
//#ifndef QT_NO_STYLE_POCKETPC

#ifndef QT_H
#include "qstyle.h"
#include "qpalette.h"
#endif // QT_H

#if defined(QT_PLUGIN)
#define Q_GUI_EXPORT_STYLE_POCKETPC
#else
#define Q_GUI_EXPORT_STYLE_POCKETPC Q_EXPORT
#endif

#ifndef Q_QDOC
class QPainter;
class Q_GUI_EXPORT_STYLE_POCKETPC QPocketPCStyle : public QStyle
{
    Q_OBJECT
public:
    QPocketPCStyle();
    virtual ~QPocketPCStyle();

    virtual void polish( QApplication* );
    virtual void polish( QWidget* );
    virtual void polishPopupMenu( QPopupMenu* );
    virtual void unPolish( QApplication* );
    virtual void unpolish( QWidget* );

    // new stuff
    void drawPrimitive( PrimitiveElement pe, QPainter *p, const QRect &r, const QPalette &pal, SFlags flags = Style_Default, const QStyleOption& = QStyleOption::Default ) const;
    void drawControl( ControlElement element, QPainter *p, const QWidget *widget, const QRect &r, const QPalette &pal, SFlags how = Style_Default, const QStyleOption& = QStyleOption::Default ) const;
    void drawComplexControl( ComplexControl control, QPainter* p, const QWidget* widget, const QRect& r, const QPalette& pal, SFlags how = Style_Default, SCFlags sub = SC_All, SCFlags subActive = SC_None, const QStyleOption& = QStyleOption::Default ) const;
    int pixelMetric( PixelMetric metric, const QWidget *widget = 0 ) const;
    QRect querySubControlMetrics( ComplexControl control, const QWidget *widget, SubControl sc, const QStyleOption& = QStyleOption::Default ) const;
    QSize sizeFromContents( ContentsType contents, const QWidget *widget, const QSize &contentsSize, const QStyleOption& = QStyleOption::Default ) const;
    QPixmap stylePixmap( StylePixmap stylepixmap, const QWidget *widget = 0, const QStyleOption& = QStyleOption::Default ) const;
    QPixmap stylePixmap( PixmapType pixmapType, const QPixmap &pix, const QPalette &pal, const QStyleOption& = QStyleOption::Default ) const;

    void drawControlMask(ControlElement,QPainter *,const QWidget *,const QRect &,const QStyleOption &) const;
    QRect subRect(SubRect,const QWidget *) const;
    void drawComplexControlMask(ComplexControl,QPainter *,const QWidget *,const QRect &,const QStyleOption &) const;
    SubControl querySubControl(ComplexControl,const QWidget *,const QPoint &,const QStyleOption &) const;
    int styleHint(StyleHint,const QWidget *,const QStyleOption &,QStyleHintReturn *) const;

private:
    // Convenience
    Qt::Dock findLocation( QWidget *p ) const;
    Qt::Dock findLocation( QPainter *p ) const;

#ifndef Q_OS_TEMP
    void modifyOriginalPalette();
    QPalette originalPal;
    bool gotOriginal;
#endif // Q_OS_TEMP



    // Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QPocketPCStyle( const QPocketPCStyle & );
    QPocketPCStyle& operator=( const QPocketPCStyle & );
#endif
};
#endif //Q_QDOC

//#endif // QT_NO_STYLE_POCKETPC
#endif

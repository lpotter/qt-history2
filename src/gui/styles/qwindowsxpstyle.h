/****************************************************************************
**
** Copyright (C) 1992-2003 Trolltech AS. All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
** EDITIONS: PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QWINDOWSXPSTYLE_H
#define QWINDOWSXPSTYLE_H

#ifndef QT_H
#include "qwindowsstyle.h"
#endif // QT_H

#if !defined(QT_NO_STYLE_WINDOWSXP) || defined(QT_PLUGIN)

#if defined(QT_PLUGIN)
#define Q_GUI_EXPORT_STYLE_WINDOWSXP
#else
#define Q_GUI_EXPORT_STYLE_WINDOWSXP Q_GUI_EXPORT
#endif

class QWindowsXPStylePrivate;

class Q_GUI_EXPORT_STYLE_WINDOWSXP QWindowsXPStyle : public QWindowsStyle
{
    Q_OBJECT
public:
    QWindowsXPStyle();
    ~QWindowsXPStyle();

    void unPolish( QApplication* );
    void polish( QApplication* );
    void polish( QWidget* );
    void unPolish( QWidget* );

    void drawPrimitive( PrimitiveElement op,
			QPainter *p,
			const QRect &r,
			const QPalette &pal,
			SFlags flags = Style_Default,
			const QStyleOption& = QStyleOption::Default ) const;

    void drawControl( ControlElement element,
		      QPainter *p,
		      const QWidget *widget,
		      const QRect &r,
		      const QPalette &pal,
		      SFlags how = Style_Default,
		      const QStyleOption& = QStyleOption::Default ) const;

    void drawControlMask( ControlElement element,
			  QPainter *p,
			  const QWidget *widget,
			  const QRect &r,
			  const QStyleOption& = QStyleOption::Default ) const;

    void drawComplexControl( ComplexControl control,
			     QPainter* p,
			     const QWidget* w,
			     const QRect& r,
			     const QPalette &pal,
			     SFlags flags = Style_Default,
			     SCFlags sub = SC_All,
			     SCFlags subActive = SC_None,
			     const QStyleOption& = QStyleOption::Default ) const;


    int pixelMetric( PixelMetric metic,
		     const QWidget *widget = 0 ) const;

    QRect querySubControlMetrics( ComplexControl control,
				  const QWidget *widget,
				  SubControl sc,
				  const QStyleOption& = QStyleOption::Default ) const;

    QSize sizeFromContents( ContentsType contents,
				    const QWidget *widget,
				    const QSize &contentsSize,
				    const QStyleOption& = QStyleOption::Default ) const;

    int styleHint( StyleHint stylehint,
			   const QWidget *widget = 0,
			   const QStyleOption& = QStyleOption::Default,
			   QStyleHintReturn* returnData = 0
			   ) const;

protected:
    bool eventFilter( QObject *o, QEvent *e );

    void updateRegion( QWidget *widget );

protected slots:
    void activeTabChanged();

private:
    QWindowsXPStylePrivate *d;

    friend class QStyleFactory;
    friend class QWindowsXPStylePrivate;
    static bool resolveSymbols();

#if defined(Q_DISABLE_COPY)
    QWindowsXPStyle( const QWindowsXPStyle & );
    QWindowsXPStyle& operator=( const QWindowsXPStyle & );
#endif
};

#endif // QT_NO_STYLE_WINDOWSXP

#endif // QWINDOWSXPSTYLE_H

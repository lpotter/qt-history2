/****************************************************************************
**
** Definition of QToolBox widget class.
**
** Copyright (C) 1992-2003 Trolltech AS. All rights reserved.
**
** This file is part of the widgets module of the Qt GUI Toolkit.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QTOOLBOX_H
#define QTOOLBOX_H

#ifndef QT_H
#include <qwidget.h>
#include <qiconset.h>
#endif // QT_H

#ifndef QT_NO_TOOLBOX

class QToolBoxPrivate;
class QWidgetList;

class Q_EXPORT QToolBox : public QWidget
{
    Q_OBJECT
    Q_PROPERTY( int currentIndex READ currentIndex WRITE setCurrentIndex )
    Q_PROPERTY( int count READ count )
    Q_PROPERTY( BackgroundMode pageBackgroundMode READ pageBackgroundMode WRITE setPageBackgroundMode )

public:
    QToolBox( QWidget *parent = 0, const char *name = 0 );
    ~QToolBox();

    void addPage( QWidget *page, const QString &label );
    void addPage( QWidget *page, const QIconSet &iconSet, const QString &label );
    void insertPage( QWidget *page, const QString &label, int index = -1 );
    void insertPage( QWidget *page, const QIconSet &iconSet, const QString &label, int index = -1 );
    void setPageEnabled( QWidget *page, bool enabled );
    bool isPageEnabled( QWidget *page ) const;

    void setPageLabel( QWidget *page, const QString &label );
    QString pageLabel( QWidget *page ) const;

    void setPageIconSet( QWidget *page, const QIconSet &iconSet );
    QIconSet pageIconSet( QWidget *page ) const;

    void setPageToolTip( QWidget *page, const QString &toolTip );
    QString pageToolTip( QWidget *page ) const;

    QWidget *currentPage() const;
    void setCurrentPage( QWidget *page );

    int currentIndex() const;
    QWidget *page( int index ) const;
    int indexOf( QWidget *page ) const;
    int count() const;

    void setPageBackgroundMode( BackgroundMode bm );
    BackgroundMode pageBackgroundMode() const;

    void removePage( QWidget *page );

public slots:
    void setCurrentIndex( int index );

signals:
    void currentChanged( int index );

private slots:
    void buttonClicked();

protected:
    void showEvent( QShowEvent *e );

private:
    void relayout();
    void activateClosestPage( QWidget *page );

private:
    QToolBoxPrivate *d;

};


inline void QToolBox::addPage( QWidget *page, const QString &label )
{ insertPage( page, QIconSet(), label ); }
inline void QToolBox::addPage( QWidget *page, const QIconSet &iconSet,
			       const QString &label )
{ insertPage( page, iconSet, label ); }
inline void QToolBox::insertPage( QWidget *page, const QString &label, int index )
{ insertPage( page, QIconSet(), label, index ); }

#endif // QT_NO_TOOLBOX
#endif

#ifndef QACTIVEX_H
#define QACTIVEX_H

#include <qwidget.h>
#include "qcomobject.h"

class QClientSite;

class QActiveX : public QWidget, public QComBase
{
public:
    QMetaObject *metaObject() const;
    const char *className() const;
    void* qt_cast( const char* );
    bool qt_invoke( int, QUObject* );
    bool qt_emit( int, QUObject* );
    bool qt_property( int, int, QVariant* );
    QObject* qObject() { return (QObject*)this; }

    QActiveX( QWidget* parent = 0, const char* name = 0 );
    QActiveX( const QString &c, QWidget *parent = 0, const char *name = 0 );
    ~QActiveX();

    void clear();

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    //void reparent( QWidget *parent, WFlags f, const QPoint &, bool showIt = FALSE );
    //void setAcceptDrops( bool on );
    //bool customWhatsThis() const;
    //void paletteChange( const QPalette & );
    void fontChange( const QFont & );
    void setUpdatesEnabled( bool );

protected:
    void enabledChange( bool old );

private:
    void initialize( IUnknown** );
    QMetaObject *parentMetaObject() const;

    QClientSite *clientsite;
};

#endif //QACTIVEX_H

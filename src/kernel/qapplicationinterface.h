#ifndef QAPPLICATIONINTERFACES_H
#define QAPPLICATIONINTERFACES_H

#ifndef QT_NO_PLUGIN

#ifndef QT_H
#include "qvariant.h"
#include "qobject.h"
#endif // QT_H
    
class QComponentInterface;

class Q_EXPORT QApplicationInterface : public QObject
{
public:
    QApplicationInterface();

    virtual QComponentInterface* requestInterface( const QCString& request ) = 0;
};

class Q_EXPORT QComponentInterface : public QApplicationInterface
{
public:
    QComponentInterface( QObject* o );

#ifndef QT_NO_PROPERTIES
    virtual QVariant requestProperty( const QCString& p );
    virtual bool requestSetProperty( const QCString& p, const QVariant& v );
#endif
    virtual bool requestConnect( const char* signal, QObject* target, const char* slot );
    virtual bool requestEvents( QObject* o );

    QComponentInterface* requestInterface( const QCString& );

protected:
    QObject* object() { return QObject::parent(); }

private:
    QObject* parent() { return QObject::parent(); }
};

#endif

#endif //QAPPLICATIONINTERFACES_H

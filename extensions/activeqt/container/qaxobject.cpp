/****************************************************************************
**
** Implementation of the QAxObject class.
**
** Copyright (C) 2001-2003 Trolltech AS. All rights reserved.
**
** This file is part of the Active Qt integration.
** EDITIONS: ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qaxobject.h"

#include <quuid.h>
#include <qmetaobject.h>

/*!
    \class QAxObject qaxobject.h
    \brief The QAxObject class provides a QObject that wraps a COM object.
\if defined(commercial)
    It is part of the <a href="commercialeditions.html">Qt Enterprise Edition</a>.
\endif

    \module QAxContainer
    \extension ActiveQt

    A QAxObject can be instantiated as an empty object, with the name
    of the COM object it should wrap, or with a pointer to the
    IUnknown that represents an existing COM object. If the COM object
    implements the IDispatch interface, the properties, methods and
    events of that object become available as Qt properties, slots and
    signals. The base class, QAxBase, provides an API to access the
    COM object directly through the IUnknown pointer.

    QAxObject is a QObject and can be used as such, e.g. it can be
    organized in an object hierarchy, receive events and connect to
    signals and slots.

    \warning
    You can subclass QAxObject, but you cannot use the Q_OBJECT macro
    in the subclass (the generated moc-file will not compile), so you
    cannot add further signals, slots or properties. This limitation is 
    due to the metaobject information generated in runtime. 
    To work around this problem, aggregate the QAxObject as a member of 
    the QObject subclass.

    \important dynamicCall() querySubObject()
*/

/*!
    Creates an empty COM object and propagates \a parent and \a name
    to the QObject constructor. To initialize the object, call \link
    QAxBase::setControl() setControl \endlink.
*/
QAxObject::QAxObject( QObject *parent, const char *name )
: QObject( parent, name )
{
}

/*!
    Creates a QAxObject that wraps the COM object \a c. \a parent and
    \a name are propagated to the QWidget contructor.

    \sa setControl()
*/
QAxObject::QAxObject( const QString &c, QObject *parent, const char *name )
: QObject( parent, name )
{
    setControl( c );
}

/*!
    Creates a QAxObject that wraps the COM object referenced by \a
    iface. \a parent and \a name are propagated to the QObject
    contructor.
*/
QAxObject::QAxObject( IUnknown *iface, QObject *parent, const char *name )
: QObject( parent, name ), QAxBase( iface )
{
}

/*!
    Releases the COM object and destroys the QAxObject,
    cleaning up all allocated resources.
*/
QAxObject::~QAxObject()
{
}

/*!
    \reimp
*/
const QMetaObject *QAxObject::metaObject() const
{
    return QAxBase::metaObject();
}

/*!
    \reimp
*/
const QMetaObject *QAxObject::parentMetaObject() const
{
    return &QObject::staticMetaObject;
}

/*!
    \reimp
*/
void *QAxObject::qt_metacast( const char *cname ) const
{
    if ( !qstrcmp( cname, "QAxObject" ) ) return (void*)this;
    if ( !qstrcmp( cname, "QAxBase" ) ) return (QAxBase*)this;
    return QObject::qt_metacast( cname );
}


/*!
    \reimp
*/
int QAxObject::qt_metacall(QMetaObject::Call call, int id, void **o)
{
    if ( QAxBase::qt_metacall(call, id, o ) )
	return TRUE;
    return QObject::qt_metacall(call, id, o );
}

/*!
    \reimp
*
bool QAxObject::qt_emit( int _id, QUObject* _o )
{
    const int index = _id - metaObject()->signalOffset();
    if ( !isNull() && index >= 0 ) {
	// get the list of connections
	QConnectionList *clist = receivers( _id );
	if ( clist ) // call the signal
	    activate_signal( clist, _o );

	return TRUE;
    }
    return QObject::qt_emit( _id, _o );
}

/*!
    \fn QObject *QAxObject::qObject()
    \reimp
*/

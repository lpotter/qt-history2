/****************************************************************************
**
** Copyright (C) 1992-2003 Trolltech AS. All rights reserved.
**
** This file is part of the tools module of the Qt GUI Toolkit.
** EDITIONS: PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qplatformdefs.h"

#include "qthreadstorage.h"
#include "qthreadinstance_p.h"

#include <string.h>

// #define QTHREADSTORAGE_DEBUG


// keep this in sync with the implementation in qthreadstorage.cpp
static const int MAX_THREAD_STORAGE = 257; // 256 maximum + 1 used in QRegExp

static bool thread_storage_init = FALSE;
static struct {
    bool used;
    void (*func)( void * );
} thread_storage_usage[MAX_THREAD_STORAGE];

static void thread_storage_id( int &id, void (*func)( void * ), bool alloc )
{
    static QMutex mutex;
    mutex.lock();

    if ( alloc ) {
	// make sure things are initialized
	if ( ! thread_storage_init )
	    memset( thread_storage_usage, 0, sizeof( thread_storage_usage ) );
	thread_storage_init = TRUE;

	for ( ; id < MAX_THREAD_STORAGE; ++id ) {
	    if ( !thread_storage_usage[id].used )
		break;
	}

	Q_ASSERT( id >= 0 && id < MAX_THREAD_STORAGE );
	thread_storage_usage[id].used = TRUE;
	thread_storage_usage[id].func = func;

#ifdef QTHREADSTORAGE_DEBUG
	qDebug( "QThreadStorageData: allocated id %d", id );
#endif // QTHREADSTORAGE_DEBUG
    } else {
	thread_storage_usage[id].used = FALSE;
	thread_storage_usage[id].func = 0;

#ifdef QTHREADSTORAGE_DEBUG
	qDebug( "QThreadStorageData: released id %d", id );
#endif // QTHREADSTORAGE_DEBUG
    }

    mutex.unlock();
}


QThreadStorageData::QThreadStorageData( void (*func)( void * ) )
    : id(0), constructed(true)
{
    thread_storage_id( id, func, TRUE );
}

QThreadStorageData::~QThreadStorageData()
{
    thread_storage_id( id, 0, FALSE );
}

void **QThreadStorageData::get() const
{
    QThreadInstance *d = QThreadInstance::current();
    QMutexLocker locker( d->mutex() );
    return d->thread_storage && d->thread_storage[id] ? &d->thread_storage[id] : 0;
}

void **QThreadStorageData::set( void *p )
{
    QThreadInstance *d = QThreadInstance::current();
    QMutexLocker locker( d->mutex() );
    if ( !d->thread_storage ) {
#ifdef QTHREADSTORAGE_DEBUG
	qDebug( "QThreadStorageData: allocating storage for thread %lx",
		(unsigned long) GetCurrentThreadId() );
#endif // QTHREADSTORAGE_DEBUG

	d->thread_storage = new void*[MAX_THREAD_STORAGE];
	memset( d->thread_storage, 0, sizeof( void* ) * MAX_THREAD_STORAGE );
    }

    // delete any previous data
    if ( d->thread_storage[id] )
	thread_storage_usage[id].func( d->thread_storage[id] );

    // store new data
    d->thread_storage[id] = p;
    return &d->thread_storage[id];
}

void QThreadStorageData::finish( void **thread_storage )
{
    if ( ! thread_storage ) return; // nothing to do

#ifdef QTHREADSTORAGE_DEBUG
    qDebug( "QThreadStorageData: destroying storage for thread %lx",
	    (unsigned long) GetCurrentThreadId() );
#endif // QTHREADSTORAGE_DEBUG

    for ( int i = 0; i < MAX_THREAD_STORAGE; ++i ) {
	if ( ! thread_storage[i] ) continue;
	if ( ! thread_storage_usage[i].used ) {
#ifdef QT_CHECK_STATE
	    qWarning( "QThreadStorage: thread %lx exited after QThreadStorage destroyed",
		      (unsigned long) GetCurrentThreadId() );
#endif // QT_CHECK_STATE
	    continue;
	}

	thread_storage_usage[i].func( thread_storage[i] );
    }

    delete [] thread_storage;
}

bool QThreadStorageData::ensure_constructed(void (*func)(void *))
{
    if (! constructed) {
        id = 0;
        constructed = true;
        thread_storage_id( id, func, TRUE );
        return false;
    }
    return true;
}

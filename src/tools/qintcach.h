/****************************************************************************
** $Id: //depot/qt/main/src/tools/qintcach.h#11 $
**
** Definition of QIntCache template/macro class
**
** Created : 950209
**
** Copyright (C) 1995-1997 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#ifndef QINTCACH_H
#define QINTCACH_H

#include "qgcache.h"


#if defined(USE_MACROCLASS)

#include "qgeneric.h"

#if !defined(name2)
#define name2(a,b)    name2_xx(a,b)
#define name2_xx(a,b) a##b
#endif

#if defined(DEFAULT_MACROCLASS)
#define QIntCachedeclare QIntCacheMdeclare
#define QIntCache QIntCacheM
#endif
#define QIntCacheM(type) name2(QIntCacheM_,type)

#define QIntCacheMdeclare(type)						      \
class QIntCacheM(type) : public QGCache					      \
{									      \
public:									      \
    QIntCacheM(type)( const QIntCacheM(type) &c ) : QGCache(c) {}	      \
    QIntCacheM(type)( int maxCost=100, int size=17 )			      \
	: QGCache( maxCost, size, FALSE, FALSE, TRUE ) {}		      \
   ~QIntCacheM(type)()		      { clear(); }			      \
    QIntCacheM(type) &operator=( const QIntCacheM(type) &c )		      \
			{ return (QIntCacheM(type)&)QGCache::operator=(c); }  \
    int	  maxCost()   const	      { return QGCache::maxCost(); }	      \
    int	  totalCost() const	      { return QGCache::totalCost(); }	      \
    void  setMaxCost( int m )	      { QGCache::setMaxCost(m); }	      \
    uint  count()     const	      { return QGCache::count(); }	      \
    uint  size()      const	      { return QGCache::size(); }	      \
    bool  isEmpty()   const	      { return QGCache::count() == 0; }	      \
    bool  insert( long k, const type *d, int c=1, int p=0 )		      \
			{ return QGCache::insert((const char*)k,(GCI)d,c,p); }\
    bool  remove( long k )   { return QGCache::remove((const char *)k); }     \
    type *take( long k )     { return (type *)QGCache::take((const char *)k);}\
    void  clear()		      { QGCache::clear(); }		      \
    type *find( long k, bool ref=TRUE ) const				      \
			{ return (type *)QGCache::find( (const char *)k,ref);}\
    type *operator[]( long k ) const					      \
			{ return (type *)QGCache::find( (const char *)k); }   \
    void  statistics() const	      { QGCache::statistics(); }	      \
private:								      \
    void  deleteItem( GCI d )	      { if ( del_item ) delete (type *)d; }   \
}

#if defined(DEFAULT_MACROCLASS)
#define QIntCacheIteratordeclare QIntCacheIteratorMdeclare
#define QIntCacheIterator QIntCacheIteratorM
#endif
#define QIntCacheIteratorM(type) name2(QIntCacheIteratorM_,type)

#define QIntCacheIteratorMdeclare(type)					      \
class QIntCacheIteratorM(type) : public QGCacheIterator			      \
{									      \
public:									      \
    QIntCacheIteratorM(type)( const QIntCacheM(type) &c )		      \
			      : QGCacheIterator( (QGCache &)c ) {}	      \
    QIntCacheIteratorM(type)( const QIntCacheIteratorM(type) &ci )	      \
			      : QGCacheIterator( (QGCacheIterator &)ci ) {}   \
    QIntCacheIteratorM(type) &operator=( const QIntCacheIteratorM(type)&ci )  \
	{ return (QIntCacheIteratorM(type)&)QGCacheIterator::operator=(ci); } \
    uint  count()   const    { return QGCacheIterator::count(); }	      \
    bool  isEmpty() const    { return QGCacheIterator::count() == 0; }	      \
    bool  atFirst() const    { return QGCacheIterator::atFirst(); }	      \
    bool  atLast()  const    { return QGCacheIterator::atLast(); }	      \
    type *toFirst()	     { return (type *)QGCacheIterator::toFirst(); }   \
    type *toLast()	     { return (type *)QGCacheIterator::toLast(); }    \
    operator type *() const  { return (type *)QGCacheIterator::get(); }	      \
    type *current()   const  { return (type *)QGCacheIterator::get(); }	      \
    const char *currentKey() const					      \
			     { return QGCacheIterator::getKey();}	      \
    type *operator()()	     { return (type *)QGCacheIterator::operator()();} \
    type *operator++()	     { return (type *)QGCacheIterator::operator++();} \
    type *operator+=(uint j) { return (type *)QGCacheIterator::operator+=(j);}\
    type *operator--()	     { return (type *)QGCacheIterator::operator--();} \
    type *operator-=(uint j) { return (type *)QGCacheIterator::operator-=(j);}\
}

#endif // USE_MACROCLASS


#if defined(USE_TEMPLATECLASS)

#if defined(DEFAULT_TEMPLATECLASS)
#undef	QIntCache
#define QIntCache QIntCacheT
#endif

template<class type> class QIntCacheT : public QGCache
{
public:
    QIntCacheT( const QIntCacheT<type> &c ) : QGCache(c) {}
    QIntCacheT( int maxCost=100, int size=17 )
	: QGCache( maxCost, size, FALSE, FALSE, TRUE ) {}
   ~QIntCacheT()	     { clear(); }
    QIntCacheT<type> &operator=( const QIntCacheT<type> &c )
			{ return (QIntCacheT<type>&)QGCache::operator=(c); }
    int	  maxCost()   const  { return QGCache::maxCost(); }
    int	  totalCost() const  { return QGCache::totalCost(); }
    void  setMaxCost( int m) { QGCache::setMaxCost(m); }
    uint  count()     const  { return QGCache::count(); }
    uint  size()      const  { return QGCache::size(); }
    bool  isEmpty()   const  { return QGCache::count() == 0; }
    bool  insert( long k, const type *d, long c=1, int p=0 )
			{ return QGCache::insert((const char*)k,(GCI)d,c,p); }
    bool  remove( long k )   { return QGCache::remove((const char *)k); }
    type *take( long k )     { return (type *)QGCache::take((const char *)k);}
    void  clear()		      { QGCache::clear(); }
    type *find( long k, bool ref=TRUE ) const
			{ return (type *)QGCache::find( (const char *)k,ref);}
    type *operator[]( long k ) const
			{ return (type *)QGCache::find( (const char *)k); }
    void  statistics() const { QGCache::statistics(); }
private:
    void  deleteItem( GCI d ){ if ( del_item ) delete (type *)d; }
};


#if defined(DEFAULT_TEMPLATECLASS)
#undef	QIntCacheIterator
#define QIntCacheIterator QIntCacheIteratorT
#endif

template<class type> class QIntCacheIteratorT : public QGCacheIterator
{
public:
    QIntCacheIteratorT( const QIntCacheT<type> &c )
	: QGCacheIterator( (QGCache &)c ) {}
    QIntCacheIteratorT( const QIntCacheIteratorT<type> &ci )
			      : QGCacheIterator((QGCacheIterator &)ci) {}
    QIntCacheIteratorT<type> &operator=( const QIntCacheIteratorT<type>&ci )
	{ return ( QIntCacheIteratorT<type>&)QGCacheIterator::operator=( ci );}
    uint  count()   const     { return QGCacheIterator::count(); }
    bool  isEmpty() const     { return QGCacheIterator::count() == 0; }
    bool  atFirst() const     { return QGCacheIterator::atFirst(); }
    bool  atLast()  const     { return QGCacheIterator::atLast(); }
    type *toFirst()	      { return (type *)QGCacheIterator::toFirst(); }
    type *toLast()	      { return (type *)QGCacheIterator::toLast(); }
    operator type *()  const  { return (type *)QGCacheIterator::get(); }
    type *current()    const  { return (type *)QGCacheIterator::get(); }
    long  currentKey() const  { return (long)QGCacheIterator::getKey();}
    type *operator()()	      { return (type *)QGCacheIterator::operator()();}
    type *operator++()	      { return (type *)QGCacheIterator::operator++(); }
    type *operator+=(uint j)  { return (type *)QGCacheIterator::operator+=(j);}
    type *operator--()	      { return (type *)QGCacheIterator::operator--(); }
    type *operator-=(uint j)  { return (type *)QGCacheIterator::operator-=(j);}
};

#endif // USE_TEMPLATECLASS


#endif // QINTCACH_H

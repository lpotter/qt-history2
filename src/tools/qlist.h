#ifndef QLIST_H
#define QLIST_H

#ifndef QT_H
#include "qiterator.h"
#include "qatomic.h"
#include <new>

#endif // QT_H

struct QListData {
    struct DataHeader {
	QAtomic ref;
	int alloc, begin, end;
	void *autoDelete;
    };
    struct Data {
	QAtomic ref;
	int alloc, begin, end;
	void *autoDelete;
	void *array[1];
    };
    Data *detach();
    void realloc(int alloc);
    static Data shared_null;
    Data *d;
    static int grow(int size);
    void **erase(void* *xi);
    void **append();
    void **append(const QListData &l);
    void **prepend();
    void **insert(int i);
    void remove(int i);
    void remove(int i, int n);
    inline int size() const { return d->end - d->begin; }
    inline bool isEmpty() const { return d->end  == d->begin; }
    inline void **at(int i) const { return d->array + d->begin + i; }
    inline void **begin() const { return d->array + d->begin; }
    inline void **end() const { return d->array + d->end; }
};

template <typename T>
class QList
{
    struct Node { void *v; inline T &t()
    { return Q_TYPEINFO_LARGE_OR_STATIC(T)? *(T*)v:*(T*)this; } };
    union { QListData p; QListData::Data *d; };

public:
    inline QList() : d(&QListData::shared_null) { ++d->ref; }
    inline QList(const QList &l) : d(l.d) { ++d->ref; }
    ~QList();
    QList &operator=(const QList &l);
    bool operator==(const QList &l) const;
    inline bool operator!=(const QList &l) const { return !(*this == l); }

    bool autoDelete() const;
    void setAutoDelete(bool enable);

    inline int size() const { return p.size(); }
    inline bool isEmpty() const { return p.isEmpty(); }
    inline void detach() { if (d->ref != 1) detach_helper(); }
    inline bool isDetached() const { return d->ref == 1; }

    void clear();
    const T &at(int i) const;
    const T &operator[](int i) const;
    T &operator[](int i);

    void append(const T &t);
    void prepend(const T &t);
    void insert(int i, const T &t);
    void replace(int i, const T &t);
    int remove(const T &t);
    void removeAt(int i);
    int take(const T &t);
    T takeAt(int i);
    int indexOf(const T &t, int from = 0) const;
    int lastIndexOf(const T &t, int from = -1) const;
    QBool contains(const T &t) const;
    int count(const T &t) const;

    class Iterator {
	typedef std::random_access_iterator_tag  iterator_category;
	typedef ptrdiff_t  difference_type;
	typedef T value_type;
	typedef T *pointer;
	typedef T &reference;
	Node *i; public:
	inline Iterator(Node *n = 0): i(n){}
	inline Iterator(const Iterator &o): i(o.i){}
	inline T &operator*() { return i->t(); }
	inline T &operator[](int j) { return i[j].t(); }
	inline bool operator==(const Iterator &o) const { return i == o.i; }
	inline bool operator!=(const Iterator &o) const { return i != o.i; }
	inline Iterator operator++() { ++i; return *this; }
	inline Iterator operator++(int) { Node *n = i; ++i; return n; }
	inline Iterator operator--() { i--; return *this; }
	inline Iterator operator--(int) { Node *n = i; i--; return n; }
	inline Iterator &operator+=(int j) { i+=j; return *this; }
	inline Iterator &operator-=(int j) { i-=j; return *this; }
	inline Iterator operator+(int j) const { return Iterator(i+j); }
	inline Iterator operator-(int j) const { return Iterator(i-j); }
	inline int operator-(Iterator j) const { return i - j.i; }
	friend class QList;
    };

    class ConstIterator {
	typedef std::random_access_iterator_tag  iterator_category;
	typedef ptrdiff_t difference_type;
	typedef T value_type;
	typedef T *pointer;
 	typedef T &reference;
	Node *i; public:
	inline ConstIterator(Node *n = 0): i(n){}
	inline ConstIterator(const ConstIterator &o): i(o.i){}
	inline ConstIterator(const Iterator &o): i(((ConstIterator&) o).i){}
	inline const T &operator*() const { return i->t(); }
	inline const T &operator[](int j) const { return i[j].t(); }
	inline bool operator==(const ConstIterator &o) const { return i == o.i; }
	inline bool operator!=(const ConstIterator &o) const { return i != o.i; }
	inline ConstIterator operator++() { ++i; return *this; }
	inline ConstIterator operator++(int) { Node *n = i; ++i; return n; }
	inline ConstIterator operator--() { i--; return *this; }
	inline ConstIterator operator--(int) { Node *n = i; i--; return n; }
	inline ConstIterator &operator+=(int j) { i+=j; return *this; }
	inline ConstIterator &operator-=(int j) { i+=j; return *this; }
	inline ConstIterator operator+(int j) const { return ConstIterator(i+j); }
	inline ConstIterator operator-(int j) const { return ConstIterator(i-j); }
	inline int operator-(ConstIterator j) const { return i - j.i; }
	friend class QList;
    };

    inline Iterator begin() { detach(); return (Node*) p.begin(); }
    inline ConstIterator begin() const { return (Node*) p.begin(); }
    inline ConstIterator constBegin() const { return (Node*) p.begin(); }
    inline Iterator end() { detach(); return (Node*) p.end(); }
    inline ConstIterator end() const { return (Node*) p.end(); }
    inline ConstIterator constEnd() const { return (Node*) p.end(); }

    Iterator insert(Iterator before, const T &t);
    Iterator erase(Iterator it);
    Iterator erase(Iterator first, Iterator last);

    inline T& first() { Q_ASSERT(!isEmpty()); return *begin(); }
    inline const T& first() const { Q_ASSERT(!isEmpty()); return *begin(); }
    T& last() { Q_ASSERT(!isEmpty()); return *(--end()); }
    const T& last() const { Q_ASSERT(!isEmpty()); return *(--end()); }

    inline void removeFirst() { Q_ASSERT(!isEmpty()); erase( begin() ); }
    inline void removeLast() { Q_ASSERT(!isEmpty()); erase( --end() ); }

    // stl compatibility
    typedef Iterator iterator;
    typedef ConstIterator const_iterator;
    inline void push_back(const T &t) { append(t); }
    inline void push_front(const T &t) { prepend(t); }
    inline T& front() { first(); }
    inline const T& front() const { first(); }
    inline T& back() { last(); }
    inline const T& back() const { last(); }
    inline void pop_front() { removeFirst(); }
    inline void pop_back() { removeLast(); }
    inline bool empty() const { return isEmpty(); }
    inline Iterator find (const T& t)
        { int i = indexOf(t); return (i == -1 ? end() : (begin()+i)); }
    inline ConstIterator find (const T& t) const
        { int i = indexOf(t); return (i == -1 ? end() : (begin()+i)); }
    inline Iterator find (Iterator it, const T& t)
	{ int i = indexOf(t, it-begin()); return i == -1 ? end() : begin()+i; }
    inline ConstIterator find (ConstIterator it, const T& t) const
	{ int i = indexOf(t, it-begin()); return i == -1 ? end() : begin()+i; }
    typedef int size_type;

#ifndef QT_NO_COMPAT
    // compatibility
    inline Iterator remove(Iterator it) { return erase(it); }
    inline int count() const { return p.size(); }
    inline int findIndex( const T& x ) const { return indexOf(x); }
#endif

    // comfort
    QList &operator+=(const QList &l);
    QList operator+(const QList &l) const
    { QList n = *this; n += l; return n; }
    inline void operator+=(const T &t)
    { append(t); }
    inline QList &operator<< (const T &t)
    { append(t); return *this; }

    inline bool ensure_constructed()
    { if (!d) { d = &QListData::shared_null; ++d->ref; return false; } return true; }

private:
    void detach_helper();
    void free(QListData::Data *d);
    inline bool canAutoDelete() const { return Q_TYPEINFO_POINTER(T); }

    inline void node_construct(Node *n, const T &t)
    {
	    if (Q_TYPEINFO_LARGE_OR_STATIC(T)) n->v = new T(t);
	    else if (Q_TYPEINFO_COMPLEX(T)) new (n) T(t);
	    else *(T*)n = t;
    }
    inline void node_destruct(Node *n)
    {
	if (Q_TYPEINFO_LARGE_OR_STATIC(T)) delete (T*)n->v;
	else if (Q_TYPEINFO_COMPLEX(T)) ((T*)n)->~T();
 	else if (d->autoDelete == this) qDelete(*(T*)n);
    }
    inline void node_take(Node *n)
    {
	if (Q_TYPEINFO_LARGE_OR_STATIC(T)) delete (T*)n->v;
	else if (Q_TYPEINFO_COMPLEX(T)) ((T*)n)->~T();
    }
    inline void node_copy(Node *from, Node *to, Node *src)
    {
	if (Q_TYPEINFO_LARGE_OR_STATIC(T))
	    while(from != to)
		(from++)->v = new T(*(T*)(src++)->v);
	else if (Q_TYPEINFO_COMPLEX(T))
	    while(from != to)
		new (from++) T(*(T*)src++);
    }
    inline void node_destruct(Node *from, Node *to, bool autoDelete )
    {
	if (Q_TYPEINFO_LARGE_OR_STATIC(T))
	    while(from != to) --to, delete (T*) to->v;
	else if (Q_TYPEINFO_COMPLEX(T))
	    while (from != to) --to, ((T*)to)->~T();
 	else if (autoDelete)
 	    while (from != to) { --to; qDelete(*(T*)to); }
    }

};

template <typename T>
inline QList<T> &QList<T>::operator=(const QList<T> &l)
{
    if (d != l.d) {
	QListData::Data *x = l.d;
	++x->ref;
	x = qAtomicSetPtr(&d, x);
	if (!--x->ref || (Q_TYPEINFO_POINTER(T) && x->autoDelete == this))
	    free(x);
    }
    return *this;
}
template <typename T>
inline typename QList<T>::Iterator QList<T>::insert(Iterator before, const T &t)
{ Node *n = (Node*) p.insert(before.i-(Node*)p.begin());
 node_construct(n,t); return n; }
template <typename T>
inline typename QList<T>::Iterator QList<T>::erase(Iterator it)
{ node_destruct(it.i);
 return (Node*) p.erase((void**)it.i); }
template <typename T>
inline const T &QList<T>::at(int i) const
{ Q_ASSERT(i >= 0 && i < size());
 return ((Node*) p.at(i))->t(); }
template <typename T>
inline const T &QList<T>::operator[](int i) const
{ Q_ASSERT(i >= 0 && i < size());
 return ((Node*) p.at(i))->t(); }
template <typename T>
inline T &QList<T>::operator[](int i)
{ Q_ASSERT(i >= 0 && i < size()); detach();
 return ((Node*) p.at(i))->t(); }
template <typename T>
inline void QList<T>::removeAt(int i)
{ if(i >= 0 && i < p.size()) { detach();
 node_destruct((Node*) p.at(i)); p.remove(i); } }
template <typename T>
inline T QList<T>::takeAt(int i)
{ Q_ASSERT(i >= 0 && i < size()); detach();
 Node*n = (Node*)p.at(i); T t = n->t(); node_take(n);
p.remove(i); return t; }
template <typename T>
inline void QList<T>::append(const T &t)
{ detach();node_construct((Node*)p.append(), t); }
template <typename T>
inline void QList<T>::prepend(const T &t)
{ detach(); node_construct(((Node*)p.prepend()), t); }
template <typename T>
inline void QList<T>::insert(int i, const T &t)
{ detach(); node_construct((Node*)p.insert(i), t); }
template <typename T>
inline void QList<T>::replace(int i, const T &t)
{ Q_ASSERT(i >= 0 && i < size()); detach();
 ((Node*)p.at(i))->t() = t; }
template <typename T>
inline bool QList<T>::autoDelete() const
{ return d->autoDelete == (void*) this; }

template <typename T>
inline void QList<T>::setAutoDelete(bool enable)
{
    Q_ASSERT(canAutoDelete());
    if (Q_TYPEINFO_POINTER(T)) {
	detach();
	 d->autoDelete = enable ? this : 0;
    }
}

template <typename T>
void QList<T>::detach_helper()
{
    Node *n = (Node*) p.begin();
    QListData::Data *x = p.detach();
    if (x)
	free(x);
    node_copy((Node*) p.begin(), (Node*) p.end(), n);
}

template <typename T>
inline QList<T>::~QList()
{
    if (!--d->ref  || (Q_TYPEINFO_POINTER(T) && d->autoDelete == this))
	free(d);
}

template <typename T>
bool QList<T>::operator== (const QList<T> &l) const
{
    if (p.size() != l.p.size())
	return false;
    if (d == l.d)
	return true;
    Node *i = (Node*) p.end();
    Node *b = (Node*) p.begin();
    Node *li = (Node*) l.p.end();
    while (i != b) {
	--i; --li;
	if (! (i->t() == li->t()))
	    return false;
    }
    return true;
}


template <typename T>
void QList<T>::free(QListData::Data *d)
{
    node_destruct((Node*)(d->array + d->begin),
		  (Node*)(d->array + d->end),
		  (d->autoDelete == this));
    d->autoDelete = 0;
    if (d->ref == 0)
	qFree(d);
}


template <typename T>
void QList<T>::clear()
{
    bool wasAutoDelete = d->autoDelete == this;
    *this = QList<T>();
    if (Q_TYPEINFO_POINTER(T) && wasAutoDelete)
	setAutoDelete(wasAutoDelete);
}

template <typename T>
int QList<T>::remove(const T &t)
{
    detach();
    int count=0, i=0;
    Node *n;
    while (i < p.size())
	if ((n = (Node*)p.at(i))->t() == t) {
	    node_destruct(n);
	    p.remove(i);
	    ++count;
	} else {
	    ++i;
	}
    return count;
}

template <typename T>
int QList<T>::take(const T &t)
{
    detach();
    int count=0, i=0;
    Node *n;
    while (i < p.size())
	if ((n = (Node*)p.at(i))->t() == t) {
	    node_take(n);
	    p.remove(i);
	    ++count;
	} else {
	    ++i;
	}
    return count;
}

template <typename T>
typename QList<T>::Iterator QList<T>::erase( typename QList<T>::Iterator first,
					     typename QList<T>::Iterator last )
{
    for ( Node *n = (Node*) first; n <= (Node*) last; ++n)
	node_destruct(n);
    p.remove(i, last - first + 1);
}

template <typename T>
QList<T> &QList<T>::operator+=(const QList<T> &l)
{
    detach();
    Node *n = (Node*)p.append(l.p);
    node_copy(n, (Node*) p.end(), (Node*) l.p.begin());
    return *this;
}

template <typename T>
int QList<T>::indexOf(const T &t, int from) const
{
    if (from < 0)
	from = QMAX(from + p.size(), 0);
    if (from < p.size()) {
	Node *n = (Node*) p.at(from -1);
	Node *e = (Node*) p.end();
	while (++n != e)
	    if (n->t() == t)
		return n - (Node*)p.begin();
    }
    return -1;
}

template <typename T>
int QList<T>::lastIndexOf(const T &t, int from) const
{
    if (from < 0)
	from += p.size();
    else if (from >= p.size())
	from = p.size()-1;
    if (from >= 0) {
	Node *b = (Node*) p.begin();
	Node *n = (Node*) p.at(from + 1);
	while (n-- != b) {
	    if (n->t() == t)
		return n - b;
	}
    }
    return -1;
}

template <typename T>
QBool QList<T>::contains(const T &t) const
{
    Node *b = (Node*) p.begin();
    Node *i = (Node*) p.end();
    while (i-- != b)
	if (i->t() == t)
	    return true;
    return false;
}

template <typename T>
int QList<T>::count(const T &t) const
{
    int c = 0;
    Node *b = (Node*) p.begin();
    Node *i = (Node*) p.end();
    while (i-- != b)
	if (i->t() == t)
	    ++c;
    return c;
}

Q_DECLARE_ITERATOR(QList)

#ifndef QT_NO_DATASTREAM
template <typename T>
QDataStream& operator>>( QDataStream& s, QList<T>& l )
{
    l.clear();
    Q_UINT32 c;
    s >> c;
    for( Q_UINT32 i = 0; i < c; ++i )
    {
	T t;
	s >> t;
	l.append( t );
	if ( s.atEnd() )
	    break;
    }
    return s;
}

template <typename T>
QDataStream& operator<<( QDataStream& s, const QList<T>& l )
{
    s << (Q_UINT32)l.size();
    typename QList<T>::ConstIterator it = l.begin();
    for( ; it != l.end(); ++it )
	s << *it;
    return s;
}
#endif // QT_NO_DATASTREAM

#endif // QLIST_H


#include "qlinkedlist.h"

QLinkedListData QLinkedListData::shared_null = {
    &QLinkedListData::shared_null, &QLinkedListData::shared_null, Q_ATOMIC_INIT(1), 0
};

/*! \class QLinkedList
    \brief The 

    ###

	* doesn't provide at(), operator[](), no index-based API
        * have to use iterators
	* find an item: use an iterator

	* can use end() as a position
*/

/*! \fn QLinkedList::QLinkedList()

    Constructs an empty list.
*/

/*! \fn QLinkedList::QLinkedList(const QLinkedList &other)

    Constructs a copy of \a other.

    This operation occurs in \l{constant time}, because QLinkedList
    is \l{implicitly shared}. This makes returning a QLinkedList from
    a function very fast. If a shared instance is modified, it will
    be copied (copy-on-write), and this takes \l{linear time}.

    \sa operator=()
*/

/*! \fn QLinkedList::~QLinkedList()

    Destroys the list. References to the values in the list and all
    iterators of this list become invalid.
*/

/*! \fn QLinkedList &QLinkedList::operator=(const QLinkedList &other)

    Assigns \a other to this list and returns a reference to this
    list.
*/

/*! \fn bool QLinkedList::operator==(const QLinkedList &other) const

    Returns true if \a other is equal to this list; otherwise returns
    false.

    Two lists are considered equal if they contain the same values in
    the same order.

    This function requires the value type to implement \c
    operator==().

    \sa operator!=()
*/

/*! \fn bool QLinkedList::operator!=(const QLinkedList &other) const

    Returns true if \a other is not equal to this list; otherwise
    returns false.

    Two lists are considered equal if they contain the same values in
    the same order.

    This function requires the value type to implement \c
    operator==().

    \sa operator==()
*/

/*! \fn int QLinkedList::size() const

    Returns the number of items in the list.

    \sa isEmpty(), count()
*/

/*! \fn void QLinkedList::detach()

    \internal
*/

/*! \fn bool QLinkedList::isDetached() const

    \internal
*/

/*! \fn bool QLinkedList::isEmpty() const

    Returns true if the list contains no items; otherwise returns
    false.

    \sa size()
*/

/*! \fn bool QLinkedList::operator!() const

    \internal
*/

/*! \fn QLinkedList::operator QSafeBool() const

    Returns true if the list contains at least one items; otherwise
    returns false.

    Example:
    \code
	static QLinkedList<QString> list;
        ...
        if (list)
	    do_something(list);
    \endcode

    This is the same as \c{!list.isEmpty()}.

    \sa isEmpty()
*/

/*! \fn void QLinkedList::clear()

    Removes all items from the list.

    \sa remove()
*/

/*! \fn void QLinkedList::append(const T &t)

    Inserts item \a t at the end of the list.

    Example:
    \code
	QLinkedList<QString> list;
        list.append("one");
        list.append("two");
        list.append("three");
        // list: [ "one", "two", "three" ]
    \endcode

    This is the same as list.insert(end(), \a t).

    \sa operator<<(), prepend(), insert()
*/

/*! \fn void QLinkedList::prepend(const T &)

    Inserts \a t at the beginning of the list.

    Example:
    \code
	QLinkedList<QString> list;
        list.prepend("one");
        list.prepend("two");
        list.prepend("three");
        // list: [ "three", "two", "one" ]
    \endcode

    This is the same as list.insert(begin(), \a t).

    \sa append(), insert()
*/

/*! \fn int QLinkedList::remove(const T &t)

    Removes all occurrences of item \a t in the list.

    Example:
    \code
	QList<QString> list;
        list << "sun" << "cloud" << "sun" << "rain";
        list.remove("sun");
        // list: [ "cloud", "rain" ]
    \endcode

    This function requires the value type to have an implementation of
    \c operator==().

    \sa insert()
*/

/*! \fn bool QLinkedList::contains(const T &t) const

    Returns true if the list contains an occurrence of the value
    \a t; otherwise returns false.

    This function requires the value type to have an implementation of
    \c operator==().

    \sa QListIterator::findNext(), QListIterator::findPrev()
*/

/*! \fn int QLinkedList::count(const T &t) const

    Returns the number of occurrences of the value \a t in the list.

    This function requires the value type to have an implementation of
    \c operator==().

    \sa contains()
*/

/*! \fn QLinkedList::iterator QLinkedList::begin()

    Returns an \l{STL-style iterator} pointing to the first item in
    the list.

    \sa constBegin(), end()
*/

/*! \fn QLinkedList::const_iterator QLinkedList::begin() const

    \overload
*/

/*! \fn QLinkedList::const_iterator QLinkedList::constBegin() const

    Returns a const \l{STL-style iterator} pointing to the first item
    in the list.

    \sa begin(), constEnd()
*/

/*! \fn QLinkedList::iterator QLinkedList::end()

    Returns an \l{STL-style iterator} pointing to the imaginary item
    after the last item in the list.

    \sa begin(), constEnd()
*/

/*! \fn QLinkedList::const_iterator QLinkedList::end() const

    \overload
*/

/*! \fn QLinkedList::const_iterator QLinkedList::constEnd() const

    Returns a const \l{STL-style iterator} pointing to the imaginary
    item after the last item in the list.

    \sa constBegin(), end()
*/

/*! \fn QLinkedList::iterator QLinkedList::insert(iterator before, const T &t)

    Inserts the value \a t in front of the item pointed to by the
    iterator \a before. Returns an iterator pointing at the inserted
    item.

    \sa erase()
*/

/*! \fn QLinkedList::iterator QLinkedList::erase(iterator pos)

    Removes the item associated with the iterator \a pos from the
    list, and returns an iterator to the next item in the list (which
    may be end()).

    \sa insert()
*/

/*! \fn QLinkedList::iterator QLinkedList::erase(iterator first, iterator last)

    \overload

    Removes all the items from \a begin up to (but not including) \a
    end.
*/

/*! \fn typedef QLinkedList::Iterator

    Qt-style synonym for QList::iterator.
*/

/*! \fn typedef QLinkedList::ConstIterator

    Qt-style synonym for QList::const_iterator.
*/

/*! \fn int QLinkedList::count() const

    Same as size().
*/

/*! \fn T& QLinkedList::first()

    Returns a reference to the first item in the list. This function
    assumes that the list isn't empty.

    \sa last(), isEmpty()
*/

/*! \fn const T& QLinkedList::first() const

    \overload
*/

/*! \fn T& QLinkedList::last()

    Returns a reference to the last item in the list. This function
    assumes that the list isn't empty.

    \sa first(), isEmpty()
*/

/*! \fn const T& QLinkedList::last() const

    \overload
*/

/*! \fn void QLinkedList::removeFirst()

    Removes the first item in the list.

    This is the same as erase(begin()).

    \sa removeLast(), erase()
*/

/*! \fn void QLinkedList::removeLast()

    Removes the last item in the list.

    \sa removeFirst(), erase()
*/

/*! \fn void QLinkedList::push_back(const T &t)

    This function is provided for STL compatibility. It is equivalent
    to append().
*/

/*! \fn void QLinkedList::push_front(const T &t)

    This function is provided for STL compatibility. It is equivalent
    to prepend().
*/

/*! \fn T& QLinkedList::front()

    This function is provided for STL compatibility. It is equivalent
    to first().
*/

/*! \fn const T& QLinkedList::front() const

    \overload
*/

/*! \fn T& QLinkedList::back()

    This function is provided for STL compatibility. It is equivalent
    to last().
*/

/*! \fn const T& QLinkedList::back() const

    \overload
*/

/*! \fn void QLinkedList::pop_front()

    This function is provided for STL compatibility. It is equivalent
    to removeFirst().
*/

/*! \fn void QLinkedList::pop_back()

    This function is provided for STL compatibility. It is equivalent
    to removeLast().
*/

/*! \fn bool QLinkedList::empty() const

    This function is provided for STL compatibility. It is equivalent
    to isEmpty().
*/

/*! \fn QLinkedList &QLinkedList::operator+=(const QLinkedList &other)

    Appends the items of the \a other list to this list and returns a
    reference to this list.

    \sa operator+(), append()
*/

/*! \fn QLinkedList QLinkedList::operator+(const QLinkedList &other) const

    Returns a list that contains all the items in this list followed
    by all the items in the \a other list.

    \sa operator+=()
*/

/*! \fn void QLinkedList::operator+=(const T &t)

    \overload

    Appends the value \a t to the list.

    \sa append(), operator<<()
*/

/*! \fn QLinkedList &QLinkedList::operator<<(const T &t)

    Appends the value \a t to the list and returns a reference to this
    list.

    \sa append(), operator+=()
*/

/*! \fn bool QLinkedList::ensure_constructed()

    \internal
*/

/*! \class QLinkedList::iterator
    \brief The QLinkedList::iterator class provides an STL-style non-const iterator for QLinkedList.

    QLinkedList provides both \l{STL-style iterators} and
    \l{Java-style iterators}. The STL-style iterators are more
    low-level and more cumbersome to use; on the other hand, they are
    slightly faster and, for developers who already know STL, have
    the advantage of familiarity.

    QLinkedList::iterator allows you to iterate over a QLinkedList
    and to modify the list item associated with the iterator. If you
    want to iterate over a const QLinkedList, you should use
    QLinkedList::const_iterator. It is generally good practice to use
    QLinkedList::const_iterator on a non-const QLinkedList as well,
    unless you need to change the QLinkedList through the iterator.
    Const iterators are slightly faster, and can improve code
    readability.

    The default QLinkedList::iterator constructor creates an
    uninitialized iterator. You must initialize it using a
    function like QLinkedList::begin(), QLinkedList::end(), or
    QLinkedList::insert() before you can start iterating. Here's a
    typical loop that prints all the items stored in a list:

    \code
	QLinkedList<QString> list;
        list.append("January");
        list.append("February");
        ...
        list.append("December");

        QLinkedList<QString, int>::iterator i;
        for (i = list.begin(); i != list.end(); ++i)
	    cout << *i << endl;
    \endcode

    STL-style iterators can be used as arguments to \l{generic
    algorithms}. For example, here's how to find an item in the list
    using the qFind() algorithm:

    \code
	QLinkedList<QString> list;
        ...
	QLinkedList<QString>::iterator it = qFind(list.begin(),
						  list.end(), "Joel");
	if (it != list.end())
	    cout << "Found Joel" << endl;
    \endcode

    Let's see a few examples of things we can do with a
    QLinkedList::iterator that we cannot do with a QLinkedList::const_iterator.
    Here's an example that increments every value stored in a
    QLinkedList\<int\> by 2:

    \code
	QLinkedList<int>::iterator i;
        for (i = list.begin(); i != list.end(); ++i)
	    *i += 2;
    \endcode

    Here's an example that removes all the items that start with an
    underscore character in a QLinkedList\<QString\>:

    \code
	QLinkedList<QString> list;
	...
	QLinkedList<QString>::iterator i = list.begin();
        while (i != list.end()) {
	    if ((*i).startsWith("_"))
		i = list.erase(i);
	    else
		++i;
        }
    \endcode

    The call to QLinkedList::erase() removes the item pointed to by
    the iterator from the list, and returns an iterator to the next
    item. Here's another way of removing an item while iterating:

    \code
	QLinkedList<QString>::iterator i = list.begin();
        while (i != list.end()) {
	    QLinkedList<QString>::iterator prev = i;
	    ++i;
            if ((*prev).startsWith("_"))
		list.erase(prev);
        }
    \endcode

    It might be tempting to write code like this:

    \code
	// WRONG
        while (i != list.end()) {
	    if ((*i).startsWith("_"))
		list.erase(i);
	    ++i;
        }
    \endcode

    However, this will potentially crash in \c{++i}, because \c i is
    a dangling iterator after the call to erase().

    Multiple iterators can be used on the same list. If you add items
    to the list, existing iterators will remain valid. If you remove
    items from the list, iterators that point to the removed items
    will become dangling iterators.

    \sa QLinkedList::const_iterator, QLinkedListMutableIterator
*/

/*! \fn QLinkedList::iterator::iterator()

    Creates an unitialized iterator.

    Functions like operator*() and operator++() should not be called
    on an unitialized iterartor. Use operator=() to assign a value to
    it before using it.
*/

/*! \fn QLinkedList::iterator::iterator(Node *n)

    \internal
*/

/*! \fn QLinkedList::iterator::iterator(const iterator &other)

    Creates a copy of \a other.
*/

/*! \fn T &QLinkedList::iterator::operator*() const

    Returns a modifiable reference to the current item.

    You can change the value of an item by using operator*() on the
    left side of an assignment, for example:

    \code
	if (*it == "Hello")
	    *it = "Bonjour";
    \endcode
*/

/*! \fn bool QLinkedList::iterator::operator==(const iterator &other) const

    Returns true if \a other points to the same item as this
    iterator; otherwise returns false.

    \sa operator!=()
*/

/*! \fn bool QLinkedList::iterator::operator!=(const iterator &other) const

    Returns true if \a other points to a different item than this
    iterator; otherwise returns false.

    \sa operator==()
*/

/*! \fn QLinkedList::iterator QLinkedList::iterator::operator++()

    The prefix ++ operator (\c{++it}) advances the iterator to the
    next item in the list and returns an iterator to the new current
    item.

    Calling this function on QLinkedList::end() leads to undefined
    results.

    \sa operator--()
*/

/*! \fn QLinkedList::iterator QLinkedList::iterator::operator++(int)

    \overload

    The postfix ++ operator (\c{it++}) advances the iterator to the
    next item in the list and returns an iterator to the previously
    current item.
*/

/*! \fn QLinkedList::iterator QLinkedList::iterator::operator--()

    The prefix -- operator (\c{--it}) makes the preceding item
    current and returns an iterator to the new current item.

    Calling this function on QLinkedList::begin() leads to undefined
    results.

    \sa operator++()
*/

/*! \fn QLinkedList::iterator QLinkedList::iterator::operator--(int)

    \overload

    The postfix -- operator (\c{it--}) makes the preceding item
    current and returns an iterator to the previously current item.
*/

/*! \fn QLinkedList::iterator QLinkedList::iterator::operator+(int j) const

    Returns an iterator to the item at \a j positions forward from
    this iterator. (If \a j is negative, the iterator goes backward.)

    This operation can be slow for large \a j values.

    \sa operator-()

*/

/*! \fn QLinkedList::iterator QLinkedList::iterator::operator-(int j) const

    Returns an iterator to the item at \a j positions backward from
    this iterator. (If \a j is negative, the iterator goes forward.)

    This operation can be slow for large \a j values.

    \sa operator+()
*/

/*! \fn QLinkedList::iterator &QLinkedList::iterator::operator+=(int j)

    Advances the iterator by \a j items. (If \a j is negative, the
    iterator goes backward.)

    \sa operator-=(), operator+()
*/

/*! \fn QLinkedList::iterator &QLinkedList::iterator::operator-=(int j)

    Makes the iterator go back by \a j items. (If \a j is negative,
    the iterator goes forward.)

    \sa operator+=(), operator-()
*/

/*! \class QLinkedList::const_iterator

    ###
*/

/*! \fn QLinkedList::const_iterator::const_iterator()

    Creates an unitialized iterator.

    Functions like operator*() and operator++() should not be called
    on an unitialized iterartor. Use operator=() to assign a value to
    it before using it.
*/

/*! \fn QLinkedList::const_iterator::const_iterator(Node *n)

    \internal
*/

/*! \fn QLinkedList::const_iterator::const_iterator(const const_iterator &other)

    Creates a copy of \a other.
*/

/*! \fn QLinkedList::const_iterator::const_iterator(iterator other)

    Creates a copy of \a other.
*/

/*! \fn const T &QLinkedList::const_iterator::operator*() const

    Returns a reference to the current item.
*/

/*! \fn bool QLinkedList::const_iterator::operator==(const const_iterator &other) const

    Returns true if \a other points to the same item as this
    iterator; otherwise returns false.

    \sa operator!=()
*/

/*! \fn bool QLinkedList::const_iterator::operator!=(const const_iterator &other) const

    Returns true if \a other points to a different item than this
    iterator; otherwise returns false.

    \sa operator==()
*/

/*! \fn QLinkedList::const_iterator QLinkedList::const_iterator::operator++()

    The prefix ++ operator (\c{++it}) advances the iterator to the
    next item in the list and returns an iterator to the new current
    item.

    Calling this function on QLinkedList::constEnd() leads to
    undefined results.

    \sa operator--()
*/

/*! \fn QLinkedList::const_iterator QLinkedList::const_iterator::operator++(int)

    \overload

    The postfix ++ operator (\c{it++}) advances the iterator to the
    next item in the list and returns an iterator to the previously
    current item.
*/

/*! \fn QLinkedList::const_iterator QLinkedList::const_iterator::operator--()

    The prefix -- operator (\c{--it}) makes the preceding item
    current and returns an iterator to the new current item.

    Calling this function on QLinkedList::begin() leads to undefined
    results.

    \sa operator++()
*/

/*! \fn QLinkedList::const_iterator QLinkedList::const_iterator::operator--(int)

    \overload

    The postfix -- operator (\c{it--}) makes the preceding item
    current and returns an iterator to the previously current item.
*/

/*! \fn QLinkedList::const_iterator QLinkedList::const_iterator::operator+(int j) const

    Returns an iterator to the item at \a j positions forward from
    this iterator. (If \a j is negative, the iterator goes backward.)

    This operation can be slow for large \a j values.

    \sa operator-()
*/

/*! \fn QLinkedList::const_iterator QLinkedList::const_iterator::operator-(int j) const

    Returns an iterator to the item at \a j positions backward from
    this iterator. (If \a j is negative, the iterator goes forward.)

    This operation can be slow for large \a j values.

    \sa operator+()
*/

/*! \fn QLinkedList::const_iterator &QLinkedList::const_iterator::operator+=(int j)

    Advances the iterator by \a j items. (If \a j is negative, the
    iterator goes backward.)

    \sa operator-=(), operator+()
*/

/*! \fn QLinkedList::const_iterator &QLinkedList::const_iterator::operator-=(int j)

    Makes the iterator go back by \a j items. (If \a j is negative,
    the iterator goes forward.)

    \sa operator+=(), operator-()
*/

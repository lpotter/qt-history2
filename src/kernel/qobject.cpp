/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qobject.cpp#38 $
**
** Implementation of QObject class
**
** Author  : Haavard Nord
** Created : 930418
**
** Copyright (C) 1993-1995 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#include "qobject.h"
#include "qobjcoll.h"
#include <ctype.h>

#if defined(DEBUG)
static char ident[] = "$Id: //depot/qt/main/src/kernel/qobject.cpp#38 $";
#endif


/*! \class QObject qobject.h

  \brief The QObject class is the base class of all Qt objects that can
  deal with signals, slots and events.

  Qt uses a very powerful mechanism, signals that are connected to
  slots, for communication between objects.  An object has zero or
  more signals and zero or more slots, and a signal may be connected
  to a slot in any object.

  When an object has changed in some way that might be interesting, it
  emits a signal to tell whoever is interested.  If that signal is
  connected to any slots, those slots get called.

  That is. the calling or emitting object doesn't need to know or care
  what slot the signal is connected to.  It works much like the
  traditional callback mechanism, but it isn't necessary to write null
  pointer checks or for loops all the time.

  QScrollBar is a good example.  In order to use it, you create a
  scroll bar, connect its newValue() signal to a slot which
  e.g. scrolls your widget.  Then, if it suits you, you can connect
  e.g.  QScrollBar::nextLine() to a suitable slot.  nextLine() is
  useless in most cases but if you need it, it's there.

  */

/*! \class QSenderObject qobject.h

  \brief Internal object used for sending signals.

  This class is not yet documented.  Our <a
  href=http://www.troll.no/>home page</a> contains a pointer to the
  current version of Qt. */

/*! \fn QMetaObject *QObject::metaObject() const
  Returns a pointer to this object's meta object.  The meta object is
  generated by the meta object compiler, and is used for e.g. signals,
  slots, inherits(), isA(). */

/*! \fn const char *QObject::name() const
  Returns this object's name (as specified to the constructor).

  The object's name is not yet used for anything except debugging
  internally.  (We do have a purpose in mind, but the purpose has not
  yet been solidified into an implementation.)

  \sa className(). */

/*! \fn const char *QObject::className() const
  Returns this object's type as an ASCII string.  This is the \e
  actual type, not what the object is used as:

  \code
  QRadioButton * rb = new QRadioButton( parent, "example" );
  Button * b;

  b = rb;
  ASSERT ( strcmp( b->name(), "example" ) == 0 )
  ASSERT ( strcmp( b->className(), "QRadioButton" ) == 0 )
  \endcode

  This happens through the magic of the moc.

  \sa name(). */

/*! \fn bool QObject::isWidgetType() const
  Returns TRUE if the object is a widget, FALSE if not. */

/*! \fn bool QObject::highPriority() const
  Returns TRUE if the object is high priority.  High priority objects
  are placed first in an object's list of children, on the assumption
  that they'll be referenced very often.

  Currently only QAccel is high priority. */

/*! \fn bool QObject::signalsBlocked() const
  Returns TRUE if this object is currently not accepting signals.  \sa
  blockSignals(). */

/*! \fn const QObjectList *QObject::children() const

  Returns a pointer to a list of this object's children.  High
  priority children are first on the list.

  This function is not for the inexperienced.

  \sa insertChild(), removeChild(), parent(). */

/*! \fn QObject *QObject::parent() const

  Returns a pointer to this object's parent.  

  This function is not for the inexperienced.

  \sa children(). */

/*! \fn bool QObject::connect( QObject *sender, const char *signal, const char *member ) const

  Connects \e signal from object \e sender to \e member in this
  object. */

/*! \fn bool QObject::disconnect( const char *signal, const QObject *receiver, const char *member )

  Disconnects \e signal from \e member of \e receiver. */

/*! \fn bool QObject::disconnect( const QObject *receiver, const char *member )

  Disconnects all signals in this object from \e member of \e receiver.

  \todo this */

/*! \fn QObject *QObject::sender()

  This functions has not yet been documented; see our <a
  href=http://www.troll.no/>home page</a> for updates.

  */


/* Remove white space from SIGNAL and SLOT names */

static QString rmWS( const char *src )
{
    QString tmp( strlen( src ) + 1 );
    register char *d = tmp.data();
    register char *s = (char *)src;
    while( *s && isspace(*s) )
        s++;
    while ( *s ) {
        while( *s && !isspace(*s) )
	    *d++ = *s++;
        while( *s && isspace(*s) )
            s++;
        if ( *s && ( isalpha(*s) || *s == '_' ) )
            *d++ = ' ';
    }
    tmp.truncate( d - tmp.data() );
    return tmp;
}

// Event functions, implemented in qapp_xxx.cpp

int   qStartTimer( long interval, QObject *obj );
bool  qKillTimer( int id );
bool  qKillTimer( QObject *obj );

void  qRemovePostedEvents( QObject * );


declare(QListM,QConnection);			// dictionary of connections
declare(QListIteratorM,QConnection);		// dictionary iterator
declare(QDictM,QListM(QConnection));
declare(QDictIteratorM,QListM(QConnection));

QMetaObject *QObject::metaObj = 0;


static void removeObjFromList( QObjectList *objList, const QObject *obj,
			       bool single=FALSE )
{
    if ( !objList )
	return;
    int index = objList->findRef( obj );
    while ( index >= 0 ) {
	objList->remove();
	if ( single )
	    return;
	index = objList->findNextRef( obj );
    }
}


// ---------------------------------------------------------------------------
// QObject member functions
//

/*! Constructs an object with parent objects \e parent and a \e name.

  The parent of a widget may be viewed as the object's owner.  For
  instance, a dialog box is the parent of the "ok" and "cancel"
  buttons inside it.

  An object destroys its child objects when it is destroyed itself.

  Sending a null pointer as \e parent makes an object with no parent.
  If the object is a widget, it will become a top-level window.

  The \link name() name \endlink is not used at present.

  \sa destroy(), destroyed(), name(), QWidget. */

QObject::QObject( QObject *parent, const char *name )
{
    if ( !objectDict )				// will create object dict
	initMetaObject();
    objname = name ? strdup(name) : 0;		// set object name
    parentObj = parent;				// set parent
    childObjects = 0;				// no children yet
    connections = 0;				// no connections yet
    senderObjects = 0;				// no signals connected yet
    eventFilters = 0;				// no filters installed
    sigSender = 0;				// no sender yet
    isSignal   = FALSE;				// assume not a signal object
    isWidget   = FALSE;				// assume not a widget object
    hiPriority = FALSE;				// normal priority
    pendTimer  = FALSE;				// no timers yet
    pendEvent  = FALSE;				// no events yet
    blockSig   = FALSE;				// not blocking signals
    if ( parentObj )				// add object to parent
	parentObj->insertChild( this );
}

/*! Destroys the object and all its children objects.

  All signals to and from the object are automatically disconnected. */

QObject::~QObject()
{
    if ( objname )
	delete objname;
    if ( pendTimer )				// might be pending timers
	qKillTimer( this );
    if ( pendEvent )				// pending posted events
	qRemovePostedEvents( this );
    if ( parentObj )				// remove it from parent object
	parentObj->removeChild( this );
    register QObject *obj;
    if ( senderObjects ) {			// disconnect from senders
	QObjectList *tmp = senderObjects;
	senderObjects = 0;
	obj = tmp->first();
	while ( obj ) {				// for all senders...
	    obj->disconnect( this );
	    obj = tmp->next();
	}
	delete tmp;
    }
    if ( connections ) {			// disconnect receivers
	QSignalDictIt it(*connections);
	QConnectionList *clist;
	while ( (clist=it.current()) ) {	// for each signal...
	    ++it;
	    register QConnection *c;
	    QConnectionListIt cit(*clist);
	    while( (c=cit.current()) ) {	// for each connected slot...
		++cit;
		if ( (obj=c->object()) )
		    removeObjFromList( obj->senderObjects, this );
	    }
	}
	delete connections;
    }
    if ( childObjects ) {			// delete children objects
	obj = childObjects->first();
	while ( obj ) {				// delete all child objects
	    obj->parentObj = 0;			// object should not remove
	    delete obj;				//   itself from the list
	    obj = childObjects->next();
	}
	delete childObjects;
    }
    delete eventFilters;
}


/*! Returns TRUE if this object is an instance of a specified class,
  otherwise FALSE.

  \code
  QTimer *t = new QTimer;       \/ QTimer inherits QObject
  t->isA("QTime");              \/ returns TRUE
  t->isA("QObject");            \/ returns FALSE
  \endcode

  \sa inherits().
*/

bool QObject::isA( const char *clname ) const	// test if is-a class
{
    return strcmp(className(),clname) == 0;
}

/*! Returns TRUE if this object is an istance of a class that inherits
  \e clname.  (A class is considered to inherit itself.)

  \code
  QTimer *t = new QTimer;       \/ QTimer inherits QObject
  t->inherits("QTimer");        \/ returns TRUE
  t->inherits("QObject");       \/ returns TRUE
  t->inherits("QButton");       \/ returns FALSE
  \endcode

  This function may be used to determine whether a given object
  supports certain features.  Qt uses it to implement keyboard
  accelerators, for instance: A keyboard accelerator is an object for
  which <code>inherits("QAccel")</code> is TRUE.

  \sa isA(). */

bool QObject::inherits( const char *clname ) const
{						// test if inherits class
    QMetaObject *meta = queryMetaObject();
    while ( meta ) {
	if ( strcmp(clname,meta->className()) == 0 )
	    return TRUE;
	meta = meta->superClass();
    }
    return FALSE;
}


/*! Returns the class name of this object.  If this doesn't work,
  you've probably forgotten the magic incantation:

  \code
  class Whatever : public QSomething {
      Q_OBJECT                  \/ You forgot this
  public:
      ....
  \endcode

  Or you might have forgotten running the <a
  href=metaobject.html>moc</a>. */

const char *QObject::className() const		// get name of class
{
    return "QObject";
}


/*! \fn const char *QObject::name() const

  Returns the name of this object.

  \sa setName(). */

/*! Sets the name of this object. \sa name(). */

void QObject::setName( const char *name )	// set object name
{
    if ( objname )
	delete objname;
    objname = name ? strdup(name) : 0;
}


/*! This virtual function receives events to an object and must
  returns TRUE if the event was recognized and processed.

  The event() function can be reimplemented to customize the behavior
  of an object. \sa QWidget::event(), installEventFilter(). */

bool QObject::event( QEvent *e )		// receive event
{
    return activate_filters( e );
}

/*!
Filters events if an event filter has been installed.

\sa installEventFilter().
*/

bool QObject::eventFilter( QObject *, QEvent * )// filter event
{
    return FALSE;				// don't do anything with it
}


bool QObject::activate_filters( QEvent *e )	// activate event filters
{
    register QObject *obj = eventFilters ? eventFilters->first() : 0;
    bool stop = FALSE;
    while ( obj ) {				// send to all filters
	stop = obj->eventFilter( this, e );	//   until one returns TRUE
	if ( stop )
	    break;
	obj = eventFilters->next();
    }
    return stop;				// don't do anything with it
}

void QObject::blockSignals( bool b )
{
    blockSig = b;
}


//
// The timer flag hasTimer is set when startTimer is called.
// It is not reset when killing the timer because more than
// one timer might be active.
//

/*! Starts a timer.  A \link QTimerEvent timer event \endlink will
  happen every \e interval milliseconds until killTimer() is called.

  If \e interval is 0, the timer event happens as often as possible.

  The return value from startTimer() may be passed to killTimer() to
  kill this timer.

  \sa QTimerEvent, QWidget::timerEvent(), killTimer(), killTimers(). */
int QObject::startTimer( long interval )	// start timer events
{
    pendTimer = TRUE;				// set timer flag
    return qStartTimer( interval, (QObject *)this );
}

/*! Kills timer \e id, which is the return value from the startTimer()
  call which created the timer.

  \sa startTimer, QTimerEvent, QWidget::timerEvent(), killTimers(). */

void QObject::killTimer( int id )		// kill timer events
{
    qKillTimer( id );
}

/*! Kill all timers associated with this object.

  \sa killTimer(), startTimer(), QWidget::timerEvent(), QTimerEvent. */

void QObject::killTimers()			// kill all timers for object
{
    qKillTimer( this );
}


QConnectionList *QObject::receivers( const char *signal ) const
{						// get receiver
    if ( connections && signal ) {
	if ( *signal == '2' ) {
	    QString s = rmWS( signal+1 );
	    return connections->find( s );	    
	}
	else
	    return connections->find( signal );
    }
    return 0;
}


void QObject::insertChild( QObject *obj )	// add object object
{
    if ( !childObjects ) {
	childObjects = new QObjectList;
	CHECK_PTR( childObjects );
    }
#if defined(CHECK_STATE)
    else if ( childObjects->findRef(obj) >= 0 ) {
	warning( "QObject::insertChild: Object %s::%s already in list",
		 obj->className(), obj->name() );
	return;
    }
#endif
    obj->parentObj = this;
    if ( obj->hiPriority )
	childObjects->insert( obj );		// high priority inserts
    else
	childObjects->append( obj );		// normal priority appends
}

void QObject::removeChild( QObject *obj )	// remove child object
{
    if ( childObjects && childObjects->findRef(obj) >= 0 ) {
	childObjects->remove();			// remove object from list
	if ( childObjects->isEmpty() ) {	// list becomes empty
	    delete childObjects;
	    childObjects = 0;			// reset children list
	}
    }
}

/*! Adds an event filter to the list. \sa removeEventFilter(),
  eventFilter(), event(). */

void QObject::installEventFilter( const QObject *obj )
{						// add event filter object
    if ( !eventFilters ) {
	eventFilters = new QObjectList;
	CHECK_PTR( eventFilters );
    }
    eventFilters->insert( obj );
}

/*! Removes an event filter from the list. \sa installEventFilter(),
  eventFilter(), event(). */

void QObject::removeEventFilter( const QObject *obj )
{						// remove event filter object
    if ( eventFilters && eventFilters->findRef(obj) >= 0 ) {
	eventFilters->remove();			// remove object from list
	if ( eventFilters->isEmpty() ) {
	    delete eventFilters;
	    eventFilters = 0;			// reset event filter list
	}
    }
}


// ---------------------------------------------------------------------------
// Signal connection management
//

#if defined(CHECK_RANGE)

static bool check_signal_macro( QObject *sender, const char *signal,
				const char *func, const char *op )
{
    int sigcode = (int)(*signal) - '0';
    if ( sigcode != SIGNAL_CODE ) {
	if ( sigcode == SLOT_CODE )
	    warning( "QObject::%s: Attempt to %s non-signal %s::%s",
		     func, op, sender->className(), signal+1 );
	else
	    warning( "QObject::%s: Use the SIGNAL macro to %s %s::%s",
		     func, op, sender->className(), signal );
	return FALSE;
    }
    return TRUE;
}

static bool check_member_code( int code, QObject *object, const char *member,
			       const char *func )
{
    if ( code != SLOT_CODE && code != SIGNAL_CODE ) {
	warning( "QObject::%s: Use the SLOT or SIGNAL macro to "
		 "%s %s::%s", func, func, object->className(), member );
	return FALSE;
    }
    return TRUE;
}

static void err_member_notfound( int code, QObject *object, const char *member,
				 const char *func )
{
    const char *type = 0;
    switch ( code ) {
	case SLOT_CODE:   type = "slot";   break;
	case SIGNAL_CODE: type = "signal"; break;
    }
    if ( strchr(member,')') == 0 )		// common typing mistake
	warning( "QObject::%s: Parentheses expected, %s %s::%s",
		 func, type, object->className(), member );
    else
	warning( "QObject::%s: No such %s %s::%s",
		 func, type, object->className(), member );
}

#endif // CHECK_RANGE


bool QObject::connect( QObject *sender,         const char *signal,
		       const QObject *receiver, const char *member )
{
#if defined(CHECK_NULL)
    if ( sender == 0 || receiver == 0 || signal == 0 || member == 0 ) {
	warning( "QObject::connect: Unexpected NULL parameter" );
	return FALSE;
    }
#endif
    QString signal_tmp( strlen(signal)+1 );
    QString member_tmp( strlen(member)+1 );
    signal_tmp = rmWS( signal );		// strip white space
    signal = signal_tmp;
    member_tmp = rmWS( member );
    member = member_tmp;

    QMetaObject *smeta = sender->queryMetaObject();
    if ( !smeta )				// no meta object
	return FALSE;

#if defined(CHECK_RANGE)
    if ( !check_signal_macro( sender, signal, "connect", "bind" ) )
	return FALSE;
#endif
    signal++;					// skip member type code
    QMetaData *sm;
    if ( !(sm=smeta->signal(signal,TRUE)) ) {	// no such signal
#if defined(CHECK_RANGE)
	err_member_notfound( SIGNAL_CODE, sender, signal, "connect" );
#endif
        return FALSE;
    }
    signal = sm->name;				// use name from meta object

    int membcode = member[0] - '0';		// get member code
    QObject *r = (QObject *)receiver;		// set receiver object
#if defined(CHECK_RANGE)
    if ( !check_member_code( membcode, r, member, "connect" ) )
	return FALSE;
#endif
    member++;					// skip code
    QMetaData *rm = 0;
    QMetaObject *rmeta = r->queryMetaObject();
    if ( !rmeta )				// no meta object
	return FALSE;
    switch ( membcode ) {			// get receiver member
	case SLOT_CODE:	  rm = rmeta->slot( member, TRUE );   break;
	case SIGNAL_CODE: rm = rmeta->signal( member, TRUE ); break;
    }
    if ( !rm ) {				// no such member
#if defined(CHECK_RANGE)
	err_member_notfound( membcode, r, member, "connect" );
#endif
	return FALSE;
    }
#if defined(CHECK_RANGE)
    const char *s1 = signal;			// check if compatible args
    const char *s2 = member;
    while ( *s1++ != '(' ) ;
    while ( *s2++ != '(' ) ;
    if ( !(*s2 == ')' || strcmp(s1,s2) == 0) ) {
	int s1len = strlen(s1);
	int s2len = strlen(s2);
	if ( !(s2len < s1len && !strncmp(s1,s2,s2len-1) && s1[s2len-1]==',') )
	    warning( "QObject::connect: Incompatible sender/receiver arguments"
		     "\n\t%s::%s --> %s::%s",
		     sender->className(), signal,
		     r->className(), member );
    }
#endif
    if ( !sender->connections ) {		// create connections dict
	sender->connections = new QSignalDict( 7, TRUE, FALSE );
	CHECK_PTR( sender->connections );
	sender->connections->setAutoDelete( TRUE );
    }
    QConnectionList *clist = sender->connections->find( signal );
    if ( !clist ) {				// create receiver list
	clist = new QConnectionList;
	CHECK_PTR( clist );
	clist->setAutoDelete( TRUE );
	sender->connections->insert( signal, clist );
    }
    clist->append( new QConnection( r, rm->ptr, rm->name ) );
    if ( !r->senderObjects ) {			// create list of senders
	r->senderObjects = new QObjectList;
	CHECK_PTR( r->senderObjects );
    }
    r->senderObjects->append( sender );		// add sender to list
    return TRUE;
}


/*! Disconnects \e signal in object \e sender from \e member in object
  \e receiver.

  If \e signal is null, disconnect() breaks any connections that pass
  the other tests.

  If \e member is null, disconnect() breaks any connections that pass
  the other tests.

  If \e receiver is null, disconnect() breaks any connections that pass
  the other tests.

  \sa connect(). */

bool QObject::disconnect( QObject *sender, const char *signal,
			  const QObject *receiver, const char *member )
{
#if defined(CHECK_NULL)
    if ( sender == 0 || (receiver == 0 && member != 0) ) {
	warning( "QObject::disconnect: Unexpected NULL parameter" );
	return FALSE;
    }
#endif
    if ( !sender->connections )			// no connected signals
	return FALSE;
    QString signal_tmp;
    QString member_tmp;
    QMetaData *rm = 0;
    QObject *r = (QObject *)receiver;
    if ( member ) {
	member_tmp.resize( strlen(member)+1 );
	member_tmp = rmWS( member );
	member = member_tmp.data();
	int membcode = member[0] - '0';
#if defined(CHECK_RANGE)
	if ( !check_member_code( membcode, r, member, "disconnect" ) )
	    return FALSE;
#endif
	member++;
	QMetaObject *rmeta = r->queryMetaObject();
	if ( !rmeta )				// no meta object
	    return FALSE;
	switch ( membcode ) {			// get receiver member
	    case SLOT_CODE:   rm = rmeta->slot( member, TRUE );   break;
	    case SIGNAL_CODE: rm = rmeta->signal( member, TRUE ); break;
	}
	if ( !rm ) {				// no such member
#if defined(CHECK_RANGE)
	    err_member_notfound( membcode, r, member, "disconnect" );
#endif
	    return FALSE;
	}
    }

    QConnectionList *clist;
    register QConnection *c;
    if ( signal == 0 ) {			// any/all signals
	QSignalDictIt it(*(sender->connections));
	while ( (clist=it.current()) ) {	// for all signals...
	    const char *curkey = it.currentKey();
	    ++it;
	    c = clist->first();
	    while ( c ) {			// for all receivers...
		if ( r == 0 ) {			// remove all receivers
		    removeObjFromList( c->object()->senderObjects, sender );
		    c = clist->next();
		}
		else if ( r == c->object() && (member == 0 ||
					       strcmp(member,c->memberName()) == 0) ) {
		    removeObjFromList( c->object()->senderObjects, sender );
		    clist->remove();
		    c = clist->current();
		}
		else
		    c = clist->next();
	    }
	    if ( r == 0 )			// disconnect all receivers
		sender->connections->remove( curkey );
	}
    }

    else {					// specific signal
	signal_tmp.resize( strlen(signal)+1 );
	signal_tmp = rmWS( signal );
	signal = signal_tmp.data();
#if defined(CHECK_RANGE)
        if ( !check_signal_macro( sender, signal, "disconnect", "unbind" ) )
	    return FALSE;
#endif
	signal++;
	clist = sender->connections->find( signal );
	if ( !clist ) {
#if defined(CHECK_RANGE)
	    QMetaObject *smeta = sender->queryMetaObject();
	    if ( !smeta )			// no meta object
		return FALSE;
	    if ( !smeta->signal(signal,TRUE) )
		warning( "QObject::disconnect: No such signal %s::%s",
			 sender->className(), signal );
#endif
	    return FALSE;
	}
	c = clist->first();
	while ( c ) {				// for all receivers...
	    if ( r == 0 ) {			// remove all receivers
		removeObjFromList( c->object()->senderObjects, sender, TRUE );
		c = clist->next();
	    }
	    else if ( r == c->object() && (member == 0 ||
					   strcmp(member,c->memberName()) == 0) ) {
		removeObjFromList( c->object()->senderObjects, sender, TRUE );
		clist->remove();
		c = clist->current();
	    }
	    else
		c = clist->next();
	}
	if ( r == 0 )				// disconnect all receivers
	    sender->connections->remove( signal );
    }
    return TRUE;
}


QMetaObject *QObject::queryMetaObject() const	// get meta object
{
    register QObject *x = (QObject *)this;	// fake const
    QMetaObject *m = x->metaObject();
    if ( !m ) {					// not meta object
	x->initMetaObject();			//   then try to create it
	m = x->metaObject();
    }
#if defined(CHECK_NULL)
    if ( !m )					// still no meta object: error
	warning( "QObject: Object %s::%s has no meta object",
		 x->className(), x->name() );
#endif
    return m;
}


void QObject::initMetaObject()			// initialize meta object
{
    metaObj = new QMetaObject( "QObject", "", 0, 0, 0, 0 );
}


// ---------------------------------------------------------------------------
// Signal activation with the most frequently used parameter/argument types.
// All other combinations are generated by the meta object compiler.
//

void QObject::activate_signal( const char *signal )
{
    QConnectionList *clist;
    if ( !connections || signalsBlocked() ||
	 !(clist=connections->find(signal)) )
	return;
    typedef void (QObject::*RT)();
    typedef RT *PRT;
    QConnectionListIt it(*clist);
    RT r;
    register QConnection *c;
    register QObject *object;
    while ( (c=it.current()) ) {
	++it;
	r = *((PRT)(c->member()));
	object = c->object();
	object->sigSender = this;
	(object->*r)();
	object->sigSender = 0;
    }
}

#define ACTIVATE_SIGNAL_WITH_PARAM(TYPE)				      \
void QObject::activate_signal( const char *signal, TYPE param )		      \
{									      \
    QConnectionList *clist;						      \
    if ( !connections || signalsBlocked() ||				      \
	 !(clist=connections->find(signal)) )				      \
	return;								      \
    typedef void (QObject::*RT)( TYPE );				      \
    typedef RT *PRT;							      \
    QConnectionListIt it(*clist);					      \
    RT r;								      \
    register QConnection *c;						      \
    register QObject *object;						      \
    while ( (c=it.current()) ) {					      \
	++it;								      \
	r = *((PRT)(c->member()));					      \
	object = c->object();						      \
	object->sigSender = this;					      \
	(object->*r)( param );						      \
	object->sigSender = 0;						      \
    }									      \
}

// We don't want to duplicate too much text so...

ACTIVATE_SIGNAL_WITH_PARAM( short )
ACTIVATE_SIGNAL_WITH_PARAM( int )
ACTIVATE_SIGNAL_WITH_PARAM( long )
ACTIVATE_SIGNAL_WITH_PARAM( const char * )


// ---------------------------------------------------------------------------
// QObject debugging output routines; to be removed before real version 1.0.
//

static void dumpRecursive( int level, QObject *object )
{
#if defined(DEBUG)
    if ( object ) {
	QString buf;
	buf.fill( '\t', level );
	const char *name = object->name() ? object->name() : "????";
	debug( "%s%s::%s", (char*)buf, object->className(), name );
	if ( object->children() ) {
	    QObjectListIt it(*object->children());
	    while ( it ) {
		dumpRecursive( level+1, it );
		++it;
	    }
	}
    }
#endif
}

void QObject::dumpObjectTree()
{
    dumpRecursive( 0, this );
}

void QObject::dumpObjectInfo()
{
#if defined(DEBUG)
    debug( "OBJECT %s::%s", className(), name() );
    debug( "  SIGNALS OUT" );
    int n = 0;
    if ( connections ) {
	QSignalDictIt it(*connections);
	QConnectionList *clist;
	while ( (clist=it.current()) ) {
	    debug( "\t%s", it.currentKey() );
	    n++;
	    ++it;
	    register QConnection *c;
	    QConnectionListIt cit(*clist);
	    while ( (c=cit.current()) ) {
		++cit;
		debug( "\t  --> %s::%s %s", c->object()->className(),
		       c->object()->name(), c->memberName() );
	    }
	}
    }
    if ( n == 0 )
	debug( "\t<None>" );
    debug( "  SIGNALS IN" );
    n = 0;
    if ( senderObjects ) {
	QObject *sender = senderObjects->first();
	while ( sender ) {
	    debug( "\t%s::%s", sender->className(), sender->name() );
	    n++;
	    sender = senderObjects->next();
	}
    }
    if ( n == 0 )
	debug( "\t<None>" );
#endif
}

/*! \page metaobjects.html

<title>
Qt toolkit - Meta Object Compiler description
</title></head><body>

<h1>Signals, slots and the Meta Object Compiler</h1>

Signals and slots are used for communication between objects.  The
signal/slot mechanism is a central feature of Qt, and is implemented
using the <dfn>moc</dfn> (Meta Object Compiler) and some preprocessor
defines.

<p>


All classes that contain signals and/or slots must inherit from
QObject or one of its subclasses, and must mention Q_OBJECT in its
declaration.

<h2>Usage</h2>

Syntactically, signals and slots are categories.  A minimal
C++ class declaration might read:

<pre>
    class Foo : public Bar
    {
    public:
        Foo();
        void setSomethig(int);
	int something();
    private:
        int internal;
    };
</pre>

This little class has an internal state and public methods to access
that state.  A small Qt class might read:

<pre>
    class QFoo : public QObject
    {
        Q_OBJECT;
    public:
        Foo( QObject *parent=0, const char *name=0);
        int something() { returns internal; }
    signals:
        void somethingChanged(int)
    public slots:
        void setSomething(int);
    private:
        int internal;
    };
</pre>

This class has the same internal state, and also public methods to
access the state, but in addition it has some support for component
programming using signals and slots: This class can tell someone that
its state has changed by emitting a signal,
<code>somethingChanged()</code>, and it has a slot which other objects
may send signals to.

<p>

To emit a signal, you say <code>emit signal(arguments)</code>.  The
next code fragment shows this.

<p>

Slots are implemented by the application programmer (that's you).
Here is a possible implementation of QFoo::setSomething():

<pre>
    void QFoo::setSomething(int s) {
	if (s != internal) {
	    internal = s;
	    emit somethingChanged(s);
	}
    }
</pre>

The example may appear useless, but anyway, here is one way to connect
two of these already useless objects together:

<pre>
    QFoo yo, go;

    connect(&yo, SIGNAL(somethingChanged(int)), &go, SLOT(setSomething(int)));
</pre>

Then a call to <code>yo.setSomething()</code> will make yo emit a
signal, which go will receive and act on.  Since this action changes
go's internal state, it too emits a signal, which nobody receives, so
it disappears into hyperspace.

<p>

In this way two objects can work together without knowing each other,
as long as there is someone around to set up a connection between
them initially.

<p>

The preprocessor changes or removes the <code>signals,</code>
<code>slots</code> and <code>emit</code> keywords so the compiler
won't see anything it can't digest.

<p>

Each meta object requires one additional Makefile rule per class and
one additional C++ source file (generated by the meta object
compiler).  Inside Qt, we have chosen to name the meta-source files
m*.cpp, the * is derived from the header file name.  So for QLCDNumber
we have one header file, <code>qlcdnum.h</code>, one moc-generated C++
file, <code>mlcdnum.cpp</code> and one real source file,
<code>qlcdnum.cpp</code>.  <code>mlcdnum.cpp</code> is generated by
this rule:

<pre>
    mlcdnum.cpp: qlcdnum.h
    	..somewhere../bin/moc qlcdnum.h -o mlcdnum.cpp
</pre>

Both C++ files are compiled and linked in the usual way.

<h2>Signals</h2>

Signals are emitted by an object when its internal state has changed
in some way that might be interesting to the object's client or owner.
Only the class that defines a signal and its subclasses can emit the
signal.

<p>

A list box, for instance, emits both <code>highlighted()</code> and
<code>activated()</code> signals.  Most object will probably only be
interested in <code>activated()</code> but some may want to know about
which item in the list box is currently highlighted.  If the signal is
interesting to two different objects you just connect the signal to
slots in both widgets.

<p>

Signals are automatically implemented by the moc and must not be
implemented in the .cpp file.  They can never have return types.

<h2>Slots</h2>

A slot is called when a signal connected to it is emitted.  Slots are
normal C++ functions and can be called normally; their only special
feature is that signals can be connected to them.  Since slots are
normal member functions with just a little extra spice, they have
access rights like everyone else.  A slot's access right determines
who can connect to it.

<p>

A <code>public slots:</code> section contains slots that anyone can connect
signals too.  This is very useful for component programming: You
create objects that know nothing about each other, connect their
signals and slots so information is passed correctly, and, like a
model railway, turn it on and leave it running.

<p>

A <code>protected slots:</code> section contains slots that this class
and its subclasses may connect signals too.  This is intended for
slots that are part of the class' implementation rather than its
interface towards the rest of the world.

<p>

A <code>private slots:</code> section contains slots that only the
class itself may connect signals too.  This is intended for very
tightly connected classes, where even subclasses aren't trusted to get
the connections right.

<p>

Of course, you can also define slots to be virtual.  We have found
this to be very useful.

<p>

Signals and slots are fairly efficient.  Of course there's some loss
of speed compared to "real" callbacks due to the increased
flexibility, but the loss is fairly small, we measured it to
approximately 50 microseconds on a SPARC 2, so the simplicity and
flexibility the mechanism affords is well worth it.

<h2>The Qt Meta Object Compiler</h2>

The meta object compiler (moc) parses a C++ header file and generates
C++ code that initializes the meta object. The meta object contains
names of all signal and slot members, as well as pointers to these
functions.

<h2>Example</h2>

Here is a simple commented example (butchered from qlcddum.h).  Unlike
most of the Qt documentation, this isn't peppered with links, so if
you want to read about <a href=qlcdnum.html>QLCDNumber</a> or any of
its parent classes please do it now, before you go through the
example.

<pre>
    #include "qframe.h"
    #include "qbitarry.h"

    class QLCDNumber : public QFrame
</pre>

QLCDNumber inherits QObject, which has most of the signal/slot
knowledge, via QFrame and QWidget, and #include's the relevant
declarations.

<pre>
    {
        Q_OBJECT
</pre>

Q_OBJECT is expanded by the preprocessor to declare several member
functions that are implemented by the moc; if you get compiler errors
along the lines of "virtual function QButton::className not defined"
you have probably forgotten to mention Q_OBJECT and run the moc.

<pre>
    public:
        QLCDNumber( QWidget *parent=0, const char *name=0 );
        QLCDNumber( uint numDigits, QWidget *parent=0, const char *name=0 );
</pre>

It's not obviously relevant to the moc, but if you inherit QWidget you
almost certainly want to have <em>parent</em> and <em>name</em>
arguments to your constructors, and pass them to the parent
constructor.

<p>

Some destructors and member functions are omitted here, the moc
ignores member functions.

<pre>
    signals:
        void    overflow();
</pre>

QLCDNumber emits a signal when it is asked to show an impossible
value.

<p>

"But I don't care about overflow," or "But I know the number won't
overflow."  Very well, then you don't connect the signal to any slot,
and everything will be fine.

<p>

"But I want to call two different error functions when the number
overflows."  Then you connect the signal to two different slots.  Qt
will call both (in arbitrary order).

<pre>
    public slots:
        void    display( int num );
        void    display( long num );
        void    display( float num );
        void    display( double num );
        void    display( const char *str );
        void    setMode( Mode );
        void    smallDecimalPoint( bool );
</pre>

A slot is a receiving function, used to get information about state
changes in other widgets.  QLCDNumber uses it, as you can see, to set
the displayed number.  Since <code>display()</code> is part of the
class' interface with the rest of the program, the slot is public.

<p>

Several of the example program connect the newValue signal of a
QScrollBar to the display slot, so the LCD number continuously shows
the value of the scroll bar.

<p>

(Note that display() is overloaded; Qt will select the appropriate
version when you connect a signal to the slot.  With callbacks, you'd
have to find five different names and keep track of the types
yourself.)

<p>

Some more irrelevant member functions have been omitted from this
example.

<pre>
    };
</pre>

<h2>Moc output</h2>

This is really internal to Qt, but for the curious, here is the meat
of the resulting mlcdnum.cpp:

<pre>
    const char *QLCDNumber::className() const
    {
        return "QLCDNumber";
    }
    
    QMetaObject *QLCDNumber::metaObj = 0;
    
    void QLCDNumber::initMetaObject()
    {
        if ( metaObj )
    	return;
        if ( !QFrame::metaObject() )
    	QFrame::initMetaObject();
</pre>

That last line is because QLCDNumber inherits QFrame.  The next part,
which sets up the table/signal structures, has been deleted for
brevity.

<pre>
    }
    
    // SIGNAL overflow
    void QLCDNumber::overflow()
    {
        activate_signal( "overflow()" );
    }
</pre>

One function is generated for each signal, and at present it almost
always is a single call to activate_signal(), which finds the
appropriate slot or slots and passes on the call.

<p>

Please don't call activate_signal() directly, we may well move some
code from activate_signal into the generated signal functions before
1.0.  More generally, don't use knowledge of Qt's intestines, or risk
unpredictable results now or later. */

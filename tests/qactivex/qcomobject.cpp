#include "qcomobject.h"

#include <quuid.h>
#include <qmetaobject.h>
#include <private/qucomextra_p.h>
#include <qdict.h>
#include <qptrdict.h>
#include <qstringlist.h>
#include <qvariant.h>
#include <qdatetime.h>
#include <private/qcom_p.h>

#include <atlbase.h>
CComModule _Module;

static QMetaObject *tempMetaObj = 0;

/*! 
    Helper functions 
*/
static inline QString vartypeToQt( VARTYPE vt )
{
    QString str;
    switch ( vt ) {
    case VT_EMPTY:
	// str = "[Empty]";
	break;
    case VT_NULL:
	// str = "[Null]";
	break;
    case VT_I2:
    case VT_I4:
	str = "int";
	break;
    case VT_R4:
    case VT_R8:
	str = "double";
	break;
    case VT_CY:
	str = "long long"; // ### 64bit struct CY { ulong lo, long hi };
	break;
    case VT_DATE:
	str = "QDateTime";
	break;
    case VT_BSTR:
	str = "QString";
	break;
    case VT_DISPATCH:
	str = "IDispatch*";
	break;
    case VT_ERROR:
	str = "long";
	break;
    case VT_BOOL:
	str = "bool";
	break;
    case VT_VARIANT:
	str = "QVariant";
	break;
    case VT_DECIMAL:
	// str = "[DECIMAL]";
	break;
    case VT_RECORD:
	// str = "[Usertype]";
	break;
    case VT_UNKNOWN:
	str = "IUnknown*";
	break;
    case VT_I1:
	str = "char";
	break;
    case VT_UI1:
	str = "unsigned char";
	break;
    case VT_UI2:
	str = "unsigned short";
	break;
    case VT_UI4:
	str = "unsigned int";
	break;
    case VT_INT:
	str = "int";
	break;
    case VT_UINT:
	str = "unsigned int";
	break;
    case VT_VOID:
	str = "void";
	break;
    case VT_HRESULT:
	str = "long";
	break;

    case VT_PTR:
	// str = "[Pointer]";
	break;
    case VT_SAFEARRAY:
	// str = "VT_ARRAY";
	break;
    case VT_CARRAY:
	// str = "[C array]";
	break;
    case VT_USERDEFINED: // most USERDEFINED types are actually long or int wrappers
	str = "int";
	break;
    case VT_LPSTR:
	str = "const char*";
	break;
    case VT_LPWSTR:
	str = "const unsigned short*";
	break;

    case VT_FILETIME:
	// str = "[FILETIME]";
	break;
    case VT_BLOB:
	// str = "[Blob]";
	break;
    case VT_STREAM:
	// str = "[Stream]";
	break;
    case VT_STORAGE:
	// str = "[Storage]";
	break;
    case VT_STREAMED_OBJECT:
	// str = "[Streamed object]";
	break;
    case VT_STORED_OBJECT:
	// str = "[Stored object]";
	break;
    case VT_BLOB_OBJECT:
	// str = "[Blob object]";
	break;
    case VT_CF:
	// str = "[Clipboard]";
	break;
    case VT_CLSID:
	// str = "GUID";
	break;
    case VT_VECTOR:
	// str = "[Vector]";
	break;

    case VT_ARRAY:
	// str = "SAFEARRAY*";
	break;
    case VT_RESERVED:
	// str = "[Reserved]";
	break;

    default:
	// str = "[Unknown]";
	break;
    }

    if ( vt & VT_BYREF )
	str += "*";

    return str;
}

static inline QString typedescToQString( TYPEDESC typedesc )
{
    QString ptype;

    VARTYPE vt = typedesc.vt;
    if ( vt == VT_PTR ) {
	vt = typedesc.lptdesc->vt;
	ptype = vartypeToQt( vt );
	if ( !!ptype ) 
	    ptype += "*";
    } else if ( vt == VT_SAFEARRAY ) {
	vt = typedesc.lpadesc->tdescElem.vt;
	ptype = vartypeToQt( vt );
	if ( !!ptype ) 
	    ptype = ptype + "[" + QString::number( typedesc.lpadesc->cDims ) + "]";
    } else {
	ptype = vartypeToQt( vt );
    }
    if ( ptype.isEmpty() )
	ptype = "UNSUPPORTED";
    return ptype;
}

static inline bool checkHRESULT( HRESULT hres )
{
    switch( hres ) {
    case S_OK:
	return TRUE;
    case DISP_E_BADPARAMCOUNT:
	return FALSE;
    case DISP_E_BADVARTYPE:
	return FALSE;
    case DISP_E_EXCEPTION:
	return FALSE;
    case DISP_E_MEMBERNOTFOUND:
	return FALSE;
    case DISP_E_NONAMEDARGS:
	return FALSE;
    case DISP_E_OVERFLOW:
	return FALSE;
    case DISP_E_PARAMNOTFOUND:
	return FALSE;
    case DISP_E_TYPEMISMATCH:
	return FALSE;
    case DISP_E_UNKNOWNINTERFACE:
	return FALSE;
    case DISP_E_UNKNOWNLCID:
	return FALSE;
    case DISP_E_PARAMNOTOPTIONAL:
	return FALSE;
    default:
	return FALSE;
    }
}


// those functions are not ours
static int monthdays[13] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
#define HALF_SECOND  (1.0/172800.0)

#include <math.h>

static inline QDateTime DATEToQDateTime( DATE ole )
{
    int year, month, day, wday, yday;
    int hour, min, sec;

    long nDays;             // Number of days since Dec. 30, 1899
    long nDaysAbsolute;     // Number of days since 1/1/0
    long nSecsInDay;        // Time in seconds since midnight
    long nMinutesInDay;     // Minutes in day
    
    long n400Years;         // Number of 400 year increments since 1/1/0
    long n400Century;       // Century within 400 year block (0,1,2 or 3)
    long n4Years;           // Number of 4 year increments since 1/1/0
    long n4Day;             // Day within 4 year block
    //  (0 is 1/1/yr1, 1460 is 12/31/yr4)
    long n4Yr;              // Year within 4 year block (0,1,2 or 3)
    BOOL bLeap4 = TRUE;     // TRUE if 4 year block includes leap year
    
    double dblDate = ole; // tempory serial date
    
    // If a valid date, then this conversion should not overflow
    nDays = (long)dblDate;
    
    // Round to the second
    dblDate += ((ole > 0.0) ? HALF_SECOND : -HALF_SECOND);
    
    nDaysAbsolute = (long)dblDate + 693959L; // Add days from 1/1/0 to 12/30/1899
    
    dblDate = fabs(dblDate);
    nSecsInDay = (long)((dblDate - floor(dblDate)) * 86400.);
    
    // Calculate the day of week (sun=1, mon=2...)
    //   -1 because 1/1/0 is Sat.  +1 because we want 1-based
    wday = (int)((nDaysAbsolute - 1) % 7L) + 1;
    
    // Leap years every 4 yrs except centuries not multiples of 400.
    n400Years = (long)(nDaysAbsolute / 146097L);
    
    // Set nDaysAbsolute to day within 400-year block
    nDaysAbsolute %= 146097L;
    
    // -1 because first century has extra day
    n400Century = (long)((nDaysAbsolute - 1) / 36524L);
    
    // Non-leap century
    if (n400Century != 0) {
	// Set nDaysAbsolute to day within century
	nDaysAbsolute = (nDaysAbsolute - 1) % 36524L;
	
	// +1 because 1st 4 year increment has 1460 days
	n4Years = (long)((nDaysAbsolute + 1) / 1461L);
	
	if (n4Years != 0) {
	    n4Day = (long)((nDaysAbsolute + 1) % 1461L);
	} else {
	    bLeap4 = FALSE;
	    n4Day = (long)nDaysAbsolute;
	}
    } else {
	// Leap century - not special case!
	n4Years = (long)(nDaysAbsolute / 1461L);
	n4Day = (long)(nDaysAbsolute % 1461L);
    }
    
    if (bLeap4) {
	// -1 because first year has 366 days
	n4Yr = (n4Day - 1) / 365;
	
	if (n4Yr != 0)
	    n4Day = (n4Day - 1) % 365;
    } else {
	n4Yr = n4Day / 365;
	n4Day %= 365;
    }
    
    // n4Day is now 0-based day of year. Save 1-based day of year, year number
    yday = (int)n4Day + 1;
    year = n400Years * 400 + n400Century * 100 + n4Years * 4 + n4Yr;
    
    // Handle leap year: before, on, and after Feb. 29.
    if (n4Yr == 0 && bLeap4) {
	// Leap Year
	if (n4Day == 59) {
	    /* Feb. 29 */
	    month = 2;
	    day = 29;
	    goto DoTime;
	}
	
	// Pretend it's not a leap year for month/day comp.
	if (n4Day >= 60)
	    --n4Day;
    }
    
    // Make n4DaY a 1-based day of non-leap year and compute
    //  month/day for everything but Feb. 29.
    ++n4Day;
    
    // Month number always >= n/32, so save some loop time */
    for ( month = (n4Day >> 5) + 1; n4Day > monthdays[month]; month++ )
	;
    
    day = (int)(n4Day - monthdays[month-1]);
    
DoTime:
    if (nSecsInDay == 0) {
	hour = min = sec = 0;
    } else {
	sec = (int)nSecsInDay % 60L;
	nMinutesInDay = nSecsInDay / 60L;
	min = (int)nMinutesInDay % 60;
	hour = (int)nMinutesInDay / 60;
    }

    QDateTime dt;
    dt.setDate( QDate( year, month, day ) );
    dt.setTime( QTime( hour, min, sec ) );
    return dt;
}

static inline DATE QDateTimeToDATE( const QDateTime &dt )
{
    QDate date = dt.date();
    QTime time = dt.time();
    int year = date.year();
    int month = date.month();
    int day = date.day();
    int hour = time.hour();
    int min = time.minute();
    int sec = time.second();
    int dim = date.daysInMonth();
    bool leap = date.leapYear( year );

    long oledate = year*365 + year/4 - year/100 + year/400 + monthdays[month-1] + day;
    if ( month <= 2 && leap )
	--oledate;
    oledate -= 693959;

    double oletime = (((long)hour * 3600L) + ((long)min * 60L) + ((long)sec)) / 86400.;

    DATE ole = (double) oledate + ( ( oledate >= 0 ) ? oletime : -oletime );
    return ole;
}

static inline QString BSTRToQString( BSTR bstr )
{
    QString str;
    if ( !bstr )
	return str;

    int len = wcslen( bstr );
    str.setUnicode( (QChar*)bstr, len );
    return str;
}

BSTR QStringToBSTR( const QString &str )
{
    BSTR bstrVal;

    int wlen = str.length();
    bstrVal = SysAllocStringByteLen( 0, wlen*2 );
    memcpy( bstrVal, str.unicode(), sizeof(QChar)*(wlen) );
    bstrVal[wlen] = 0;

    return bstrVal;
}

static inline QString constRefify( const QString& type )
{
    QString crtype;

    if ( type == "QString" )
	crtype = "const QString&";
    else if ( type == "QDateTime" )
	crtype = "const QDateTime&";
    else if ( type == "QVariant" )
	crtype = "const QVariant&";
    else 
	crtype = type;

    return crtype;
}

static inline void QStringToQUType( const QString& type, QUParameter *param )
{
    param->typeExtra = 0;
    if ( type == "int" || type == "long" ) {
	param->type = &static_QUType_int;
    } else if ( type == "bool" ) {
	param->type = &static_QUType_bool;
    } else if ( type == "QString" || type == "const QString&" ) {
	param->type = &static_QUType_QString;
    } else if ( type == "double" ) {
	param->type = &static_QUType_double;
    } else if ( type == "QVariant" || type == "const QVariant&" ) {
	param->type = &static_QUType_QVariant;
    } else if ( type == "IUnknown*" ) {
	param->type = &static_QUType_iface;
	param->typeExtra = "QUnknownInterface";
    } else if ( type == "IDispatch*" ) {
	param->type = &static_QUType_idisp;
	param->typeExtra = "QDispatchInterface";
    } else {
	param->type = &static_QUType_ptr;
	QString ptype = type;
	if ( ptype.right(1) == "*" )
	    ptype.remove( ptype.length()-1, 1 );
	param->typeExtra = new char[ ptype.length() + 1 ];
	param->typeExtra = qstrcpy( (char*)param->typeExtra, ptype );
    }
}

static inline void VARIANTToQUObject( VARIANT arg, QUObject *obj )
{
    if ( arg.vt & VT_BYREF ) {
	VARTYPE vt2 = arg.vt & ~VT_BYREF;
	switch ( vt2 ) {
	case VT_UI1:
	    static_QUType_ptr.set( obj, arg.pbVal );
	    break;
	case VT_I2:
	    static_QUType_ptr.set( obj, arg.piVal );
	    break;
	case VT_I4:
	    static_QUType_ptr.set( obj, arg.plVal );
	    break;
	case VT_ERROR:
	    static_QUType_ptr.set( obj, arg.pscode );
	    break;
	case VT_R4:
	    static_QUType_ptr.set( obj, arg.pfltVal );
	    break;
	case VT_R8:
	    static_QUType_ptr.set( obj, arg.pdblVal );
	    break;
	case VT_DATE:
	    static_QUType_ptr.set( obj, new QDateTime( DATEToQDateTime( *arg.pdate ) ) );
	    break;
	case VT_BOOL:
	    static_QUType_ptr.set( obj, arg.pboolVal );
	    break;
	case VT_BSTR:
	    static_QUType_ptr.set( obj, BSTRToQString( *arg.pbstrVal ) );
	    break;
	case VT_UNKNOWN:
	    static_QUType_ptr.set( obj, *arg.ppunkVal );
	    break;
	case VT_DISPATCH:
	    static_QUType_ptr.set( obj, *arg.ppdispVal );
	    break;
	default:
	    break;
	}
    } else switch ( arg.vt ) {
    case VT_UI1: // byte -> int
	static_QUType_int.set( obj, arg.bVal );
	break;
    case VT_I2: // short -> int
	static_QUType_int.set( obj, arg.iVal );
	break;
    case VT_I4: // long -> int
	static_QUType_int.set( obj, arg.lVal );
	break;
    case VT_ERROR: // SCODE == long
	static_QUType_int.set( obj, arg.scode );
	break;
    case VT_R4: // float -> double
	static_QUType_double.set( obj, arg.fltVal );
	break;
    case VT_R8: // double -> double
	static_QUType_double.set( obj, arg.dblVal );
	break;
    case VT_CY: // Currency -> ###
	break;
    case VT_DATE: // DATE -> QDateTime
	static_QUType_ptr.set( obj, new QDateTime( DATEToQDateTime( arg.date ) ) );
	break;
    case VT_BOOL: // bool -> bool
	static_QUType_bool.set( obj, arg.boolVal );
	break;
    case VT_BSTR: // bstr -> QString
	static_QUType_QString.set( obj, BSTRToQString( arg.bstrVal ) );
	break;
    case VT_UNKNOWN:  // IUnknown -> void*
	static_QUType_ptr.set( obj, arg.punkVal );
	break;
    case VT_DISPATCH: // IDispatch -> void*
	static_QUType_ptr.set( obj, arg.pdispVal );
	break;
    default:
	break;
    }
}

static inline VARIANT QVariantToVARIANT( const QVariant &var, const char *type = 0 )
{
    VARIANT arg;
    arg.vt = VT_EMPTY;

    switch ( var.type() ) {
    case QVariant::String:
	arg.vt = VT_BSTR;
	arg.bstrVal = QStringToBSTR( var.toString() );
	break;
    case QVariant::Int:
	if( !qstrcmp( type, "bool" ) ) {
	    arg.vt = VT_BOOL;
	    arg.boolVal = var.toBool();
	    break;
	}
	arg.vt = VT_I4;
	arg.lVal = var.toInt();
	break;
    case QVariant::UInt:
	arg.vt = VT_UI4;
	arg.ulVal = var.toUInt();
	break;
    case QVariant::Bool:
	arg.vt = VT_BOOL;
	arg.boolVal = var.toBool();
	break;
    case QVariant::Double:
	arg.vt = VT_R8;
	arg.dblVal = var.toDouble();
	break;
    case QVariant::CString:
	arg.vt = VT_BSTR;
	arg.bstrVal = QStringToBSTR( var.toCString() );
	break;
    case QVariant::Date:
    case QVariant::Time:
    case QVariant::DateTime:
	arg.vt = VT_DATE;
	arg.date = QDateTimeToDATE( var.toDateTime() );
	break;
    default:
	break;
    }

    return arg;
}

static inline void QVariantToQUObject( const QVariant &var, QUObject &obj )
{
    switch ( var.type() ) {
    case QVariant::Invalid:
    case QVariant::Map:
    case QVariant::List:
    break;
    case QVariant::String:
	static_QUType_QString.set( &obj, var.toString() );
	break;
    case QVariant::StringList:
    case QVariant::Font:
    case QVariant::Pixmap:
    case QVariant::Brush:
    case QVariant::Rect:
    case QVariant::Size:
    case QVariant::Color:
    case QVariant::Palette:
    case QVariant::ColorGroup:
    case QVariant::IconSet:
    case QVariant::Point:
    case QVariant::Image:
	break;
    case QVariant::Int:
	static_QUType_int.set( &obj, var.toInt() );
	break;
    case QVariant::UInt:
	static_QUType_int.set( &obj, var.toUInt() );
	break;
    case QVariant::Bool:
	static_QUType_bool.set( &obj, var.toDouble() );
	break;
    case QVariant::Double:
	static_QUType_double.set( &obj, var.toDouble() );
	break;
    case QVariant::CString:
	static_QUType_charstar.set( &obj, var.toCString() );
	break;
    case QVariant::PointArray:
    case QVariant::Region:
    case QVariant::Bitmap:
    case QVariant::Cursor:
    case QVariant::SizePolicy:
	break;
    case QVariant::Date:
	static_QUType_ptr.set( &obj, new QDateTime( var.toDate() ) );
	break;
    case QVariant::Time:
	static_QUType_ptr.set( &obj, new QDateTime( QDate(), var.toTime() ) );
	break;
    case QVariant::DateTime:
	static_QUType_ptr.set( &obj, new QDateTime( var.toDateTime() ) );
	break;
    case QVariant::ByteArray:
    case QVariant::BitArray:
    case QVariant::KeySequence:
    default:
	break;
    }
}

/*!
    \class QAxEventSink qactivex.cpp

    The QAxEventSink class implements the event sink for all
    IConnectionPoints implemented in the COM object.
*/

class QAxEventSink : public IDispatch,
		     public IPropertyNotifySink
{
public:
    QAxEventSink( QComBase *com ) : combase( com ), ref( 1 ) {}
    virtual ~QAxEventSink() {}

    // add a connection
    void addConnection( IConnectionPoint *cpoint, IID iid ) 
    {	
	ULONG cookie;
	cpoint->Advise( (IUnknown*)(IDispatch*)this, &cookie );
	Connection i( cpoint, iid, cookie );
	connections.append( i ); 
	cpoint->AddRef();
    }
    
    // disconnect from all connection points
    void unadvise()
    {
	QValueListIterator<Connection> it = connections.begin();
	while ( it != connections.end() ) {
	    Connection connection = *it;
	    ++it;
	    connection.cpoint->Unadvise( connection.cookie );
	    connection.cpoint->Release();
	    connections.remove( connection );
	}
    }

    void addSignal( DISPID memid, const QString &name )
    {
	sigs.insert( memid, name );
    }
    void addProperty( DISPID propid, const QString &name )
    {
	props.insert( propid, name );
    }

    // IUnknown
    unsigned long __stdcall AddRef()
    {
	return ref++;
    }
    unsigned long __stdcall Release()
    {
	if ( !--ref ) {
	    delete this;
	    return 0;
	}
	return ref;
    }
    HRESULT __stdcall QueryInterface( REFIID riid, void **ppvObject )
    {
	*ppvObject = 0;
	if ( riid == IID_IUnknown)
	    *ppvObject = this;
	else if ( riid == IID_IDispatch )
	    *ppvObject = this;
	else if ( riid == IID_IPropertyNotifySink )
	    *ppvObject = this;
	else if ( connections.contains( Connection(0,riid,0) ) )
	    *ppvObject = this;
	else
	    return E_NOINTERFACE;

	AddRef();
	return S_OK;
    }

    // IDispatch
    HRESULT __stdcall GetTypeInfoCount( unsigned int *count ) { return E_NOTIMPL; }
    HRESULT __stdcall GetTypeInfo( UINT, LCID, ITypeInfo **info ) { return E_NOTIMPL; }
    HRESULT __stdcall GetIDsOfNames( const _GUID &, wchar_t **, unsigned int, unsigned long, long * ) { return E_NOTIMPL; }

    HRESULT __stdcall Invoke( DISPID dispIdMember, 
			      REFIID riid, 
			      LCID lcid, 
			      WORD wFlags, 
			      DISPPARAMS *pDispParams, 
			      VARIANT *pVarResult, 
			      EXCEPINFO *pExcepInfo, 
			      UINT *puArgErr )
    {
	// verify input
	if ( riid != IID_NULL )
	    return DISP_E_UNKNOWNINTERFACE;
	if ( wFlags != DISPATCH_METHOD )
	    return DISP_E_MEMBERNOTFOUND;

	QMetaObject *meta = combase->metaObject();
	QString signame = sigs[dispIdMember];
	if ( !meta || signame.isEmpty() )
	    return DISP_E_MEMBERNOTFOUND;

	// emit the signal "as is"
	int index = meta->findSignal( "signal(const QString&,int,void*)" );
	Q_ASSERT( index != -1 );
	QUObject o[4];
	static_QUType_QString.set(o+1,signame);
	static_QUType_int.set(o+2,pDispParams->cArgs);
	static_QUType_ptr.set(o+3,pDispParams->rgvarg);
	combase->qt_emit( index, o );

	// get the signal information from the metaobject
	index = meta->findSignal( signame );
	const QMetaData *signal = meta->signal( index - meta->signalOffset() );
	if ( !signal )
	    return DISP_E_MEMBERNOTFOUND;

	// verify parameter count
	int pcount = signal->method->count;
	int argcount = pDispParams->cArgs;
	if ( pcount > argcount )
	    return DISP_E_PARAMNOTOPTIONAL;
	else if ( pcount < argcount )
	    return DISP_E_BADPARAMCOUNT;

	// setup parameters
	const QUParameter *params = signal->method->parameters;
	QUObject *objects = pcount ? new QUObject[pcount+1] : 0;
	for ( int p = 0; p < pcount; ++p ) // map the VARIANT to the QUObject
	    VARIANTToQUObject( pDispParams->rgvarg[ pcount-p-1 ], objects + p + 1 );

	// emit the generated signal
	bool ret = combase->qt_emit( index, objects );

	for ( p = 0; p < pcount; ++p ) { // update the VARIANT for references and free memory
	    VARIANT *arg = &(pDispParams->rgvarg[ pcount-p-1 ]);
	    VARTYPE vt = arg->vt;
	    QUObject *obj = objects + p + 1;
	    switch ( vt )
	    {
	    case VT_DATE:
	    case VT_DATE|VT_BYREF:
		{
		    QDateTime *dt = (QDateTime*)static_QUType_ptr.get( obj );
		    if ( vt & VT_BYREF )
			*arg->pdate = QDateTimeToDATE( *dt );
		    delete dt;
		}
		break;
	    case VT_BSTR|VT_BYREF:
		{
		    BSTR *bstr = arg->pbstrVal;
		    QString *str = (QString*)static_QUType_ptr.get( obj );
		    *bstr = QStringToBSTR( *str );
		    delete str;
		}
	    case VT_UNKNOWN|VT_BYREF:
		{
		    IUnknown *iface = (IUnknown*)static_QUType_ptr.get( obj );
		    *arg->ppunkVal = iface;
		}
		break;

	    case VT_DISPATCH|VT_BYREF:
		{
		    IDispatch *iface = (IDispatch*)static_QUType_ptr.get( obj );
		    *arg->ppdispVal = iface;
		}
		break;

	    default:
		break;
	    }
	}
	delete [] objects;
	return ret ? S_OK : DISP_E_MEMBERNOTFOUND;
    }

    // IPropertyNotifySink
    HRESULT __stdcall OnChanged( DISPID dispID )
    {
	QString name = props[dispID];
	if ( !!name )
	    qDebug( "Property %s changed", name.latin1() );

	return S_OK;
    }
    HRESULT __stdcall OnRequestEdit( DISPID /*dispID*/ )
    {
	return S_OK;
    }

private:
    struct Connection
    {
	Connection() {}
	Connection( IConnectionPoint *point, IID i, ULONG c ) 
	    : cpoint( point ), iid(i), cookie(c) 
	{}

	IConnectionPoint *cpoint;
	IID iid;
	ULONG cookie;

	bool operator==( const Connection &other ) const { return iid == other.iid; }
    };
    QValueList<Connection> connections;
    QMap<DISPID, QString> sigs;
    QMap<DISPID, QString> props;

    QComBase *combase;
    long ref;
};



/*!
    \class QComBase qactivex.h
    \brief The QComBase class is an abstract class that provides properties 
	   and slots to initalize and access an COM object.
*/

/*!
    Creates an empty QComBase widget. 
    Use setControl() to instantiate an ActiveX control.
*/
QComBase::QComBase( IUnknown *iface )
: ptr( iface ), eventSink( 0 ), metaobj( 0 )
{
    if ( ptr )
	ptr->AddRef();
}

/*!
    Shuts down the COM object and destroys the QComBase object.

    \sa clear()
*/
QComBase::~QComBase()
{
    clear();
}

/*!
    \property control
    \brief the name of the COM object.
*/
void QComBase::setControl( const QString &c )
{
    if ( c == ctrl )
	return;

    clear();
    ctrl = c;
    if ( !!ctrl )
	initialize( &ptr );
}

QString QComBase::control() const
{
    return ctrl;
}

/*!
    Disconnects and destroys the COM object.

    If you reimplement this function you will also have to reimplement
    the destructor to call clear(), and call this implementation at the
    end of your clear() function.
*/
void QComBase::clear()
{
    if ( eventSink ) {
	eventSink->unadvise();
	eventSink->Release();
	eventSink = 0;
    }

    if ( ptr ) {
	ptr->Release();
	ptr = 0;

	_Module.Term();
	CoUninitialize();
    }

    ctrl = QString(0);

    if ( metaobj ) {
	int i;
	// clean up class info
	for ( i = 0; i < metaobj->numClassInfo(); ++i ) {
	    QClassInfo *info = (QClassInfo*)metaobj->classInfo( i );
	    delete [] (char*)info->name;
	    delete [] (char*)info->value;
	}
	if ( metaobj->numClassInfo() )
	    delete [] (QClassInfo*)metaobj->classInfo( 0 );

	// clean up slot info
	for ( i = 0; i < metaobj->numSlots(); ++i ) {
	    const QMetaData *slot_data = metaobj->slot( i );
	    QUMethod *slot = (QUMethod*)slot_data->method;
	    if ( slot ) {
		delete [] (char*)slot->name;
		for ( int p = 0; p < slot->count; ++p ) {
		    const QUParameter *param = &(slot->parameters[p]);
		    delete [] (char*)param->name;
		    if ( QUType::isEqual( param->type, &static_QUType_ptr ) ) {
			const char *type = (const char*)param->typeExtra;
			delete [] (char*)type;
		    }
		}
		delete [] (QUParameter*)slot->parameters;
		delete slot;
	    }
	    delete [] (char*)slot_data->name;
	}
	if ( metaobj->numSlots() )
	    delete [] (QMetaData*)metaobj->slot( 0 );

	// clean up signal info
	for ( i = 1; i < metaobj->numSignals(); ++i ) { // 0 is the static signal
	    const QMetaData *signal_data = metaobj->signal( i );
	    QUMethod *signal = (QUMethod*)signal_data->method;
	    if ( signal ) {
		delete [] (char*)signal->name;
		for ( int p = 0; p < signal->count; ++p ) {
		    const QUParameter *param = &(signal->parameters[p]);
		    delete [] (char*)param->name;
		    if ( QUType::isEqual( param->type, &static_QUType_ptr ) ) {
			const char *type = (const char*)param->typeExtra;
			delete [] (char*)type;
		    }
		}
		delete [] (QUParameter*)signal->parameters;
		delete signal;
	    }
	    delete [] (char*)signal_data->name;
	}
	if ( metaobj->numSignals() )
	    delete [] (QMetaData*)metaobj->signal( 0 );

	for ( i = 1; i < metaobj->numProperties(); ++i ) {
	    const QMetaProperty *property = metaobj->property( i );
	    delete [] (char*)property->n;
	    delete [] (char*)property->t;
	}
	if ( metaobj->numProperties() )
	    delete [] (QMetaProperty*)metaobj->property( 0 );
    }
    delete metaobj;
    metaobj = 0;
}

/*!
    ###
*/
long QComBase::queryInterface( const QUuid &uuid, void **iface ) const
{
    *iface = 0;
    if ( ptr && !uuid.isNull() )
	return ptr->QueryInterface( uuid, iface );
    
    return E_NOTIMPL;
}

/*!
    \reimp
*/
QMetaObject *QComBase::metaObject() const
{
    if ( metaobj )
	return metaobj;
    QMetaObject* parentObject = parentMetaObject();

    // one signal and one property are always there
    static const QUParameter param_signal_0[] = {
	{ "name", &static_QUType_QString, 0, QUParameter::In },
	{ "argc", &static_QUType_int, 0, QUParameter::In },
	{ "argv", &static_QUType_ptr, "void", QUParameter::In }
    };
    static const QUMethod signal_0 = {"signal", 3, param_signal_0 };
    static const QMetaData signal_tbl[] = {
	{ "signal(const QString&,int,void*)", &signal_0, QMetaData::Public }
    };
    static const QMetaProperty props_tbl[1] = {
 	{ "QString","control", 259, (QMetaObject**)&tempMetaObj, 0, -1 }
    };

    if ( !ptr ) {
	if ( tempMetaObj )
	    return tempMetaObj;

	tempMetaObj = QMetaObject::new_metaobject(
	    "QComBase", parentObject,
	    0, 0,
	    signal_tbl, 1,
#ifndef QT_NO_PROPERTIES
	    props_tbl, 1,
	    0, 0,
#endif // QT_NO_PROPERTIES
	    0, 0 );

	return tempMetaObj;
    }

    // the rest is generated from the IDispatch implementation
    QComBase* that = (QComBase*)this; // mutable

    QDict<QUMethod> slotlist; // QUMethods deleted in ~QActiveX
    QDict<QUMethod< signallist; // QUMethods deleted in ~QActiveX
    QDict<QMetaProperty> proplist;
    proplist.setAutoDelete( TRUE ); // deep copied when creating metaobject
    QDict<QString> infolist; 
    infolist.setAutoDelete( TRUE ); // deep copied when creating metaobject
    QDict<QString> enumlist; 
    enumlist.setAutoDelete( TRUE ); // deep copied when creating metaobject

    // create default signal and slots

    CComPtr<IDispatch> disp;
    ptr->QueryInterface( IID_IDispatch, (void**)&disp );
    if ( disp ) {
	UINT count;
	disp->GetTypeInfoCount( &count );
	// this is either 0 or 1, but anyway...
	if ( count ) {
	    CComPtr<ITypeInfo> info;
	    disp->GetTypeInfo( 0, LOCALE_SYSTEM_DEFAULT, &info );
	    // read type information
	    while ( info ) {
		ushort nFuncs = 0;
		ushort nVars = 0;
		ushort nImpl = 0;
		// get information about type
		TYPEATTR *typeattr;
		info->GetTypeAttr( &typeattr );
		if ( typeattr ) {
		    if ( ( typeattr->typekind != TKIND_DISPATCH && typeattr->typekind != TKIND_INTERFACE ) ||
			 ( typeattr->guid == IID_IDispatch || typeattr->guid == IID_IUnknown ) ) {
			info->ReleaseTypeAttr( typeattr );
			break;
		    }
		    
		    // UUID
		    QUuid uuid( typeattr->guid );
		    infolist.insert( QString("Interface %1").arg(infolist.count()+1), new QString( uuid.toString().upper() ) );

		    // get number of functions, variables, and implemented interfaces
		    nFuncs = typeattr->cFuncs;
		    nVars = typeattr->cVars;
		    nImpl = typeattr->cImplTypes;

		    info->ReleaseTypeAttr( typeattr );
		}

		// get information about all functions
		for ( ushort fd = 0; fd < nFuncs ; ++fd ) {
		    FUNCDESC *funcdesc;
		    info->GetFuncDesc( fd, &funcdesc );
		    if ( !funcdesc )
			break;

		    // get function prototype
		    QString function;
		    QString prototype;
		    QStringList parameters;
		    QStringList paramTypes;

		    // get return value
		    TYPEDESC typedesc = funcdesc->elemdescFunc.tdesc;
		    QString returnType = typedescToQString( typedesc );
		    if ( funcdesc->invkind == INVOKE_FUNC && returnType != "void" ) {
			parameters << "return";
			paramTypes << returnType;
		    }

		    BSTR bstrNames[256];
		    UINT maxNames = 255;
		    UINT maxNamesOut;
		    info->GetNames( funcdesc->memid, (BSTR*)&bstrNames, maxNames, &maxNamesOut );
		    for ( int p = 0; p < (int)maxNamesOut; ++ p ) {
			QString paramName = BSTRToQString( bstrNames[p] );
			SysFreeString( bstrNames[p] );

			// function name
			if ( !p ) {
			    function = paramName;
			    prototype = function + "(";
			    continue;
			}

			// parameter
			bool optional = p > funcdesc->cParams - funcdesc->cParamsOpt;
			QString ptype;
			TYPEDESC tdesc = funcdesc->lprgelemdescParam[p - (funcdesc->invkind == INVOKE_FUNC) ? 1 : 0 ].tdesc;
			ptype = typedescToQString( tdesc );
			if ( funcdesc->invkind == INVOKE_FUNC )
			    ptype = constRefify( ptype );

			prototype += ptype;
			if ( optional )
			    ptype += "=0";
			paramTypes << ptype;
			parameters << paramName;
			if ( p < funcdesc->cParams )
			    prototype += ",";
		    }
		    if ( !!prototype )
			prototype += ")";

		    // get type of function
		    switch( funcdesc->invkind ) {
		    case INVOKE_PROPERTYGET: // property
		    case INVOKE_PROPERTYPUT:
			{
			    if ( funcdesc->cParams > 1 )
				qFatal( "Too many parameters in property" );
			    QMetaProperty *prop = proplist[function];
			    if ( !prop ) {
				if ( !eventSink )
				    that->eventSink = new QAxEventSink( that );
				eventSink->addProperty( funcdesc->memid, function );

				prop = new QMetaProperty;
				proplist.insert( function, prop );
				prop->meta = (QMetaObject**)&metaobj;
				prop->_id = -1;
				prop->enumData = 0;
				prop->flags = 0;
				QString ptype = paramTypes[0];
				prop->t = new char[ptype.length()+1];
				prop->t = qstrcpy( (char*)prop->t, ptype );
				QString propname = function;
				prop->n = new char[propname.length()+1];
				prop->n = qstrcpy( (char*)prop->n, propname );
			    }
			    if ( funcdesc->invkind == INVOKE_PROPERTYGET ) {
				prop->flags |= QMetaProperty::Readable;
				break;
			    } else {
				prop->flags |= QMetaProperty::Writable;
				// fall through to generate put function as slot
			    }
			}

		    case INVOKE_FUNC: // method
			{
			    if ( funcdesc->invkind == INVOKE_PROPERTYPUT ) {
				function = "set" + function;
				prototype = "set" + prototype;
				if ( slotlist.find( prototype ) )
				    break;
			    }
			    bool defargs;
			    QString defprototype = prototype;
			    do {
				defargs = FALSE;
				QUMethod *slot = new QUMethod;
				slot->name = new char[function.length()+1];
				slot->name = qstrcpy( (char*)slot->name, function );
				slot->count = parameters.count();
				QUParameter *params = slot->count ? new QUParameter[slot->count] : 0;
				int offset = parameters[0] == "return" ? 1 : 0;
				for ( int p = 0; p< slot->count; ++p ) {
				    QString paramName = parameters[p];
				    QString paramType = paramTypes[p];
				    if ( paramType.right( 2 ) == "=0" ) {
					paramType.truncate( paramType.length()-2 );
					paramTypes[p] = paramType;
					defargs = TRUE;
					slot->count = p;
					prototype = function + "(";
					for ( int pp = offset; pp < p; ++pp ) {
					    prototype += paramTypes[pp];
					    if ( pp < p-1 )
						prototype += ",";
					}
					prototype += ")";
					break;
				    }
				    params[p].name = new char[paramName.length()+1];
				    params[p].name = qstrcpy( (char*)params[p].name, paramName );
				    params[p].inOut = 0;
				    if ( !p && paramName == "return" ) {
					params[p].inOut = QUParameter::Out;
				    } else if ( funcdesc->lprgelemdescParam + p - offset ) {
					ushort inout = funcdesc->lprgelemdescParam[p-offset].paramdesc.wParamFlags;
					if ( inout & PARAMFLAG_FIN )
					    params[p].inOut |= QUParameter::In;
					if ( inout & PARAMFLAG_FOUT )
					    params[p].inOut |= QUParameter::Out;
				    }

				    QStringToQUType( paramType, params + p );
				}

				slot->parameters = params;
				slotlist.insert( prototype, slot );
				prototype = defprototype;
			    } while ( defargs );
			}
			break;

		    default:
			break;
		    }

#if 0 // documentation in metaobject would be cool?
		    // get function documentation
		    BSTR bstrDocu;
		    info->GetDocumentation( funcdesc->memid, 0, &bstrDocu, 0, 0 );
		    QString strDocu = BSTRToQString( bstrDocu );
		    if ( !!strDocu )
			desc += "[" + strDocu + "]";
		    desc += "\n";
		    SysFreeString( bstrDocu );
#endif

		    info->ReleaseFuncDesc( funcdesc );
		}
		
		// get information about all variables
		for ( ushort vd = 0; vd < nVars; ++vd ) {
		    VARDESC *vardesc;
		    info->GetVarDesc( vd, &vardesc );
		    if ( !vardesc )
			break;

		    // no use if it's not a dispatched variable
		    if ( vardesc->varkind != VAR_DISPATCH ) {
			info->ReleaseVarDesc( vardesc );
			continue;
		    }

		    // get variable type
		    TYPEDESC typedesc = vardesc->elemdescVar.tdesc;
		    QString variableType = typedescToQString( typedesc );

		    // get variable name
		    QString variableName;

		    BSTR bstrNames[256];
		    UINT maxNames = 255;
		    UINT maxNamesOut;
		    info->GetNames( vardesc->memid, (BSTR*)&bstrNames, maxNames, &maxNamesOut );
		    for ( int v = 0; v < (int)maxNamesOut; ++v ) {
			QString varName = BSTRToQString( bstrNames[v] );
			SysFreeString( bstrNames[v] );

			if ( !v ) {
			    variableName = varName;
			    continue;
			}
		    }

		    // generate meta property
		    QMetaProperty *prop = proplist[variableName];
		    if ( !prop ) {
			if ( !eventSink )
			    that->eventSink = new QAxEventSink( that );
			eventSink->addProperty( vardesc->memid, variableName );

			prop = new QMetaProperty;
			proplist.insert( variableName, prop );
			prop->meta = (QMetaObject**)&metaobj;
			prop->_id = -1;
			prop->enumData = 0;
			prop->flags = QMetaProperty::Readable | QMetaProperty::Writable;;

			prop->t = new char[variableType.length()+1];
			prop->t = qstrcpy( (char*)prop->t, variableType );
			prop->n = new char[variableName.length()+1];
			prop->n = qstrcpy( (char*)prop->n, variableName );
		    }

		    // generate a set slot
		    variableType = constRefify( variableType );

		    QString function = "set" + variableName;
		    QString prototype = function + "(" + variableType + ")";

		    if ( !slotlist.find( prototype ) ) {
			QUMethod *slot = new QUMethod;
			slot->name = new char[ function.length() + 1 ];
			slot->name = qstrcpy( (char*)slot->name, function );
			slot->count = 1;
			QUParameter *params = new QUParameter;
			params->inOut = QUParameter::In;
			params->name = new char[ variableName.length() + 1 ];
			params->name = qstrcpy( (char*)params->name, variableName );

			QStringToQUType( variableType, params );

			slot->parameters = params;
			slotlist.insert( prototype, slot );
		    }

#if 0 // documentation in metaobject would be cool?
		    // get function documentation
		    BSTR bstrDocu;
		    info->GetDocumentation( vardesc->memid, 0, &bstrDocu, 0, 0 );
		    QString strDocu = BSTRToQString( bstrDocu );
		    if ( !!strDocu )
			desc += "[" + strDocu + "]";
		    desc += "\n";
		    SysFreeString( bstrDocu );
#endif
		    info->ReleaseVarDesc( vardesc );
		}

		if ( !nImpl )
		    break;

		// go up one base class
		HREFTYPE pRefType;
		info->GetRefTypeOfImplType( 0, &pRefType );
		CComPtr<ITypeInfo> baseInfo;
		info->GetRefTypeInfo( pRefType, &baseInfo );
		if ( info == baseInfo ) // IUnknown inherits IUnknown ???
		    break;
		info = baseInfo;
	    }
	}
    }

    CComPtr<IConnectionPointContainer> cpoints;
    ptr->QueryInterface( IID_IConnectionPointContainer, (void**)&cpoints );
    if ( cpoints ) {
	CComPtr<IProvideClassInfo> classinfo;
	cpoints->QueryInterface( IID_IProvideClassInfo, (void**)&classinfo );

	CComPtr<IEnumConnectionPoints> epoints;
	cpoints->EnumConnectionPoints( &epoints );
	if ( epoints ) {
	    ULONG c = 1;
	    epoints->Reset();
	    do {
		CComPtr<IConnectionPoint> cpoint;
		epoints->Next( c, &cpoint, &c );
		if ( !c )
		    break;

		IID iid;
		cpoint->GetConnectionInterface( &iid );
		if ( !eventSink )
		    that->eventSink = new QAxEventSink( that );
		eventSink->addConnection( cpoint, iid );

		if ( classinfo ) {
		    CComPtr<ITypeInfo> info;
		    CComPtr<ITypeInfo> eventinfo;
		    classinfo->GetClassInfo( &info );
		    if ( info ) { // this is the type info of the component, not the event interface
			// get information about type
			TYPEATTR *typeattr;
			info->GetTypeAttr( &typeattr );
			if ( typeattr ) {
			    // UUID
			    if ( !infolist.find( "CoClass" ) ) {
				QUuid uuid( typeattr->guid );
				infolist.insert( "CoClass", new QString( uuid.toString().upper() ) );
				QString version( "%1.%1" );
				version = version.arg( typeattr->wMajorVerNum ).arg( typeattr->wMinorVerNum );
				infolist.insert( "Version", new QString( version ) );
			    }

			    // test if one of the interfaces implemented is the one we're looking for
			    for ( int impl = 0; impl < typeattr->cImplTypes; ++impl ) {
				// get the ITypeInfo for the interface
				HREFTYPE reftype;
				info->GetRefTypeOfImplType( impl, &reftype );
				CComPtr<ITypeInfo> eventtype;
				info->GetRefTypeInfo( reftype, &eventtype );
				if ( eventtype ) {
				    TYPEATTR *eventattr;
				    eventtype->GetTypeAttr( &eventattr );
				    // this is it
				    if ( eventattr && eventattr->guid == iid ) {
					eventinfo = eventtype;
					break;
				    }
				    eventtype->ReleaseTypeAttr( eventattr );
				}
			    }
			    info->ReleaseTypeAttr( typeattr );

			    // what about other event interfaces?
			    if ( eventinfo ) {
				TYPEATTR *eventattr;
				eventinfo->GetTypeAttr( &eventattr );
				// Number of functions
				ushort nEvents = eventattr->cFuncs;

				// get information about all event functions
				for ( UINT fd = 0; fd < nEvents; ++fd ) {
				    FUNCDESC *funcdesc;
				    eventinfo->GetFuncDesc( fd, &funcdesc );
				    if ( !funcdesc )
					break;
				    if ( funcdesc->invkind != INVOKE_FUNC ||
					 funcdesc->funckind != FUNC_DISPATCH ) {
					info->ReleaseFuncDesc( funcdesc );
					continue;
				    }

				    // get return value
				    TYPEDESC typedesc = funcdesc->elemdescFunc.tdesc;
				    QString returnType = typedescToQString( typedesc );

				    // get event function prototype
				    QString function;
				    QString prototype;
				    QStringList parameters;
				    QStringList paramTypes;

				    BSTR bstrNames[256];
				    UINT maxNames = 255;
				    UINT maxNamesOut;
				    eventinfo->GetNames( funcdesc->memid, (BSTR*)&bstrNames, maxNames, &maxNamesOut );
				    for ( int p = 0; p < (int)maxNamesOut; ++ p ) {
					QString paramName = BSTRToQString( bstrNames[p] );
					SysFreeString( bstrNames[p] );

					// function name
					if ( !p ) {
					    function = paramName;
					    prototype = function + "(";
					    continue;
					}

					// parameter
					bool optional = p > funcdesc->cParams - funcdesc->cParamsOpt;
					
					TYPEDESC tdesc = funcdesc->lprgelemdescParam[p - (funcdesc->invkind == INVOKE_FUNC) ? 1 : 0 ].tdesc;
					QString ptype = typedescToQString( tdesc );
					if ( funcdesc->invkind == INVOKE_FUNC )
					    ptype = constRefify( ptype );

					paramTypes << ptype;
					parameters << paramName;
					prototype += ptype;
					if ( optional )
					    prototype += "=0";
					if ( p < funcdesc->cParams )
					    prototype += ",";
				    }
				    if ( !!prototype )
					prototype += ")";

				    if ( signallist.find( prototype ) )
					continue;

				    QUMethod *signal = new QUMethod;
				    signal->name = new char[function.length()+1];
				    signal->name = qstrcpy( (char*)signal->name, function );
				    signal->count = parameters.count();
				    QUParameter *params = signal->count ? new QUParameter[signal->count] : 0;
				    for ( p = 0; p< signal->count; ++p ) {
					QString paramName = parameters[p];
					QString paramType = paramTypes[p];
					params[p].name = new char[paramName.length()+1];
					params[p].name = qstrcpy( (char*)params[p].name, paramName );
					params[p].inOut = 0;
					if ( funcdesc->lprgelemdescParam + p ) {
					    ushort inout = funcdesc->lprgelemdescParam[p].paramdesc.wParamFlags;
					    if ( inout & PARAMFLAG_FIN )
						params[p].inOut |= QUParameter::In;
					    if ( inout & PARAMFLAG_FOUT )
						params[p].inOut |= QUParameter::Out;
					}

					QStringToQUType( paramType, params + p );
				    }
				    signal->parameters = params;
				    signallist.insert( prototype, signal );
				    eventSink->addSignal( funcdesc->memid, prototype );

#if 0 // documentation in metaobject would be cool?
				    // get function documentation
				    BSTR bstrDocu;
				    eventinfo->GetDocumentation( funcdesc->memid, 0, &bstrDocu, 0, 0 );
				    QString strDocu = BSTRToQString( bstrDocu );
				    if ( !!strDocu )
					desc += "[" + strDocu + "]";
				    desc += "\n";
				    SysFreeString( bstrDocu );
#endif
				    eventinfo->ReleaseFuncDesc( funcdesc );
				}
			    }
			}
		    }
		}
	    } while ( c );
	}
    }

    // setup slot data
    int index = 0;
    QMetaData *const slot_data = slotlist.count() ? new QMetaData[slotlist.count()] : 0;
    QDictIterator<QUMethod> slot_it( slotlist );
    while ( slot_it.current() ) {
	QUMethod *slot = slot_it.current();
	slot_data[index].name = new char[slot_it.currentKey().length()+1];
	slot_data[index].name = qstrcpy( (char*)slot_data[index].name, slot_it.currentKey() );
	slot_data[index].method = slot;
	slot_data[index].access = QMetaData::Public;

	++index;
	++slot_it;
    }

    // setup signal data
    index = 0;
    QMetaData *const signal_data = new QMetaData[signallist.count()+1];
    if ( signal_data ) {
	signal_data[index] = signal_tbl[index];
	++index;
    }

    QDictIterator<QUMethod> signal_it( signallist );
    while ( signal_it.current() ) {
	QUMethod *signal = signal_it.current();
	signal_data[index].name = new char[signal_it.currentKey().length()+1];
	signal_data[index].name = qstrcpy( (char*)signal_data[index].name, signal_it.currentKey() );
	signal_data[index].method = signal;
	signal_data[index].access = QMetaData::Public;

	++index;
	++signal_it;
    }

    // setup property data
    index = 0;
    QMetaProperty *const prop_data = new QMetaProperty[proplist.count()+1];
    if ( prop_data ) {
	static const QMetaProperty props_tbl[1] = {
 	    { "QString","control", 259, (QMetaObject**)&metaobj, 0, -1 }
	};

	prop_data[index] = props_tbl[index];
	++index;
    }
    QDictIterator<QMetaProperty> prop_it( proplist );
    while ( prop_it.current() ) {
	QMetaProperty *prop = prop_it.current();
	prop_data[index].t = prop->t;
	prop_data[index].n = prop->n;
	prop_data[index].flags = prop->flags;
	prop_data[index].meta = prop->meta;
	prop_data[index].enumData = prop->enumData;
	prop_data[index]._id = prop->_id;

	++index;
	++prop_it;
    }

    // setup enum data
    index = 0;
    QMetaEnum *const enum_data = enumlist.count() ? new QMetaEnum[enumlist.count()] : 0;
    QDictIterator<QString> enum_it( enumlist );
    while ( enum_it.current() ) {
	QString info = *enum_it.current();
	QString key = enum_it.currentKey();
	enum_data[index].name = new char[key.length()+1];
	enum_data[index].name = qstrcpy( (char*)enum_data[index].name, key );
	//###
	++index;
	++enum_it;
    }

    // setup class info data
    index = 0;
    QClassInfo *const class_info = new QClassInfo[infolist.count()];
    QDictIterator<QString> info_it( infolist );
    while ( info_it.current() ) {
	QString info = *info_it.current();
	QString key = info_it.currentKey();
	class_info[index].name = new char[key.length()+1];
	class_info[index].name = qstrcpy( (char*)class_info[index].name, key );
	class_info[index].value = new char[info.length()+1];
	class_info[index].value = qstrcpy( (char*)class_info[index].value, info );

	++index;
	++info_it;
    }

    // put the metaobject together
    that->metaobj = QMetaObject::new_metaobject( 
	className(), parentObject, 
	slot_data, slotlist.count(),
	signal_data, signallist.count()+1,
	prop_data, proplist.count()+1,
	enum_data, enumlist.count(),
	class_info, infolist.count() );

    return metaobj;
}

/*!
    \reimp
*/
bool QComBase::qt_invoke( int _id, QUObject* _o )
{
    const int index = _id - metaObject()->slotOffset();
    if ( !ptr || index < 0 )
	return FALSE;

    // get the IDispatch
    CComPtr<IDispatch> disp;
    ptr->QueryInterface( IID_IDispatch, (void**)&disp );
    if ( !disp )
	return FALSE;

    // get the slot information
    const QMetaData *slot_data = metaObject()->slot( index );
    if ( !slot_data )
	return FALSE;
    const QUMethod *slot = slot_data->method;
    if ( !slot )
	return FALSE;

    // Get the Dispatch ID of the method to be called
    bool fakedslot = FALSE;
    DISPID dispid;
    OLECHAR *names = (unsigned short*)qt_winTchar(slot->name, TRUE );
    disp->GetIDsOfNames( IID_NULL, &names, 1, LOCALE_SYSTEM_DEFAULT, &dispid );
    if ( dispid == DISPID_UNKNOWN ) {
	// see if we are calling a property set function as a slot
	if ( QString( slot->name ).left( 3 ) != "set" )
	    return FALSE;
	QString realname = slot->name;
	realname = realname.right( realname.length() - 3 );
	OLECHAR *realnames = (unsigned short*)qt_winTchar(realname, TRUE );
	disp->GetIDsOfNames( IID_NULL, &realnames, 1, LOCALE_SYSTEM_DEFAULT, &dispid );
	if ( dispid == DISPID_UNKNOWN )
	    return FALSE;

	fakedslot = TRUE;
    }

    // setup the parameters
    VARIANT ret; // Invoke initializes it
    VARIANT *pret = 0;
    if ( slot->parameters && ( slot->parameters[0].inOut & QUParameter::Out ) ) // slot has return value
	pret = &ret;
    int slotcount = slot->count - ( pret ? 1 : 0 );

    VARIANT arg;
    DISPPARAMS params;
    DISPID dispidNamed = DISPID_PROPERTYPUT;
    params.cArgs = slotcount;
    params.cNamedArgs = fakedslot ? 1 : 0;
    params.rgdispidNamedArgs = fakedslot ? &dispidNamed : 0;
    params.rgvarg = slot->count ? new VARIANTARG[slotcount] : 0;
    for ( int p = 0; p < slotcount; ++p ) {
	QUObject *obj = _o + p + 1;
	// map the QUObject's type to the VARIANT. ### Maybe it would be better 
	// to convert the QUObject to what the VARIANT is supposed to be, since
	// QUType is rather powerful, and VARIANT is not...
	if ( QUType::isEqual( obj->type, &static_QUType_int ) ) {
	    arg.vt = VT_I4;
	    arg.lVal = static_QUType_int.get( obj );
	} else if ( QUType::isEqual( obj->type, &static_QUType_QString ) ) {
	    arg.vt = VT_BSTR;
	    arg.bstrVal = QStringToBSTR( static_QUType_QString.get( obj ) );
	} else if ( QUType::isEqual( obj->type, &static_QUType_charstar ) ) {
	    arg.vt = VT_BSTR;
	    arg.bstrVal = QStringToBSTR( static_QUType_charstar.get( obj ) );
	} else if ( QUType::isEqual( obj->type, &static_QUType_bool ) ) {
	    arg.vt = VT_BOOL;
	    arg.boolVal = static_QUType_bool.get( obj );
	} else if ( QUType::isEqual( obj->type, &static_QUType_double ) ) {
	    arg.vt = VT_R8;
	    arg.dblVal = static_QUType_double.get( obj );
	} else if ( QUType::isEqual( obj->type, &static_QUType_enum ) ) {
	    arg.vt = VT_I4;
	    arg.lVal = static_QUType_enum.get( obj );
	} else if ( QUType::isEqual( obj->type, &static_QUType_QVariant ) ) {
	    arg = QVariantToVARIANT( static_QUType_QVariant.get( obj ) );
	} else if ( QUType::isEqual( obj->type, &static_QUType_idisp ) ) {
	    arg.vt = VT_DISPATCH;
	    arg.pdispVal = (IDispatch*)static_QUType_ptr.get( obj );
	} else if ( QUType::isEqual( obj->type, &static_QUType_iface ) ) {
	    arg.vt = VT_UNKNOWN;
	    arg.punkVal = (IUnknown*)static_QUType_ptr.get( obj );
	} else if ( QUType::isEqual( obj->type, &static_QUType_ptr ) ) {
	    const QUParameter *param = slot->parameters + p + ( pret ? 1 : 0 );
	    const char *type = (const char*)param->typeExtra;
	    if ( !qstrcmp( type, "int" ) ) {
		arg.vt = VT_I4;
		arg.lVal = *(int*)static_QUType_ptr.get( obj );
	    } else if ( !qstrcmp( type, "QString" ) || !qstrcmp( type, "const QString&" ) ) {
		arg.vt = VT_BSTR;
		arg.bstrVal = QStringToBSTR( *(QString*)static_QUType_ptr.get( obj ) );
	    } else if ( !qstrcmp( type, "QDateTime" ) || !qstrcmp( type, "const QDateTime&" ) ) {
		arg.vt = VT_DATE;
		arg.date = QDateTimeToDATE( *(QDateTime*)static_QUType_ptr.get( obj ) );
	    } else {
		arg.vt = VT_UI4;
		arg.ulVal = (Q_ULONG)static_QUType_ptr.get( obj );
	    }
	    //###
	} else {
	    arg.vt = VT_EMPTY;
	}
	params.rgvarg[ slotcount - p - 1 ] = arg;
    }
    // call the method
    UINT argerr = 0;
    HRESULT hres;
    if ( fakedslot && slotcount == 1 ) {
	hres = disp->Invoke( dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYPUT, &params, pret, 0, &argerr );
    } else {
	hres = disp->Invoke( dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &params, pret, 0, &argerr );	    
    }

    // get return value
    if ( pret )
	VARIANTToQUObject( ret, _o );
    // clean up
    for ( p = 0; p < slot->count; ++p ) {
	if ( params.rgvarg[p].vt == VT_BSTR )
	    SysFreeString( params.rgvarg[p].bstrVal );
    }
    delete [] params.rgvarg;

    return checkHRESULT( hres );
}

/*!
    \reimp
*/
bool QComBase::qt_property( int _id, int _f, QVariant* _v )
{
    const int index = _id - metaObject()->propertyOffset();
    if ( index == 0 ) { // control property
	switch( _f ) {
	case 0: setControl( _v->toString() ); break;
	case 1: *_v = control(); break;
	default: if ( _f > 5 ) return FALSE;
	}
	return TRUE;
    } else if ( ptr && index >= 0 ) {
	// get the IDispatch
	CComPtr<IDispatch> disp;
	ptr->QueryInterface( IID_IDispatch, (void**)&disp );
	if ( !disp )
	    return FALSE;

	// get the property information
	const QMetaProperty *prop = metaObject()->property( index );
	if ( !prop )
	    return FALSE;

	//  get the Dispatch ID of the function to be called
	DISPID dispid;
	OLECHAR *names = (unsigned short*)qt_winTchar(prop->n, TRUE );
	disp->GetIDsOfNames( IID_NULL, &names, 1, LOCALE_SYSTEM_DEFAULT, &dispid );
	if ( dispid == DISPID_UNKNOWN )
	    return FALSE;
	
	// call the setter function
	if ( _f == 0 ) { 
	    VARIANTARG arg;
	    arg.vt = VT_ERROR;
	    arg.scode = DISP_E_TYPEMISMATCH;

	    // map QVariant to VARIANTARG. ### Maybe it would be better 
	    // to convert the QVariant to what the VARIANT is supposed to be,
	    // but we don't know that from the QMetaProperty of course.
	    if ( qstrcmp( prop->type(), _v->typeName() ) ) {
		QVariant::Type type = QVariant::nameToType( prop->type() );
		if ( type != QVariant::Invalid )
		    _v->cast( type );
	    }
	    arg = QVariantToVARIANT( *_v, prop->type() );

	    if ( arg.vt == VT_EMPTY ) {
		qDebug( "QActiveX::setProperty(): Unhandled property type" );
		return FALSE;
	    }
	    DISPPARAMS params;
	    DISPID dispidNamed = DISPID_PROPERTYPUT;
	    params.rgvarg = &arg;
	    params.cArgs = 1;
	    params.rgdispidNamedArgs = &dispidNamed;
	    params.cNamedArgs = 1;

	    UINT argerr = 0;
	    HRESULT hres = disp->Invoke( dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYPUT, &params, 0, 0, &argerr );
	    if ( arg.vt == VT_BSTR )
		SysFreeString( arg.bstrVal );
	    return checkHRESULT( hres );
	} else if ( _f == 1 ) { // call the getter function
	    VARIANTARG arg;
	    DISPPARAMS params;
	    params.cArgs = 0;
	    params.cNamedArgs = 0;
	    params.rgdispidNamedArgs = 0;
	    params.rgvarg = 0;

	    HRESULT hres = disp->Invoke( dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &params, &arg, 0, 0 );
	    if ( !checkHRESULT( hres ) )
		return FALSE;

	    // map result VARIANTARG to QVariant
	    QVariant var;
	    switch( arg.vt ) {
	    case VT_UI1:
		var = arg.bVal;
		break;
	    case VT_I2:
		var = arg.iVal;
		break;
	    case VT_ERROR:
	    case VT_I4:
		var = arg.lVal;
		break;
	    case VT_R4:
		var = (double)arg.fltVal;
		break;
	    case VT_R8:
		var = arg.dblVal;
		break;
	    case VT_CY: // __int64 -> int ###
		var = arg.cyVal.Hi;
		break;
	    case VT_DATE: // DATE -> QDateTime
		var = DATEToQDateTime( arg.date );
		break;
	    case VT_BOOL:
		var = QVariant( arg.boolVal, 42 );
		break;
	    case VT_BSTR:
		var = BSTRToQString( arg.bstrVal );
		break;
	    case VT_I1:
		var = arg.cVal;
		break;
	    case VT_UI2:
		var = arg.uiVal;
		break;
	    case VT_UI4:
		var = (long)arg.ulVal;
		break;
	    case VT_INT:
		var = arg.intVal;
		break;
	    case VT_UINT:
		var = arg.uintVal;
		break;
	    case VT_DISPATCH: // IDispatch* -> int ###
		var = (Q_LONG)arg.pdispVal;
		break;
	    case VT_UNKNOWN: // IUnkonwn* -> int ###
		var = (Q_LONG)arg.punkVal;
		break;
	    case VT_EMPTY:
		// empty VARIANT type return
		break;

	    default:

		break;
	    }
	    *_v = var;
	    return ( _v->isValid() );
	} else if ( _f < 6 ) {
	    return TRUE;
	}
    }
    return FALSE;
}

/*!
    Calls \a function and passes parameters \a var1 ... \a var8, and returns
    the value, or an invalid variant if \a function does not return a value.
*/
QVariant QComBase::dynamicCall( const QCString &function, const QVariant &var1, 
							 const QVariant &var2, 
							 const QVariant &var3, 
							 const QVariant &var4, 
							 const QVariant &var5, 
							 const QVariant &var6, 
							 const QVariant &var7, 
							 const QVariant &var8 )
{
    QUObject obj[9];
    // obj[0] is the result
    QVariantToQUObject( var1, obj[1] );
    QVariantToQUObject( var2, obj[2] );
    QVariantToQUObject( var3, obj[3] );
    QVariantToQUObject( var4, obj[4] );
    QVariantToQUObject( var5, obj[5] );
    QVariantToQUObject( var6, obj[6] );
    QVariantToQUObject( var7, obj[7] );
    QVariantToQUObject( var8, obj[8] );

    QVariant result;

    const QMetaData *slot_data = 0;
    const QUMethod *slot = 0;
    const QMetaObject *meta = metaObject();
    int index = 0;
    do {
	slot_data = meta->slot( index, TRUE );
	if ( slot_data ) {
	    slot = slot_data->method;
	    if ( !qstrcmp( slot->name, function ) )
		break;
	} else {
	    slot = 0;
	}
	++index;
    } while ( slot_data );

    if ( slot ) {
	qt_invoke( index, obj );
	if ( QUType::isEqual( obj->type, &static_QUType_int ) ) {
	    result = static_QUType_int.get( obj );
	} else if ( QUType::isEqual( obj->type, &static_QUType_QString ) ) {
	    result = static_QUType_QString.get( obj );
	} else if ( QUType::isEqual( obj->type, &static_QUType_charstar ) ) {
	    result = static_QUType_charstar.get( obj );
	} else if ( QUType::isEqual( obj->type, &static_QUType_bool ) ) {
	    result = static_QUType_bool.get( obj );
	} else if ( QUType::isEqual( obj->type, &static_QUType_double ) ) {
	    result = static_QUType_double.get( obj );
	} else if ( QUType::isEqual( obj->type, &static_QUType_enum ) ) {
	    result = static_QUType_enum.get( obj );
	} else if ( QUType::isEqual( obj->type, &static_QUType_QVariant ) ) {
	    result = static_QUType_QVariant.get( obj );
	} else if ( QUType::isEqual( obj->type, &static_QUType_idisp ) ) {
	    //###
	} else if ( QUType::isEqual( obj->type, &static_QUType_iface ) ) {
	    //###
	} else if ( QUType::isEqual( obj->type, &static_QUType_ptr ) && slot->parameters ) {
	    const QUParameter *param = slot->parameters;
	    const char *type = (const char*)param->typeExtra;
	    if ( !qstrcmp( type, "int" ) ) {
		result = *(int*)static_QUType_ptr.get( obj );
	    } else if ( !qstrcmp( type, "QString" ) || !qstrcmp( type, "const QString&" ) ) {
		result = *(QString*)static_QUType_ptr.get( obj );
	    } else if ( !qstrcmp( type, "QDateTime" ) || !qstrcmp( type, "const QDateTime&" ) ) {
		result = *(QDateTime*)static_QUType_ptr.get( obj );
	    }
	    //###
	}
    }
#if defined(QT_CHECK_RANGE)
    else {
	const char *coclass = meta->classInfo( "CoClass" );
	qWarning( "QActiveX::dynamicCall: %s: No such method in %s %s", (const char*)function, control().latin1(), 
	    coclass ? coclass: "(unknown)" );
    }
#endif
    return result;
}

/*!
    \class QComObject qactivex.h
    \brief The QComObject class provides a QObject that wraps a COM object.
*/

/*!
    Creates an empty COM object. To initialize the object, use \link QComBase::setControl setControl \endlink.
*/
QComObject::QComObject( QObject *parent, const char *name )
: QObject( parent, name )
{
}

/*! 
    Creates a QComObject that wraps the COM object \a c.
*/
QComObject::QComObject( const QString &c, QObject *parent, const char *name )
: QObject( parent, name )
{
    setControl( c );
}

/*!
    Creates a QComObject that wraps the COM object referenced by \a iface.
*/
QComObject::QComObject( IUnknown *iface, QObject *parent, const char *name )
: QObject( parent, name ), QComBase( iface )
{
}

/*!
    Releases the COM object and destroys the QComObject,
    cleaning all allocated resources.
*/
QComObject::~QComObject()
{
}

/*!
    \reimp
*/
const char *QComObject::className() const
{
    return "QComObject";
}

/*!
    Initializes the COM object.
*/
void QComObject::initialize( IUnknown **ptr )
{
    QUuid uuid( control() );
    if ( *ptr || uuid.isNull() )
	return;
    CoInitialize( 0 );
    _Module.Init( 0, GetModuleHandle( 0 ) );

    *ptr = 0;
    CoCreateInstance( uuid, 0, CLSCTX_ALL, IID_IUnknown, (void**)ptr );
    if ( !ptr ) {
	_Module.Term();
	CoUninitialize();
    }
}

/*!
    \reimp
*/
QMetaObject *QComObject::metaObject() const
{
    return QComBase::metaObject();
}

/*!
    \reimp
*/
QMetaObject *QComObject::parentMetaObject() const
{
    return QObject::staticMetaObject();
}

/*!
    \reimp
*/
void *QComObject::qt_cast( const char *cname )
{
    if ( !qstrcmp( cname, "QComObject" ) ) return this;
    if ( !qstrcmp( cname, "QComBase" ) ) return (QComBase*)this;
    return QObject::qt_cast( cname );
}


/*!
    \reimp
*/
bool QComObject::qt_invoke( int _id, QUObject *_o )
{
    if ( QComBase::qt_invoke( _id, _o ) )
	return TRUE;
    return QObject::qt_invoke( _id, _o );
}

/*!
    \reimp
*/
bool QComObject::qt_emit( int _id, QUObject* _o )
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
  \reimp
*/
bool QComObject::qt_property( int _id, int _f, QVariant *_v )
{
    if ( QComBase::qt_property( _id, _f, _v ) )
	return TRUE;
    return QObject::qt_property( _id, _f, _v );
}

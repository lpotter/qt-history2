#include "qsql_odbc.h"

#if defined (_OS_WIN32_)
#include <qt_windows.h>
#endif
#include <qdatetime.h>

#include <sql.h>
#include <sqlext.h>

class QODBCPrivate
{
public:
    QODBCPrivate()
    : hEnv(0), hDbc(0), hStmt(0)
    {}
    SQLHANDLE hEnv;
    SQLHANDLE hDbc;
    SQLHANDLE hStmt;
};

QString WarnODBCHandle(int handleType, SQLHANDLE handle)
{
    char state_[SQL_SQLSTATE_SIZE+1];
    SQLINTEGER nativeCode_;
    char description_[SQL_MAX_MESSAGE_LENGTH];
    SQLSMALLINT tmp;
    SQLRETURN r = SQLGetDiagRec( handleType,
    				 handle,
				 1,
			         (SQLCHAR*)state_,
				 &nativeCode_,
			    	 (SQLCHAR*)description_,
				 SQL_MAX_MESSAGE_LENGTH-1,
				 &tmp);
    if ( r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO )
	return QString( (const char*)description_ );
    return QString::null;
}

QString ODBCWarn( const QODBCPrivate* odbc)
{
    return ( WarnODBCHandle( SQL_HANDLE_ENV, odbc->hEnv ) + WarnODBCHandle( SQL_HANDLE_DBC, odbc->hDbc ) + WarnODBCHandle( SQL_HANDLE_STMT, odbc->hStmt ) );
}

void qSystemWarning( const QString& message, const QODBCPrivate* odbc )
{
#ifdef CHECK_RANGE
    qWarning( message + "\tError:" + ODBCWarn( odbc ) );
#endif
}

QSqlError makeError( const QString& err, int type, const QODBCPrivate* p )
{
    return QSqlError( "QODBC: " + err, ODBCWarn(p), type );
}

QSqlFieldInfo makeFieldInfo( const QODBCPrivate* p, int i  )
{
    SQLCHAR colName[255];
    SQLSMALLINT colNameLen;
    SQLSMALLINT colType;
    SQLUINTEGER colSize;
    SQLSMALLINT colScale;
    SQLSMALLINT nullable;
    SQLRETURN r = SQLDescribeCol( p->hStmt,
			i+1,
			colName,
			sizeof(colName),
			&colNameLen,
			&colType,
			&colSize,
			&colScale,
			&nullable);
#ifdef CHECK_RANGE
    if ( r != SQL_SUCCESS )
	qSystemWarning( QString("Unable to describe column %1").arg(i), p );
#endif
    QVariant::Type type;
    switch ( colType ) {
    case SQL_DECIMAL:
    case SQL_NUMERIC:
    case SQL_REAL:
    case SQL_FLOAT:
    case SQL_DOUBLE:
	type = QVariant::Double;
	break;
    case SQL_SMALLINT:
    case SQL_INTEGER:
    case SQL_BIT:
    case SQL_TINYINT:
    case SQL_BIGINT:
	type = QVariant::Int;
	break;
//     case SQL_BINARY:
//     case SQL_VARBINARY:
//     case SQL_LONGVARBINARY:
// 	type = QSqlFieldInfo::Binary;
// 	break;
    case SQL_DATE:
    case SQL_TYPE_DATE:
	type = QVariant::Date;
	break;
    case SQL_TIME:
    case SQL_TYPE_TIME:
	type = QVariant::Time;
	break;
    case SQL_TIMESTAMP:
    case SQL_TYPE_TIMESTAMP:
	type = QVariant::DateTime;
	break;
    default:
    case SQL_CHAR:
    case SQL_VARCHAR:
    case SQL_LONGVARCHAR:
	type = QVariant::String;
	break;
    }
    return QSqlFieldInfo( QString((char*)colName), type, (int)colSize, (int)colScale );
}


QODBCDriver::QODBCDriver( QObject * parent, const char * name )
: QSqlDriver(parent,name ? name : "QODBC")
{
    init();
}

void QODBCDriver::init()
{
    d = new QODBCPrivate();
}

QODBCDriver::~QODBCDriver()
{
    qDebug("start ~QODBCDriver()");
    cleanup();
    delete d;
    qDebug("end ~QODBCDriver()");
}

bool QODBCDriver::open( const QString & db,
    			const QString & user,
			const QString & password,
			const QString & host)
{
    qDebug("QODBCDriver::open");
    if ( isOpen() ) {
	qDebug("open, closing first");
	close();
    }
    SQLRETURN r;
    r = SQLAllocHandle( SQL_HANDLE_ENV,
    			SQL_NULL_HANDLE,
			&d->hEnv);
    if ( r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO ) {
#ifdef CHECK_RANGE
	qSystemWarning( "Unable to allocate environment", d );
#endif
	setOpenError( TRUE );
        return FALSE;
    }
    r = SQLSetEnvAttr( d->hEnv,
    			SQL_ATTR_ODBC_VERSION,
			(SQLPOINTER)SQL_OV_ODBC2,
			SQL_IS_UINTEGER );
    r = SQLAllocHandle(SQL_HANDLE_DBC,
    			d->hEnv,
			&d->hDbc);
    if ( r != SQL_SUCCESS ) {
#ifdef CHECK_RANGE
	qSystemWarning("Unable to allocate connection", d );
#endif
	setOpenError( TRUE );
        return FALSE;
    }
    qDebug("QODBC, handles allocated.");
    QString connQStr = "DSN=" + db + ";UID=" + user + ";PWD=" + password + ";";
    SQLCHAR connOut[255];
    SWORD       cb;
    r = SQLDriverConnect( d->hDbc,
			    NULL,
			    (SQLCHAR*)connQStr.local8Bit().data(),
			    SQL_NTS,
			    connOut,
			    255,
			    &cb,
			    SQL_DRIVER_NOPROMPT);
    if ( r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO ) {
	setLastError( makeError( "Unable to connect", QSqlError::Connection, d ) );
	setOpenError( TRUE );
	return FALSE;
    }
    qDebug("QODBC, connection complete.");
    SQLUSMALLINT txn;
    SQLSMALLINT t;
    r = SQLGetInfo( d->hDbc,
		   	(SQLUSMALLINT)SQL_TXN_CAPABLE,
			&txn,
			sizeof(txn),
			&t);
    if ( txn == SQL_TC_NONE )
	setTransactionSupport( FALSE );
    else
	setTransactionSupport( TRUE );
    qDebug("QODBC, open.");
    setOpen( TRUE );
    return TRUE;
    Q_CONST_UNUSED(host);
}

void QODBCDriver::close()
{
    qDebug("QODBCDriver::close()");
    cleanup();
    setOpen( FALSE );
    setOpenError( FALSE );
}

void QODBCDriver::cleanup()
{
    qDebug("QODBCDriver::cleanup()");
    if ( isOpen() || isOpenError() ) {
    	if ( d->hDbc ) {
		SQLRETURN r = SQLDisconnect( d->hDbc );
		r = SQLFreeHandle( SQL_HANDLE_DBC, d->hDbc );
#ifdef CHECK_RANGE
		if ( r != SQL_SUCCESS )
		    qSystemWarning( "Unable to free connection handle", d );
#endif
		d->hDbc = 0;
		qDebug("ODBC connection handle freed");
    	}
    	if ( d->hEnv ) {
		SQLRETURN r = SQLFreeHandle( SQL_HANDLE_ENV, d->hEnv );
#ifdef CHECK_RANGE
		if ( r != SQL_SUCCESS )
		    qSystemWarning( "Unable to free environment handle", d );
#endif
		qDebug("ODBC environment handle freed");
		d->hEnv = 0;
    	}
    }
}

QSql QODBCDriver::createResult() const
{
    return QSql( new QODBCResult( this, d ) );
}

bool QODBCDriver::beginTransaction()
{
    SQLUINTEGER ac(SQL_AUTOCOMMIT_OFF);
    SQLRETURN r  = SQLSetConnectAttr( d->hDbc,
			    SQL_ATTR_AUTOCOMMIT,
			    (SQLPOINTER)ac,
			    sizeof(ac));
    if ( r != SQL_SUCCESS ) {
	setLastError( makeError( "Unable to disable autocommit", QSqlError::Transaction, d ) );
	return FALSE;
    }
    return TRUE;
}

bool QODBCDriver::commitTransaction()
{
    SQLRETURN r = SQLEndTran( SQL_HANDLE_ENV,
    				d->hEnv,
				SQL_COMMIT);
    if ( r != SQL_SUCCESS ) {
	setLastError( makeError("Unable to commit transaction", QSqlError::Transaction, d ) );
	return FALSE;
    }
    return endTrans();
}

bool QODBCDriver::rollbackTransaction()
{
    SQLRETURN r = SQLEndTran( SQL_HANDLE_ENV,
    				d->hEnv,
				SQL_ROLLBACK);
    if ( r != SQL_SUCCESS ) {
    	setLastError( makeError( "Unable to rollback transaction", QSqlError::Transaction, d ) );
	return FALSE;
    }
    return endTrans();
}

bool QODBCDriver::endTrans()
{
    SQLUINTEGER ac(SQL_AUTOCOMMIT_ON);
    SQLRETURN r  = SQLSetConnectAttr( d->hDbc,
			    SQL_ATTR_AUTOCOMMIT,
			    (SQLPOINTER)ac,
			    sizeof(ac));
    if ( r != SQL_SUCCESS ) {
    	setLastError( makeError( "Unable to enable autocommit", QSqlError::Transaction, d ) );
	return FALSE;
    }
    return TRUE;
}

QSqlFieldInfoList QODBCDriver::table( const QString& name ) const
{
    return view( "select * from " + name + " where (0=1);");
}

////////////////////////////////////////////////////////////////////////////

QODBCResultInfo::QODBCResultInfo( const QODBCPrivate* p )
: QSqlResultInfo()
{
    SQLRETURN r;
    SQLSMALLINT count;
    r = SQLNumResultCols( p->hStmt, &count );
#ifdef CHECK_RANGE
    if ( r != SQL_SUCCESS )
	qSystemWarning( "Unable to count result columns", p );
#endif
    if ( count > 0 && r == SQL_SUCCESS ) {
	for ( int i = 0; i < count; ++i ) {
            appendField( makeFieldInfo( p, i ) );
        }
    }
    SQLINTEGER affectedRowCount(0);
    r = SQLRowCount( p->hStmt, &affectedRowCount);
    if ( r == SQL_SUCCESS )
	setAffectedRows( affectedRowCount );
#ifdef CHECK_RANGE
    else
	qSystemWarning( "Unable to count affected rows", p );
#endif
    int at(0);
    SQLINTEGER currRow(0);
    r = SQLGetStmtAttr( p->hStmt,
    			SQL_ROW_NUMBER,
			&currRow,
			SQL_IS_INTEGER,
			0);
    at = currRow;
    r = SQLFetchScroll( p->hStmt,
                        SQL_FETCH_LAST,
                        0);
    if ( r == SQL_SUCCESS ) {
	r = SQLGetStmtAttr( p->hStmt,
    			SQL_ROW_NUMBER,
			&currRow,
			SQL_IS_INTEGER,
			0);
	if ( r == SQL_SUCCESS )
	    setSize( currRow );
	r = SQLFetchScroll( p->hStmt,
                        SQL_FETCH_ABSOLUTE,
                        currRow);
	if ( r != SQL_SUCCESS )
	    qSystemWarning("Unable to restore position", p );
    }
}

QODBCResultInfo::~QODBCResultInfo()
{
}

////////////////////////////////////////////////////////////////////////////

QODBCResult::QODBCResult( const QODBCDriver * db, QODBCPrivate* p )
: QSqlResult(db),
  resultInfo(0)
{
    d = new QODBCPrivate();
    (*d) = (*p);
}

QODBCResult::~QODBCResult()
{
    qDebug("~QODBCResult()");
    if ( d->hStmt ) {
//	SQLRETURN r = SQLFreeHandle( SQL_HANDLE_STMT, d->hStmt );
//#ifdef CHECK_RANGE
//	if ( r!= SQL_SUCCESS )
//	    qSystemWarning( "Unable to free statement handle", d );
//#endif
    	SQLRETURN r = SQLFreeStmt( d->hStmt, SQL_CLOSE );
    	if ( r != SQL_SUCCESS ) {
#ifdef CHECK_RANGE
		qSystemWarning( "Unable to close statement", d );
#endif
    	}

    }
    delete d;
    if ( resultInfo )
        delete resultInfo;
}

const QSqlResultInfo* QODBCResult::info()
{
    if ( resultInfo ) {
	delete resultInfo;
	resultInfo = 0;
    }
    if ( isActive() )
	resultInfo = new QODBCResultInfo( d );
    return resultInfo;
}

bool QODBCResult::reset ( const QString& query )
{
    setActive( FALSE );
    setAt( BeforeFirst );
    SQLRETURN r;
    if ( d->hStmt ) {
//    	r = SQLFreeHandle( SQL_HANDLE_STMT, d->hStmt );
//#ifdef CHECK_RANGE
//	if ( r!= SQL_SUCCESS )
//	    qSystemWarning( "Unable to free statement handle", d );
//#endif
    	r = SQLFreeStmt( d->hStmt, SQL_CLOSE );
    	if ( r != SQL_SUCCESS ) {
#ifdef CHECK_RANGE
		qSystemWarning( "Unable to close statement", d );
#endif
		return FALSE;
    	}
    }
    r  = SQLAllocHandle( SQL_HANDLE_STMT,
    			d->hDbc,
			&d->hStmt );
    if ( r != SQL_SUCCESS ) {
#ifdef CHECK_RANGE
	qSystemWarning( "Unable to allocate statement handle", d );
#endif
	return FALSE;
    }
    r = SQLSetStmtAttr( d->hStmt,
                        SQL_ATTR_CURSOR_TYPE,
                        (SQLPOINTER)SQL_CURSOR_STATIC,
                        SQL_IS_UINTEGER );
    if ( r != SQL_SUCCESS ) {
#ifdef CHECK_RANGE
	qSystemWarning( "Unable to set statement attribute", d );
#endif
	return FALSE;
    }
    r = SQLExecDirect( d->hStmt,
			(SQLCHAR*)query.local8Bit().data(),
 			SQL_NTS );
    if ( r == SQL_NEED_DATA ) {
	qSystemWarning( "Need data", d );
    }
    if ( r != SQL_SUCCESS ) {
	setLastError( makeError( "Unable to execute statement", QSqlError::Statement, d ) );
	return FALSE;
    }
    setActive( TRUE) ;
    return FALSE;
}

bool QODBCResult::fetch(int i)
{
    if ( i == at() )
        return TRUE;
    fieldCache.clear();
    nullCache.clear();
    int actualIdx = i + 1;
    if ( actualIdx <= 0 ) {
	setAt( BeforeFirst );
	return FALSE;
    }
    SQLRETURN r;
    r = SQLFetchScroll( d->hStmt,
                       	SQL_FETCH_ABSOLUTE,
                       	actualIdx );
    if ( r != SQL_SUCCESS )
        return FALSE;
    setAt( i );
    return TRUE;
}

bool QODBCResult::fetchFirst()
{
    SQLRETURN r;
    fieldCache.clear();
    nullCache.clear();
    r = SQLFetchScroll( d->hStmt,
                       SQL_FETCH_FIRST,
                       0 );
    if ( r != SQL_SUCCESS )
    	return FALSE;
    setAt( 0 );
    return TRUE;
}

bool QODBCResult::fetchLast()
{
    SQLRETURN r;
    fieldCache.clear();
    nullCache.clear();
    r = SQLFetchScroll( d->hStmt,
                       SQL_FETCH_LAST,
                       0 );
    if ( r != SQL_SUCCESS ) {
    	return FALSE;
    }
    SQLINTEGER currRow;
    r = SQLGetStmtAttr( d->hStmt,
    			SQL_ROW_NUMBER,
			&currRow,
			SQL_IS_INTEGER,
			0 );
    if ( r != SQL_SUCCESS )
    	return FALSE;
    setAt( currRow-1 );
    return TRUE;
}

QVariant QODBCResult::data( int field )
{
    if ( fieldCache.contains( field ) )
	return fieldCache[ field ];
    SQLRETURN r(0);
    SQLINTEGER lengthIndicator = 0;
    bool isNull = FALSE;
    int current = fieldCache.count();
    for ( ; current < (field + 1); current++ ) {
	QSqlFieldInfo info = makeFieldInfo( d, field );
	switch ( info.type ) {
	case QVariant::Int:
	    SQLINTEGER intbuf;
	    isNull = FALSE;
	    r = SQLGetData( d->hStmt,
			    current+1,
			    SQL_C_SLONG,
			    (SQLPOINTER)&intbuf,
			    0,
			    &lengthIndicator );
	    if ( r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO ) {
		if ( lengthIndicator == SQL_NULL_DATA )
		    isNull = TRUE;
	    }
	    fieldCache[ current ] = QVariant( (int)intbuf );
	    nullCache[ current ] = isNull;
	    break;
	case QVariant::Double:
	    SQLDOUBLE dblbuf;
	    isNull = FALSE;
	    r = SQLGetData( d->hStmt,
			    current+1,
			    SQL_C_DOUBLE,
			    (SQLPOINTER)&dblbuf,
			    0,
			    &lengthIndicator );
	    if ( r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO ) {
		if ( lengthIndicator == SQL_NULL_DATA )
		    isNull = TRUE;
	    }
	    fieldCache[ current ] = QVariant( dblbuf );
	    nullCache[ current ] = isNull;
	    break;
	case QVariant::Date:
	    DATE_STRUCT dbuf;
	    isNull = FALSE;
	    r = SQLGetData( d->hStmt,
			    current+1,
			    SQL_C_TYPE_DATE,
			    (SQLPOINTER)&dbuf,
			    0,
			    &lengthIndicator );
	    if ( r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO ) {
		if ( lengthIndicator == SQL_NULL_DATA )
		    isNull = TRUE;
	    }
	    fieldCache[ current ] = QVariant( QDate( dbuf.year, dbuf.month, dbuf.day ) );
	    nullCache[ current ] = isNull;
	    break;
	case QVariant::Time:
	    TIME_STRUCT tbuf;
	    isNull = FALSE;
	    r = SQLGetData( d->hStmt,
			    current+1,
			    SQL_C_TYPE_TIME,
			    (SQLPOINTER)&tbuf,
			    0,
			    &lengthIndicator );
	    if ( r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO ) {
		if ( lengthIndicator == SQL_NULL_DATA )
		    isNull = TRUE;
	    }
	    fieldCache[ current ] = QVariant( QTime( tbuf.hour, tbuf.minute, tbuf.second ) );
	    nullCache[ current ] = isNull;
	    break;
	case QVariant::DateTime:
	    TIMESTAMP_STRUCT dtbuf;
	    isNull = FALSE;
	    r = SQLGetData( d->hStmt,
			    current+1,
			    SQL_C_TYPE_TIMESTAMP,
			    (SQLPOINTER)&dtbuf,
			    0,
			    &lengthIndicator );
	    if ( r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO ) {
		if ( lengthIndicator == SQL_NULL_DATA )
		    isNull = TRUE;
	    }
	    fieldCache[ current ] = QVariant( QDateTime( QDate( dtbuf.year, dtbuf.month, dtbuf.day ), QTime( dtbuf.hour, dtbuf.minute, dtbuf.second ) ) );
	    nullCache[ current ] = isNull;
	    break;

	    //     case QSqlFieldInfo::Binary:
	    // 	return QVariant( );
	    // 	break;

	default:
	case QVariant::String:
	    SQLCHAR* buf = new SQLCHAR[info.length];
	    isNull = FALSE;
	    QString fieldVal;
	    while ( TRUE ) {
		r = SQLGetData( d->hStmt,
				current+1,
				SQL_C_CHAR,
				(SQLPOINTER)buf,
				sizeof(buf),
				&lengthIndicator );
		if ( r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO ) {
		    if ( lengthIndicator == SQL_NO_TOTAL )
			fieldVal += QString( (char*)buf );  // keep going
		    else if ( lengthIndicator == SQL_NULL_DATA ) {
			fieldVal = QString::null;
			isNull = TRUE;
			break;
		    } else {
			if ( r == SQL_SUCCESS ) {
			    fieldVal += QString( (char*)buf );
			    break;
			} else
			    fieldVal += QString( (char*)buf );
		    }
		} else {
		    fieldVal += QString::null;
		    break;
		}
	    }
	    delete buf;
	    fieldCache[ current ] = QVariant( fieldVal );
	    nullCache[ current ] = isNull;
	    break;
	}
    }
    return fieldCache[ --current ];
}

bool QODBCResult::isNull( int field ) const
{
    return nullCache[ field ];
}

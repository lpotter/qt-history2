/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qsql_sqlite.h"

#include <qcorevariant.h>
#include <qdatetime.h>
#include <qfile.h>
#include <qregexp.h>
#include <qsqlerror.h>
#include <qsqlfield.h>
#include <qsqlindex.h>
#include <qstringlist.h>

#if !defined Q_WS_WIN32
# include <unistd.h>
#endif
#include <sqlite.h>

typedef struct sqlite_vm sqlite_vm;

static QCoreVariant::Type nameToType(const QString& typeName)
{
    QString tName = typeName.toUpper();
    if (tName.startsWith(QLatin1String("INT")))
        return QCoreVariant::Int;
    if (tName.startsWith(QLatin1String("FLOAT")) || tName.startsWith(QLatin1String("NUMERIC")))
        return QCoreVariant::Double;
    if (tName.startsWith(QLatin1String("BOOL")))
        return QCoreVariant::Bool;
    // SQLite is typeless - consider everything else as string
    return QCoreVariant::String;
}

class QSQLiteDriverPrivate
{
public:
    QSQLiteDriverPrivate();
    sqlite *access;
    bool utf8;
};

QSQLiteDriverPrivate::QSQLiteDriverPrivate() : access(0)
{
    utf8 = (qstrcmp(sqlite_encoding, "UTF-8") == 0);
}

class QSQLiteResultPrivate
{
public:
    QSQLiteResultPrivate(QSQLiteResult *res);
    void cleanup();
    bool fetchNext(QSqlCachedResult::ValueCache &values, int idx, bool initialFetch);
    bool isSelect();
    // initializes the recordInfo and the cache
    void init(const char **cnames, int numCols);
    void finalize();

    QSQLiteResult* q;
    sqlite *access;

    // and we have too keep our own struct for the data (sqlite works via
    // callback.
    const char *currentTail;
    sqlite_vm *currentMachine;

    uint skippedStatus: 1; // the status of the fetchNext() that's skipped
    uint skipRow: 1; // skip the next fetchNext()?
    uint utf8: 1;
    QSqlRecord rInf;
};

static const uint initial_cache_size = 128;

QSQLiteResultPrivate::QSQLiteResultPrivate(QSQLiteResult* res) : q(res), access(0), currentTail(0),
    currentMachine(0), skippedStatus(false), skipRow(false), utf8(false)
{
}

void QSQLiteResultPrivate::cleanup()
{
    finalize();
    rInf.clear();
    currentTail = 0;
    currentMachine = 0;
    skippedStatus = false;
    skipRow = false;
    q->setAt(QSql::BeforeFirst);
    q->setActive(false);
    q->cleanup();
}

void QSQLiteResultPrivate::finalize()
{
    if (!currentMachine)
        return;

    char* err = 0;
    int res = sqlite_finalize(currentMachine, &err);
    if (err) {
        q->setLastError(QSqlError(QLatin1String("Unable to fetch results"),
                                  QString::fromAscii(err),
                                  QSqlError::StatementError, res));
        sqlite_freemem(err);
    }
    currentMachine = 0;
}

// called on first fetch
void QSQLiteResultPrivate::init(const char **cnames, int numCols)
{
    if (!cnames)
        return;

    rInf.clear();
    if (numCols <= 0)
        return;
    q->init(numCols);

    for (int i = 0; i < numCols; ++i) {
        const char* lastDot = strrchr(cnames[i], '.');
        const char* fieldName = lastDot ? lastDot + 1 : cnames[i];
        rInf.append(QSqlField(QString::fromAscii(fieldName),
                              nameToType(QString::fromAscii(cnames[i+numCols]))));
    }
}

bool QSQLiteResultPrivate::fetchNext(QSqlCachedResult::ValueCache &values, int idx, bool initialFetch)
{
    // may be caching.
    const char **fvals;
    const char **cnames;
    int colNum;
    int res;
    int i;

    if (skipRow) {
        // already fetched
        Q_ASSERT(!initialFetch);
        skipRow = false;
        return skippedStatus;
    }
    skipRow = initialFetch;

    if (!currentMachine)
        return false;

    // keep trying while busy, wish I could implement this better.
    while ((res = sqlite_step(currentMachine, &colNum, &fvals, &cnames)) == SQLITE_BUSY) {
        // sleep instead requesting result again immidiately.
#if defined Q_WS_WIN32
        Sleep(1000);
#else
        sleep(1);
#endif
    }

    switch(res) {
    case SQLITE_ROW:
        // check to see if should fill out columns
        if (rInf.isEmpty())
            // must be first call.
            init(cnames, colNum);
        if (!fvals)
            return false;
        if (idx < 0 && !initialFetch)
            return true;
        for (i = 0; i < colNum; ++i)
            values[i + idx] = utf8 ? QString::fromUtf8(fvals[i]) : QString::fromAscii(fvals[i]);
        return true;
    case SQLITE_DONE:
        if (rInf.isEmpty())
            // must be first call.
            init(cnames, colNum);
        q->setAt(QSql::AfterLast);
        return false;
    case SQLITE_ERROR:
    case SQLITE_MISUSE:
    default:
        // something wrong, don't get col info, but still return false
        finalize(); // finalize to get the error message.
        q->setAt(QSql::AfterLast);
        return false;
    }
    return false;
}

QSQLiteResult::QSQLiteResult(const QSQLiteDriver* db)
: QSqlCachedResult(db)
{
    d = new QSQLiteResultPrivate(this);
    d->access = db->d->access;
    d->utf8 = db->d->utf8;
}

QSQLiteResult::~QSQLiteResult()
{
    d->cleanup();
    delete d;
}

/*
   Execute \a query.
*/
bool QSQLiteResult::reset (const QString& query)
{
    // this is where we build a query.
    if (!driver())
        return false;
    if (!driver()-> isOpen() || driver()->isOpenError())
        return false;

    d->cleanup();

    // Um, ok.  callback based so.... pass private static function for this.
    setSelect(false);
    char *err = 0;
    int res = sqlite_compile(d->access,
                                d->utf8 ? query.utf8() : query.ascii(),
                                &(d->currentTail),
                                &(d->currentMachine),
                                &err);
    if (res != SQLITE_OK || err) {
        setLastError(QSqlError(QLatin1String("Unable to execute statement"),
                     QString::fromAscii(err),
                     QSqlError::StatementError, res));
        sqlite_freemem(err);
    }
    //if (*d->currentTail != '\000' then there is more sql to eval
    if (!d->currentMachine) {
        setActive(false);
        return false;
    }
    // we have to fetch one row to find out about
    // the structure of the result set
    d->skippedStatus = d->fetchNext(cache(), 0, true);
    setSelect(!d->rInf.isEmpty());
    setActive(true);
    return true;
}

bool QSQLiteResult::gotoNext(QSqlCachedResult::ValueCache& row, int idx)
{
    return d->fetchNext(row, idx, false);
}

int QSQLiteResult::size()
{
    return -1;
}

int QSQLiteResult::numRowsAffected()
{
    return sqlite_changes(d->access);
}

QSqlRecord QSQLiteResult::record() const
{
    if (!isActive() || !isSelect())
        return QSqlRecord();
    return d->rInf;
}

/////////////////////////////////////////////////////////

QSQLiteDriver::QSQLiteDriver(QObject * parent)
    : QSqlDriver(parent)
{
    d = new QSQLiteDriverPrivate();
}

QSQLiteDriver::QSQLiteDriver(sqlite *connection, QObject *parent)
    : QSqlDriver(parent)
{
    d = new QSQLiteDriverPrivate();
    d->access = connection;
    setOpen(true);
    setOpenError(false);
}


QSQLiteDriver::~QSQLiteDriver()
{
    delete d;
}

bool QSQLiteDriver::hasFeature(DriverFeature f) const
{
    switch (f) {
    case Transactions:
        return true;
    case Unicode:
        return d->utf8;
//   case BLOB:
    default:
        return false;
    }
}

/*
   SQLite dbs have no user name, passwords, hosts or ports.
   just file names.
*/
bool QSQLiteDriver::open(const QString & db, const QString &, const QString &, const QString &, int, const QString &)
{
    if (isOpen())
        close();

    if (db.isEmpty())
        return false;

    char* err = 0;
    d->access = sqlite_open(QFile::encodeName(db), 0, &err);
    if (err) {
        setLastError(QSqlError(QLatin1String("Error to open database"), QString::fromAscii(err),
                     QSqlError::ConnectionError));
        sqlite_freemem(err);
        err = 0;
    }

    if (d->access) {
        setOpen(true);
        setOpenError(false);
        return true;
    }
    setOpenError(true);
    return false;
}

void QSQLiteDriver::close()
{
    if (isOpen()) {
        sqlite_close(d->access);
        d->access = 0;
        setOpen(false);
        setOpenError(false);
    }
}

QSqlResult *QSQLiteDriver::createResult() const
{
    return new QSQLiteResult(this);
}

bool QSQLiteDriver::beginTransaction()
{
    if (!isOpen() || isOpenError())
        return false;

    char* err;
    int res = sqlite_exec(d->access, "BEGIN", 0, this, &err);

    if (res == SQLITE_OK)
        return true;

    setLastError(QSqlError(QLatin1String("Unable to begin transaction"),
                           QString::fromAscii(err), QSqlError::TransactionError, res));
    sqlite_freemem(err);
    return false;
}

bool QSQLiteDriver::commitTransaction()
{
    if (!isOpen() || isOpenError())
        return false;

    char* err;
    int res = sqlite_exec(d->access, "COMMIT", 0, this, &err);

    if (res == SQLITE_OK)
        return true;

    setLastError(QSqlError(QLatin1String("Unable to commit transaction"),
                QString::fromAscii(err), QSqlError::TransactionError, res));
    sqlite_freemem(err);
    return false;
}

bool QSQLiteDriver::rollbackTransaction()
{
    if (!isOpen() || isOpenError())
        return false;

    char* err;
    int res = sqlite_exec(d->access, "ROLLBACK", 0, this, &err);

    if (res == SQLITE_OK)
        return true;

    setLastError(QSqlError(QLatin1String("Unable to rollback Transaction"),
                           QString::fromAscii(err), QSqlError::TransactionError, res));
    sqlite_freemem(err);
    return false;
}

QStringList QSQLiteDriver::tables(QSql::TableType type) const
{
    QStringList res;
    if (!isOpen())
        return res;

    QSqlQuery q(createResult());
    q.setForwardOnly(true);
    if ((type & QSql::Tables) && (type & QSql::Views))
        q.exec(QLatin1String("SELECT name FROM sqlite_master WHERE type='table' OR type='view'"));
    else if (type & QSql::Tables)
        q.exec(QLatin1String("SELECT name FROM sqlite_master WHERE type='table'"));
    else if (type & QSql::Views)
        q.exec(QLatin1String("SELECT name FROM sqlite_master WHERE type='view'"));

    if (q.isActive()) {
        while(q.next())
            res.append(q.value(0).toString());
    }

    if (type & QSql::SystemTables) {
        // there are no internal tables beside this one:
        res.append(QLatin1String("sqlite_master"));
    }

    return res;
}

QSqlIndex QSQLiteDriver::primaryIndex(const QString &tblname) const
{
    QSqlRecord rec(record(tblname)); // expensive :(

    if (!isOpen())
        return QSqlIndex();

    QSqlQuery q(createResult());
    q.setForwardOnly(true);
    // finrst find a UNIQUE INDEX
    q.exec(QLatin1String("PRAGMA index_list('") + tblname + QLatin1String("');"));
    QString indexname;
    while(q.next()) {
        if (q.value(2).toInt()==1) {
            indexname = q.value(1).toString();
            break;
        }
    }
    if (indexname.isEmpty())
        return QSqlIndex();

    q.exec(QLatin1String("PRAGMA index_info('") + indexname + QLatin1String("');"));

    QSqlIndex index(indexname);
    while(q.next()) {
        QString name = q.value(2).toString();
        QCoreVariant::Type type = QCoreVariant::Invalid;
        if (rec.contains(name))
            type = rec.field(name).type();
        index.append(QSqlField(name, type));
    }
    return index;
}

QSqlRecord QSQLiteDriver::record(const QString &tbl) const
{
    if (!isOpen())
        return QSqlRecord();

    QSqlQuery q(createResult());
    q.setForwardOnly(true);
    q.exec(QLatin1String("SELECT * FROM ") + tbl + QLatin1String(" LIMIT 1"));
    return q.record();
}

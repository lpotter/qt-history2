/****************************************************************************
**
** Implementation of QPixmapCache class.
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qpixmapcache.h"
#include "qcache.h"
#include "qcleanuphandler.h"
#include "qobject.h"

/*!
    \class QPixmapCache

    \brief The QPixmapCache class provides an application-global cache for
    pixmaps.

    \ingroup environment
    \ingroup multimedia

    This class is a tool for optimized drawing with QPixmap. You can
    use it to store temporary pixmaps that are expensive to generate
    without using more storage space than cacheLimit(). Use insert()
    to insert pixmaps, find() to find them and clear() to empty the
    cache.

    For example, QRadioButton has a non-trivial visual representation
    so we don't want to regenerate a pixmap whenever a radio button is
    displayed or changes state. In the function
    QRadioButton::drawButton(), we do not draw the radio button
    directly. Instead, we first check the global pixmap cache for a
    pixmap with the key "$qt_radio_nnn_", where \c nnn is a numerical
    value that specifies the the radio button state. If a pixmap is
    found, we bitBlt() it onto the widget and return. Otherwise, we
    create a new pixmap, draw the radio button in the pixmap, and
    finally insert the pixmap in the global pixmap cache, using the
    key above. The bitBlt() is ten times faster than drawing the
    radio button. All radio buttons in the program share the cached
    pixmap since QPixmapCache is application-global.

    QPixmapCache contains no member data, only static functions to
    access the global pixmap cache. It creates an internal QCache for
    caching the pixmaps.

    The cache associates a pixmap with a string (key). If two pixmaps
    are inserted into the cache using equal keys, then the last pixmap
    will hide the first pixmap. The QHash and QCache classes do
    exactly the same.

    The cache becomes full when the total size of all pixmaps in the
    cache exceeds cacheLimit(). The initial cache limit is 1024 KByte
    (1 MByte); it is changed with setCacheLimit(). A pixmap takes
    roughly width*height*depth/8 bytes of memory.

    See the \l QCache documentation for more details about the cache
    mechanism.
*/


static int cache_limit = 1024;                        // 1024 KB cache limit

class QPMCache : public QObject, public QCache<QString, QPixmap>
{
    Q_OBJECT
public:
    QPMCache()
        : QObject(0),
          QCache<QString, QPixmap>(cache_limit * 1024),
          id(0), ps(0), t(false) { }
    ~QPMCache() { }

    void timerEvent(QTimerEvent *);
    bool insert(const QString& k, const QPixmap &d, int c);

private:
    int id;
    int ps;
    bool t;
};


/*
  This is supposed to cut the cache size down by about 80-90% in a
  minute once the application becomes idle, to let any inserted pixmap
  remain in the cache for some time before it becomes a candidate for
  cleaning-up, and to not cut down the size of the cache while the
  cache is in active use.

  When the last pixmap has been deleted from the cache, kill the
  timer so Qt won't keep the CPU from going into sleep mode.
*/

void QPMCache::timerEvent(QTimerEvent *)
{
    int mc = maxCost();
    bool nt = totalCost() == ps;
    setMaxCost(nt ? totalCost() * 3 / 4 : totalCost() -1);
    setMaxCost(mc);
    ps = totalCost();

    if (!size()) {
        killTimer(id);
        id = 0;
    } else if (nt != t) {
        killTimer(id);
        id = startTimer(nt ? 10000 : 30000);
        t = nt;
    }
}

bool QPMCache::insert(const QString& k, const QPixmap &d, int c)
{
    QCache<QString, QPixmap>::insert(k, new QPixmap(d), c);
    if (!id) {
        id = startTimer(30000);
        t = false;
    }
    return true;
}

static QPMCache *pm_cache = 0;
static QSingleCleanupHandler<QPMCache> qpm_cleanup_cache;

/*!
  \obsolete

    Returns the pixmap associated with the \a key in the cache, or
    null if there is no such pixmap.

    \warning If valid, you should copy the pixmap immediately (this is
    fast). Subsequent insertions into the cache could cause the
    pointer to become invalid. For this reason, we recommend you use
    find(const QString&, QPixmap&) instead.

    Example:
    \code
        QPixmap* pp;
        QPixmap p;
        if ((pp=QPixmapCache::find("my_big_image", pm))) {
            p = *pp;
        } else {
            p.load("bigimage.png");
            QPixmapCache::insert("my_big_image", new QPixmap(p));
        }
        painter->drawPixmap(0, 0, p);
    \endcode
*/

QPixmap *QPixmapCache::find(const QString &key)
{
    if (!pm_cache)
        return 0;
    return pm_cache->find(key);
}


/*!
    \overload

    Looks for a cached pixmap associated with the \a key in the cache.
    If a pixmap is found, the function sets \a pm to that pixmap and
    returns true; otherwise leaves \a pm alone and returns false.

    Example:
    \code
        QPixmap pm;
        if (!QPixmapCache::find("my_big_image", pm)) {
            pm.load("bigimage.png");
            QPixmapCache::insert("my_big_image", pm);
        }
        painter->drawPixmap(0, 0, pm);
    \endcode
*/

bool QPixmapCache::find(const QString &key, QPixmap& pm)
{
    if (!pm_cache)
        return false;
    QPixmap *ptr = pm_cache->find(key);
    if (ptr)
        pm = *ptr;
    return ptr != 0;
}


/*!
    \obsolete
    Inserts the pixmap \a pm associated with \a key into the cache.
    Returns true if successful, or false if the pixmap is too big for the cache.

    \warning \a pm must be allocated on the heap (using \c new).

    If this function returns false, you must delete \a pm yourself.

    If this function returns true, do not use \a pm afterwards or
    keep references to it because any other insertions into the cache,
    whether from anywhere in the application or within Qt itself, could cause
    the pixmap to be discarded from the cache and the pointer to
    become invalid.

    Due to these dangers, we strongly recommend that you use
    insert(const QString&, const QPixmap&) instead.
*/

bool QPixmapCache::insert(const QString &key, QPixmap *pm)
{
    if (!pm_cache) {
        pm_cache = new QPMCache;
        qpm_cleanup_cache.set(&pm_cache);
    }
    return pm_cache->insert(key, *pm, pm->width()*pm->height()*pm->depth() / 8);
}

/*!
    Inserts a copy of the pixmap \a pm associated with the \a key into
    the cache.

    All pixmaps inserted by the Qt library have a key starting with
    "$qt", so your own pixmap keys should never begin "$qt".

    When a pixmap is inserted and the cache is about to exceed its
    limit, it removes pixmaps until there is enough room for the
    pixmap to be inserted.

    The oldest pixmaps (least recently accessed in the cache) are
    deleted when more space is needed.

    \sa setCacheLimit().
*/

void QPixmapCache::insert(const QString &key, const QPixmap &pm)
{
    if (!pm_cache) {
        pm_cache = new QPMCache;
        qpm_cleanup_cache.set(&pm_cache);
    }
    pm_cache->insert(key, pm, pm.width() * pm.height() * pm.depth() / 8);
}

/*!
    Returns the cache limit (in kilobytes).

    The default setting is 1024 kilobytes.

    \sa setCacheLimit().
*/

int QPixmapCache::cacheLimit()
{
    return cache_limit;
}

/*!
    Sets the cache limit to \a n kilobytes.

    The default setting is 1024 kilobytes.

    \sa cacheLimit()
*/

void QPixmapCache::setCacheLimit(int n)
{
    cache_limit = n;
    if (pm_cache)
        pm_cache->setMaxCost(1024 * cache_limit);
}

/*!
  Removes the pixmap associated with \a key from the cache.
*/
void QPixmapCache::remove(const QString &key)
{
    if (pm_cache)
        pm_cache->remove(key);
}


/*!
    Removes all pixmaps from the cache.
*/

void QPixmapCache::clear()
{
    if (pm_cache)
        pm_cache->clear();
}

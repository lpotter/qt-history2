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

#include "qtextbrowser.h"
#include "qtextedit_p.h"

#include <qstack.h>
#include <qapplication.h>
#include <qevent.h>
#include <qdesktopwidget.h>
#include <qdebug.h>
#include <qabstracttextdocumentlayout.h>
#include <qurl.h>
#include "private/qtextdocumentlayout_p.h"
#include "private/qtexthtmlparser_p.h"
#include <qtextcodec.h>
#include <qpainter.h>
#include <qdir.h>
#include <qwhatsthis.h>

#define d d_func()
#define q q_func()

class QTextBrowserPrivate : public QTextEditPrivate
{
    Q_DECLARE_PUBLIC(QTextBrowser)
public:
    QTextBrowserPrivate() : textOrSourceChanged(false), forceLoadOnSourceChange(false) {}

    QStack<QString> stack;
    QStack<QString> forwardStack;
    QString home;
    QString currentURL;
    QString currentAnchor;

    /*flag necessary to give the linkClicked() signal some meaningful
      semantics when somebody connected to it calls setText() or
      setSource() */
    bool textOrSourceChanged;

    bool forceLoadOnSourceChange;

    QString resolvePath(const QString &name) const;

    inline void documentModified()
    {
        textOrSourceChanged = true;
        forceLoadOnSourceChange = true;
    }
};

static bool isAbsoluteFileName(const QString &name)
{
    return !name.isEmpty()
           && (name[0] == '/'
#if defined(Q_WS_WIN)
               || (name[0].isLetter() && name[1] == QLatin1Char(':')) || name.startsWith("\\\\")
#endif
              );

}

QString QTextBrowserPrivate::resolvePath(const QString &name) const
{
    if (isAbsoluteFileName(name))
        return name;

    if (d->stack.isEmpty())
        return name;

    QFileInfo path(QFileInfo(currentURL).absolutePath(), name);
    return path.absoluteFilePath();
}

/*!
    \class QTextBrowser qtextbrowser.h
    \brief The QTextBrowser class provides a rich text browser with hypertext navigation.

    \ingroup text

    This class extends QTextEdit (in read-only mode), adding some
    navigation functionality so that users can follow links in
    hypertext documents. The contents of QTextEdit is set with
    setText(), but QTextBrowser has an additional function,
    setSource(), which makes it possible to set the text to a named
    document. The name is looked up in the directory of the current document
    factory. If a document name ends with an anchor (for example, "\c
    #anchor"), the text browser automatically scrolls to that position
    (using scrollToAnchor()). When the user clicks on a hyperlink, the
    browser will call setSource() itself, with the link's \c href
    value as argument. You can track the current source by connetion
    to the sourceChanged() signal.

    QTextBrowser provides backward() and forward() slots which you can
    use to implement Back and Forward buttons. The home() slot sets
    the text to the very first document displayed. The linkClicked()
    signal is emitted when the user clicks a link.

    If you want to provide your users with editable rich text use
    QTextEdit. If you want a text browser without hypertext navigation
    use QTextEdit, and use QTextEdit::setReadOnly() to disable
    editing. If you just need to display a small piece of rich text
    use QSimpleRichText or QLabel.
*/

/*!
    Constructs an empty QTextBrowser with parent \a parent.
*/
QTextBrowser::QTextBrowser(QWidget *parent)
    : QTextEdit(*new QTextBrowserPrivate, parent)
{
    setReadOnly(true);
    setUndoRedoEnabled(false);
    d->viewport->setMouseTracking(true);
}

#ifdef QT_COMPAT
/*!
    Use one of the constructors that doesn't take the \a name
    argument and then use setObjectName() instead.
*/
QTextBrowser::QTextBrowser(QWidget *parent, const char *name)
    : QTextEdit(*new QTextBrowserPrivate, parent)
{
    setObjectName(name);
    setReadOnly(true);
    setUndoRedoEnabled(false);
    d->viewport->setMouseTracking(true);
    connect(document(), SIGNAL(contentsChanged()), q, SLOT(documentModified()));
}
#endif

/*!
    \internal
*/
QTextBrowser::~QTextBrowser()
{
}

/*!
    \property QTextBrowser::source
    \brief the name of the displayed document.

    This is a an empty string if no document is displayed or if the
    source is unknown.

    When setting this property QTextBrowser tries to find a document
    with the specified name in the directory of the current source,
    unless the value is an absolute file path. It also checks for
    optional anchors and scrolls the document accordingly

    If the first tag in the document is \c{<qt type=detail>}, the
    document is displayed as a popup rather than as new document in
    the browser window itself. Otherwise, the document is displayed
    normally in the text browser with the text set to the contents of
    the named document with setText().
*/
QString QTextBrowser::source() const
{
    if (d->stack.isEmpty())
        return QString::null;
    else
        return d->resolvePath(d->stack.top());
}

/*!
    Reloads the current set source.
*/
void QTextBrowser::reload()
{
    QString s = d->currentURL;
    d->currentURL = QString::null;
    setSource(s);
}

void QTextBrowser::setSource(const QString& name)
{
    qDebug() << "QTextBrowser::setSource(" << name << ")";
    if (isVisible())
        qApp->setOverrideCursor(Qt::WaitCursor);

    d->textOrSourceChanged = true;
    QString source = name;
    QString anchor;
    int hash = name.indexOf('#');
    if (hash != -1) {
        source = name.left(hash);
        anchor = name.mid(hash+1);
    }

    if (source.startsWith("file:"))
        source = source.mid(6);

    QString url = d->resolvePath(source);
    QString txt;

    bool doSetText = false;

    if (!source.isEmpty() && (url != d->currentURL || d->forceLoadOnSourceChange)) {
        QFile f(url);
        if (f.open(IO_ReadOnly)) {
            QByteArray data = f.readAll();
            QTextCodec *codec = QTextHtmlParser::codecForStream(data);
            txt = codec->toUnicode(data);

            if (txt.isEmpty())
                qWarning("QTextBrowser: no document for %s", source.latin1());
        } else {
            qWarning("QTextBrowser: cannot open '%s' for reading", url.toLocal8Bit().data());
        }

        if (isVisible()) {
            QString firstTag = txt.left(txt.indexOf('>') + 1);
            if (firstTag.left(3) == "<qt" && firstTag.contains("type") && firstTag.contains("detail")) {
                qApp->restoreOverrideCursor();
                QWhatsThis::showText(QCursor::pos(), txt, this);
                return;
            }
        }

        d->currentURL = url;
        doSetText = true;
    }
    d->forceLoadOnSourceChange = false;

    d->currentAnchor = anchor;

    if (!anchor.isEmpty()) {
        url += '#';
        url += anchor;
    }

    if (d->home.isEmpty())
        d->home = url;

    if (d->stack.isEmpty() || d->stack.top() != url)
        d->stack.push(name);

    int stackCount = d->stack.count();
    if (d->stack.top() == name)
        stackCount--;
    emit backwardAvailable(stackCount > 0);

    stackCount = d->forwardStack.count();
    if (d->forwardStack.isEmpty() || d->forwardStack.top() == name)
        stackCount--;
    emit forwardAvailable(stackCount > 0);

    if (doSetText)
        QTextEdit::setHtml(txt);

    if (!anchor.isEmpty()) {
        scrollToAnchor(anchor);
    } else {
        d->hbar->setValue(0);
        d->vbar->setValue(0);
    }

    if (isVisible())
        qApp->restoreOverrideCursor();

    emit sourceChanged(url);
}

/*!
    \fn void QTextBrowser::backwardAvailable(bool available)

    This signal is emitted when the availability of backward()
    changes. \a available is false when the user is at home();
    otherwise it is true.
*/

/*!
    \fn void QTextBrowser::forwardAvailable(bool available)

    This signal is emitted when the availability of forward() changes.
    \a available is true after the user navigates backward() and false
    when the user navigates or goes forward().
*/

/*!
    \fn void QTextBrowser::sourceChanged(const QString& src)

    This signal is emitted when the mime source has changed, \a src
    being the new source.

    Source changes happen both programmatically when calling
    setSource(), forward(), backword() or home() or when the user
    clicks on links or presses the equivalent key sequences.
*/

/*!  \fn void QTextBrowser::highlighted (const QString &link)

    This signal is emitted when the user has selected but not
    activated a link in the document. \a link is the value of the \c
    href i.e. the name of the target document.
*/

/*!
    \fn void QTextBrowser::linkClicked(const QString& link)

    This signal is emitted when the user clicks a link. The \a link is
    the value of the \c href i.e. the name of the target document.

    The \a link will be the absolute location of the document, based
    on the value of the anchor's href tag and the current context of
    the document.

    \sa anchorClicked()
*/

/*!
    \fn void QTextBrowser::anchorClicked(const QString &link)

    This signal is emitted when the user clicks an anchor. The \a link is
    the value of the \c href i.e. the name of the target document.

    \sa linkClicked()
*/

/*!
    Changes the document displayed to the previous document in the
    list of documents built by navigating links. Does nothing if there
    is no previous document.

    \sa forward(), backwardAvailable()
*/
void QTextBrowser::backward()
{
    if (d->stack.count() <= 1)
        return;
    d->forwardStack.push(d->stack.pop());
    setSource(d->stack.pop());
    emit forwardAvailable(true);
}

/*!
    Changes the document displayed to the next document in the list of
    documents built by navigating links. Does nothing if there is no
    next document.

    \sa backward(), forwardAvailable()
*/
void QTextBrowser::forward()
{
    if (d->forwardStack.isEmpty())
        return;
    setSource(d->forwardStack.pop());
    emit forwardAvailable(!d->forwardStack.isEmpty());
}

/*!
    Changes the document displayed to be the first document the
    browser displayed.
*/
void QTextBrowser::home()
{
    if (!d->home.isNull())
        setSource(d->home);
}

/*!
    The event \a ev is used to provide the following keyboard shortcuts:
    \table
    \header \i Keypress            \i Action
    \row \i Alt+Left Arrow  \i \l backward()
    \row \i Alt+Right Arrow \i \l forward()
    \row \i Alt+Up Arrow    \i \l home()
    \endtable
*/
void QTextBrowser::keyPressEvent(QKeyEvent *ev)
{
    if (ev->state() & Qt::AltButton) {
        switch (ev->key()) {
        case Qt::Key_Right:
            forward();
            ev->accept();
            return;
        case Qt::Key_Left:
            backward();
            ev->accept();
            return;
        case Qt::Key_Up:
            home();
            ev->accept();
            return;
        }
    }
    QTextEdit::keyPressEvent(ev);
}

/*!
    \reimp
*/
void QTextBrowser::mouseMoveEvent(QMouseEvent *ev)
{
    QTextEdit::mouseMoveEvent(ev);

    QString anchor = d->doc->documentLayout()->anchorAt(d->translateCoordinates(ev->pos()));
    if (anchor.isEmpty()) {
        d->viewport->setCursor(Qt::ArrowCursor);
        emit highlighted(QString::null);
    } else {
        d->viewport->setCursor(Qt::PointingHandCursor);

        QUrl url = QUrl(d->currentURL).resolved(anchor);
        emit highlighted(url.toString());
    }

}

/*!
    \reimp
*/
void QTextBrowser::mouseReleaseEvent(QMouseEvent *ev)
{
    QTextEdit::mouseReleaseEvent(ev);

    QString anchor = d->doc->documentLayout()->anchorAt(d->translateCoordinates(ev->pos()));
    if (!anchor.isEmpty()) {
        d->textOrSourceChanged = false;

        QUrl url = QUrl(d->currentURL).resolved(anchor);
        emit linkClicked(url.toString());

        // compat signal. the name is set to null. the 'name' makes no sense as it is
        // an attribute for specifying a destination.
        emit anchorClicked(QString::null, anchor);
        emit anchorClicked(anchor);

        if (!d->textOrSourceChanged)
            setSource(anchor);
    }
}

/*!
    This function is called when the document to display contains images. For each
    image loadImage is called at least once. \a name corresponds to the name attribute
    of QTextImageFormat or the source attribute of the html img tag.

    The default implementation tries to locate the image by interpreting \a name as
    a file name. If it is not an absolute path it tries to find the image in the same
    directory as the current source.
*/
QImage QTextBrowser::loadImage(const QString &name)
{
    QImage img;
    img.load(d->resolvePath(name));
    return img;
}

#include "moc_qtextbrowser.cpp"


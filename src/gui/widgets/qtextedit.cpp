/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qtextedit_p.h"
#include "qlineedit.h"

QStringList QTextEditMimeData::formats() const
{
    if (!fragment.isEmpty())
        return QStringList() << QString::fromLatin1("text/plain") << QString::fromLatin1("text/html");
    else
        return QMimeData::formats();
}

QVariant QTextEditMimeData::retrieveData(const QString &mimeType, QVariant::Type type) const
{
    if (!fragment.isEmpty())
        setup();
    return QMimeData::retrieveData(mimeType, type);
}

void QTextEditMimeData::setup() const
{
    QTextEditMimeData *that = const_cast<QTextEditMimeData *>(this);
    that->setData(QLatin1String("text/html"), fragment.toHtml("utf-8").toUtf8());
    that->setText(fragment.toPlainText());
    fragment = QTextDocumentFragment();
}

#ifndef QT_NO_TEXTEDIT

#include <qfont.h>
#include <qpainter.h>
#include <qevent.h>
#include <qdebug.h>
#include <qmime.h>
#include <qdrag.h>
#include <qclipboard.h>
#include <qmenu.h>
#include <qstyle.h>
#include <qtimer.h>
#include "private/qtextdocumentlayout_p.h"
#include "qtextdocument.h"
#include "private/qtextdocument_p.h"
#include "qtextlist.h"
#include "private/qtextcontrol_p.h"

#include <qtextformat.h>
#include <qdatetime.h>
#include <qapplication.h>
#include <limits.h>
#include <qtexttable.h>
#include <qvariant.h>

#include <qinputcontext.h>

class QTextEditControl : public QTextControl
{
public:
    inline QTextEditControl(QObject *parent) : QTextControl(parent) {}

    virtual QMimeData *createMimeDataFromSelection() const {
        QTextEdit *ed = qobject_cast<QTextEdit *>(parent());
        if (!ed)
            return QTextControl::createMimeDataFromSelection();
        return ed->createMimeDataFromSelection();
    }
    virtual bool canInsertFromMimeData(const QMimeData *source) const {
        QTextEdit *ed = qobject_cast<QTextEdit *>(parent());
        if (!ed)
            return QTextControl::canInsertFromMimeData(source);
        return ed->canInsertFromMimeData(source);
    }
    virtual void insertFromMimeData(const QMimeData *source) {
        QTextEdit *ed = qobject_cast<QTextEdit *>(parent());
        if (!ed)
            QTextControl::insertFromMimeData(source);
        else
            ed->insertFromMimeData(source);
    }
};

QTextEditPrivate::QTextEditPrivate()
    : control(0),
      autoFormatting(QTextEdit::AutoNone), tabChangesFocus(false),
      lineWrap(QTextEdit::WidgetWidth), lineWrapColumnOrWidth(0),
      ignoreAutomaticScrollbarAdjustement(false), textFormat(Qt::AutoText),
      preferRichText(false)
{}

void QTextEditPrivate::createAutoBulletList()
{
    QTextCursor cursor = control->textCursor();
    cursor.beginEditBlock();

    QTextBlockFormat blockFmt = cursor.blockFormat();

    QTextListFormat listFmt;
    listFmt.setStyle(QTextListFormat::ListDisc);
    listFmt.setIndent(blockFmt.indent() + 1);

    blockFmt.setIndent(0);
    cursor.setBlockFormat(blockFmt);

    cursor.createList(listFmt);

    cursor.endEditBlock();
    control->setTextCursor(cursor);
}

void QTextEditPrivate::init(const QString &html)
{
    Q_Q(QTextEdit);
    control = new QTextEditControl(q);
    control->setDragAndDropSource(viewport);

    QObject::connect(control, SIGNAL(microFocusChanged()), q, SLOT(_q_updateMicroFocus()));
    QObject::connect(control, SIGNAL(documentSizeChanged(QSizeF)), q, SLOT(_q_adjustScrollbars()));
    QObject::connect(control, SIGNAL(updateRequest(const QRectF &)), q, SLOT(_q_repaintContents(const QRectF &)));
    QObject::connect(control, SIGNAL(visibilityRequest(const QRectF &)), q, SLOT(_q_ensureVisible(const QRectF &)));
    QObject::connect(control, SIGNAL(currentCharFormatChanged(const QTextCharFormat &)),
                     q, SLOT(_q_currentCharFormatChanged(const QTextCharFormat &)));

    QObject::connect(control, SIGNAL(textChanged()), q, SIGNAL(textChanged()));
    QObject::connect(control, SIGNAL(undoAvailable(bool)), q, SIGNAL(undoAvailable(bool)));
    QObject::connect(control, SIGNAL(redoAvailable(bool)), q, SIGNAL(redoAvailable(bool)));
    QObject::connect(control, SIGNAL(copyAvailable(bool)), q, SIGNAL(copyAvailable(bool)));
    QObject::connect(control, SIGNAL(selectionChanged()), q, SIGNAL(selectionChanged()));
    QObject::connect(control, SIGNAL(cursorPositionChanged()), q, SIGNAL(cursorPositionChanged()));

    QTextDocument *doc = control->document();
    doc->documentLayout()->setPaintDevice(viewport);
    doc->setDefaultFont(q->font());

    if (!html.isEmpty())
        control->setHtml(html);

    hbar->setSingleStep(20);
    vbar->setSingleStep(20);

    viewport->setBackgroundRole(QPalette::Base);
    q->setAcceptDrops(true);
    q->setFocusPolicy(Qt::WheelFocus);
    q->setAttribute(Qt::WA_KeyCompression);
    q->setAttribute(Qt::WA_InputMethodEnabled);

#ifndef QT_NO_CURSOR
    viewport->setCursor(Qt::IBeamCursor);
#endif
}

void QTextEditPrivate::_q_repaintContents(const QRectF &contentsRect)
{
    const int xOffset = horizontalOffset();
    const int yOffset = verticalOffset();
    const QRect visibleRect(xOffset, yOffset, viewport->width(), viewport->height());

    QRect r = contentsRect.toRect().intersect(visibleRect);
    if (r.isEmpty())
        return;

    r.translate(-xOffset, -yOffset);
    viewport->update(r);
}

void QTextEditPrivate::pageUpDown(QTextCursor::MoveOperation op, QTextCursor::MoveMode moveMode)
{
    QTextCursor cursor = control->textCursor();
    bool moved = false;
    qreal lastY = control->cursorRect(cursor).top();
    qreal distance = 0;
    // move using movePosition to keep the cursor's x
    do {
        qreal y = control->cursorRect(cursor).top();
        distance += qAbs(y - lastY);
        lastY = y;
        moved = cursor.movePosition(op, moveMode);
    } while (moved && distance < viewport->height());

    if (moved) {
        if (op == QTextCursor::Up) {
            cursor.movePosition(QTextCursor::Down, moveMode);
            vbar->triggerAction(QAbstractSlider::SliderPageStepSub);
        } else {
            cursor.movePosition(QTextCursor::Up, moveMode);
            vbar->triggerAction(QAbstractSlider::SliderPageStepAdd);
        }
    }
    control->setTextCursor(cursor);
}

#ifndef QT_NO_SCROLLBAR
void QTextEditPrivate::_q_adjustScrollbars()
{
    if (ignoreAutomaticScrollbarAdjustement)
        return;

    QTextDocument *doc = control->document();
    QAbstractTextDocumentLayout *layout = doc->documentLayout();

    const QSize viewportSize = viewport->size();
    QSize docSize;

    if (QTextDocumentLayout *tlayout = qobject_cast<QTextDocumentLayout *>(layout)) {
        docSize = tlayout->dynamicDocumentSize().toSize();
        int percentageDone = tlayout->layoutStatus();
        // extrapolate height
        if (percentageDone > 0)
            docSize.setHeight(docSize.height() * 100 / percentageDone);
    } else {
        docSize = layout->documentSize().toSize();
    }

    hbar->setRange(0, docSize.width() - viewportSize.width());
    hbar->setPageStep(viewportSize.width());

    vbar->setRange(0, docSize.height() - viewportSize.height());
    vbar->setPageStep(viewportSize.height());

    // if we are in left-to-right mode widening the document due to
    // lazy layouting does not require a repaint. If in right-to-left
    // the scrollbar has the value zero and it visually has the maximum
    // value (it is visually at the right), then widening the document
    // keeps it at value zero but visually adjusts it to the new maximum
    // on the right, hence we need an update.
    if (q_func()->isRightToLeft())
        viewport->update();
}
#endif

// rect is in content coordinates
void QTextEditPrivate::_q_ensureVisible(const QRectF &_rect)
{
    const QRect rect = _rect.toRect();
    const int visibleWidth = viewport->width();
    const int visibleHeight = viewport->height();

    if (rect.x() < horizontalOffset())
        hbar->setValue(rect.x());
    else if (rect.x() + rect.width() > horizontalOffset() + visibleWidth)
        hbar->setValue(rect.x() + rect.width() - visibleWidth);

    if (rect.y() < verticalOffset())
        vbar->setValue(rect.y());
    else if (rect.y() + rect.height() > verticalOffset() + visibleHeight)
        vbar->setValue(rect.y() + rect.height() - visibleHeight);
}

void QTextEditPrivate::ensureViewportLayouted()
{
    QAbstractTextDocumentLayout *layout = control->document()->documentLayout();
    if (!layout)
        return;
    if (QTextDocumentLayout *tlayout = qobject_cast<QTextDocumentLayout *>(layout))
        tlayout->ensureLayouted(verticalOffset() + viewport->height());
}

/*!
    \class QTextEdit
    \brief The QTextEdit class provides a widget that is used to edit and display
    both plain and rich text.

    \ingroup text
    \mainclass

    \tableofcontents

    \section1 Introduction and Concepts

    QTextEdit is an advanced WYSIWYG viewer/editor supporting rich
    text formatting using HTML-style tags. It is optimized to handle
    large documents and to respond quickly to user input.

    QTextEdit works on paragraphs and characters. A paragraph is a
    formatted string which is word-wrapped to fit into the width of
    the widget. By default when reading plain text, one newline
    signifies a paragraph. A document consists of zero or more
    paragraphs. The words in the paragraph are aligned in accordance
    with the paragraph's alignment. Paragraphs are separated by hard
    line breaks. Each character within a paragraph has its own
    attributes, for example, font and color.

    QTextEdit can display images, lists and tables. If the text is
    too large to view within the text edit's viewport, scrollbars will
    appear. The text edit can load both plain text and HTML files (a
    subset of HTML 3.2 and 4).

    If you just need to display a small piece of rich text use QLabel.

    Note that we do not intend to add a full-featured web browser
    widget to Qt (because that would easily double Qt's size and only
    a few applications would benefit from it). The rich
    text support in Qt is designed to provide a fast, portable and
    efficient way to add reasonable online help facilities to
    applications, and to provide a basis for rich text editors.

    \section1 Using QTextEdit as a Display Widget

    QTextEdit can display a large HTML subset, including tables and
    images.

    The text is set or replaced using setHtml() which deletes any
    existing text and replaces it with the text passed in the
    setHtml() call. If you call setHtml() with legacy HTML, and then
    call text(), the text that is returned may have different markup,
    but will render the same. The entire text can be deleted with clear().

    Text itself can be inserted using the QTextCursor class or using the
    convenience functions insertHtml(), insertPlainText(), append() or
    paste(). QTextCursor is also able to insert complex objects like tables
    or lists into the document, and it deals with creating selections
    and applying changes to selected text.

    By default the text edit wraps words at whitespace to fit within
    the text edit widget. The setLineWrapMode() function is used to
    specify the kind of line wrap you want, or \l NoWrap if you don't
    want any wrapping. Call setLineWrapMode() to set a fixed pixel width
    \l FixedPixelWidth, or character column (e.g. 80 column) \l
    FixedColumnWidth with the pixels or columns specified with
    setLineWrapColumnOrWidth(). If you use word wrap to the widget's width
    \l WidgetWidth, you can specify whether to break on whitespace or
    anywhere with setWordWrapMode().

    The find() function can be used to find and select a given string
    within the text.

    \section2 Read-only Key Bindings

    When QTextEdit is used read-only the key bindings are limited to
    navigation, and text may only be selected with the mouse:
    \table
    \header \i Keypresses \i Action
    \row \i Qt::UpArrow        \i Moves one line up.
    \row \i Qt::DownArrow        \i Moves one line down.
    \row \i Qt::LeftArrow        \i Moves one character to the left.
    \row \i Qt::RightArrow        \i Moves one character to the right.
    \row \i PageUp        \i Moves one (viewport) page up.
    \row \i PageDown        \i Moves one (viewport) page down.
    \row \i Home        \i Moves to the beginning of the text.
    \row \i End                \i Moves to the end of the text.
    \row \i Alt+Wheel
         \i Scrolls the page horizontally (the Wheel is the mouse wheel).
    \row \i Ctrl+Wheel        \i Zooms the text.
    \row \i Ctrl+A            \i Selects all text.
    \endtable

    The text edit may be able to provide some meta-information. For
    example, the documentTitle() function will return the text from
    within HTML \c{<title>} tags.

    \section1 Using QTextEdit as an Editor

    All the information about using QTextEdit as a display widget also
    applies here.

    The current char format's attributes are set with setFontItalic(),
    setFontWeight(), setFontUnderline(), setFontFamily(),
    setFontPointSize(), setTextColor() and setCurrentFont(). The current
    paragraph's alignment is set with setAlignment().

    Selection of text is handled by the QTextCursor class, which provides
    functionality for creating selections, retrieving the text contents or
    deleting selections. You can retrieve the object that corresponds with
    the user-visible cursor using the textCursor() method. If you want to set
    a selection in QTextEdit just create one on a QTextCursor object and
    then make that cursor the visible cursor using setCursor(). The selection
    can be copied to the clipboard with copy(), or cut to the clipboard with
    cut(). The entire text can be selected using selectAll().

    When the cursor is moved and the underlying formatting attributes change,
    the currentCharFormatChanged() signal is emitted to reflect the new attributes
    at the new cursor position.

    QTextEdit holds a QTextDocument object which can be retrieved using the
    document() method. You can also set your own document object using setDocument().
    QTextDocument emits a textChanged() signal if the text changes and it also
    provides a isModified() function which will return true if the text has been
    modified since it was either loaded or since the last call to setModified
    with false as argument. In addition it provides methods for undo and redo.

    \section2 Editing Key Bindings

    The list of key bindings which are implemented for editing:
    \table
    \header \i Keypresses \i Action
    \row \i Backspace \i Deletes the character to the left of the cursor.
    \row \i Delete \i Deletes the character to the right of the cursor.
    \row \i Ctrl+C \i Copy the selected text to the clipboard.
    \row \i Ctrl+Insert \i Copy the selected text to the clipboard.
    \row \i Ctrl+K \i Deletes to the end of the line.
    \row \i Ctrl+V \i Pastes the clipboard text into text edit.
    \row \i Shift+Insert \i Pastes the clipboard text into text edit.
    \row \i Ctrl+X \i Deletes the selected text and copies it to the clipboard.
    \row \i Shift+Delete \i Deletes the selected text and copies it to the clipboard.
    \row \i Ctrl+Z \i Undoes the last operation.
    \row \i Ctrl+Y \i Redoes the last operation.
    \row \i LeftArrow \i Moves the cursor one character to the left.
    \row \i Ctrl+LeftArrow \i Moves the cursor one word to the left.
    \row \i RightArrow \i Moves the cursor one character to the right.
    \row \i Ctrl+RightArrow \i Moves the cursor one word to the right.
    \row \i UpArrow \i Moves the cursor one line up.
    \row \i Ctrl+UpArrow \i Moves the cursor one word up.
    \row \i DownArrow \i Moves the cursor one line down.
    \row \i Ctrl+Down Arrow \i Moves the cursor one word down.
    \row \i PageUp \i Moves the cursor one page up.
    \row \i PageDown \i Moves the cursor one page down.
    \row \i Home \i Moves the cursor to the beginning of the line.
    \row \i Ctrl+Home \i Moves the cursor to the beginning of the text.
    \row \i End \i Moves the cursor to the end of the line.
    \row \i Ctrl+End \i Moves the cursor to the end of the text.
    \row \i Alt+Wheel \i Scrolls the page horizontally (the Wheel is the mouse wheel).
    \row \i Ctrl+Wheel \i Zooms the text.
    \endtable

    To select (mark) text hold down the Shift key whilst pressing one
    of the movement keystrokes, for example, \e{Shift+Right Arrow}
    will select the character to the right, and \e{Shift+Ctrl+Right
    Arrow} will select the word to the right, etc.

    \sa QTextDocument, QTextCursor, {Application Example}, {Syntax Highlighter Example}
*/

/*!
    \property QTextEdit::undoRedoEnabled
    \brief whether undo and redo are enabled

    Users are only able to undo or redo actions if this property is
    true, and if there is an action that can be undone (or redone).
*/

/*!
    \enum QTextEdit::LineWrapMode

    \value NoWrap
    \value WidgetWidth
    \value FixedPixelWidth
    \value FixedColumnWidth
*/

/*!
    \enum QTextEdit::AutoFormattingFlag

    \value AutoNone Don't do any automatic formatting.
    \value AutoBulletList Automatically create bullet lists (e.g. when
    the user enters an asterisk ('*') in the left most column, or
    presses Enter in an existing list item.
    \value AutoAll Apply all automatic formatting. Currently only
    automatic bullet lists are supported.
*/

/*!
    \enum QTextEdit::CursorAction

    \value MoveBackward
    \value MoveForward
    \value MoveWordBackward
    \value MoveWordForward
    \value MoveUp
    \value MoveDown
    \value MoveLineStart
    \value MoveLineEnd
    \value MoveHome
    \value MoveEnd
    \value MovePageUp
    \value MovePageDown

    \omitvalue MovePgUp
    \omitvalue MovePgDown
*/

/*!
    Constructs an empty QTextEdit with parent \a
    parent.
*/
QTextEdit::QTextEdit(QWidget *parent)
    : QAbstractScrollArea(*new QTextEditPrivate, parent)
{
    Q_D(QTextEdit);
    d->init();
}

/*!
    \internal
*/
QTextEdit::QTextEdit(QTextEditPrivate &dd, QWidget *parent)
    : QAbstractScrollArea(dd, parent)
{
    Q_D(QTextEdit);
    d->init();
}

/*!
    Constructs a QTextEdit with parent \a parent. The text edit will display
    the text \a text. The text is interpreted as html.
*/
QTextEdit::QTextEdit(const QString &text, QWidget *parent)
    : QAbstractScrollArea(*new QTextEditPrivate, parent)
{
    Q_D(QTextEdit);
    d->init(text);
}

#ifdef QT3_SUPPORT
/*!
    Use one of the constructors that doesn't take the \a name
    argument and then use setObjectName() instead.
*/
QTextEdit::QTextEdit(QWidget *parent, const char *name)
    : QAbstractScrollArea(*new QTextEditPrivate, parent)
{
    Q_D(QTextEdit);
    d->init();
    setObjectName(QString::fromAscii(name));
}
#endif


/*!
    Destructor.
*/
QTextEdit::~QTextEdit()
{
}

/*!
    Returns the point size of the font of the current format.

    \sa setFontFamily() setCurrentFont() setFontPointSize()
*/
qreal QTextEdit::fontPointSize() const
{
    Q_D(const QTextEdit);
    return d->control->textCursor().charFormat().fontPointSize();
}

/*!
    Returns the font family of the current format.

    \sa setFontFamily() setCurrentFont() setFontPointSize()
*/
QString QTextEdit::fontFamily() const
{
    Q_D(const QTextEdit);
    return d->control->textCursor().charFormat().fontFamily();
}

/*!
    Returns the font weight of the current format.

    \sa setFontWeight() setCurrentFont() setFontPointSize() QFont::Weight
*/
int QTextEdit::fontWeight() const
{
    Q_D(const QTextEdit);
    return d->control->textCursor().charFormat().fontWeight();
}

/*!
    Returns true if the font of the current format is underlined; otherwise returns
    false.

    \sa setFontUnderline()
*/
bool QTextEdit::fontUnderline() const
{
    Q_D(const QTextEdit);
    return d->control->textCursor().charFormat().fontUnderline();
}

/*!
    Returns true if the font of the current format is italic; otherwise returns
    false.

    \sa setFontItalic()
*/
bool QTextEdit::fontItalic() const
{
    Q_D(const QTextEdit);
    return d->control->textCursor().charFormat().fontItalic();
}

/*!
    Returns the text color of the current format.

    \sa setTextColor()
*/
QColor QTextEdit::textColor() const
{
    Q_D(const QTextEdit);
    return d->control->textCursor().charFormat().foreground().color();
}

/*!
    Returns the font of the current format.

    \sa setCurrentFont() setFontFamily() setFontPointSize()
*/
QFont QTextEdit::currentFont() const
{
    Q_D(const QTextEdit);
    return d->control->textCursor().charFormat().font();
}

/*!
    Sets the alignment of the current paragraph to \a a. Valid
    alignments are Qt::AlignLeft, Qt::AlignRight,
    Qt::AlignJustify and Qt::AlignCenter (which centers
    horizontally).
*/
void QTextEdit::setAlignment(Qt::Alignment a)
{
    Q_D(QTextEdit);
    QTextBlockFormat fmt;
    fmt.setAlignment(a);
    QTextCursor cursor = d->control->textCursor();
    cursor.mergeBlockFormat(fmt);
    d->control->setTextCursor(cursor);
}

/*!
    Returns the alignment of the current paragraph.

    \sa setAlignment()
*/
Qt::Alignment QTextEdit::alignment() const
{
    Q_D(const QTextEdit);
    return d->control->textCursor().blockFormat().alignment();
}

/*!
    Makes \a document the new document of the text editor.

    The parent QObject of the provided document remains the owner
    of the object. If the current document is a child of the text
    editor, then it is deleted.

    \sa document()
*/
void QTextEdit::setDocument(QTextDocument *document)
{
    Q_D(QTextEdit);
    d->control->setDocument(document);
    d->relayoutDocument();
}

/*!
    Returns a pointer to the underlying document.

    \sa setDocument()
*/
QTextDocument *QTextEdit::document() const
{
    Q_D(const QTextEdit);
    return d->control->document();
}

/*!
    Sets the visible \a cursor.
*/
void QTextEdit::setTextCursor(const QTextCursor &cursor)
{
    Q_D(QTextEdit);
    d->control->setTextCursor(cursor);
}

/*!
    Returns a copy of the QTextCursor that represents the currently visible cursor.
    Note that changes on the returned cursor do not affect QTextEdit's cursor; use
    setTextCursor() to update the visible cursor.
 */
QTextCursor QTextEdit::textCursor() const
{
    Q_D(const QTextEdit);
    return d->control->textCursor();
}

/*!
    Sets the font family of the current format to \a fontFamily.

    \sa fontFamily() setCurrentFont()
*/
void QTextEdit::setFontFamily(const QString &fontFamily)
{
    QTextCharFormat fmt;
    fmt.setFontFamily(fontFamily);
    mergeCurrentCharFormat(fmt);
}

/*!
    Sets the point size of the current format to \a s.

    Note that if \a s is zero or negative, the behavior of this
    function is not defined.

    \sa fontPointSize() setCurrentFont() setFontFamily()
*/
void QTextEdit::setFontPointSize(qreal s)
{
    QTextCharFormat fmt;
    fmt.setFontPointSize(s);
    mergeCurrentCharFormat(fmt);
}

/*!
    \fn void QTextEdit::setFontWeight(int weight)

    Sets the font weight of the current format to the given \a weight,
    where the value used is in the range defined by the QFont::Weight
    enum.

    \sa fontWeight(), setCurrentFont(), setFontFamily()
*/
void QTextEdit::setFontWeight(int w)
{
    QTextCharFormat fmt;
    fmt.setFontWeight(w);
    mergeCurrentCharFormat(fmt);
}

/*!
    If \a underline is true, sets the current format to underline;
    otherwise sets the current format to non-underline.

    \sa fontUnderline()
*/
void QTextEdit::setFontUnderline(bool underline)
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(underline);
    mergeCurrentCharFormat(fmt);
}

/*!
    If \a italic is true, sets the current format to italic;
    otherwise sets the current format to non-italic.

    \sa fontItalic()
*/
void QTextEdit::setFontItalic(bool italic)
{
    QTextCharFormat fmt;
    fmt.setFontItalic(italic);
    mergeCurrentCharFormat(fmt);
}

/*!
    Sets the text color of the current format to \a c.

    \sa textColor()
*/
void QTextEdit::setTextColor(const QColor &c)
{
    QTextCharFormat fmt;
    fmt.setForeground(QBrush(c));
    mergeCurrentCharFormat(fmt);
}

/*!
    Sets the font of the current format to \a f.

    \sa currentFont() setFontPointSize() setFontFamily()
*/
void QTextEdit::setCurrentFont(const QFont &f)
{
    QTextCharFormat fmt;
    fmt.setFont(f);
    mergeCurrentCharFormat(fmt);
}

/*!
    \fn void QTextEdit::undo() const

    Undoes the last operation.

    If there is no operation to undo, i.e. there is no undo step in
    the undo/redo history, nothing happens.

    \sa redo()
*/
void QTextEdit::undo()
{
    Q_D(QTextEdit);
    d->control->undo();
}

void QTextEdit::redo()
{
    Q_D(QTextEdit);
    d->control->redo();
}

/*!
    \fn void QTextEdit::redo() const

    Redoes the last operation.

    If there is no operation to redo, i.e. there is no redo step in
    the undo/redo history, nothing happens.

    \sa undo()
*/

#ifndef QT_NO_CLIPBOARD
/*!
    Copies the selected text to the clipboard and deletes it from
    the text edit.

    If there is no selected text nothing happens.

    \sa copy() paste()
*/

void QTextEdit::cut()
{
    Q_D(QTextEdit);
    d->control->cut();
}

/*!
    Copies any selected text to the clipboard.

    \sa copyAvailable()
*/

void QTextEdit::copy()
{
    Q_D(QTextEdit);
    d->control->copy();
}

/*!
    Pastes the text from the clipboard into the text edit at the
    current cursor position.

    If there is no text in the clipboard nothing happens.

    To change the behavior of this function, i.e. to modify what
    QTextEdit can paste and how it is being pasted, reimplement the
    virtual canInsertFromMimeData() and insertFromMimeData()
    functions.

    \sa cut() copy()
*/

void QTextEdit::paste()
{
    Q_D(QTextEdit);
    d->control->paste();
}
#endif

/*!
    Deletes all the text in the text edit.

    Note that the undo/redo history is cleared by this function.

    \sa cut() setPlainText() setHtml()
*/
void QTextEdit::clear()
{
    Q_D(QTextEdit);
    // clears and sets empty content
    d->control->clear();
}


/*!
    Selects all text.

    \sa copy() cut() textCursor()
 */
void QTextEdit::selectAll()
{
    Q_D(QTextEdit);
    d->control->selectAll();
}

/*! \internal
*/
bool QTextEdit::event(QEvent *e)
{
    Q_D(QTextEdit);
    if (e->type() == QEvent::ContextMenu
        && static_cast<QContextMenuEvent *>(e)->reason() == QContextMenuEvent::Keyboard) {
        Q_D(QTextEdit);
        ensureCursorVisible();
        const QPoint cursorPos = cursorRect().center();
        QContextMenuEvent ce(QContextMenuEvent::Keyboard, cursorPos, d->viewport->mapToGlobal(cursorPos));
        ce.setAccepted(e->isAccepted());
        const bool result = QAbstractScrollArea::event(&ce);
        e->setAccepted(ce.isAccepted());
        return result;
    } else if (e->type() == QEvent::ShortcutOverride) {
        d->control->event(e);
    }
    return QAbstractScrollArea::event(e);
}

/*! \internal
*/

void QTextEdit::timerEvent(QTimerEvent *e)
{
    Q_D(QTextEdit);
    if (e->timerId() == d->autoScrollTimer.timerId()) {
        const QPoint globalPos = QCursor::pos();
        const QPoint pos = d->viewport->mapFromGlobal(globalPos);
        QMouseEvent ev(QEvent::MouseMove, pos, globalPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        mouseMoveEvent(&ev);
    }
#ifdef QT_KEYPAD_NAVIGATION
    else if (e->timerId() == d->deleteAllTimer.timerId()) {
        d->deleteAllTimer.stop();
        clear();
    }
#endif
}

/*!
    Changes the text of the text edit to the string \a text.
    Any previous text is removed.

    \a text is interpreted as plain text.

    Note that the undo/redo history is cleared by this function.

    \sa toPlainText()
*/

void QTextEdit::setPlainText(const QString &text)
{
    Q_D(QTextEdit);
    d->control->setPlainText(text);
    d->preferRichText = false;
}

/*!
    \fn QString QTextEdit::toPlainText() const

    Returns the text of the text edit as plain text.

    \sa QTextEdit::setPlainText()
 */


/*!
    \property QTextEdit::html

    This property provides an HTML interface to the text of the text edit.

    toHtml() returns the text of the text edit as html.

    setHtml() changes the text of the text edit.  Any previous text is
    removed. The input text is interpreted as rich text in html format.

    Note that the undo/redo history is cleared by calling setHtml().

    \sa {Supported HTML Subset}
*/

void QTextEdit::setHtml(const QString &text)
{
    Q_D(QTextEdit);
    d->control->setHtml(text);
    d->preferRichText = true;
}

/*! \reimp
*/
void QTextEdit::keyPressEvent(QKeyEvent *e)
{
    Q_D(QTextEdit);

    if (d->control->isReadOnly()) {
        switch (e->key()) {
            case Qt::Key_Home:
                e->accept();
                d->vbar->triggerAction(QAbstractSlider::SliderToMinimum);
                break;
            case Qt::Key_End:
                e->accept();
                d->vbar->triggerAction(QAbstractSlider::SliderToMaximum);
                break;
            case Qt::Key_Space:
                e->accept();
                if (e->modifiers() & Qt::ShiftModifier)
                    d->vbar->triggerAction(QAbstractSlider::SliderPageStepSub);
                else
                    d->vbar->triggerAction(QAbstractSlider::SliderPageStepAdd);
            default:
                d->control->event(e);
                if (!e->isAccepted()) {
                    QAbstractScrollArea::keyPressEvent(e);
                }
                break;
        }
        return;
    }
    switch (e->key()) {
        case Qt::Key_PageDown:
            e->accept();
            d->pageUpDown(QTextCursor::Down, e->modifiers() & Qt::ShiftModifier
                                             ? QTextCursor::KeepAnchor
                                             : QTextCursor::MoveAnchor);
            return;
        case Qt::Key_PageUp:
            e->accept();
            d->pageUpDown(QTextCursor::Up, e->modifiers() & Qt::ShiftModifier
                                           ? QTextCursor::KeepAnchor
                                           : QTextCursor::MoveAnchor);
            return;
#ifdef QT_KEYPAD_NAVIGATION
        case Qt::Key_Select:
            if (QApplication::keypadNavigationEnabled())
                setEditFocus(!hasEditFocus());
            break;
        case Qt::Key_Back:
        case Qt::Key_No:
            if (!QApplication::keypadNavigationEnabled()
                    || (QApplication::keypadNavigationEnabled() && !hasEditFocus())) {
                e->ignore();
                return;
            }
            break;
#endif
        default:
#ifdef QT_KEYPAD_NAVIGATION
            if (QApplication::keypadNavigationEnabled()) {
                if (!hasEditFocus() && !(e->modifiers() & Qt::ControlModifier)) {
                    if (e->text()[0].isPrint()) {
                        setEditFocus(true);
                        clear();
                    } else {
                        e->ignore();
                        return;
                    }
                }
            }
#endif
            break;
    }

    {
        QTextCursor cursor = d->control->textCursor();
        const QString text = e->text();
        if (cursor.atBlockStart()
            && (d->autoFormatting & AutoBulletList)
            && (text.length() == 1)
            && (text.at(0) == QLatin1Char('-') || text.at(0) == QLatin1Char('*'))
            && (!cursor.currentList())) {

            d->createAutoBulletList();
            e->accept();
            return;
        }
    }

    d->control->event(e);
#ifdef QT_KEYPAD_NAVIGATION
    if (!e->isAccepted()) {
        switch (e->key()) {
            case Qt::Key_Up:
            case Qt::Key_Down:
                if (QApplication::keypadNavigationEnabled()) {
                    // Cursor position didn't change, so we want to leave
                    // these keys to change focus.
                    e->ignore();
                    return;
                }
                break;
            case Qt::Key_Back:
                if (!e->isAutoRepeat()) {
                    if (QApplication::keypadNavigationEnabled()) {
                        if (document()->isEmpty()) {
                            setEditFocus(false);
                        } else if (!d->deleteAllTimer.isActive()) {
                            d->deleteAllTimer.start(750, this);
                        }
                    } else {
                        e->ignore();
                        return;
                    }
                }
                break;
            default: break;
        }
    }
#endif
}

/*! \reimp
*/
void QTextEdit::keyReleaseEvent(QKeyEvent *e)
{
#ifdef QT_KEYPAD_NAVIGATION
    Q_D(QTextEdit);
    if (QApplication::keypadNavigationEnabled()) {
        if (!e->isAutoRepeat() && e->key() == Qt::Key_Back
            && d->deleteAllTimer.isActive()) {
            d->deleteAllTimer.stop();
            QTextCursor cursor = d->control->textCursor();
            QTextBlockFormat blockFmt = cursor.blockFormat();

            QTextList *list = cursor.currentList();
            if (list && cursor.atBlockStart()) {
                list->remove(cursor.block());
            } else if (cursor.atBlockStart() && blockFmt.indent() > 0) {
                blockFmt.setIndent(blockFmt.indent() - 1);
                cursor.setBlockFormat(blockFmt);
            } else {
                cursor.deletePreviousChar();
            }
        }
    }
#else
    Q_UNUSED(e);
#endif
}

/*!
    Loads the resource specified by the given \a type and \a name.

    This function is an extension of QTextDocument::loadResource().

    \sa QTextDocument::loadResource()
*/
QVariant QTextEdit::loadResource(int type, const QUrl &name)
{
    Q_UNUSED(type);
    Q_UNUSED(name);
    return QVariant();
}

/*! \reimp
*/
void QTextEdit::resizeEvent(QResizeEvent *e)
{
    Q_D(QTextEdit);
    if (d->lineWrap == WidgetWidth) {
        if (e->oldSize().width() == e->size().width()
            && e->oldSize().height() != e->size().height())
            d->_q_adjustScrollbars();
        else
            d->relayoutDocument();
    } else {
        d->_q_adjustScrollbars();
    }
}

void QTextEditPrivate::relayoutDocument()
{
    QTextDocument *doc = control->document();
    QAbstractTextDocumentLayout *layout = doc->documentLayout();

    if (QTextDocumentLayout *tlayout = qobject_cast<QTextDocumentLayout *>(layout)) {
        if (lineWrap == QTextEdit::FixedColumnWidth)
            tlayout->setFixedColumnWidth(lineWrapColumnOrWidth);
        else
            tlayout->setFixedColumnWidth(-1);
    }

    QTextDocumentLayout *tlayout = qobject_cast<QTextDocumentLayout *>(layout);
    QSize lastUsedSize;
    if (tlayout)
        lastUsedSize = tlayout->dynamicDocumentSize().toSize();
    else
        lastUsedSize = layout->documentSize().toSize();

    // ignore calls to _q_adjustScrollbars caused by an emission of the
    // usedSizeChanged() signal in the layout, as we're calling it
    // later on our own anyway (or deliberately not) .
    ignoreAutomaticScrollbarAdjustement = true;

    int width = 0;
    switch (lineWrap) {
        case QTextEdit::NoWrap:
            width = -1;
            break;
        case QTextEdit::WidgetWidth:
            width = viewport->width();
            break;
        case QTextEdit::FixedPixelWidth:
            width = lineWrapColumnOrWidth;
            break;
        case QTextEdit::FixedColumnWidth:
            width = 0;
            break;
    }

    doc->setPageSize(QSize(width, INT_MAX));
    if (tlayout)
        tlayout->ensureLayouted(verticalOffset() + viewport->height());

    ignoreAutomaticScrollbarAdjustement = false;

    QSize usedSize;
    if (tlayout)
        usedSize = tlayout->dynamicDocumentSize().toSize();
    else
        usedSize = layout->documentSize().toSize();

    // this is an obscure situation in the layout that can happen:
    // if a character at the end of a line is the tallest one and therefore
    // influencing the total height of the line and the line right below it
    // is always taller though, then it can happen that if due to line breaking
    // that tall character wraps into the lower line the document not only shrinks
    // horizontally (causing the character to wrap in the first place) but also
    // vertically, because the original line is now smaller and the one below kept
    // its size. So a layout with less width _can_ take up less vertical space, too.
    // If the wider case causes a vertical scrollbar to appear and the narrower one
    // (narrower because the vertical scrollbar takes up horizontal space)) to disappear
    // again then we have an endless loop, as _q_adjustScrollBars sets new ranges on the
    // scrollbars, the QAbstractScrollArea will find out about it and try to show/hide the scrollbars
    // again. That's why we try to detect this case here and break out.
    //
    // (if you change this please also check the layoutingLoop() testcase in
    // QTextEdit's autotests)
    if (lastUsedSize.isValid()
        && !vbar->isHidden()
        && viewport->width() < lastUsedSize.width()
        && usedSize.height() < lastUsedSize.height()
        && usedSize.height() <= viewport->height())
        return;

    _q_adjustScrollbars();
}

void QTextEditPrivate::paint(QPainter *p, QPaintEvent *e)
{
    const int xOffset = horizontalOffset();
    const int yOffset = verticalOffset();

    QRect r = e->rect();
    p->translate(-xOffset, -yOffset);
    r.translate(xOffset, yOffset);

    control->drawContents(p, r);
}

/*! \reimp
*/
void QTextEdit::paintEvent(QPaintEvent *e)
{
    Q_D(QTextEdit);
    QPainter p(d->viewport);
    d->paint(&p, e);
}

void QTextEditPrivate::_q_currentCharFormatChanged(const QTextCharFormat &fmt)
{
    Q_Q(QTextEdit);
    emit q->currentCharFormatChanged(fmt);
#ifdef QT3_SUPPORT
    // compat signals
    emit q->currentFontChanged(fmt.font());
    emit q->currentColorChanged(fmt.foreground().color());
#endif
}

void QTextEditPrivate::sendTranslatedMouseEvent(QMouseEvent *e)
{
    const QPoint translatedOffset(e->pos().x() + horizontalOffset(),
                                  e->pos().y() + verticalOffset());
    QMouseEvent translatedEvent(e->type(), translatedOffset,
                                e->globalPos(), e->button(), e->buttons(), e->modifiers());
    control->event(&translatedEvent);
}

/*! \reimp
*/
void QTextEdit::mousePressEvent(QMouseEvent *e)
{
    Q_D(QTextEdit);
    // ### IM,
    d->sendTranslatedMouseEvent(e);
}

/*! \reimp
*/
void QTextEdit::mouseMoveEvent(QMouseEvent *e)
{
    Q_D(QTextEdit);
    const QPoint pos = e->pos();
    // ### IM
    d->sendTranslatedMouseEvent(e);
    if (!(e->buttons() & Qt::LeftButton))
        return;
    if (d->autoScrollTimer.isActive()) {
        if (d->viewport->rect().contains(pos))
            d->autoScrollTimer.stop();
    } else {
        if (!d->viewport->rect().contains(pos))
            d->autoScrollTimer.start(100, this);
    }
}

/*! \reimp
*/
void QTextEdit::mouseReleaseEvent(QMouseEvent *e)
{
    Q_D(QTextEdit);
    d->autoScrollTimer.stop();
    // ### IM
    d->sendTranslatedMouseEvent(e);
}

/*! \reimp
*/
void QTextEdit::mouseDoubleClickEvent(QMouseEvent *e)
{
    Q_D(QTextEdit);
    // ### IM
    d->sendTranslatedMouseEvent(e);
}

/*! \reimp
*/
bool QTextEdit::focusNextPrevChild(bool next)
{
    Q_D(const QTextEdit);
    if (!d->tabChangesFocus && !d->control->isReadOnly())
        return false;
    return QAbstractScrollArea::focusNextPrevChild(next);
}

/*!
  Shows the standard context menu created with createStandardContextMenu().

  If you do not want the text edit to have a context menu, you can set
  its \l contextMenuPolicy to Qt::NoContextMenu. If you want to
  customize the context menu, reimplement this function. If you want
  to extend the standard context menu, reimplement this function, call
  createStandardContextMenu() and extend the menu returned.

  Information about the event is passed in \a e.

    \code
    void TextEdit::contextMenuEvent(QContextMenuEvent * e) {
            QMenu *menu = createStandardContextMenu();
            menu->addAction(My Menu Item");
            //...
            menu->exec(e->globalPos());
            delete menu;
    }
    \endcode
*/
void QTextEdit::contextMenuEvent(QContextMenuEvent *e)
{
    Q_D(QTextEdit);
    QContextMenuEvent translatedEvent(e->reason(), d->mapToContents(e->pos()), e->globalPos());
    d->control->event(&translatedEvent);
}

#ifndef QT_NO_DRAGANDDROP
/*! \reimp
*/
void QTextEdit::dragEnterEvent(QDragEnterEvent *e)
{
    Q_D(QTextEdit);
    d->control->event(e);
}

/*! \reimp
*/
void QTextEdit::dragLeaveEvent(QDragLeaveEvent *e)
{
    Q_D(QTextEdit);
    d->control->event(e);
}

class QPublicDropEvent : public QDropEvent
{
public:
    inline void setPos(const QPoint &pos)
    { p = pos; }
};

/*! \reimp
*/
void QTextEdit::dragMoveEvent(QDragMoveEvent *e)
{
    Q_D(QTextEdit);
    reinterpret_cast<QPublicDropEvent *>(e)->setPos(d->mapToContents(e->pos()));
    d->control->event(e);
}

/*! \reimp
*/
void QTextEdit::dropEvent(QDropEvent *e)
{
    Q_D(QTextEdit);
    static_cast<QPublicDropEvent *>(e)->setPos(d->mapToContents(e->pos()));
    d->control->event(e);
}

#endif // QT_NO_DRAGANDDROP

/*! \reimp
 */
void QTextEdit::inputMethodEvent(QInputMethodEvent *e)
{
    Q_D(QTextEdit);
#ifdef QT_KEYPAD_NAVIGATION
    if (!d->control->isReadOnly()
        && QApplication::keypadNavigationEnabled()
        && !hasEditFocus())
        setEditFocus(true);
#endif
    d->control->event(e);
}

/*!\reimp
*/
void QTextEdit::scrollContentsBy(int dx, int dy)
{
    Q_D(QTextEdit);
    if (isRightToLeft())
        dx = -dx;
    d->viewport->scroll(dx, dy);
}

/*!\reimp
*/
QVariant QTextEdit::inputMethodQuery(Qt::InputMethodQuery property) const
{
    Q_D(const QTextEdit);
    QVariant v = d->control->inputMethodQuery(property);
    const QPoint offset(-d->horizontalOffset(), -d->verticalOffset());
    if (v.type() == QVariant::RectF)
        v = v.toRectF().toRect().translated(offset);
    else if (v.type() == QVariant::PointF)
        v = v.toPointF().toPoint() + offset;
    else if (v.type() == QVariant::Rect)
        v = v.toRect().translated(offset);
    else if (v.type() == QVariant::Point)
        v = v.toPoint() + offset;
    return v;
}

/*! \reimp
*/
void QTextEdit::focusInEvent(QFocusEvent *e)
{
    Q_D(QTextEdit);
    d->control->event(e);
}

/*! \reimp
*/
void QTextEdit::focusOutEvent(QFocusEvent *e)
{
    Q_D(QTextEdit);
    d->control->event(e);
}

/*! \reimp
*/
void QTextEdit::showEvent(QShowEvent *)
{
    Q_D(QTextEdit);
    if (!d->anchorToScrollToWhenVisible.isEmpty()) {
        scrollToAnchor(d->anchorToScrollToWhenVisible);
        d->anchorToScrollToWhenVisible.clear();
    }
}

/*! \reimp
*/
void QTextEdit::changeEvent(QEvent *e)
{
    Q_D(QTextEdit);
    QAbstractScrollArea::changeEvent(e);
    if (e->type() == QEvent::ApplicationFontChange
        || e->type() == QEvent::FontChange) {
        d->control->document()->setDefaultFont(font());
    }  else if(e->type() == QEvent::ActivationChange) {
        if (!isActiveWindow())
            d->autoScrollTimer.stop();
    } else if (e->type() == QEvent::EnabledChange) {
        e->setAccepted(isEnabled());
        d->control->event(e);
    }
}

/*! \reimp
*/
#ifndef QT_NO_WHEELEVENT
void QTextEdit::wheelEvent(QWheelEvent *e)
{
    Q_D(QTextEdit);
    if (d->control->isReadOnly()) {
        if (e->modifiers() & Qt::ControlModifier) {
            const int delta = e->delta();
            if (delta > 0)
                zoomOut();
            else if (delta < 0)
                zoomIn();
            return;
        }
    }
    QAbstractScrollArea::wheelEvent(e);
    updateMicroFocus();
}
#endif

#ifndef QT_NO_CONTEXTMENU
/*!  This function creates the standard context menu which is shown
  when the user clicks on the line edit with the right mouse
  button. It is called from the default contextMenuEvent() handler.
  The popup menu's ownership is transferred to the caller.
*/

QMenu *QTextEdit::createStandardContextMenu()
{
    Q_D(QTextEdit);
    return d->control->createStandardContextMenu();
}
#endif // QT_NO_CONTEXTMENU

/*!
  returns a QTextCursor at position \a pos (in viewport coordinates).
*/
QTextCursor QTextEdit::cursorForPosition(const QPoint &pos) const
{
    Q_D(const QTextEdit);
    return d->control->cursorForPosition(d->mapToContents(pos));
}

/*!
  returns a rectangle (in viewport coordinates) that includes the
  \a cursor.
 */
QRect QTextEdit::cursorRect(const QTextCursor &cursor) const
{
    Q_D(const QTextEdit);
    if (cursor.isNull())
        return QRect();

    QRect r = d->control->cursorRect(cursor).toRect();
    r.translate(-d->horizontalOffset(),-d->verticalOffset());
    return r;
}

/*!
  returns a rectangle (in viewport coordinates) that includes the
  cursor of the text edit.
 */
QRect QTextEdit::cursorRect() const
{
    Q_D(const QTextEdit);
    QRect r = d->control->cursorRect().toRect();
    r.translate(-d->horizontalOffset(),-d->verticalOffset());
    return r;
}


/*!
    Returns the reference of the anchor at position \a pos, or an
    empty string if no anchor exists at that point.
*/
QString QTextEdit::anchorAt(const QPoint& pos) const
{
    Q_D(const QTextEdit);
    return d->control->anchorAt(d->mapToContents(pos));
}

/*!
   \property QTextEdit::overwriteMode
   \since 4.1
*/

bool QTextEdit::overwriteMode() const
{
    Q_D(const QTextEdit);
    return d->control->overwriteMode();
}

void QTextEdit::setOverwriteMode(bool overwrite)
{
    Q_D(QTextEdit);
    d->control->setOverwriteMode(overwrite);
}

/*!
    \property QTextEdit::tabStopWidth
    \brief the tab stop width in pixels
    \since 4.1
*/

int QTextEdit::tabStopWidth() const
{
    Q_D(const QTextEdit);
    return d->control->tabStopWidth();
}

void QTextEdit::setTabStopWidth(int width)
{
    Q_D(QTextEdit);
    d->control->setTabStopWidth(width);
}

/*!
    \since 4.2
    \property QTextEdit::cursorWidth

    This property specifies the width of the cursor in pixels. The default value is 1.
*/
int QTextEdit::cursorWidth() const
{
    Q_D(const QTextEdit);
    return d->control->cursorWidth();
}

void QTextEdit::setCursorWidth(int width)
{
    Q_D(QTextEdit);
    d->control->setCursorWidth(width);
}

/*!
    \property QTextEdit::acceptRichText
    \brief whether the text edit accepts rich text insertions by the user
    \since 4.1

    When this propery is set to false text edit will accept only
    plain text input from the user. For example through clipboard or drag and drop.

    This property's default is true.
*/

bool QTextEdit::acceptRichText() const
{
    Q_D(const QTextEdit);
    return d->control->acceptRichText();
}

void QTextEdit::setAcceptRichText(bool accept)
{
    Q_D(QTextEdit);
    d->control->setAcceptRichText(accept);
}

/*!
    \class QTextEdit::ExtraSelection
    \since 4.2
    \brief The QTextEdit::ExtraSelection structure provides a way of specifying a
           character format for a given selection in a document
*/

/*!
    \variable QTextEdit::ExtraSelection::cursor
    A cursor that contains a selection in a QTextDocument
*/

/*!
    \variable QTextEdit::ExtraSelection::format
    A format that is used to specify a foreground or background brush/color
    for the selection.
*/

/*!
    \since 4.2
    This function allows temporarily marking certain regions in the document
    with a given color, specified as \a selections. This can be useful for
    example in a programming editor to mark a whole line of text with a given
    background color to indicate the existance of a breakpoint.

    \sa QTextEdit::ExtraSelection, extraSelections()
*/
void QTextEdit::setExtraSelections(const QList<ExtraSelection> &selections)
{
    Q_D(QTextEdit);
    // ######
    QList<QTextControl::ExtraSelection> sels;
    for (int i = 0; i < selections.count(); ++i) {
        QTextControl::ExtraSelection s;
        s.cursor = selections.at(i).cursor;
        s.format = selections.at(i).format;
        sels.append(s);
    }
    d->control->setExtraSelections(sels);
}

/*!
    \since 4.2
    Returns previously set extra selections.

    \sa setExtraSelections()
*/
QList<QTextEdit::ExtraSelection> QTextEdit::extraSelections() const
{
    Q_D(const QTextEdit);
    // ############
    QList<QTextControl::ExtraSelection> sels = d->control->extraSelections();
    QList<QTextEdit::ExtraSelection> selections;
    for (int i = 0; i < sels.count(); ++i) {
        QTextEdit::ExtraSelection s;
        s.cursor = sels.at(i).cursor;
        s.format = sels.at(i).format;
        selections.append(s);
    }
    return selections;
}

/*!
    This function returns a new MIME data object to represent the contents
    of the text edit's current selection. It is called when the selection needs
    to be encapsulated into a new QMimeData object; for example, when a drag
    and drop operation is started, or when data is copyied to the clipboard.

    If you reimplement this function, note that the ownership of the returned
    QMimeData object is passed to the caller. The selection can be retrieved
    by using the textCursor() function.
*/
QMimeData *QTextEdit::createMimeDataFromSelection() const
{
    Q_D(const QTextEdit);
    return d->control->QTextControl::createMimeDataFromSelection();
}

/*!
    This function returns true if the contents of the MIME data object, specified
    by \a source, can be decoded and inserted into the document. It is called
    for example when during a drag operation the mouse enters this widget and it
    is necessary to determine whether it is possible to accept the drag.
 */
bool QTextEdit::canInsertFromMimeData(const QMimeData *source) const
{
    Q_D(const QTextEdit);
    return d->control->QTextControl::canInsertFromMimeData(source);
}

/*!
    This function inserts the contents of the MIME data object, specified
    by \a source, into the text edit at the current cursor position. It is
    called whenever text is inserted as the result of a clipboard paste
    operation, or when the text edit accepts data from a drag and drop
    operation.
*/
void QTextEdit::insertFromMimeData(const QMimeData *source)
{
    Q_D(QTextEdit);
    d->control->QTextControl::insertFromMimeData(source);
}

/*!
    \property QTextEdit::readOnly
    \brief whether the text edit is read-only

    In a read-only text edit the user can only navigate through the
    text and select text; modifying the text is not possible.

    This property's default is false.
*/

bool QTextEdit::isReadOnly() const
{
    Q_D(const QTextEdit);
    return d->control->isReadOnly();
}

void QTextEdit::setReadOnly(bool ro)
{
    Q_D(QTextEdit);
    d->control->setReadOnly(ro);
}

/*!
    If the editor has a selection then the properties of \a modifier are
    applied to the selection. Without a selection the properties are applied
    to the word under the cursor. In addition they are always merged into
    the current char format.

    \sa QTextCursor::mergeCharFormat()
 */
void QTextEdit::mergeCurrentCharFormat(const QTextCharFormat &modifier)
{
    Q_D(QTextEdit);
    d->control->mergeCurrentCharFormat(modifier);
}

/*!
    Sets the char format that is be used when inserting new text to
    \a format .
 */
void QTextEdit::setCurrentCharFormat(const QTextCharFormat &format)
{
    Q_D(QTextEdit);
    d->control->setCurrentCharFormat(format);
}

/*!
    Returns the char format that is used when inserting new text.
 */
QTextCharFormat QTextEdit::currentCharFormat() const
{
    Q_D(const QTextEdit);
    return d->control->currentCharFormat();
}

/*!
    \property QTextEdit::autoFormatting
    \brief the enabled set of auto formatting features

    The value can be any combination of the values in the
    AutoFormattingFlag enum.  The default is AutoNone. Choose
    AutoAll to enable all automatic formatting.

    Currently, the only automatic formatting feature provided is
    AutoBulletList; future versions of Qt may offer more.
*/

QTextEdit::AutoFormatting QTextEdit::autoFormatting() const
{
    Q_D(const QTextEdit);
    return d->autoFormatting;
}

void QTextEdit::setAutoFormatting(AutoFormatting features)
{
    Q_D(QTextEdit);
    d->autoFormatting = features;
}

/*!
    Convenience slot that inserts \a text at the current
    cursor position.

    It is equivalent to

    \code
    edit->textCursor().insertText(text);
    \endcode
 */
void QTextEdit::insertPlainText(const QString &text)
{
    Q_D(QTextEdit);
    d->control->insertPlainText(text);
}

/*!
    Convenience slot that inserts \a text which is assumed to be of
    html formatting at the current cursor position.

    It is equivalent to:

    \code
    QTextDocumentFragment fragment = QTextDocumentFragment::fromHtml(text);
    edit->textCursor().insertFragment(fragment);
    \endcode
 */
void QTextEdit::insertHtml(const QString &text)
{
    Q_D(QTextEdit);
    d->control->insertHtml(text);
}

/*!
    Scrolls the text edit so that the anchor with the given \a name is
    visible; does nothing if the \a name is empty, or is already
    visible, or isn't found.
*/
void QTextEdit::scrollToAnchor(const QString &name)
{
    Q_D(QTextEdit);
    if (name.isEmpty())
        return;

    if (!isVisible()) {
        d->anchorToScrollToWhenVisible = name;
        return;
    }

    d->control->ensureAnchorIsVisible(name);
}

/*!
    \fn QTextEdit::zoomIn(int range)

    Zooms in on the text by by making the base font size \a range
    points larger and recalculating all font sizes to be the new size.
    This does not change the size of any images.

    \sa zoomOut()
*/
void QTextEdit::zoomIn(int range)
{
    QFont f = font();
    f.setPointSize(f.pointSize() + range);
    setFont(f);
}

/*!
    \fn QTextEdit::zoomOut(int range)

    \overload

    Zooms out on the text by making the base font size \a range points
    smaller and recalculating all font sizes to be the new size. This
    does not change the size of any images.

    \sa zoomIn()
*/
void QTextEdit::zoomOut(int range)
{
    QFont f = font();
    f.setPointSize(qMax(1, f.pointSize() - range));
    setFont(f);
}

/*! \property QTextEdit::tabChangesFocus
  \brief whether \gui Tab changes focus or is accepted as input

  In some occasions text edits should not allow the user to input
  tabulators or change indentation using the \gui Tab key, as this breaks
  the focus chain. The default is false.

*/

bool QTextEdit::tabChangesFocus() const
{
    Q_D(const QTextEdit);
    return d->tabChangesFocus;
}

void QTextEdit::setTabChangesFocus(bool b)
{
    Q_D(QTextEdit);
    d->tabChangesFocus = b;
}

/*!
    \property QTextEdit::documentTitle
    \brief the title of the document parsed from the text.
*/

/*!
    \property QTextEdit::lineWrapMode
    \brief the line wrap mode

    The default mode is WidgetWidth which causes words to be
    wrapped at the right edge of the text edit. Wrapping occurs at
    whitespace, keeping whole words intact. If you want wrapping to
    occur within words use setWrapPolicy(). If you set a wrap mode of
    FixedPixelWidth or FixedColumnWidth you should also call
    setWrapColumnOrWidth() with the width you want.

    \sa lineWrapColumnOrWidth
*/

QTextEdit::LineWrapMode QTextEdit::lineWrapMode() const
{
    Q_D(const QTextEdit);
    return d->lineWrap;
}

void QTextEdit::setLineWrapMode(LineWrapMode wrap)
{
    Q_D(QTextEdit);
    if (d->lineWrap == wrap)
        return;
    d->lineWrap = wrap;
    d->relayoutDocument();
}

/*!
    \property QTextEdit::lineWrapColumnOrWidth
    \brief the position (in pixels or columns depending on the wrap mode) where text will be wrapped

    If the wrap mode is FixedPixelWidth, the value is the number of
    pixels from the left edge of the text edit at which text should be
    wrapped. If the wrap mode is FixedColumnWidth, the value is the
    column number (in character columns) from the left edge of the
    text edit at which text should be wrapped.

    \sa lineWrapMode
*/

int QTextEdit::lineWrapColumnOrWidth() const
{
    Q_D(const QTextEdit);
    return d->lineWrapColumnOrWidth;
}

void QTextEdit::setLineWrapColumnOrWidth(int w)
{
    Q_D(QTextEdit);
    d->lineWrapColumnOrWidth = w;
    d->relayoutDocument();
}

/*!
    \property QTextEdit::wordWrapMode
    \brief the mode QTextEdit will use when wrapping text by words

    \sa QTextOption::WrapMode
*/

QTextOption::WrapMode QTextEdit::wordWrapMode() const
{
    Q_D(const QTextEdit);
    return d->control->wordWrapMode();
}

void QTextEdit::setWordWrapMode(QTextOption::WrapMode mode)
{
    Q_D(QTextEdit);
    d->control->setWordWrapMode(mode);
}

/*!
    Finds the next occurrence of the string, \a exp, using the given
    \a options. Returns true if \a exp was found and changes the
    cursor to select the match; otherwise returns false.
*/
bool QTextEdit::find(const QString &exp, QTextDocument::FindFlags options)
{
    Q_D(QTextEdit);
    return d->control->find(exp, options);
}

/*!
    \fn void QTextEdit::copyAvailable(bool yes)

    This signal is emitted when text is selected or de-selected in the
    text edit.

    When text is selected this signal will be emitted with \a yes set
    to true. If no text has been selected or if the selected text is
    de-selected this signal is emitted with \a yes set to false.

    If \a yes is true then copy() can be used to copy the selection to
    the clipboard. If \a yes is false then copy() does nothing.

    \sa selectionChanged()
*/

/*!
    \fn void QTextEdit::currentCharFormatChanged(const QTextCharFormat &f)

    This signal is emitted if the current character format has changed, for
    example caused by a change of the cursor position.

    The new format is \a f.

    \sa setCurrentCharFormat()
*/

/*!
    \fn void QTextEdit::selectionChanged()

    This signal is emitted whenever the selection changes.

    \sa copyAvailable()
*/

/*!
    \fn void QTextEdit::cursorPositionChanged()

    This signal is emitted whenever the position of the
    cursor changed.
*/

#ifdef QT3_SUPPORT
/*!
    Use the QTextCursor() class instead.
*/
void QTextEdit::moveCursor(CursorAction action, QTextCursor::MoveMode mode)
{
    Q_D(QTextEdit);
    if (action == MovePageUp) {
        d->pageUpDown(QTextCursor::Up, mode);
        return;
    } else if (action == MovePageDown) {
        d->pageUpDown(QTextCursor::Down, mode);
        return;
    }

    QTextCursor cursor = d->control->textCursor();
    QTextCursor::MoveOperation op = QTextCursor::NoMove;
    switch (action) {
        case MoveBackward: op = QTextCursor::Left; break;
        case MoveForward: op = QTextCursor::Right; break;
        case MoveWordBackward: op = QTextCursor::WordLeft; break;
        case MoveWordForward: op = QTextCursor::WordRight; break;
        case MoveUp: op = QTextCursor::Up; break;
        case MoveDown: op = QTextCursor::Down; break;
        case MoveLineStart: op = QTextCursor::StartOfLine; break;
        case MoveLineEnd: op = QTextCursor::EndOfLine; break;
        case MoveHome: op = QTextCursor::Start; break;
        case MoveEnd: op = QTextCursor::End; break;
        default: return;
    }
    cursor.movePosition(op, mode);
    d->control->setTextCursor(cursor);
}

/*!
    Use the QTextCursor() class instead.
*/
void QTextEdit::moveCursor(CursorAction action, bool select)
{
    moveCursor(action, select ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);
}

/*!
    \fn void QTextEdit::moveCursor(CursorAction action, bool select)

    Use the QTextCursor() class instead.
*/

/*!
    Executes keyboard action \a action.

    Use the QTextCursor API instead.

    \sa textCursor()
*/
void QTextEdit::doKeyboardAction(KeyboardAction action)
{
    Q_D(QTextEdit);
    QTextCursor cursor = d->control->textCursor();
    switch (action) {
        case ActionBackspace: cursor.deletePreviousChar(); break;
        case ActionDelete: cursor.deleteChar(); break;
        case ActionReturn: cursor.insertBlock(); break;
        case ActionKill: {
                QTextBlock block = cursor.block();
                if (cursor.position() == block.position() + block.length() - 2)
                    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
                else
                    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                cursor.deleteChar();
                break;
            }
        case ActionWordBackspace:
            cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
            cursor.deletePreviousChar();
            break;
        case ActionWordDelete:
            cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
            cursor.deleteChar();
            break;
    }
    d->control->setTextCursor(cursor);
}

/*!
    Sets the text edit's \a text. The text can be plain text or HTML
    and the text edit will try to guess the right format.

    Use setHtml() or setPlainText() directly to avoid text edit's guessing.
*/
void QTextEdit::setText(const QString &text)
{
    Q_D(QTextEdit);
    if (d->textFormat == Qt::AutoText)
        d->textFormat = Qt::mightBeRichText(text) ? Qt::RichText : Qt::PlainText;
    if (d->textFormat == Qt::RichText || d->textFormat == Qt::LogText)
        setHtml(text);
    else
        setPlainText(text);
}

/*!
    Returns all the text in the text edit as plain text.
*/
QString QTextEdit::text() const
{
    Q_D(const QTextEdit);
    if (d->textFormat == Qt::RichText || d->textFormat == Qt::LogText || (d->textFormat == Qt::AutoText && d->preferRichText))
        return d->control->toHtml();
    else
        return d->control->toPlainText();
}


/*!
    Sets the text format to format \a f.

    \sa textFormat()
*/
void QTextEdit::setTextFormat(Qt::TextFormat f)
{
    Q_D(QTextEdit);
    d->textFormat = f;
}

/*!
    Returns the text format.

    \sa setTextFormat()
*/
Qt::TextFormat QTextEdit::textFormat() const
{
    Q_D(const QTextEdit);
    return d->textFormat;
}

#endif // QT3_SUPPORT


/*!
    Appends a new paragraph with \a text to the end of the text edit.
*/
void QTextEdit::append(const QString &text)
{
    Q_D(QTextEdit);
    const bool atBottom = d->verticalOffset() >= d->vbar->maximum();
    d->control->append(text);
    if (atBottom)
        d->vbar->setValue(d->vbar->maximum());
}

/*!
    Ensures that the cursor is visible by scrolling the text edit if
    necessary.
*/
void QTextEdit::ensureCursorVisible()
{
    Q_D(QTextEdit);
    d->control->ensureCursorVisible();
}


/*!
    \enum QTextEdit::KeyboardAction

    \compat

    \value ActionBackspace
    \value ActionDelete
    \value ActionReturn
    \value ActionKill
    \value ActionWordBackspace
    \value ActionWordDelete
*/

/*!
    \fn bool QTextEdit::find(const QString &exp, bool cs, bool wo)

    Use the find() overload that takes a QTextDocument::FindFlags
    argument.
*/

/*!
    \fn void QTextEdit::sync()

    Does nothing.
*/

/*!
    \fn void QTextEdit::setBold(bool b)

    Use setFontWeight() instead.
*/

/*!
    \fn void QTextEdit::setUnderline(bool b)

    Use setFontUnderline() instead.
*/

/*!
    \fn void QTextEdit::setItalic(bool i)

    Use setFontItalic() instead.
*/

/*!
    \fn void QTextEdit::setFamily(const QString &family)

    Use setFontFamily() instead.
*/

/*!
    \fn void QTextEdit::setPointSize(int size)

    Use setFontPointSize() instead.
*/

/*!
    \fn bool QTextEdit::italic() const

    Use fontItalic() instead.
*/

/*!
    \fn bool QTextEdit::bold() const

    Use fontWeight() >= QFont::Bold instead.
*/

/*!
    \fn bool QTextEdit::underline() const

    Use fontUnderline() instead.
*/

/*!
    \fn QString QTextEdit::family() const

    Use fontFamily() instead.
*/

/*!
    \fn int QTextEdit::pointSize() const

    Use int(fontPointSize()+0.5) instead.
*/

/*!
    \fn bool QTextEdit::hasSelectedText() const

    Use textCursor().hasSelection() instead.
*/

/*!
    \fn QString QTextEdit::selectedText() const

    Use textCursor().selectedText() instead.
*/

/*!
    \fn bool QTextEdit::isUndoAvailable() const

    Use document()->isUndoAvailable() instead.
*/

/*!
    \fn bool QTextEdit::isRedoAvailable() const

    Use document()->isRedoAvailable() instead.
*/

/*!
    \fn void QTextEdit::insert(const QString &text)

    Use insertPlainText() instead.
*/

/*!
    \fn bool QTextEdit::isModified() const

    Use document()->isModified() instead.
*/

/*!
    \fn QColor QTextEdit::color() const

    Use textColor() instead.
*/

/*!
    \fn void QTextEdit::textChanged()

    This signal is emitted whenever the document's content changes; for
    example, when text is inserted or deleted, or when formatting is applied.
*/

/*!
    \fn void QTextEdit::undoAvailable(bool available)

    This signal is emitted whenever undo operations become available
    (\a available is true) or unavailable (\a available is false).
*/

/*!
    \fn void QTextEdit::redoAvailable(bool available)

    This signal is emitted whenever redo operations become available
    (\a available is true) or unavailable (\a available is false).
*/

/*!
    \fn void QTextEdit::currentFontChanged(const QFont &font)

    Use currentCharFormatChanged() instead.
*/

/*!
    \fn void QTextEdit::currentColorChanged(const QColor &color)

    Use currentCharFormatChanged() instead.
*/

/*!
    \fn void QTextEdit::setModified(bool m)

    Use document->setModified() instead.
*/

/*!
    \fn void QTextEdit::setColor(const QColor &color)

    Use setTextColor() instead.
*/
#endif // QT_NO_TEXTEDIT

#ifndef QT_NO_CONTEXTMENU
#define NUM_CONTROL_CHARACTERS 10
const struct QUnicodeControlCharacter {
    const char *text;
    ushort character;
} qt_controlCharacters[NUM_CONTROL_CHARACTERS] = {
    { QT_TRANSLATE_NOOP("QUnicodeControlCharacterMenu", "LRM Left-to-right mark"), 0x200e },
    { QT_TRANSLATE_NOOP("QUnicodeControlCharacterMenu", "RLM Right-to-left mark"), 0x200f },
    { QT_TRANSLATE_NOOP("QUnicodeControlCharacterMenu", "ZWJ Zero width joiner"), 0x200d },
    { QT_TRANSLATE_NOOP("QUnicodeControlCharacterMenu", "ZWNJ Zero width non-joiner"), 0x200c },
    { QT_TRANSLATE_NOOP("QUnicodeControlCharacterMenu", "ZWSP Zero width space"), 0x200b },
    { QT_TRANSLATE_NOOP("QUnicodeControlCharacterMenu", "LRE Start of left-to-right embedding"), 0x202a },
    { QT_TRANSLATE_NOOP("QUnicodeControlCharacterMenu", "RLE Start of right-to-left embedding"), 0x202b },
    { QT_TRANSLATE_NOOP("QUnicodeControlCharacterMenu", "LRO Start of left-to-right override"), 0x202d },
    { QT_TRANSLATE_NOOP("QUnicodeControlCharacterMenu", "RLO Start of right-to-left override"), 0x202e },
    { QT_TRANSLATE_NOOP("QUnicodeControlCharacterMenu", "PDF Pop directional formatting"), 0x202c },
};

QUnicodeControlCharacterMenu::QUnicodeControlCharacterMenu(QObject *_editWidget, QWidget *parent)
    : QMenu(parent), editWidget(_editWidget)
{
    setTitle(tr("Insert Unicode control character"));
    for (int i = 0; i < NUM_CONTROL_CHARACTERS; ++i) {
        addAction(tr(qt_controlCharacters[i].text), this, SLOT(menuActionTriggered()));
    }
}

void QUnicodeControlCharacterMenu::menuActionTriggered()
{
    QAction *a = qobject_cast<QAction *>(sender());
    int idx = actions().indexOf(a);
    if (idx < 0 || idx >= NUM_CONTROL_CHARACTERS)
        return;
    QChar c(qt_controlCharacters[idx].character);
    QString str(c);

#ifndef QT_NO_TEXTEDIT
    if (QTextEdit *edit = qobject_cast<QTextEdit *>(editWidget)) {
        edit->insertPlainText(str);
        return;
    }
#endif
    if (QTextControl *control = qobject_cast<QTextControl *>(editWidget)) {
        control->insertPlainText(str);
    }
#ifndef QT_NO_LINEEDIT
    if (QLineEdit *edit = qobject_cast<QLineEdit *>(editWidget)) {
        edit->insert(str);
        return;
    }
#endif
}
#endif // QT_NO_CONTEXTMENU

#include "moc_qtextedit.cpp"
#include "moc_qtextedit_p.cpp"

/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qitemdelegate.h"

#ifndef QT_NO_ITEMVIEWS
#include <qabstractitemmodel.h>
#include <qapplication.h>
#include <qbrush.h>
#include <qlineedit.h>
#include <qpainter.h>
#include <qpalette.h>
#include <qpoint.h>
#include <qrect.h>
#include <qsize.h>
#include <qstyle.h>
#include <qstyleoption.h>
#include <qevent.h>
#include <qpixmap.h>
#include <qbitmap.h>
#include <qpixmapcache.h>
#include <qitemeditorfactory.h>
#include <qmetaobject.h>
#include <qtextlayout.h>
#include <private/qobject_p.h>
#include <private/qdnd_p.h>
#include <qdebug.h>
#include <qlocale.h>

#include <limits.h>

class QItemDelegatePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QItemDelegate)

public:
    QItemDelegatePrivate() : f(0), clipPainting(false) {}

    inline const QItemEditorFactory *editorFactory() const
        { return f ? f : QItemEditorFactory::defaultFactory(); }

    inline QIcon::Mode iconMode(QStyle::State state) const
        {
            if (!(state & QStyle::State_Enabled)) return QIcon::Disabled;
            if (state & QStyle::State_Selected) return QIcon::Selected;
            return QIcon::Normal;
        }

    inline QIcon::State iconState(QStyle::State state) const
        { return state & QStyle::State_Open ? QIcon::On : QIcon::Off; }

    void _q_commitDataAndCloseEditor(QWidget *editor);

    QItemEditorFactory *f;
    bool clipPainting;

    QRect textLayoutBounds(const QStyleOptionViewItemV2 &options) const;
    QSizeF doTextLayout(int lineWidth) const;
    mutable QTextLayout textLayout;
    mutable QTextOption textOption;
};

void QItemDelegatePrivate::_q_commitDataAndCloseEditor(QWidget *editor)
{
    Q_Q(QItemDelegate);
    emit q->commitData(editor);
    emit q->closeEditor(editor, QAbstractItemDelegate::SubmitModelCache);
}

QRect QItemDelegatePrivate::textLayoutBounds(const QStyleOptionViewItemV2 &option) const
{
    QRect rect = option.rect;
    const bool wrapText = option.features & QStyleOptionViewItemV2::WrapText;
    switch (option.decorationPosition) {
    case QStyleOptionViewItem::Left:
    case QStyleOptionViewItem::Right:
        rect.setWidth(INT_MAX >> 6);
        break;
    case QStyleOptionViewItem::Top:
    case QStyleOptionViewItem::Bottom:
        rect.setWidth(wrapText ? option.decorationSize.width() : (INT_MAX >> 6));
        break;
    }

    return rect;
}

QSizeF QItemDelegatePrivate::doTextLayout(int lineWidth) const
{
    QFontMetrics fontMetrics(textLayout.font());
    int leading = fontMetrics.leading();
    qreal height = 0;
    qreal widthUsed = 0;
    textLayout.beginLayout();
    while (true) {
        QTextLine line = textLayout.createLine();
        if (!line.isValid())
            break;
        line.setLineWidth(lineWidth);
        height += leading;
        line.setPosition(QPointF(0, height));
        height += line.height();
        widthUsed = qMax(widthUsed, line.naturalTextWidth());
    }
    textLayout.endLayout();
    return QSizeF(widthUsed, height);
}

/*!
    \class QItemDelegate

    \brief The QItemDelegate class provides display and editing facilities for
    data items from a model.

    \ingroup model-view
    \mainclass

    QItemDelegate can be used to provide custom display features and editor
    widgets for item views based on QAbstractItemView subclasses. Using a
    delegate for this purpose allows the display and editing mechanisms to be
    customized and developed independently from the model and view.

    The QItemDelegate class is one of the \l{Model/View Classes}
    and is part of Qt's \l{Model/View Programming}{model/view framework}.

    When displaying items from a custom model in a standard view, it is
    often sufficient to simply ensure that the model returns appropriate
    data for each of the \l{Qt::ItemDataRole}{roles} that determine the
    appearance of items in views. The default delegate used by Qt's
    standard views uses this role information to display items in most
    of the common forms expected by users. However, it is sometimes
    necessary to have even more control over the appearance of items than
    the default delegate can provide.

    This class provides default implementations of the functions for
    painting item data in a view, and editing data obtained from a model.
    Default implementations of the paint() and sizeHint() virtual functions,
    defined in QAbstractItemDelegate, are provided to ensure that the
    delegate implements the correct basic behavior expected by views. You
    can reimplement these functions in subclasses to customize the
    appearance of items.

    Delegates can be used to manipulate item data in two complementary ways:
    by processing events in the normal manner, or by implementing a
    custom editor widget. The item delegate takes the approach of providing
    a widget for editing purposes that can be supplied to
    QAbstractItemView::setDelegate() or the equivalent function in
    subclasses of QAbstractItemView.

    Only the standard editing functions for widget-based delegates are
    reimplemented here: editor() returns the widget used to change data
    from the model; setEditorData() provides the widget with data to
    manipulate; updateEditorGeometry() ensures that the editor is displayed
    correctly with respect to the item view; setModelData() returns the
    updated data to the model; releaseEditor() indicates that the user has
    completed editing the data, and that the editor widget can be destroyed.

    \section1 Standard Roles and Data Types

    The default delegate used by the standard views supplied with Qt
    associates each standard role (defined by Qt::ItemDataRole) with certain
    data types. Models that return data in these types can influence the
    appearance of the delegate as described in the following table.

    \table
    \header \o Role \o Accepted Types
    \omit
    \row    \o \l Qt::AccessibleDescriptionRole \o QString
    \row    \o \l Qt::AccessibleTextRole \o QString
    \endomit
    \row    \o \l Qt::BackgroundRole \o QBrush
    \row    \o \l Qt::BackgroundColorRole \o QColor (obsolete; use Qt::BackgroundRole instead)
    \row    \o \l Qt::CheckStateRole \o Qt::CheckState
    \row    \o \l Qt::DecorationRole \o QIcon and QColor
    \row    \o \l Qt::DisplayRole \o QString and types with a string representation
    \row    \o \l Qt::EditRole \o See QItemEditorFactory for details
    \row    \o \l Qt::FontRole \o QFont
    \row    \o \l Qt::SizeHintRole \o QSize
    \omit
    \row    \o \l Qt::StatusTipRole \o
    \endomit
    \row    \o \l Qt::TextAlignmentRole \o Qt::Alignment
    \row    \o \l Qt::ForegroundRole \o QBrush
    \row    \o \l Qt::TextColorRole \o QColor (obsolete; use Qt::ForegroundRole instead)
    \omit
    \row    \o \l Qt::ToolTipRole
    \row    \o \l Qt::WhatsThisRole
    \endomit
    \endtable

    If the default delegate does not allow the level of customization that
    you need, either for display purposes or for editing data, it is possible to
    subclass QItemDelegate to implement the desired behavior.

    \section1 Subclassing

    When subclassing QItemDelegate to create a delegate that displays items
    using a custom renderer, it is important to ensure that the delegate can
    render items suitably for all the required states; e.g. selected,
    disabled, checked. The documentation for the paint() function contains
    some hints to show how this can be achieved.

    Custom editing features for can be added by subclassing QItemDelegate and
    reimplementing createEditor(), setEditorData(), setModelData(), and
    updateEditorGeometry(). This process is described in the
    \l{Spin Box Delegate example}.

    \sa {Model/View Programming}, QAbstractItemDelegate, {Spin Box Delegate Example},
        {Settings Editor Example}, {Icons Example}
*/

/*!
    Constructs an item delegate with the given \a parent.
*/

QItemDelegate::QItemDelegate(QObject *parent)
    : QAbstractItemDelegate(*new QItemDelegatePrivate(), parent)
{

}

/*!
    Destroys the item delegate.
*/

QItemDelegate::~QItemDelegate()
{
}

/*!
  \property QItemDelegate::clipping
  \brief if the delegate should clip the paint events.

  This property will set the paint clip to the size of the item.
  The default value is off.  It is useful for cases such
  as when images are larger then the size of the item.
*/

bool QItemDelegate::hasClipping() const
{
    Q_D(const QItemDelegate);
    return d->clipPainting;
}

void QItemDelegate::setClipping(bool clip)
{
    Q_D(QItemDelegate);
    d->clipPainting = clip;
}

/*!
    Renders the delegate using the given \a painter and style \a option for
    the item specified by \a index.

    When reimplementing this function in a subclass, you should update the area
    held by the option's \l{QStyleOption::rect}{rect} variable, using the
    option's \l{QStyleOption::state}{state} variable to determine the state of
    the item to be displayed, and adjust the way it is painted accordingly.

    For example, a selected item may need to be displayed differently to
    unselected items, as shown in the following code:

    \quotefromfile itemviews/pixelator/pixeldelegate.cpp
    \skipto QStyle::State_Selected
    \printuntil else
    \dots

    After painting, you should ensure that the painter is returned to its
    the state it was supplied in when this function was called. For example,
    it may be useful to call QPainter::save() before painting and
    QPainter::restore() afterwards.

    \sa QStyle::State
*/

void QItemDelegate::paint(QPainter *painter,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const
{
    Q_D(const QItemDelegate);
    Q_ASSERT(index.isValid());
    QStyleOptionViewItemV2 opt = setOptions(index, option);
    const QStyleOptionViewItemV2 *v2 = qstyleoption_cast<const QStyleOptionViewItemV2 *>(&option);
    opt.features = v2 ? v2->features : QStyleOptionViewItemV2::ViewItemFeatures(QStyleOptionViewItemV2::None);

    // prepare
    painter->save();
    if (d->clipPainting)
        painter->setClipRect(opt.rect);

    // get the data and the rectangles

    QVariant value;

    QIcon icon;
    QIcon::Mode iconMode = d->iconMode(option.state);
    QIcon::State iconState = d->iconState(option.state);

    QPixmap pixmap;
    QRect decorationRect;
    value = index.data(Qt::DecorationRole);
    if (value.isValid()) {
        if (value.type() == QVariant::Icon) {
            icon = qvariant_cast<QIcon>(value);
            decorationRect = QRect(QPoint(0, 0),
                                   icon.actualSize(option.decorationSize, iconMode, iconState));
        } else {
            pixmap = decoration(opt, value);
            decorationRect = QRect(QPoint(0, 0), option.decorationSize).intersected(pixmap.rect());
        }
    }

    QString text;
    QRect displayRect;
    value = index.data(Qt::DisplayRole);
    if (value.isValid()) {
        if (value.type() == QVariant::Double)
            text = QLocale().toString(value.toDouble());
        else
            text = value.toString();
        displayRect = textRectangle(painter, d->textLayoutBounds(opt), opt.font, text);
    }

    QRect checkRect;
    Qt::CheckState checkState = Qt::Unchecked;
    value = index.data(Qt::CheckStateRole);
    if (value.isValid()) {
        checkState = static_cast<Qt::CheckState>(value.toInt());
        checkRect = check(opt, opt.rect, value);
    }

    // do the layout

    doLayout(opt, &checkRect, &decorationRect, &displayRect, false);

    // draw the item

    drawBackground(painter, opt, index);
    drawCheck(painter, opt, checkRect, checkState);
    if (!icon.isNull())
        icon.paint(painter, decorationRect, option.decorationAlignment, iconMode, iconState);
    else
        drawDecoration(painter, opt, decorationRect, pixmap);
    drawDisplay(painter, opt, displayRect, text);
    drawFocus(painter, opt, text.isEmpty() ? QRect() : displayRect);

    // done
    painter->restore();
}

/*!
    Returns the size needed by the delegate to display the item
    specified by \a index, taking into account the style information
    provided by \a option.
*/

QSize QItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                              const QModelIndex &index) const
{
    QVariant value = index.data(Qt::SizeHintRole);
    if (value.isValid())
        return qvariant_cast<QSize>(value);
    QRect decorationRect = rect(option, index, Qt::DecorationRole);
    QRect displayRect = rect(option, index, Qt::DisplayRole);
    QRect checkRect = rect(option, index, Qt::CheckStateRole);

    doLayout(option, &checkRect, &decorationRect, &displayRect, true);

    return (decorationRect|displayRect|checkRect).size();
}

/*!
    Returns the widget used to edit the item specified by \a index
    for editing. The \a parent widget and style \a option are used to
    control how the editor widget appears.

    \sa QAbstractItemDelegate::createEditor()
*/

QWidget *QItemDelegate::createEditor(QWidget *parent,
                                     const QStyleOptionViewItem &,
                                     const QModelIndex &index) const
{
    Q_D(const QItemDelegate);
    if (!index.isValid())
        return 0;
    QVariant::Type t = static_cast<QVariant::Type>(index.data(Qt::EditRole).userType());
    const QItemEditorFactory *factory = d->f;
    if (factory == 0)
        factory = QItemEditorFactory::defaultFactory();
    return factory->createEditor(t, parent);
}

/*!
    Sets the data to be displayed and edited by the \a editor for the
    item specified by \a index.

    The default implementation uses the editor's user property to set values.

    \sa QMetaProperty::isUser()
*/

void QItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
#ifdef QT_NO_PROPERTIES
    Q_UNUSED(editor);
    Q_UNUSED(index);
#else
    Q_D(const QItemDelegate);
    QVariant v = index.data(Qt::EditRole);
    QByteArray n = editor->metaObject()->userProperty().name();
    if (n.isEmpty())
        n = d->editorFactory()->valuePropertyName(static_cast<QVariant::Type>(v.userType()));
    if (!n.isEmpty())
        editor->setProperty(n, v);
#endif
}

/*!
    Sets the data for the specified \a model and item \a index from that
    supplied by the \a editor.

    The default implementation uses the editor's user property to get values.

    \sa QMetaProperty::isUser()
*/

void QItemDelegate::setModelData(QWidget *editor,
                                 QAbstractItemModel *model,
                                 const QModelIndex &index) const
{
#ifdef QT_NO_PROPERTIES
    Q_UNUSED(model);
    Q_UNUSED(editor);
    Q_UNUSED(index);
#else
    Q_D(const QItemDelegate);
    Q_ASSERT(model);
    Q_ASSERT(editor);
    QByteArray n = editor->metaObject()->userProperty().name();
    if (n.isEmpty())
        n = d->editorFactory()->valuePropertyName(
            static_cast<QVariant::Type>(model->data(index, Qt::EditRole).userType()));
    if (!n.isEmpty())
        model->setData(index, editor->property(n), Qt::EditRole);
#endif
}

/*!
    Updates the \a editor for the item specified by \a index
    according to the style \a option given.
*/

void QItemDelegate::updateEditorGeometry(QWidget *editor,
                                         const QStyleOptionViewItem &option,
                                         const QModelIndex &index) const
{
    if (!editor)
        return;
    Q_ASSERT(index.isValid());
    QPixmap pixmap = decoration(option, index.data(Qt::DecorationRole));
    QString text = index.data(Qt::DisplayRole).toString();
    QRect pixmapRect = QRect(QPoint(0, 0), option.decorationSize).intersected(pixmap.rect());
    QRect textRect = textRectangle(0, option.rect, option.font, text);
    QRect checkRect = check(option, textRect, index.data(Qt::CheckStateRole));
    QStyleOptionViewItem opt = option;
    opt.showDecorationSelected = true; // let the editor take up all available space
    doLayout(opt, &checkRect, &pixmapRect, &textRect, false);
    editor->setGeometry(textRect);
}

/*!
  Returns the editor factory used by the item delegate.
  If no editor factory is set, the function will return null.

  \sa setItemEditorFactory()
*/
QItemEditorFactory *QItemDelegate::itemEditorFactory() const
{
    Q_D(const QItemDelegate);
    return d->f;
}

/*!
  Sets the editor factory to be used by the item delegate to be the \a factory
  specified. If no editor factory is set, the item delegate will use the
  default editor factory.

  \sa itemEditorFactory()
*/
void QItemDelegate::setItemEditorFactory(QItemEditorFactory *factory)
{
    Q_D(QItemDelegate);
    d->f = factory;
}

/*!
   Renders the item view \a text within the rectangle specified by \a rect
   using the given \a painter and style \a option.
*/

void QItemDelegate::drawDisplay(QPainter *painter, const QStyleOptionViewItem &option,
                                const QRect &rect, const QString &text) const
{
    Q_D(const QItemDelegate);

    if (text.isEmpty())
        return;

    QPen pen = painter->pen();
    QPalette::ColorGroup cg = option.state & QStyle::State_Enabled
                              ? QPalette::Normal : QPalette::Disabled;
    if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
        cg = QPalette::Inactive;
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(rect, option.palette.brush(cg, QPalette::Highlight));
        painter->setPen(option.palette.color(cg, QPalette::HighlightedText));
    } else {
        painter->setPen(option.palette.color(cg, QPalette::Text));
    }

    if (option.state & QStyle::State_Editing) {
        painter->save();
        painter->setPen(option.palette.color(cg, QPalette::Text));
        painter->drawRect(rect.adjusted(0, 0, -1, -1));
        painter->restore();
    }

    const QStyleOptionViewItemV2 opt = option;
    const int textMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1;
    QRect textRect = rect.adjusted(textMargin, 0, -textMargin, 0); // remove width padding
    const bool wrapText = opt.features & QStyleOptionViewItemV2::WrapText;
    d->textOption.setWrapMode(wrapText ? QTextOption::WordWrap : QTextOption::ManualWrap);
    d->textOption.setTextDirection(option.direction);
    d->textOption.setAlignment(option.displayAlignment);
    d->textLayout.setTextOption(d->textOption);
    d->textLayout.setFont(option.font);
    d->textLayout.setText(QString(text).replace(QLatin1Char('\n'), QChar::LineSeparator));

    QSizeF textLayoutSize = d->doTextLayout(textRect.width());

    if (textRect.width() < textLayoutSize.width()
        || textRect.height() < textLayoutSize.height()) {
        const QString elided = option.fontMetrics.elidedText(text,
                                                             option.textElideMode,
                                                             textRect.width());
        d->textLayout.setText(elided);
        textLayoutSize = d->doTextLayout(textRect.width());
    }

    textRect.setTop(textRect.top() + (textRect.height()/2) - (textLayoutSize.toSize().height()/2));

    d->textLayout.draw(painter, textRect.topLeft(), QVector<QTextLayout::FormatRange>(), textRect);
}

/*!
    Renders the decoration \a pixmap within the rectangle specified by
    \a rect using the given \a painter and style \a option.
*/
void QItemDelegate::drawDecoration(QPainter *painter, const QStyleOptionViewItem &option,
                                   const QRect &rect, const QPixmap &pixmap) const
{
    if (pixmap.isNull() || !rect.isValid())
        return;
    QPoint p = QStyle::alignedRect(option.direction, option.decorationAlignment,
                                   pixmap.size(), rect).topLeft();
    if (option.state & QStyle::State_Selected) {
        QPixmap *pm = selected(pixmap, option.palette, option.state & QStyle::State_Enabled);
        painter->drawPixmap(p, *pm);
    } else {
        painter->drawPixmap(p, pixmap);
    }
}

/*!
    Renders the region within the rectangle specified by \a rect, indicating
    that it has the focus, using the given \a painter and style \a option.
*/

void QItemDelegate::drawFocus(QPainter *painter,
                              const QStyleOptionViewItem &option,
                              const QRect &rect) const
{
    if ((option.state & QStyle::State_HasFocus) == 0 || !rect.isValid())
        return;
    QStyleOptionFocusRect o;
    o.QStyleOption::operator=(option);
    o.rect = rect;
    o.state |= QStyle::State_KeyboardFocusChange;
    QPalette::ColorGroup cg = (option.state & QStyle::State_Enabled)
                              ? QPalette::Normal : QPalette::Disabled;
    o.backgroundColor = option.palette.color(cg, (option.state & QStyle::State_Selected)
                                             ? QPalette::Highlight : QPalette::Background);
    QApplication::style()->drawPrimitive(QStyle::PE_FrameFocusRect, &o, painter);
}

/*!
    Renders a check indicator within the rectangle specified by \a
    rect, using the given \a painter and style \a option, using the
    given \a state.
*/

void QItemDelegate::drawCheck(QPainter *painter,
                              const QStyleOptionViewItem &option,
                              const QRect &rect, Qt::CheckState state) const
{
    if (!rect.isValid())
        return;

    QStyleOptionViewItem opt(option);
    opt.rect = rect;
    opt.state = opt.state & ~QStyle::State_HasFocus;

    switch (state) {
    case Qt::Unchecked:
        opt.state |= QStyle::State_Off;
        break;
    case Qt::PartiallyChecked:
        opt.state |= QStyle::State_NoChange;
        break;
    case Qt::Checked:
        opt.state |= QStyle::State_On;
        break;
    }

    QApplication::style()->drawPrimitive(QStyle::PE_IndicatorViewItemCheck, &opt, painter);
}

/*!
    Renders the item background for the given \a index,
    using the given \a painter and style \a option.
*/

void QItemDelegate::drawBackground(QPainter *painter,
                                   const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
    if (option.showDecorationSelected && (option.state & QStyle::State_Selected)) {
        QPalette::ColorGroup cg = option.state & QStyle::State_Enabled
                                  ? QPalette::Normal : QPalette::Disabled;
        if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
            cg = QPalette::Inactive;

        painter->fillRect(option.rect, option.palette.brush(cg, QPalette::Highlight));
    } else {
        QVariant value = index.data(Qt::BackgroundRole);
        if (qVariantCanConvert<QBrush>(value)) {
            QPointF oldBO = painter->brushOrigin();
            painter->setBrushOrigin(option.rect.topLeft());
            painter->fillRect(option.rect, qvariant_cast<QBrush>(value));
            painter->setBrushOrigin(oldBO);
        }
    }
}


/*!
    \internal
*/

void QItemDelegate::doLayout(const QStyleOptionViewItem &option,
                             QRect *checkRect, QRect *pixmapRect, QRect *textRect,
                             bool hint) const
{
    Q_ASSERT(checkRect && pixmapRect && textRect);
    const int textMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1;
    int x = option.rect.left();
    int y = option.rect.top();
    int w, h;

    textRect->adjust(-textMargin, 0, textMargin, 0); // add width padding
    if (textRect->height() == 0)
        textRect->setHeight(option.fontMetrics.lineSpacing());

    QSize pm(0, 0);
    if (pixmapRect->isValid()) {
        pm = pixmapRect->size();
        pm.rwidth() += 2 * textMargin;
    }
    if (hint) {
        h = qMax(checkRect->height(), qMax(textRect->height(), pm.height()));
        if (option.decorationPosition == QStyleOptionViewItem::Left
            || option.decorationPosition == QStyleOptionViewItem::Right) {
            w = textRect->width() + pm.width();
        } else {
            w = qMax(textRect->width(), pm.width());
        }
    } else {
        w = option.rect.width();
        h = option.rect.height();
    }

    int cw = 0;
    QRect check;
    if (checkRect->isValid()) {
        cw = checkRect->width() + 2 * textMargin;
        if (hint) w += cw;
        if (option.direction == Qt::RightToLeft) {
            check.setRect(x + w - cw, y, cw, h);
        } else {
            check.setRect(x, y, cw, h);
        }
    }

    // at this point w should be the *total* width

    QRect display;
    QRect decoration;
    switch (option.decorationPosition) {
    case QStyleOptionViewItem::Top: {
        if (!pm.isEmpty())
            pm.setHeight(pm.height() + textMargin); // add space
        h = hint ? textRect->height() : h - pm.height();

        if (option.direction == Qt::RightToLeft) {
            decoration.setRect(x, y, w - cw, pm.height());
            display.setRect(x, y + pm.height(), w - cw, h);
        } else {
            decoration.setRect(x + cw, y, w - cw, pm.height());
            display.setRect(x + cw, y + pm.height(), w - cw, h);
        }
        break; }
    case QStyleOptionViewItem::Bottom: {
        if (!textRect->isEmpty())
            textRect->setHeight(textRect->height() + textMargin); // add space
        h = hint ? textRect->height() + pm.height() : h;

        if (option.direction == Qt::RightToLeft) {
            display.setRect(x, y, w - cw, textRect->height());
            decoration.setRect(x, y + textRect->height(), w - cw, h - textRect->height());
        } else {
            display.setRect(x + cw, y, w - cw, textRect->height());
            decoration.setRect(x + cw, y + textRect->height(), w - cw, h - textRect->height());
        }
        break; }
    case QStyleOptionViewItem::Left: {
        if (option.direction == Qt::LeftToRight) {
            decoration.setRect(x + cw, y, pm.width(), h);
            display.setRect(decoration.right() + 1, y, w - pm.width() - cw, h);
        } else {
            display.setRect(x, y, w - pm.width() - cw, h);
            decoration.setRect(display.right() + 1, y, pm.width(), h);
        }
        break; }
    case QStyleOptionViewItem::Right: {
        if (option.direction == Qt::LeftToRight) {
            display.setRect(x + cw, y, w - pm.width() - cw, h);
            decoration.setRect(display.right() + 1, y, pm.width(), h);
        } else {
            decoration.setRect(x, y, pm.width(), h);
            display.setRect(decoration.right() + 1, y, w - pm.width() - cw, h);
        }
        break; }
    default:
        qWarning("doLayout: decoration position is invalid");
        decoration = *pixmapRect;
        break;
    }

    if (!hint) { // we only need to do the internal layout if we are going to paint
        *checkRect = QStyle::alignedRect(option.direction, Qt::AlignCenter,
                                         checkRect->size(), check);
        *pixmapRect = QStyle::alignedRect(option.direction, option.decorationAlignment,
                                          pixmapRect->size(), decoration);
        // the text takes up all awailable space, unless the decoration is not shown as selected
        if (option.showDecorationSelected)
            *textRect = display;
        else
            *textRect = QStyle::alignedRect(option.direction, option.displayAlignment,
                                            textRect->size().boundedTo(display.size()), display);
    } else {
        *checkRect = check;
        *pixmapRect = decoration;
        *textRect = display;
    }
}

/*!
    \internal

    Returns the pixmap used to decorate the root of the item view.
    The style \a option controls the appearance of the root; the \a variant
    refers to the data associated with an item.
*/

QPixmap QItemDelegate::decoration(const QStyleOptionViewItem &option, const QVariant &variant) const
{
    Q_D(const QItemDelegate);
    switch (variant.type()) {
    case QVariant::Icon: {
        QIcon::Mode mode = d->iconMode(option.state);
        QIcon::State state = d->iconState(option.state);
        return qvariant_cast<QIcon>(variant).pixmap(option.decorationSize, mode, state); }
    case QVariant::Color: {
        static QPixmap pixmap(option.decorationSize);
        pixmap.fill(qvariant_cast<QColor>(variant));
        return pixmap; }
    default:
        break;
    }

    return qvariant_cast<QPixmap>(variant);
}

// hacky but faster version of "QString::sprintf("%d-%d", i, enabled)"
static QString qPixmapSerial(quint64 i, bool enabled)
{
    ushort arr[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', '0' + enabled };
    ushort *ptr = &arr[16];

    while (i > 0) {
        // hey - it's our internal representation, so use the ascii character after '9'
        // instead of 'a' for hex
        *(--ptr) = '0' + i % 16;
        i >>= 4;
    }

    return QString::fromUtf16(ptr, int(&arr[sizeof(arr) / sizeof(ushort)] - ptr));
}

/*!
  \internal
  Returns the selected version of the given \a pixmap using the given \a palette.
  The \a enabled argument decides whether the normal or disabled highlight color of
  the palette is used.
*/
QPixmap *QItemDelegate::selected(const QPixmap &pixmap, const QPalette &palette, bool enabled) const
{
    QString key = qPixmapSerial(qt_pixmap_id(pixmap), enabled);
    QPixmap *pm = QPixmapCache::find(key);
    if (!pm) {
        QImage img = pixmap.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);

        QColor color = palette.color(enabled ? QPalette::Normal : QPalette::Disabled,
                                     QPalette::Highlight);
        color.setAlphaF(0.3);

        QPainter painter(&img);
        painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
        painter.fillRect(0, 0, img.width(), img.height(), color);
        painter.end();

        QPixmap selected = QPixmap(QPixmap::fromImage(img));
        QPixmapCache::insert(key, selected);
        pm = QPixmapCache::find(key);
    }
    return pm;
}

/*!
  \internal
*/

QRect QItemDelegate::rect(const QStyleOptionViewItem &option,
                          const QModelIndex &index, int role) const
{
    Q_D(const QItemDelegate);
    QVariant value = index.data(role);
    if (role == Qt::CheckStateRole)
        return check(option, option.rect, value);
    if (value.isValid()) {
        switch (value.type()) {
        case QVariant::Invalid:
            break;
        case QVariant::Pixmap:
            return QRect(QPoint(0, 0), qvariant_cast<QPixmap>(value).size());
        case QVariant::Image:
            return QRect(QPoint(0, 0), qvariant_cast<QImage>(value).size());
        case QVariant::Icon: {
            QIcon::Mode mode = d->iconMode(option.state);
            QIcon::State state = d->iconState(option.state);
            QIcon icon = qvariant_cast<QIcon>(value);
            QSize size = icon.actualSize(option.decorationSize, mode, state);
            return QRect(QPoint(0, 0), size); }
        case QVariant::Color:
            return QRect(QPoint(0, 0), option.decorationSize);
        case QVariant::String:
        default: {
            QString text = value.toString();
            value = index.data(Qt::FontRole);
            QFont fnt = qvariant_cast<QFont>(value).resolve(option.font);
            return textRectangle(0, d->textLayoutBounds(option), fnt, text); }
        }
    }
    return QRect();
}

/*!
  \internal
*/
QRect QItemDelegate::check(const QStyleOptionViewItem &option,
                           const QRect &bounding, const QVariant &value) const
{
    if (value.isValid()) {
        QStyleOptionButton opt;
        opt.QStyleOption::operator=(option);
        opt.rect = bounding;
        return QApplication::style()->subElementRect(QStyle::SE_ViewItemCheckIndicator, &opt);
    }
    return QRect();
}

/*!
  \internal
*/
QRect QItemDelegate::textRectangle(QPainter * /*painter*/, const QRect &rect,
                                   const QFont &font, const QString &text) const
{
    Q_D(const QItemDelegate);
    d->textOption.setWrapMode(QTextOption::WordWrap);
    d->textLayout.setTextOption(d->textOption);
    d->textLayout.setFont(font);
    d->textLayout.setText(QString(text).replace(QLatin1Char('\n'), QChar::LineSeparator));
    const QSize size = d->doTextLayout(rect.width()).toSize();
    const int textMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1;
    return QRect(0, 0, size.width() + 2 * textMargin, size.height());
}

/*!
    If the \a object is the current editor: if the \a event is an Esc
    key press the current edit is cancelled and ended, or if the \a
    event is an Enter or Return key press the current edit is accepted
    and ended. If editing is ended the event filter returns true to
    signify that it has handled the event; in all other cases it does
    nothing and returns false to signify that the event hasn't been
    handled.

    \sa closeEditor()
*/

bool QItemDelegate::eventFilter(QObject *object, QEvent *event)
{
    QWidget *editor = ::qobject_cast<QWidget*>(object);
    if (!editor)
        return false;
    if (event->type() == QEvent::KeyPress) {
        switch (static_cast<QKeyEvent *>(event)->key()) {
        case Qt::Key_Tab:
            emit commitData(editor);
            emit closeEditor(editor, QAbstractItemDelegate::EditNextItem);
            return true;
        case Qt::Key_Backtab:
            emit commitData(editor);
            emit closeEditor(editor, QAbstractItemDelegate::EditPreviousItem);
            return true;
        case Qt::Key_Enter:
        case Qt::Key_Return:
            // We want the editor to be able to process the key press
            // before committing the data (e.g. so it can do
            // validation/fixup of the input).
            QMetaObject::invokeMethod(this, "_q_commitDataAndCloseEditor",
                                      Qt::QueuedConnection, Q_ARG(QWidget*, editor));
            return false;
        case Qt::Key_Escape:
            // don't commit data
            emit closeEditor(editor, QAbstractItemDelegate::RevertModelCache);
            break;
        default:
            return false;
        }
        if (editor->parentWidget())
            editor->parentWidget()->setFocus();
        return true;
    } else if (event->type() == QEvent::FocusOut) {
        if (!editor->isActiveWindow() || (QApplication::focusWidget() != editor)) {
            QWidget *w = QApplication::focusWidget();
            while (w) { // dont worry about focus changes internally in the editor
                if (w == editor)
                    return false;
                w = w->parentWidget();
            }
#ifndef QT_NO_DRAGANDDROP
            // The window may lose focus during an drag operation.
            // i.e when dragging involves the taskbar on Windows.
            if (QDragManager::self() && QDragManager::self()->object != 0)
                return false;
#endif
            // Opening a modal dialog will start a new eventloop
            // that will process the deleteLater event.
            if (QApplication::activeModalWidget())
                return false;
            emit commitData(editor);
            emit closeEditor(editor, NoHint);
        }
    }
    return false;
}

/*!
  \reimp
*/

bool QItemDelegate::editorEvent(QEvent *event,
                                QAbstractItemModel *model,
                                const QStyleOptionViewItem &option,
                                const QModelIndex &index)
{
    Q_ASSERT(event);
    Q_ASSERT(model);

    // make sure that the item is checkable
    Qt::ItemFlags flags = model->flags(index);
    if (!(flags & Qt::ItemIsUserCheckable) || !((flags & Qt::ItemIsEnabled)))
        return false;

    // make sure that we have the right event type
    if (event->type() == QEvent::MouseButtonRelease) {
        const int textMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1;
        QRect checkRect = QStyle::alignedRect(option.direction, Qt::AlignLeft | Qt::AlignVCenter,
                                              check(option, option.rect, Qt::Checked).size(),
                                              QRect(option.rect.x() + textMargin, option.rect.y(),
                                                    option.rect.width(), option.rect.height()));
        if (!checkRect.contains(static_cast<QMouseEvent*>(event)->pos()))
            return false;
    } else if (event->type() == QEvent::KeyPress) {
        if (static_cast<QKeyEvent*>(event)->key() != Qt::Key_Space)
            return false;
    } else {
        return false;
    }

    // make sure that we have a check state
    QVariant value = index.data(Qt::CheckStateRole);
    if (!value.isValid())
        return false;

    Qt::CheckState state = (static_cast<Qt::CheckState>(value.toInt()) == Qt::Checked
                            ? Qt::Unchecked : Qt::Checked);
    return model->setData(index, state, Qt::CheckStateRole);
}

/*!
  \internal
*/

QStyleOptionViewItem QItemDelegate::setOptions(const QModelIndex &index,
                                               const QStyleOptionViewItem &option) const
{
    QStyleOptionViewItem opt = option;

    // set font
    QVariant value = index.data(Qt::FontRole);
    if (value.isValid()){
        opt.font = qvariant_cast<QFont>(value).resolve(opt.font);
        opt.fontMetrics = QFontMetrics(opt.font);
    }

    // set text alignment
    value = index.data(Qt::TextAlignmentRole);
    if (value.isValid())
        opt.displayAlignment = (Qt::Alignment)value.toInt();

    // set foreground brush
    value = index.data(Qt::ForegroundRole);
    if (qVariantCanConvert<QBrush>(value))
        opt.palette.setBrush(QPalette::Text, qvariant_cast<QBrush>(value));

    return opt;
}

#include "moc_qitemdelegate.cpp"

#endif // QT_NO_ITEMVIEWS

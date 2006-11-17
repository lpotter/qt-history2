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

#include "qcombobox.h"

#ifndef QT_NO_COMBOBOX
#include <qstylepainter.h>
#include <qlineedit.h>
#include <qapplication.h>
#include <qdesktopwidget.h>
#include <qlistview.h>
#include <qitemdelegate.h>
#include <qstandarditemmodel.h>
#include <qmap.h>
#include <qevent.h>
#include <qlayout.h>
#include <qscrollbar.h>
#ifndef QT_NO_IM
#include "qinputcontext.h"
#endif
#include <private/qcombobox_p.h>
#include <private/qabstractitemmodel_p.h>
#include <qdebug.h>

#ifdef Q_WS_X11
#include <private/qt_x11_p.h>
#endif

QComboBoxPrivate::QComboBoxPrivate()
    : QWidgetPrivate(),
      model(QAbstractItemModelPrivate::staticEmptyModel()),
      lineEdit(0),
      container(0),
      insertPolicy(QComboBox::InsertAtBottom),
      sizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow),
      minimumContentsLength(0),
      shownOnce(false),
      autoCompletion(true),
      duplicatesEnabled(false),
      frame(true),
      maxVisibleItems(10),
      maxCount(INT_MAX),
      modelColumn(0),
      arrowState(QStyle::State_None),
      hoverControl(QStyle::SC_None),
      autoCompletionCaseSensitivity(Qt::CaseInsensitive),
      indexBeforeChange(-1)
#ifndef QT_NO_COMPLETER
      , completer(0)
#endif
{
}

QStyleOptionMenuItem QComboMenuDelegate::getStyleOption(const QStyleOptionViewItem &option,
                                                  const QModelIndex &index) const
{
    QStyleOptionMenuItem menuOption;
    menuOption.palette = QApplication::palette("QMenu");
    menuOption.state = QStyle::State_None;
    if (mCombo->window()->isActiveWindow())
        menuOption.state = QStyle::State_Active;
    if ((option.state & QStyle::State_Enabled) && (index.model()->flags(index) & Qt::ItemIsEnabled))
        menuOption.state |= QStyle::State_Enabled;
    else
        menuOption.palette.setCurrentColorGroup(QPalette::Disabled);
    if (option.state & QStyle::State_Selected)
        menuOption.state |= QStyle::State_Selected;
    menuOption.checkType = QStyleOptionMenuItem::NonExclusive;
    menuOption.checked = mCombo->currentIndex() == index.row();
    menuOption.menuItemType = QStyleOptionMenuItem::Normal;
    QVariant decoration = index.model()->data(index, Qt::DecorationRole);
    if (decoration.type() == QVariant::Pixmap )
        menuOption.icon = QIcon(qvariant_cast<QPixmap>(decoration));
    else
        menuOption.icon = qvariant_cast<QIcon>(decoration);

    menuOption.text = index.model()->data(index, Qt::DisplayRole).toString()
                           .replace(QLatin1Char('&'), QLatin1String("&&"));
    menuOption.tabWidth = 0;
    menuOption.maxIconWidth =  option.decorationSize.width() + 4;
    menuOption.menuRect = option.rect;
    menuOption.rect = option.rect;
    extern QHash<QByteArray, QFont> *qt_app_fonts_hash();
    menuOption.font = qt_app_fonts_hash()->value("QComboMenuItem", mCombo->font());
    return menuOption;
}

void QComboBoxPrivate::updateArrow(QStyle::StateFlag state)
{
    Q_Q(QComboBox);
    if (arrowState == state)
        return;
    arrowState = state;
    QStyleOptionComboBox opt;
    q->initStyleOption(&opt);
    q->update(q->style()->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxArrow));
}

void QComboBoxPrivate::_q_modelReset()
{
    Q_Q(QComboBox);
    if (lineEdit) {
        lineEdit->setText(QString());
        updateLineEditGeometry();
    }
    q->update();
}

void QComboBoxPrivate::_q_modelDestroyed()
{
    model = QAbstractItemModelPrivate::staticEmptyModel();
}


//Windows and KDE allows menus to cover the taskbar, while GNOME and Mac don't
QRect QComboBoxPrivate::popupGeometry(int screen) const
{
#ifdef Q_WS_WIN
    return QApplication::desktop()->screenGeometry(screen);
#elif defined Q_WS_X11
    if (X11->desktopEnvironment == DE_KDE)
        return QApplication::desktop()->screenGeometry(screen);
    else
        return QApplication::desktop()->availableGeometry(screen);
#else
        return QApplication::desktop()->availableGeometry(screen);
#endif
}

bool QComboBoxPrivate::updateHoverControl(const QPoint &pos)
{

    Q_Q(QComboBox);
    QRect lastHoverRect = hoverRect;
    QStyle::SubControl lastHoverControl = hoverControl;
    bool doesHover = q->testAttribute(Qt::WA_Hover);
    if (lastHoverControl != newHoverControl(pos) && doesHover) {
        q->update(lastHoverRect);
        q->update(hoverRect);
        return true;
    }
    return !doesHover;
}

QStyle::SubControl QComboBoxPrivate::newHoverControl(const QPoint &pos)
{
    Q_Q(QComboBox);
    QStyleOptionComboBox opt;
    q->initStyleOption(&opt);
    opt.subControls = QStyle::SC_All;
    hoverControl = q->style()->hitTestComplexControl(QStyle::CC_ComboBox, &opt, pos, q);
    hoverRect = (hoverControl != QStyle::SC_None)
                   ? q->style()->subControlRect(QStyle::CC_ComboBox, &opt, hoverControl, q)
                   : QRect();
    return hoverControl;
}

/*
    Computes a size hint based on the maximum width
    for the items in the combobox.
*/
int QComboBoxPrivate::computeWidthHint() const
{
    Q_Q(const QComboBox);

    int width = 0;
    const int count = q->count();
    const int iconWidth = q->iconSize().width() + 4;
    const QFontMetrics &fontMetrics = q->fontMetrics();

    for (int i = 0; i < count; ++i) {
        const int textWidth = fontMetrics.width(q->itemText(i));
        if (q->itemIcon(i).isNull())
            width = (qMax(width, textWidth));
        else
            width = (qMax(width, textWidth + iconWidth));
    }

    QStyleOptionComboBox opt;
    q->initStyleOption(&opt);
    QSize tmp(width, 0);
    tmp = q->style()->sizeFromContents(QStyle::CT_ComboBox, &opt, tmp, q);
    return tmp.width();
}

QSize QComboBoxPrivate::recomputeSizeHint(QSize &sh) const
{
    Q_Q(const QComboBox);
    if (!sh.isValid()) {
        bool hasIcon = sizeAdjustPolicy == QComboBox::AdjustToMinimumContentsLengthWithIcon ? true : false;
        int count = q->count();
        QSize iconSize = q->iconSize();
        const QFontMetrics &fm = q->fontMetrics();

        // text width
        if (&sh == &sizeHint || minimumContentsLength == 0) {
            switch (sizeAdjustPolicy) {
            case QComboBox::AdjustToContents:
            case QComboBox::AdjustToContentsOnFirstShow:
                if (count == 0) {
                    sh.rwidth() = 7 * fm.width(QLatin1Char('x'));
                } else {
                    for (int i = 0; i < count; ++i) {
                        if (!q->itemIcon(i).isNull()) {
                            hasIcon = true;
                            sh.setWidth(qMax(sh.width(), fm.width(q->itemText(i)) + iconSize.width() + 4));
                        } else {
                            sh.setWidth(qMax(sh.width(), fm.width(q->itemText(i))));
                        }
                    }
                }
                break;
            case QComboBox::AdjustToMinimumContentsLength:
                for (int i = 0; i < count && !hasIcon; ++i)
                    hasIcon = !q->itemIcon(i).isNull();
            default:
                ;
            }
        } else {
            for (int i = 0; i < count && !hasIcon; ++i)
                hasIcon = !q->itemIcon(i).isNull();
        }
        if (minimumContentsLength > 0)
            sh.setWidth(qMax(sh.width(), minimumContentsLength * fm.width(QLatin1Char('X')) + (hasIcon ? iconSize.width() + 4 : 0)));


        // height
        sh.setHeight(qMax(fm.lineSpacing(), 14) + 2);
        if (hasIcon) {
            sh.setHeight(qMax(sh.height(), iconSize.height() + 2));
        }

        // add style and strut values
        QStyleOptionComboBox opt;
        q->initStyleOption(&opt);
        sh = q->style()->sizeFromContents(QStyle::CT_ComboBox, &opt, sh, q);
    }
    return sh.expandedTo(QApplication::globalStrut());
}

void QComboBoxPrivate::adjustComboBoxSize()
{
    viewContainer()->adjustSizeTimer.start(20, container);
}

void QComboBoxPrivateContainer::timerEvent(QTimerEvent *timerEvent)
{
    if (timerEvent->timerId() == adjustSizeTimer.timerId()) {
        adjustSizeTimer.stop();
        if (combo->sizeAdjustPolicy() == QComboBox::AdjustToContents) {
            combo->adjustSize();
            combo->update();
        }
    }
}

QComboBoxPrivateContainer::QComboBoxPrivateContainer(QAbstractItemView *itemView, QComboBox *parent)
    : QFrame(parent, Qt::Popup), combo(parent), view(0), top(0), bottom(0)
{
    // we need the combobox and itemview
    Q_ASSERT(parent);
    Q_ASSERT(itemView);

    setAttribute(Qt::WA_WindowPropagation);

    // setup container
    blockMouseReleaseTimer.setSingleShot(true);

    // we need a vertical layout
    QBoxLayout *layout =  new QBoxLayout(QBoxLayout::TopToBottom, this);
    layout->setSpacing(0);
    layout->setMargin(0);

    // set item view
    setItemView(itemView);

    // add scroller arrows if style needs them
    QStyleOptionComboBox opt = comboStyleOption();
    if (style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, this)) {
        top = new QComboBoxPrivateScroller(QAbstractSlider::SliderSingleStepSub, this);
        bottom = new QComboBoxPrivateScroller(QAbstractSlider::SliderSingleStepAdd, this);
        top->hide();
        bottom->hide();
    } else {
        setLineWidth(1);
    }

    setFrameStyle(style()->styleHint(QStyle::SH_ComboBox_PopupFrameStyle, &opt, this));

    if (top) {
        layout->insertWidget(0, top);
        connect(top, SIGNAL(doScroll(int)), this, SLOT(scrollItemView(int)));
    }
    if (bottom) {
        layout->addWidget(bottom);
        connect(bottom, SIGNAL(doScroll(int)), this, SLOT(scrollItemView(int)));
    }
}

void QComboBoxPrivateContainer::scrollItemView(int action)
{
#ifndef QT_NO_SCROLLBAR
    if (view->verticalScrollBar())
        view->verticalScrollBar()->triggerAction(static_cast<QAbstractSlider::SliderAction>(action));
#endif
}

/*
    Hides or shows the scrollers when we emulate a popupmenu
*/
void QComboBoxPrivateContainer::updateScrollers()
{
#ifndef QT_NO_SCROLLBAR
    if (!top || !bottom)
        return;

    if (isVisible() == false)
        return;

    QStyleOptionComboBox opt = comboStyleOption();
    if (combo->style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, combo) &&
        view->verticalScrollBar()->minimum() < view->verticalScrollBar()->maximum()) {

        bool needTop = view->verticalScrollBar()->value()
                       > (view->verticalScrollBar()->minimum() + spacing());
        bool needBottom = view->verticalScrollBar()->value()
                          < (view->verticalScrollBar()->maximum() - spacing()*2);
        if (needTop)
            top->show();
        else
            top->hide();
        if (needBottom)
            bottom->show();
        else
            bottom->hide();
    } else {
        top->hide();
        bottom->hide();
    }
#endif // QT_NO_SCROLLBAR
}

/*
    Cleans up when the view is destroyed.
*/
void QComboBoxPrivateContainer::viewDestroyed()
{
    view = 0;
    setItemView(new QComboBoxListView());
}

/*
    Sets currentIndex on entered if the LeftButton is not pressed. This
    means that if mouseTracking(...) is on, we setCurrentIndex and select
    even when LeftButton is not pressed.
*/
void QComboBoxPrivateContainer::setCurrentIndex(const QModelIndex &index)
{
    view->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
}

/*
    Returns the item view used for the combobox popup.
*/
QAbstractItemView *QComboBoxPrivateContainer::itemView() const
{
    return view;
}

/*!
    Sets the item view to be used for the combobox popup.
*/
void QComboBoxPrivateContainer::setItemView(QAbstractItemView *itemView)
{
    Q_ASSERT(itemView);

    // clean up old one
    if (view) {
        view->removeEventFilter(this);
        view->viewport()->removeEventFilter(this);
#ifndef QT_NO_SCROLLBAR
        disconnect(view->verticalScrollBar(), SIGNAL(valueChanged(int)),
                   this, SLOT(updateScrollers()));
        disconnect(view->verticalScrollBar(), SIGNAL(rangeChanged(int,int)),
                   this, SLOT(updateScrollers()));
#endif
        disconnect(view, SIGNAL(entered(QModelIndex)),
                   this, SLOT(setCurrentIndex(QModelIndex)));
        disconnect(view, SIGNAL(destroyed()),
                   this, SLOT(viewDestroyed()));
        delete view;
        view = 0;
    }

    // setup the item view
    view = itemView;
    view->setParent(this);
    qobject_cast<QBoxLayout*>(layout())->insertWidget(top ? 1 : 0, view);
    view->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    view->installEventFilter(this);
    view->viewport()->installEventFilter(this);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QStyleOptionComboBox opt = comboStyleOption();
#ifndef QT_NO_SCROLLBAR
    if (style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, combo))
        view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
#endif
    if (style()->styleHint(QStyle::SH_ComboBox_ListMouseTracking, &opt, combo) ||
        style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, combo)) {
        view->setMouseTracking(true);
    }
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setFrameStyle(QFrame::NoFrame);
    view->setLineWidth(0);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
#ifndef QT_NO_SCROLLBAR
    connect(view->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(updateScrollers()));
    connect(view->verticalScrollBar(), SIGNAL(rangeChanged(int,int)),
            this, SLOT(updateScrollers()));
#endif
    connect(view, SIGNAL(entered(QModelIndex)),
            this, SLOT(setCurrentIndex(QModelIndex)));
    connect(view, SIGNAL(destroyed()),
            this, SLOT(viewDestroyed()));
}

/*!
    Returns the spacing between the items in the view.
*/
int QComboBoxPrivateContainer::spacing() const
{
    QListView *lview = qobject_cast<QListView*>(view);
    if (lview)
        return lview->spacing();
    return 0;
}

void QComboBoxPrivateContainer::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::StyleChange) {
        QStyleOptionComboBox opt = comboStyleOption();
        view->setMouseTracking(style()->styleHint(QStyle::SH_ComboBox_ListMouseTracking, &opt, combo) ||
                               style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, combo));
    }
    QWidget::changeEvent(e);
}


bool QComboBoxPrivateContainer::eventFilter(QObject *o, QEvent *e)
{
    switch (e->type()) {
    case QEvent::KeyPress:
        switch (static_cast<QKeyEvent*>(e)->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
#ifdef QT_KEYPAD_NAVIGATION
        case Qt::Key_Select:
#endif
            if (view->currentIndex().isValid() && (view->currentIndex().flags() & Qt::ItemIsEnabled) ) {
                combo->hidePopup();
                emit itemSelected(view->currentIndex());
            }
            return true;
        case Qt::Key_Down:
            if (!(static_cast<QKeyEvent*>(e)->modifiers() & Qt::AltModifier))
                break;
            // fall through
        case Qt::Key_F4:
        case Qt::Key_Escape:
#ifdef QT_KEYPAD_NAVIGATION
        case Qt::Key_Back:
#endif
            combo->hidePopup();
            return true;
        default:
            break;
        }
    break;
    case QEvent::MouseMove: {
        if (isVisible()) {
            QMouseEvent *m = static_cast<QMouseEvent *>(e);
            QPoint vector = m->pos() - initialClickPosition;
            if (vector.manhattanLength() > 9 && blockMouseReleaseTimer.isActive())
                blockMouseReleaseTimer.stop();
        }
        break;
    }
    case QEvent::MouseButtonRelease: {
        QMouseEvent *m = static_cast<QMouseEvent *>(e);
        if (isVisible() && view->rect().contains(m->pos()) && view->currentIndex().isValid()
            && !blockMouseReleaseTimer.isActive()
            && (view->currentIndex().flags() & Qt::ItemIsEnabled)) {
            combo->hidePopup();
            emit itemSelected(view->currentIndex());
            return true;
        }
        break;
    }
    default:
        break;
    }
    return QFrame::eventFilter(o, e);
}

void QComboBoxPrivateContainer::showEvent(QShowEvent *)
{
        combo->update();
}

void QComboBoxPrivateContainer::hideEvent(QHideEvent *)
{
    emit resetButton();
    combo->update();
}

void QComboBoxPrivateContainer::mousePressEvent(QMouseEvent *e)
{

    QStyleOptionComboBox opt = comboStyleOption();
    opt.subControls = QStyle::SC_All;
    opt.activeSubControls = QStyle::SC_ComboBoxArrow;
    QStyle::SubControl sc = style()->hitTestComplexControl(QStyle::CC_ComboBox, &opt,
                                                           combo->mapFromGlobal(e->globalPos()),
                                                           combo);
    if ((combo->isEditable() && sc == QStyle::SC_ComboBoxArrow)
        || (!combo->isEditable() && sc != QStyle::SC_None))
        setAttribute(Qt::WA_NoMouseReplay);
    QFrame::mousePressEvent(e);
}

void QComboBoxPrivateContainer::mouseReleaseEvent(QMouseEvent *e)
{
    Q_UNUSED(e);
    emit resetButton();
}

QStyleOptionComboBox QComboBoxPrivateContainer::comboStyleOption() const
{
    QStyleOptionComboBox opt;
    opt.state = QStyle::State_None;
    opt.rect = combo->rect();
    opt.palette = combo->palette();
    opt.init(combo);
    opt.subControls = QStyle::SC_All;
    opt.activeSubControls = QStyle::SC_None;
    opt.editable = combo->isEditable();
    return opt;
}



/*!
    \enum QComboBox::InsertPolicy

    This enum specifies what the QComboBox should do when a new string is
    entered by the user.

    \value NoInsert             The string will not be inserted into the combobox.
    \value InsertAtTop          The string will be inserted as the first item in the combobox.
    \value InsertAtCurrent      The current item will be \e replaced by the string.
    \value InsertAtBottom       The string will be inserted after the last item in the combobox.
    \value InsertAfterCurrent   The string is inserted after the current item in the combobox.
    \value InsertBeforeCurrent  The string is inserted before the current item in the combobox.
    \value InsertAlphabetically The string is inserted in the alphabetic order in the combobox.
    \omitvalue NoInsertion
    \omitvalue AtTop
    \omitvalue AtCurrent
    \omitvalue AtBottom
    \omitvalue AfterCurrent
    \omitvalue BeforeCurrent
*/

/*!
    \enum QComboBox::SizeAdjustPolicy

    This enum specifies how the size hint of the QComboBox should
    adjust when new content is added or content changes.

    \value AdjustToContents              The combobox will always adjust to the contents
    \value AdjustToContentsOnFirstShow   The combobox will adjust to its contents the first time it is show.
    \value AdjustToMinimumContentsLength Use AdjustToContents or AdjustToContentsOnFirstShow instead.
    \value AdjustToMinimumContentsLengthWithIcon The combobox will adjust to \l minimumContentsLength plus space for an icon. For performance reasons use this policy on large models.
*/

/*!
    \fn void QComboBox::activated(int index)

    This signal is sent when an item in the combobox is activated by
    the user. The item's \a index is given.

*/

/*!
    \fn void QComboBox::activated(const QString &text)

    This signal is sent when an item in the combobox is activated by
    the user. The item's \a text is given.

*/

/*!
    \fn void QComboBox::highlighted(int index)

    This signal is sent when an item in the combobox popup list is highlighted by
    the user. The item's \a index is given.
*/

/*!
    \fn void QComboBox::highlighted(const QString &text)

    This signal is sent when an item in the combobox popup list is highlighted by
    the user. The item's \a text is given.
*/

/*!
    \fn void QComboBox::currentIndexChanged(int index)
    \since 4.1

    This signal is sent whenever the currentIndex in the combobox
    changes either through user interaction or programmatically. The
    item's \a index is given or -1 if the combobox becomes empty or the
    currentIndex was reset.
*/

/*!
    \fn void QComboBox::currentIndexChanged(const QString &text)
    \since 4.1

    This signal is sent whenever the currentIndex in the combobox
    changes either through user interaction or programmatically.  The
    item's \a text is given.
*/

/*!
    Constructs a combobox with the given \a parent, using the default
    model QStandardItemModel.
*/
QComboBox::QComboBox(QWidget *parent)
    : QWidget(*new QComboBoxPrivate(), parent, 0)
{
    Q_D(QComboBox);
    d->init();
}

/*!
  \internal
*/
QComboBox::QComboBox(QComboBoxPrivate &dd, QWidget *parent)
    : QWidget(dd, parent, 0)
{
    Q_D(QComboBox);
    d->init();
}

#ifdef QT3_SUPPORT
/*!
    Use one of the constructors that doesn't take the \a name
    argument and then use setObjectName() instead.
*/
QComboBox::QComboBox(QWidget *parent, const char *name)
    : QWidget(*new QComboBoxPrivate(), parent, 0)
{
    Q_D(QComboBox);
    d->init();
    setObjectName(QString::fromAscii(name));
}

/*!
    Use one of the constructors that doesn't take the \a name
    argument and then use setObjectName() instead.
*/
QComboBox::QComboBox(bool rw, QWidget *parent, const char *name)
    : QWidget(*new QComboBoxPrivate(), parent, 0)
{
    Q_D(QComboBox);
    d->init();
    setEditable(rw);
    setObjectName(QString::fromAscii(name));
}

#endif //QT3_SUPPORT

/*!
    \class QComboBox qcombobox.h
    \brief The QComboBox widget is a combined button and popup list.

    \ingroup basic
    \mainclass

    A QComboBox provides a means of presenting a list of options to the user
    in a way that takes up the minimum amount of screen space.

    A combobox is a selection widget that displays the current item,
    and can pop up a list of selectable items. A combobox may be editable,
    allowing the user to modify each item in the list.

    Comboboxes can contain pixmaps as well as strings; the
    insertItem() and changeItem() functions are suitably overloaded.
    For editable comboboxes, the function clearEdit() is provided,
    to clear the displayed string without changing the combobox's
    contents.

    There are two signals emitted if the current item of a combobox
    changes, currentIndexChanged() and activated().
    currentIndexChanged() is always emitted regardless if the change
    was done programmatically or by user interaction, while
    activated() is only emitted when the change is caused by user
    interaction. The highlighted() signal is emitted when the user
    highlights an item in the combobox popup list. All three signals
    exist in two versions, one with a QString argument and one with an
    \c int argument. If the user selectes or highlights a pixmap, only
    the \c int signals are emitted. Whenever the text of an editable
    combobox is changed the editTextChanged() signal is emitted.

    When the user enters a new string in an editable combobox, the
    widget may or may not insert it, and it can insert it in several
    locations. The default policy is is \l AtBottom but you can change
    this using setInsertPolicy().

    It is possible to constrain the input to an editable combobox
    using QValidator; see setValidator(). By default, any input is
    accepted.

    A combobox can be populated using the insert functions,
    insertStringList() and insertItem() for example. Items can be
    changed with changeItem(). An item can be removed with
    removeItem() and all items can be removed with clear(). The text
    of the current item is returned by currentText(), and the text of
    a numbered item is returned with text(). The current item can be
    set with setCurrentIndex(). The number of items in the combobox is
    returned by count(); the maximum number of items can be set with
    setMaxCount(). You can allow editing using setEditable(). For
    editable comboboxes you can set auto-completion using
    setCompleter() and whether or not the user can add duplicates
    is set with setDuplicatesEnabled().

    \image qstyle-comboboxes.png Comboboxes in the different built-in styles.

    \sa QLineEdit, QSpinBox, QRadioButton, QButtonGroup,
        {fowler}{GUI Design Handbook: Combo Box, Drop-Down List Box}
*/

void QComboBoxPrivate::init()
{
    Q_Q(QComboBox);
    q->setFocusPolicy(Qt::WheelFocus);
    q->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

QComboBoxPrivateContainer* QComboBoxPrivate::viewContainer()
{
    if (container)
        return container;

    Q_Q(QComboBox);
    container = new QComboBoxPrivateContainer(new QComboBoxListView(), q);
    container->itemView()->setModel(q->model());
    container->itemView()->setTextElideMode(Qt::ElideMiddle);
    QStyleOptionComboBox opt;
    q->initStyleOption(&opt);
    if (q->style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, q))
        q->setItemDelegate(new QComboMenuDelegate(container->itemView(), q));
    container->setLayoutDirection(Qt::LayoutDirection(
                                    q->style()->styleHint(QStyle::SH_ComboBox_LayoutDirection,
                                                          &opt, q)));
    QObject::connect(container, SIGNAL(itemSelected(QModelIndex)),
                     q, SLOT(_q_itemSelected(QModelIndex)));
    QObject::connect(container->itemView()->selectionModel(),
                     SIGNAL(currentChanged(QModelIndex,QModelIndex)),
                     q, SLOT(_q_emitHighlighted(QModelIndex)));
    QObject::connect(container, SIGNAL(resetButton()), q, SLOT(_q_resetButton()));
    return container;
}


void QComboBoxPrivate::_q_resetButton()
{
    updateArrow(QStyle::State_None);
}

void QComboBoxPrivate::_q_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    Q_Q(QComboBox);
    if (topLeft.parent() != root)
        return;

    if (sizeAdjustPolicy == QComboBox::AdjustToContents) {
        sizeHint = QSize();
        adjustComboBoxSize();
        q->updateGeometry();
    }

    if (currentIndex.row() >= topLeft.row() && currentIndex.row() <= bottomRight.row()) {
        if (lineEdit) {
            lineEdit->setText(q->itemText(currentIndex.row()));
            updateLineEditGeometry();
        }
        q->update();
    }
}

void QComboBoxPrivate::_q_rowsAboutToBeInserted(const QModelIndex & parent,
                                             int /*start*/, int /*end*/)
{
    if (parent != root)
        return;
    indexBeforeChange = currentIndex.row();
}

void QComboBoxPrivate::_q_rowsInserted(const QModelIndex &parent, int start, int end)
{
    Q_UNUSED(start);
    Q_UNUSED(end);
    Q_Q(QComboBox);
    if (parent != root)
        return;

    if (sizeAdjustPolicy == QComboBox::AdjustToContents) {
        sizeHint = QSize();
        adjustComboBoxSize();
        q->updateGeometry();
    }

    // set current index if combo was previously empty
    if (start == 0 && (end - start + 1) == q->count() && !currentIndex.isValid()) {
        q->setCurrentIndex(0);
    // need to emit changed if model updated index "silently"
    } else if (currentIndex.row() != indexBeforeChange) {
        if (lineEdit) {
            lineEdit->setText(q->itemText(currentIndex.row()));
            updateLineEditGeometry();
        }
        q->update();
        _q_emitCurrentIndexChanged(currentIndex.row());
    }
}

void QComboBoxPrivate::_q_rowsAboutToBeRemoved(const QModelIndex &parent, int /*start*/, int /*end*/)
{
    if (parent != root)
        return;

    indexBeforeChange = currentIndex.row();
}

void QComboBoxPrivate::_q_rowsRemoved(const QModelIndex &parent, int /*start*/, int /*end*/)
{
    Q_Q(QComboBox);
    if (parent != root)
        return;

    if (sizeAdjustPolicy == QComboBox::AdjustToContents) {
        sizeHint = QSize();
        adjustComboBoxSize();
        q->updateGeometry();
    }

    // model has changed the currentIndex
    if (currentIndex.row() != indexBeforeChange) {
        if (!currentIndex.isValid() && q->count()) {
            q->setCurrentIndex(qMin(q->count() - 1, qMax(indexBeforeChange, 0)));
            return;
        }

        if (lineEdit) {
            lineEdit->setText(q->itemText(currentIndex.row()));
            updateLineEditGeometry();
        }
        q->update();
        _q_emitCurrentIndexChanged(currentIndex.row());
    }
}


/*!
    Initialize \a option with the values from this QComboBox. This method
    is useful for subclasses when they need a QStyleOptionComboBox, but don't want
    to fill in all the information themselves.

    \sa QStyleOption::initFrom()
*/
void QComboBox::initStyleOption(QStyleOptionComboBox *option) const
{
    if (!option)
        return;

    Q_D(const QComboBox);
    option->initFrom(this);
    if (!isEditable() && hasFocus())
        option->state |= QStyle::State_Selected;
    option->subControls = QStyle::SC_All;
    if (d->arrowState == QStyle::State_Sunken) {
        option->activeSubControls = QStyle::SC_ComboBoxArrow;
        option->state |= d->arrowState;
    } else {
        option->activeSubControls = d->hoverControl;
    }
    option->editable = isEditable();
    option->frame = d->frame;
    if (d->currentIndex.isValid()) {
        option->currentText = currentText();
        option->currentIcon = itemIcon(d->currentIndex.row());
    }
    option->iconSize = iconSize();
    if (d->container && d->container->isVisible())
        option->state |= QStyle::State_On;
}

void QComboBoxPrivate::updateLineEditGeometry()
{
    if (!lineEdit)
        return;

    Q_Q(QComboBox);
    QStyleOptionComboBox opt;
    q->initStyleOption(&opt);
    QRect editRect = q->style()->subControlRect(QStyle::CC_ComboBox, &opt,
                                                QStyle::SC_ComboBoxEditField, q);
    if (!q->itemIcon(q->currentIndex()).isNull()) {
        QRect comboRect(editRect);
        editRect.setWidth(editRect.width() - q->iconSize().width() - 4);
        editRect = QStyle::alignedRect(q->layoutDirection(), Qt::AlignRight,
                                       editRect.size(), comboRect);
    }
    lineEdit->setGeometry(editRect);
}

void QComboBoxPrivate::_q_returnPressed()
{
    Q_Q(QComboBox);
    if (lineEdit && !lineEdit->text().isEmpty()) {
        if (q->count() >= maxCount && !(this->insertPolicy == QComboBox::InsertAtCurrent))
            return;
        lineEdit->deselect();
        lineEdit->end(false);
        QString text = lineEdit->text();
        // check for duplicates (if not enabled) and quit
        int index = -1;
        if (!duplicatesEnabled) {
            index = q->findText(text, Qt::MatchFixedString | Qt::MatchCaseSensitive);
            if (index == -1)
                index = q->findText(text, Qt::MatchFixedString);
            if (index != -1) {
                q->setCurrentIndex(index);
                emitActivated(currentIndex);
                return;
            }
        }
        switch (insertPolicy) {
        case QComboBox::InsertAtTop:
            index = 0;
            break;
        case QComboBox::InsertAtBottom:
            index = q->count();
            break;
        case QComboBox::InsertAtCurrent:
        case QComboBox::InsertAfterCurrent:
        case QComboBox::InsertBeforeCurrent:
            if (!q->count() || !currentIndex.isValid())
                index = 0;
            else if (insertPolicy == QComboBox::InsertAtCurrent)
                q->setItemText(q->currentIndex(), text);
            else if (insertPolicy == QComboBox::InsertAfterCurrent)
                index = q->currentIndex() + 1;
            else if (insertPolicy == QComboBox::InsertBeforeCurrent)
                index = q->currentIndex();
            break;
        case QComboBox::InsertAlphabetically:
            index = 0;
            for (int i=0; i< q->count(); i++, index++ ) {
                if (text.toLower() < q->itemText(i).toLower())
                    break;
            }
            break;
        case QComboBox::NoInsert:
        default:
            break;
        }
        if (index >= 0) {
            q->insertItem(index, text);
            q->setCurrentIndex(index);
            emitActivated(currentIndex);
        }
    }
}

void QComboBoxPrivate::_q_itemSelected(const QModelIndex &item)
{
    Q_Q(QComboBox);
    if (item != currentIndex) {
        q->setCurrentIndex(item.row());
    } else if (lineEdit) {
        lineEdit->selectAll();
        lineEdit->setText(q->itemText(currentIndex.row()));
    }
    emitActivated(currentIndex);
}

void QComboBoxPrivate::emitActivated(const QModelIndex &index)
{
    Q_Q(QComboBox);
    if (!index.isValid())
        return;
    QString text(itemText(index));
    emit q->activated(index.row());
    emit q->activated(text);
}

void QComboBoxPrivate::_q_emitHighlighted(const QModelIndex &index)
{
    Q_Q(QComboBox);
    if (!index.isValid())
        return;
    QString text(itemText(index));
    emit q->highlighted(index.row());
    emit q->highlighted(text);
}

void QComboBoxPrivate::_q_emitCurrentIndexChanged(int index)
{
    Q_Q(QComboBox);
    QModelIndex mi = q->model()->index(index, modelColumn, q->rootModelIndex());
    QString text;
    if (mi.isValid())
        text = itemText(mi);
    emit q->currentIndexChanged(index);
    emit q->currentIndexChanged(text);
}

QString QComboBoxPrivate::itemText(const QModelIndex &index) const
{
    return q_func()->model()->data(index, itemRole()).toString();
}

int QComboBoxPrivate::itemRole() const
{
    return q_func()->isEditable() ? Qt::EditRole : Qt::DisplayRole;
}

/*!
    Destroys the combobox.
*/
QComboBox::~QComboBox()
{
    // ### check delegateparent and delete delegate if us?
    Q_D(QComboBox);

    disconnect(d->model, SIGNAL(destroyed()),
               this, SLOT(_q_modelDestroyed()));
}

/*!
    \property QComboBox::maxVisibleItems
    \brief the maximum allowed size on screen of the combobox

    This property is ignored for non-editable comboboxes in Mac
    style.
*/
int QComboBox::maxVisibleItems() const
{
    Q_D(const QComboBox);
    return d->maxVisibleItems;
}

void QComboBox::setMaxVisibleItems(int maxItems)
{
    Q_D(QComboBox);
    if (maxItems < 0) {
        qWarning("QComboBox::setMaxVisibleItems: "
                 "Invalid max visible items (%d) must be >= 0", maxItems);
        return;
    }
    d->maxVisibleItems = maxItems;
}

/*!
    \property QComboBox::count
    \brief the number of items in the combobox
*/
int QComboBox::count() const
{
    return model()->rowCount(rootModelIndex());
}

/*!
    \property QComboBox::maxCount
    \brief the maximum number of items allowed in the combobox

    Note: If you set the maximum number to be less then the current
    amount of items in the combobox, the extra items will be
    truncated. This also applies if you have set an external model on
    the combobox.
*/
void QComboBox::setMaxCount(int max)
{
    Q_D(QComboBox);
    if (max < 0) {
        qWarning("QComboBox::setMaxCount: Invalid count (%d) must be >= 0", max);
        return;
    }

    if (max < count())
        model()->removeRows(max, count() - max, rootModelIndex());

    d->maxCount = max;
}

int QComboBox::maxCount() const
{
    Q_D(const QComboBox);
    return d->maxCount;
}

#ifndef QT_NO_COMPLETER

/*!
    \property QComboBox::autoCompletion
    \brief whether the combobox provides auto-completion for editable items
    \since 4.1
    \obsolete

    Use setCompleter() instead.

    \sa editable
*/

/*!
    \obsolete

    Use setCompleter() instead.
*/
bool QComboBox::autoCompletion() const
{
    Q_D(const QComboBox);
    return d->autoCompletion;
}

/*!
    \obsolete

    Use setCompleter() instead.
*/
void QComboBox::setAutoCompletion(bool enable)
{
    Q_D(QComboBox);
    d->autoCompletion = enable;
    if (!d->lineEdit)
        return;
    if (enable) {
        if (d->lineEdit->completer())
            return;
        d->completer = new QCompleter(d->lineEdit);
        d->completer->setCaseSensitivity(d->autoCompletionCaseSensitivity);
        d->completer->setCompletionMode(QCompleter::InlineCompletion);
        d->completer->setModel(model());
        d->completer->setCompletionColumn(d->modelColumn);
        d->lineEdit->setCompleter(d->completer);
        d->completer->setWidget(this);
    } else {
        d->lineEdit->setCompleter(0);
    }
}

/*!
    \property QComboBox::autoCompletionCaseSensitivity
    \brief whether string comparisons are case-sensitive or case-insensitive for auto-completion
    \obsolete

    By default, this property is Qt::CaseInsensitive.

    Use setCompleter() instead. Case sensitivity of the auto completion can be
    changed using QCompleter::setCaseSensitivity().

    \sa autoCompletion
*/

/*!
    \obsolete

    Use setCompleter() and QCompleter::setCaseSensitivity() instead.
*/
Qt::CaseSensitivity QComboBox::autoCompletionCaseSensitivity() const
{
    Q_D(const QComboBox);
    return d->autoCompletionCaseSensitivity;
}

/*!
    \obsolete

    Use setCompleter() and QCompleter::setCaseSensitivity() instead.
*/
void QComboBox::setAutoCompletionCaseSensitivity(Qt::CaseSensitivity sensitivity)
{
    Q_D(QComboBox);
    d->autoCompletionCaseSensitivity = sensitivity;
    if (d->lineEdit && d->lineEdit->completer())
        d->lineEdit->completer()->setCaseSensitivity(sensitivity);
}

#endif // QT_NO_COMPLETER

/*!
    \property QComboBox::duplicatesEnabled
    \brief whether the user can enter duplicate items into the combobox

    Note that it is always possible to programatically insert duplicate items into the
    combobox.
*/
bool QComboBox::duplicatesEnabled() const
{
    Q_D(const QComboBox);
    return d->duplicatesEnabled;
}

void QComboBox::setDuplicatesEnabled(bool enable)
{
    Q_D(QComboBox);
    d->duplicatesEnabled = enable;
}

/*!  \fn int QComboBox::findText(const QString &text, Qt::MatchFlags flags = Qt::MatchExactly|Qt::MatchCaseSensitive) const

  Returns the index of the item containing the given \a text; otherwise
  returns -1.

  The \a flags specify how the items in the combobox are searched.
*/

/*!
  Returns the index of the item containing the given \a data for the
  given \a role; otherwise returns -1.

  The \a flags specify how the items in the combobox are searched.
*/
int QComboBox::findData(const QVariant &data, int role, Qt::MatchFlags flags) const
{
    Q_D(const QComboBox);
    QModelIndexList result;
    QModelIndex start = model()->index(0, d->modelColumn, rootModelIndex());
    result = model()->match(start, role, data, 1, flags);
    if (result.isEmpty())
        return -1;
    return result.first().row();
}

/*!
    \property QComboBox::insertPolicy
    \brief the policy used to determine where user-inserted items should
    appear in the combobox

    The default value is \l AtBottom, indicating that new items will appear
    at the bottom of the list of items.

    \sa InsertPolicy
*/

QComboBox::InsertPolicy QComboBox::insertPolicy() const
{
    Q_D(const QComboBox);
    return d->insertPolicy;
}

void QComboBox::setInsertPolicy(InsertPolicy policy)
{
    Q_D(QComboBox);
    d->insertPolicy = policy;
}

/*!
    \property QComboBox::sizeAdjustPolicy
    \brief the policy describing how the size of the combobox changes
    when the content changes

    The default value is \l AdjustToContentsOnFirstShow.

    \sa SizeAdjustPolicy
*/

QComboBox::SizeAdjustPolicy QComboBox::sizeAdjustPolicy() const
{
    Q_D(const QComboBox);
    return d->sizeAdjustPolicy;
}

void QComboBox::setSizeAdjustPolicy(QComboBox::SizeAdjustPolicy policy)
{
    Q_D(QComboBox);
    if (policy == d->sizeAdjustPolicy)
        return;

    d->sizeAdjustPolicy = policy;
    d->sizeHint = QSize();
    d->adjustComboBoxSize();
    updateGeometry();
}

/*!
    \property QComboBox::minimumContentsLength
    \brief the minimum number of characters that should fit into the combobox.

    The default value is 0.

    If this property is set to a positive value, the
    minimumSizeHint() and sizeHint() take it into account.

    \sa sizeAdjustPolicy
*/
int QComboBox::minimumContentsLength() const
{
    Q_D(const QComboBox);
    return d->minimumContentsLength;
}

void QComboBox::setMinimumContentsLength(int characters)
{
    Q_D(QComboBox);
    if (characters == d->minimumContentsLength || characters < 0)
        return;

    d->minimumContentsLength = characters;

    if (d->sizeAdjustPolicy == AdjustToContents
            || d->sizeAdjustPolicy == AdjustToMinimumContentsLength
            || d->sizeAdjustPolicy == AdjustToMinimumContentsLengthWithIcon) {
        d->sizeHint = QSize();
        d->adjustComboBoxSize();
        updateGeometry();
    }
}

/*!
    \property QComboBox::iconSize
    \brief the size of the icons shown in the combobox.

    Unless explicitly set this returns the default value of the current style.
*/

QSize QComboBox::iconSize() const
{
    Q_D(const QComboBox);
    if (d->iconSize.isValid())
        return d->iconSize;

    int iconWidth = style()->pixelMetric(QStyle::PM_SmallIconSize);
    return QSize(iconWidth, iconWidth);
}

void QComboBox::setIconSize(const QSize &size)
{
    Q_D(QComboBox);
    if (size == d->iconSize)
        return;

    view()->setIconSize(size);
    d->iconSize = size;
    d->sizeHint = QSize();
    updateGeometry();
}

/*!
    \property QComboBox::editable
    \brief whether the combobox can be edited by the user
*/
bool QComboBox::isEditable() const
{
    Q_D(const QComboBox);
    return d->lineEdit != 0;
}

void QComboBox::setEditable(bool editable)
{
    Q_D(QComboBox);
    if (isEditable() == editable)
        return;
    setAttribute(Qt::WA_InputMethodEnabled, editable);

    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    if (editable) {
        if (style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, this)) {
            setItemDelegate(new QItemDelegate(view()));
            d->viewContainer()->updateScrollers();
            view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        }
        QLineEdit *le = new QLineEdit(this);
        le->setLayoutDirection(Qt::LayoutDirection(
                               style()->styleHint(QStyle::SH_ComboBox_LayoutDirection,
                                                  &opt, this)));
        setLineEdit(le);
    } else {
        if (style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, this)) {
            setItemDelegate(new QComboMenuDelegate(view(), this));
            d->viewContainer()->updateScrollers();
            view()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
        delete d->lineEdit;
        d->lineEdit = 0;
    }
}

/*!
    Sets the line \a edit to use instead of the current line edit widget.

    The combo box takes ownership of the line edit.
*/
void QComboBox::setLineEdit(QLineEdit *edit)
{
    Q_D(QComboBox);
    if (!edit) {
        qWarning("QComboBox::setLineEdit: cannot set a 0 line edit");
        return;
    }

    if (edit == d->lineEdit)
        return;

    edit->setText(currentText());
    delete d->lineEdit;

    d->lineEdit = edit;
    if (d->lineEdit->parent() != this)
	d->lineEdit->setParent(this);
    connect(d->lineEdit, SIGNAL(returnPressed()), this, SLOT(_q_returnPressed()));
    connect(d->lineEdit, SIGNAL(textChanged(QString)), this, SIGNAL(editTextChanged(QString)));
#ifdef QT3_SUPPORT
    connect(d->lineEdit, SIGNAL(textChanged(QString)), this, SIGNAL(textChanged(QString)));
#endif
    d->lineEdit->setFrame(false);
    d->lineEdit->setContextMenuPolicy(Qt::NoContextMenu);
    d->lineEdit->setFocusProxy(this);
#ifndef QT_NO_COMPLETER
    setAutoCompletion(d->autoCompletion);
#endif
    setAttribute(Qt::WA_InputMethodEnabled);
    d->updateLineEditGeometry();

    if (isVisible())
        d->lineEdit->show();

    update();
}

/*!
    Returns the line edit used to edit items in the combobox, or 0 if there
    is no line edit.

    Only editable combo boxes have a line edit.
*/
QLineEdit *QComboBox::lineEdit() const
{
    Q_D(const QComboBox);
    return d->lineEdit;
}

#ifndef QT_NO_VALIDATOR
/*!
    \fn void QComboBox::setValidator(const QValidator *validator)

    Sets the \a validator to use instead of the current validator.
*/

void QComboBox::setValidator(const QValidator *v)
{
    Q_D(QComboBox);
    if (d->lineEdit)
        d->lineEdit->setValidator(v);
}

/*!
    Returns the validator that is used to constrain text input for the
    combobox.

    \sa editable
*/
const QValidator *QComboBox::validator() const
{
    Q_D(const QComboBox);
    return d->lineEdit ? d->lineEdit->validator() : 0;
}
#endif // QT_NO_VALIDATOR

#ifndef QT_NO_COMPLETER

/*!
    \fn void QComboBox::setCompleter(QCompleter *completer)
    \since 4.2

    Sets the \a completer to use instead of the current completer.
    If \a completer is 0, auto completion is disabled.

    By default, for an editable combo box, a QCompleter that
    performs case insensitive inline completion is automatically created.
*/
void QComboBox::setCompleter(QCompleter *c)
{
    Q_D(QComboBox);
    if (!d->lineEdit)
        return;
    d->lineEdit->setCompleter(c);
    if (c)
        c->setWidget(this);
}

/*!
    \since 4.2

    Returns the completer that is used to auto complete text input for the
    combobox.

    \sa editable
*/
QCompleter *QComboBox::completer() const
{
    Q_D(const QComboBox);
    return d->lineEdit ? d->lineEdit->completer() : 0;
}

#endif // QT_NO_COMPLETER

/*!
    Returns the item delegate used by the popup list view.

    \sa setItemDelegate()
*/
QAbstractItemDelegate *QComboBox::itemDelegate() const
{
    return view()->itemDelegate();
}

/*!
    Sets the item \a delegate for the popup list view.
    The combobox takes ownership of the delegate.

    \sa itemDelegate()
*/
void QComboBox::setItemDelegate(QAbstractItemDelegate *delegate)
{
    if (!delegate) {
        qWarning("QComboBox::setItemDelegate: cannot set a 0 delegate");
        return;
    }
    delete view()->itemDelegate();
    view()->setItemDelegate(delegate);
}

/*!
    Returns the model used by the combobox.
*/

QAbstractItemModel *QComboBox::model() const
{
    Q_D(const QComboBox);
    if (d->model == QAbstractItemModelPrivate::staticEmptyModel()) {
        QComboBox *that = const_cast<QComboBox*>(this);
        that->setModel(new QStandardItemModel(0, 1, that));
    }
    return d->model;
}

/*!
    Sets the model to be \a model. \a model must not be 0.
    If you want to clear the contents of a model, call clear().

    \sa clear()
*/
void QComboBox::setModel(QAbstractItemModel *model)
{
    Q_D(QComboBox);
    if (!model) {
        qWarning("QComboBox::setModel: cannot set a 0 model");
        return;
    }

#ifndef QT_NO_COMPLETER
    if (d->lineEdit && d->lineEdit->completer()
        && d->lineEdit->completer() == d->completer)
        d->lineEdit->completer()->setModel(model);
#endif

    disconnect(d->model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
               this, SLOT(_q_dataChanged(QModelIndex,QModelIndex)));
    disconnect(d->model, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
               this, SLOT(_q_rowsAboutToBeInserted(QModelIndex,int,int)));
    disconnect(d->model, SIGNAL(rowsInserted(QModelIndex,int,int)),
               this, SLOT(_q_rowsInserted(QModelIndex,int,int)));
    disconnect(d->model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
               this, SLOT(_q_rowsAboutToBeRemoved(QModelIndex,int,int)));
    disconnect(d->model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
               this, SLOT(_q_rowsRemoved(QModelIndex,int,int)));
    disconnect(d->model, SIGNAL(destroyed()),
               this, SLOT(_q_modelDestroyed()));
    disconnect(d->model, SIGNAL(modelReset()),
               this, SLOT(_q_modelReset()));
    if (d->model->QObject::parent() == this)
        delete d->model;

    d->model = (model ? model : QAbstractItemModelPrivate::staticEmptyModel());

    connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(_q_dataChanged(QModelIndex,QModelIndex)));
    connect(model, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
            this, SLOT(_q_rowsAboutToBeInserted(QModelIndex,int,int)));
    connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(_q_rowsInserted(QModelIndex,int,int)));
    connect(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
            this, SLOT(_q_rowsAboutToBeRemoved(QModelIndex,int,int)));
    connect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(_q_rowsRemoved(QModelIndex,int,int)));
    connect(model, SIGNAL(destroyed()),
            this, SLOT(_q_modelDestroyed()));
    connect(model, SIGNAL(modelReset()),
            this, SLOT(_q_modelReset()));

    if (d->container)
        d->container->itemView()->setModel(model);

    if (count()) {
        for (int pos=0; pos < count(); pos++) {
            if (d->model->index(pos, d->modelColumn, rootModelIndex()).flags() & Qt::ItemIsEnabled) {
                setCurrentIndex(pos);
                break;
            }
        }
    } else
        setCurrentIndex(-1);
}

/*!
    Returns the root model item index for the items in the combobox.

    \sa setRootModelIndex()
*/

QModelIndex QComboBox::rootModelIndex() const
{
    Q_D(const QComboBox);
    return QModelIndex(d->root);
}

/*!
    Sets the root model item \a index for the items in the combobox.

    \sa rootModelIndex()
*/
void QComboBox::setRootModelIndex(const QModelIndex &index)
{
    Q_D(QComboBox);
    d->root = QPersistentModelIndex(index);
    view()->setRootIndex(index);
    update();
}

/*!
    \property QComboBox::currentIndex
    \brief the index of the current item in the combobox. The index
    can change when inserting or removing items. Returns -1 if no
    current item is set or the combobox is empty.
*/
int QComboBox::currentIndex() const
{
    Q_D(const QComboBox);
    return d->currentIndex.row();
}

void QComboBox::setCurrentIndex(int index)
{
    Q_D(QComboBox);
    QModelIndex mi = model()->index(index, d->modelColumn, rootModelIndex());

    bool indexChanged = (mi != d->currentIndex);
    if (indexChanged)
        d->currentIndex = QPersistentModelIndex(mi);
    if (d->lineEdit && d->lineEdit->text() != itemText(d->currentIndex.row())) {
        d->lineEdit->setText(itemText(d->currentIndex.row()));
        d->updateLineEditGeometry();
    }
    if (indexChanged) {
        update();
        d->_q_emitCurrentIndexChanged(d->currentIndex.row());
    }
}

/*!
    \property QComboBox::currentText
    \brief the text of the current item
*/
QString QComboBox::currentText() const
{
    Q_D(const QComboBox);
    if (d->lineEdit)
        return d->lineEdit->text();
    else if (d->currentIndex.isValid())
	return d->itemText(d->currentIndex);
    else
	return QString();
}

/*!
    Returns the text for the given \a index in the combobox.
*/
QString QComboBox::itemText(int index) const
{
    Q_D(const QComboBox);
    QModelIndex mi = model()->index(index, d->modelColumn, rootModelIndex());
    return d->itemText(mi);
}

/*!
    Returns the icon for the given \a index in the combobox.
*/
QIcon QComboBox::itemIcon(int index) const
{
    Q_D(const QComboBox);
    QVariant decoration = model()->data(model()->index(index, d->modelColumn, rootModelIndex()), Qt::DecorationRole);
    if (decoration.type() == QVariant::Pixmap)
        return QIcon(qvariant_cast<QPixmap>(decoration));
    else
        return qvariant_cast<QIcon>(decoration);
}

/*!
   Returns the data for the given \a role in the given \a index in the
   combobox, or QVariant::Invalid if there is no data for this role.
*/
QVariant QComboBox::itemData(int index, int role) const
{
    Q_D(const QComboBox);
    QModelIndex mi = model()->index(index, d->modelColumn, rootModelIndex());
    return model()->data(mi, role);
}

/*!
  \fn void QComboBox::insertItem(int index, const QString &text, const QVariant &userData)

  Inserts the \a text and \a userData into the combobox at the given \a index.

  If the index is equal to or higher than the total number of items, the new item
  is appended to the list of existing items. If the index is zero or negative, the
  new item is prepended to the list of existing items.

  \sa insertItems()
*/

/*!
    Inserts the \a icon, \a text and \a userData into the combobox at the given \a index.

    If the index is equal to or higher than the total number of items, the new item
    is appended to the list of existing items. If the index is zero or negative, the
    new item is prepended to the list of existing items.

    \sa insertItems()
*/
void QComboBox::insertItem(int index, const QIcon &icon, const QString &text, const QVariant &userData)
{
    Q_D(QComboBox);
    if (index < 0)
        index = 0;
    else if (index > count())
        index = count();
    if (index >= d->maxCount)
        return;

    QModelIndex item;
    disconnect(model(), SIGNAL(rowsInserted(QModelIndex,int,int)),
               this, SLOT(_q_rowsInserted(QModelIndex,int,int)));
    disconnect(model(), SIGNAL(dataChanged(QModelIndex,QModelIndex)),
               this, SLOT(_q_dataChanged(QModelIndex,QModelIndex)));
    if (model()->insertRows(index, 1, rootModelIndex())) {
        item = model()->index(index, d->modelColumn, rootModelIndex());
        QMap<int, QVariant> values;
        if (!text.isNull()) values.insert(Qt::EditRole, text);
        if (!icon.isNull()) values.insert(Qt::DecorationRole, icon);
        if (!userData.isNull()) values.insert(Qt::UserRole, userData);
        if (!values.isEmpty()) model()->setItemData(item, values);
        d->_q_rowsInserted(rootModelIndex(), index, index);
    }
    connect(model(), SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(_q_rowsInserted(QModelIndex,int,int)));
    connect(model(), SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(_q_dataChanged(QModelIndex,QModelIndex)));

    int mc = count();
    if (mc > d->maxCount)
        model()->removeRows(mc - 1, mc - d->maxCount, rootModelIndex());
}

/*!
    Inserts the strings from the \a list into the combobox as separate items,
    starting at the \a index specified.

    If the index is equal to or higher than the total number of items, the new items
    are appended to the list of existing items. If the index is zero or negative, the
    new items are prepended to the list of existing items.

    \sa insertItem()
    */
void QComboBox::insertItems(int index, const QStringList &list)
{
    Q_D(QComboBox);
    if (list.isEmpty())
        return;

    if (index < 0)
        index = 0;
    else if (index < 0 || index > count())
        index = count();

    int insertCount = qMin(d->maxCount - index, list.count());
    if (insertCount <= 0)
        return;
    disconnect(model(), SIGNAL(rowsInserted(QModelIndex,int,int)),
               this, SLOT(_q_rowsInserted(QModelIndex,int,int)));
    disconnect(model(), SIGNAL(dataChanged(QModelIndex,QModelIndex)),
               this, SLOT(_q_dataChanged(QModelIndex,QModelIndex)));
    if (model()->insertRows(index, insertCount, rootModelIndex())) {
        QModelIndex item;
        for (int i = 0; i < insertCount; ++i) {
            item = model()->index(i+index, d->modelColumn, rootModelIndex());
            model()->setData(item, list.at(i), Qt::EditRole);
        }
        d->_q_rowsInserted(rootModelIndex(), index, index + insertCount - 1);
    }
    connect(model(), SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(_q_rowsInserted(QModelIndex,int,int)));
    connect(model(), SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(_q_dataChanged(QModelIndex,QModelIndex)));
    int mc = count();
    if (mc > d->maxCount)
        model()->removeRows(d->maxCount, mc - d->maxCount, rootModelIndex());
}

/*!
    Removes the item at the given \a index from the combobox.
    This will update the current index if the index is removed.
*/
void QComboBox::removeItem(int index)
{
    Q_ASSERT(index >= 0 && index < count());
    model()->removeRows(index, 1, rootModelIndex());
}

/*!
    Sets the \a text for the item on the given \a index in the combobox.
*/
void QComboBox::setItemText(int index, const QString &text)
{
    Q_D(const QComboBox);
    QModelIndex item = model()->index(index, d->modelColumn, rootModelIndex());
    if (item.isValid()) {
        model()->setData(item, text, Qt::EditRole);
    }
}

/*!
    Sets the \a icon for the item on the given \a index in the combobox.
*/
void QComboBox::setItemIcon(int index, const QIcon &icon)
{
    Q_D(const QComboBox);
    QModelIndex item = model()->index(index, d->modelColumn, rootModelIndex());
    if (item.isValid()) {
        model()->setData(item, icon, Qt::DecorationRole);
    }
}

/*!
    Sets the data \a role for the item on the given \a index in the combobox
    to the specified \a value.
*/
void QComboBox::setItemData(int index, const QVariant &value, int role)
{
    Q_D(const QComboBox);
    QModelIndex item = model()->index(index, d->modelColumn, rootModelIndex());
    if (item.isValid()) {
        model()->setData(item, value, role);
    }
}

/*!
    Returns the list view used for the combobox popup.
*/
QAbstractItemView *QComboBox::view() const
{
    Q_D(const QComboBox);
    return const_cast<QComboBoxPrivate*>(d)->viewContainer()->itemView();
}

/*!
  Sets the view to be used in the combobox popup to the given \a
  itemView. The combobox takes ownership of the view.

  Note: If you want to use the convenience views (like QListWidget,
  QTableWidget or QTreeWidget), make sure to call setModel() on the
  combobox with the convenience widgets model before calling this
  function.
*/
void QComboBox::setView(QAbstractItemView *itemView)
{
    Q_D(QComboBox);
    if (!itemView) {
        qWarning("QComboBox::setView: cannot set a 0 view");
        return;
    }

    if (itemView->model() != d->model)
        itemView->setModel(d->model);
    d->viewContainer()->setItemView(itemView);
}

/*!
    \reimp
*/
QSize QComboBox::minimumSizeHint() const
{
    Q_D(const QComboBox);
    return d->recomputeSizeHint(d->minimumSizeHint);
}

/*!
    \reimp

    This implementation caches the size hint to avoid resizing when
    the contents change dynamically. To invalidate the cached value
    change the \l sizeAdjustPolicy.
*/
QSize QComboBox::sizeHint() const
{
    Q_D(const QComboBox);
    return d->recomputeSizeHint(d->sizeHint);
}

/*!
    Displays the list of items in the combobox. If the list is empty then
    the no items will be shown.
*/
void QComboBox::showPopup()
{
    Q_D(QComboBox);
    if (count() <= 0)
        return;

    // set current item and select it
    view()->selectionModel()->setCurrentIndex(d->currentIndex,
                                              QItemSelectionModel::ClearAndSelect);
    QComboBoxPrivateContainer* container = d->viewContainer();
    // use top item as height for complete listView
    int itemHeight = view()->sizeHintForIndex(model()->index(0, d->modelColumn, rootModelIndex())).height()
                     + container->spacing();
    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    QRect listRect(style()->subControlRect(QStyle::CC_ComboBox, &opt,
                                           QStyle::SC_ComboBoxListBoxPopup, this));
    QRect screen = d->popupGeometry(QApplication::desktop()->screenNumber(this));
    QPoint below = mapToGlobal(listRect.bottomLeft());
    int belowHeight = screen.bottom() - below.y();
    QPoint above = mapToGlobal(listRect.topLeft());
    int aboveHeight = above.y() - screen.y();

    if (style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, this))
        listRect.setHeight(itemHeight * count());
    else
        listRect.setHeight(itemHeight * qMin(d->maxVisibleItems, count()));
    listRect.setHeight(listRect.height() + 2*container->spacing()
                       + style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &opt, this) * 2);

    // Make sure the popup is wide enough to display its contents.
    if (style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, this)) {
        const int diff = d->computeWidthHint() - width();
        if (diff > 0)
            listRect.setWidth(listRect.width() + diff);
    }

    // make sure the widget fits on screen
    if (listRect.width() > screen.width() )
        listRect.setWidth(screen.width());
    if (mapToGlobal(listRect.bottomRight()).x() > screen.right()) {
        below.setX(screen.x() + screen.width() - listRect.width());
        above.setX(screen.x() + screen.width() - listRect.width());
    }
    if (mapToGlobal(listRect.topLeft()).x() < screen.x() ) {
        below.setX(screen.x());
        above.setX(screen.x());
    }

    if (style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, this)) {

        // Position horizontally.
        listRect.moveLeft(above.x());

        // Position vertically so the curently selected item lines up
        // with the combo box.
        const QRect currentItemRect = view()->visualRect(view()->currentIndex());
        const int offset = listRect.top() -  currentItemRect.top();
        listRect.moveTop(above.y() + offset);

        // Clamp the listRect height and vertical position so we don't expand outside the
        // available screen geometry.This may override the vertical position, but it is more
        // important to show as much as possible of the popup.
        const int height = qMin(listRect.height(), screen.height());
        listRect.setHeight(height);
        if (listRect.top() < screen.top())
            listRect.moveTop(screen.top());
        if (listRect.bottom() > screen.bottom())
            listRect.moveBottom(screen.bottom());
    } else if (listRect.height() <= belowHeight) {
        listRect.moveTopLeft(below);
    } else if (listRect.height() <= aboveHeight) {
        listRect.moveBottomLeft(above);
    } else if (belowHeight >= aboveHeight) {
        listRect.setHeight(belowHeight);
        listRect.moveTopLeft(below);
    } else {
        listRect.setHeight(aboveHeight);
        listRect.moveBottomLeft(above);
    }

#ifndef QT_NO_IM
    if (QInputContext *qic = inputContext())
        qic->reset();
#endif
    QScrollBar *sb = view()->horizontalScrollBar();
    Qt::ScrollBarPolicy policy = view()->horizontalScrollBarPolicy();
    bool needHorizontalScrollBar = (policy == Qt::ScrollBarAsNeeded || policy == Qt::ScrollBarAlwaysOn)
        && sb->minimum() < sb->maximum();
    if (needHorizontalScrollBar) {
        listRect.adjust(0, 0, 0, sb->height());
    }
    container->setGeometry(listRect);

    if (style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, this))
        view()->scrollTo(view()->currentIndex(), QAbstractItemView::PositionAtCenter);
    else
        view()->scrollTo(view()->currentIndex(), QAbstractItemView::EnsureVisible);

    container->raise();
    container->show();
    container->updateScrollers();
    view()->setFocus();
}

/*!
    Hides the list of items in the combobox if it is currently visible;
    otherwise this function does nothing.
*/
void QComboBox::hidePopup()
{
    Q_D(QComboBox);
    if (d->container && d->container->isVisible())
        d->container->hide();
#ifdef QT_KEYPAD_NAVIGATION
    if (QApplication::keypadNavigationEnabled() && isEditable() && hasFocus())
        setEditFocus(true);
#endif
}

/*!
    Clears the combobox, removing all items.

    Note: If you have set an external model on the combobox this model
    will still be cleared when calling this function.
*/
void QComboBox::clear()
{
    model()->removeRows(0, model()->rowCount(rootModelIndex()), rootModelIndex());
}

/*!
    \fn void QComboBox::clearValidator()

    Use setValidator(0) instead.
*/

/*!
    Clears the contents of the line edit used for editing in the combobox.
*/
void QComboBox::clearEditText()
{
    Q_D(QComboBox);
    if (d->lineEdit)
        d->lineEdit->clear();
}

/*!
    Sets the \a text in the combobox's text edit.
*/
void QComboBox::setEditText(const QString &text)
{
    Q_D(QComboBox);
    if (d->lineEdit)
        d->lineEdit->setText(text);
}

/*!
    \reimp
*/
void QComboBox::focusInEvent(QFocusEvent *e)
{
    Q_D(QComboBox);
    update();
    if (d->lineEdit)
        d->lineEdit->event(e);
}

/*!
    \reimp
*/
void QComboBox::focusOutEvent(QFocusEvent *e)
{
    Q_D(QComboBox);
    update();
    if (d->lineEdit)
        d->lineEdit->event(e);
}

/*! \reimp */
void QComboBox::changeEvent(QEvent *e)
{
    Q_D(QComboBox);
    switch (e->type()) {
    case QEvent::StyleChange:
        d->sizeHint = QSize(); // invalidate size hint
        if (d->lineEdit)
            d->updateLineEditGeometry();
        //### need to update scrollers etc. as well here
        break;
    case QEvent::EnabledChange:
        if (!isEnabled())
            hidePopup();
        break;
    case QEvent::PaletteChange:
        d->viewContainer()->setPalette(palette());
        break;
    case QEvent::FontChange:
        d->sizeHint = QSize(); // invalidate size hint
        d->viewContainer()->setFont(font());
        if (d->lineEdit)
            d->updateLineEditGeometry();
        break;
    default:
        break;
    }
    QWidget::changeEvent(e);
}

/*!
    \reimp
*/
void QComboBox::resizeEvent(QResizeEvent *)
{
    Q_D(QComboBox);
    d->updateLineEditGeometry();
}

/*!
    \reimp
*/
void QComboBox::paintEvent(QPaintEvent *)
{
    QStylePainter painter(this);
    painter.setPen(palette().color(QPalette::Text));

    // draw the combobox frame, focusrect and selected etc.
    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    painter.drawComplexControl(QStyle::CC_ComboBox, opt);

    // draw the icon and text
    painter.drawControl(QStyle::CE_ComboBoxLabel, opt);
}

/*!
    \reimp
*/
void QComboBox::showEvent(QShowEvent *e)
{
    Q_D(QComboBox);
    if (!d->shownOnce && d->sizeAdjustPolicy == QComboBox::AdjustToContentsOnFirstShow) {
        d->sizeHint = QSize();
        updateGeometry();
    }
    d->shownOnce = true;
    QWidget::showEvent(e);
}

/*!
    \reimp
*/
void QComboBox::hideEvent(QHideEvent *)
{
    hidePopup();
}

/*!
    \reimp
*/
bool QComboBox::event(QEvent *event)
{
    Q_D(QComboBox);
    switch(event->type()) {
    case QEvent::LayoutDirectionChange:
    case QEvent::ApplicationLayoutDirectionChange:
        d->updateLineEditGeometry();
        break;
    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
    case QEvent::HoverMove:
    if (const QHoverEvent *he = static_cast<const QHoverEvent *>(event))
        d->updateHoverControl(he->pos());
        break;
    case QEvent::ShortcutOverride:
        if (d->lineEdit)
            return d->lineEdit->event(event);
        break;
#ifdef QT_KEYPAD_NAVIGATION
    case QEvent::EnterEditFocus:
        if (!d->lineEdit)
            setEditFocus(false); // We never want edit focus if we are not editable
        break;
#endif
    default:
        break;
    }
    return QWidget::event(event);
}

/*!
    \reimp
*/
void QComboBox::mousePressEvent(QMouseEvent *e)
{
    Q_D(QComboBox);
    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    QStyle::SubControl sc = style()->hitTestComplexControl(QStyle::CC_ComboBox, &opt, e->pos(),
                                                           this);
    if ((sc == QStyle::SC_ComboBoxArrow || !isEditable())
        && !d->viewContainer()->isVisible()) {
        if (sc == QStyle::SC_ComboBoxArrow)
            d->updateArrow(QStyle::State_Sunken);
        d->viewContainer()->blockMouseReleaseTimer.start(QApplication::doubleClickInterval());
        d->viewContainer()->initialClickPosition = e->pos();
        showPopup();
    }
}

/*!
    \reimp
*/
void QComboBox::mouseReleaseEvent(QMouseEvent *e)
{
    Q_D(QComboBox);
    Q_UNUSED(e);
    d->updateArrow(QStyle::State_None);
}

/*!
    \reimp
*/
void QComboBox::keyPressEvent(QKeyEvent *e)
{
    Q_D(QComboBox);

#ifndef QT_NO_COMPLETER
    if (d->lineEdit && d->lineEdit->completer() && d->lineEdit->completer()->popup()->isVisible()) {
        // provide same autocompletion support as line edit
        d->lineEdit->event(e);
        return;
    }
#endif

    enum Move { NoMove=0 , MoveUp , MoveDown , MoveFirst , MoveLast};

    Move move = NoMove;
    int newIndex = currentIndex();
    switch (e->key()) {
    case Qt::Key_Up:
        if (e->modifiers() & Qt::ControlModifier)
            break; // pass to line edit for auto completion
    case Qt::Key_PageUp:
#ifdef QT_KEYPAD_NAVIGATION
        if (QApplication::keypadNavigationEnabled())
            e->ignore();
        else
#endif
        move = MoveUp;
        break;
    case Qt::Key_Down:
        if (e->modifiers() & Qt::AltModifier) {
            showPopup();
            return;
        } else if (e->modifiers() & Qt::ControlModifier)
            break; // pass to line edit for auto completion
        // fall through
    case Qt::Key_PageDown:
#ifdef QT_KEYPAD_NAVIGATION
        if (QApplication::keypadNavigationEnabled())
            e->ignore();
        else
#endif
        move = MoveDown;
        break;
    case Qt::Key_Home:
        if (!d->lineEdit)
            move = MoveFirst;
        break;
    case Qt::Key_End:
        if (!d->lineEdit)
            move = MoveLast;
        break;
    case Qt::Key_F4:
        if (!e->modifiers()) {
            showPopup();
            return;
        }
        break;
    case Qt::Key_Space:
        if (!d->lineEdit) {
            showPopup();
            return;
        }
    case Qt::Key_Enter:
    case Qt::Key_Return:
    case Qt::Key_Escape:
        if (!d->lineEdit)
            e->ignore();
        break;
#ifdef QT_KEYPAD_NAVIGATION
    case Qt::Key_Select:
        if (QApplication::keypadNavigationEnabled()
                && (!hasEditFocus() || !d->lineEdit)) {
            showPopup();
            return;
        }
        break;
    case Qt::Key_Left:
        if (QApplication::keypadNavigationEnabled()
                && (!hasEditFocus() || !d->lineEdit))
            move = MoveUp;
        break;
    case Qt::Key_Right:
        if (QApplication::keypadNavigationEnabled()
                && (!hasEditFocus() || !d->lineEdit))
            move = MoveDown;
        break;
    case Qt::Key_Back:
        if (QApplication::keypadNavigationEnabled()) {
            if (!hasEditFocus() || !d->lineEdit)
                e->ignore();
        } else {
            e->ignore(); // let the surounding dialog have it
        }
        break;
#endif
    default:
        if (!d->lineEdit && !e->text().isEmpty()) {
            // use keyboardSearch from the listView so we do not duplicate code
            view()->setCurrentIndex(d->currentIndex);
            int currentRow = view()->currentIndex().row();
            view()->keyboardSearch(e->text());
            if (currentRow != view()->currentIndex().row()) {
                setCurrentIndex(view()->currentIndex().row());
                d->emitActivated(d->currentIndex);
            }
        }
    }

    if (move != NoMove) {
        switch (move) {
        case MoveFirst:
            newIndex = -1;
        case MoveDown:
            newIndex++;
            while ((newIndex < count()) && !(model()->flags(model()->index(newIndex,d->modelColumn,rootModelIndex())) & Qt::ItemIsEnabled))
                newIndex++;
            break;
        case MoveLast:
            newIndex = count();
        case MoveUp:
            newIndex--;
            while ((newIndex >= 0) && !(model()->flags(model()->index(newIndex,d->modelColumn,rootModelIndex())) & Qt::ItemIsEnabled))
                newIndex--;
            break;
        default:
            break;
        }

        if (newIndex >= 0 && newIndex < count() && newIndex != currentIndex()) {
            setCurrentIndex(newIndex);
            d->emitActivated(d->currentIndex);
        }
    } else if (d->lineEdit) {
        d->lineEdit->event(e);
    }
}


/*!
    \reimp
*/
void QComboBox::keyReleaseEvent(QKeyEvent *e)
{
    Q_D(QComboBox);
    if (d->lineEdit)
        d->lineEdit->event(e);
}

/*!
    \reimp
*/
#ifndef QT_NO_WHEELEVENT
void QComboBox::wheelEvent(QWheelEvent *e)
{
    Q_D(QComboBox);
    if (!d->viewContainer()->isVisible()) {
        int newIndex = currentIndex();

        if (e->delta() > 0) {
            newIndex--;
            while ((newIndex >= 0) && !(model()->flags(model()->index(newIndex,d->modelColumn,rootModelIndex())) & Qt::ItemIsEnabled))
                newIndex--;
        } else {
            newIndex++;
            while ((newIndex < count()) && !(model()->flags(model()->index(newIndex,d->modelColumn,rootModelIndex())) & Qt::ItemIsEnabled))
                newIndex++;
        }

        if (newIndex >= 0 && newIndex < count() && newIndex != currentIndex()) {
            setCurrentIndex(newIndex);
            d->emitActivated(d->currentIndex);
        }
        e->accept();
    }
}
#endif

/*!
    \reimp
*/
void QComboBox::contextMenuEvent(QContextMenuEvent *e)
{
    Q_D(QComboBox);
    if (d->lineEdit) {
        Qt::ContextMenuPolicy p = d->lineEdit->contextMenuPolicy();
        d->lineEdit->setContextMenuPolicy(Qt::DefaultContextMenu);
        d->lineEdit->event(e);
        d->lineEdit->setContextMenuPolicy(p);
    }
}

/*!
    \reimp
*/
void QComboBox::inputMethodEvent(QInputMethodEvent *e)
{
    Q_D(QComboBox);
    if (d->lineEdit)
        d->lineEdit->event(e);
}

/*!
    \reimp
*/
QVariant QComboBox::inputMethodQuery(Qt::InputMethodQuery query) const
{
    Q_D(const QComboBox);
    if (d->lineEdit)
        return d->lineEdit->inputMethodQuery(query);
    return QWidget::inputMethodQuery(query);
}

/*!
    \fn bool QComboBox::editable() const

    Use isEditable() instead.
*/

/*!
    \fn void QComboBox::insertItem(const QPixmap &pixmap, int index)

    Use an insertItem() function that takes a QIcon instead, for
    example, insertItem(index, QIcon(pixmap)).
*/

/*!
    \fn void QComboBox::insertItem(const QPixmap &pixmap, const QString &text, int index)

    Use an insertItem() function that takes a QIcon instead, for
    example, insertItem(index, QIcon(pixmap), text).

    \sa insertItems()
*/

/*!
    \fn void QComboBox::changeItem(const QString &text, int index)

    Use setItemText() instead.
*/

/*!
    \fn void QComboBox::changeItem(const QPixmap &pixmap, int index)

    Use setItemIcon() instead, for example,
    setItemIcon(index, QIcon(pixmap)).
*/

/*!
    \fn void QComboBox::changeItem(const QPixmap &pixmap, const QString &text, int index)

    Use setItem() instead, for example, setItem(index, QIcon(pixmap),text).
*/

/*!
    \fn void QComboBox::addItem(const QString &text, const QVariant &userData)

    Adds an item to the combobox with the given \a text, and containing the
    specified \a userData. The item is appended to the list of existing items.
*/

/*!
    \fn void QComboBox::addItem(const QIcon &icon, const QString &text,
                                const QVariant &userData)

    Adds an item to the combobox with the given \a icon and \a text, and
    containing the specified \a userData. The item is appended to the list of
    existing items.
*/

/*!
    \fn void QComboBox::addItems(const QStringList &texts)

    Adds each of the strings in the given \a texts to the combobox. Each item
    is appended to the list of existing items in turn.
*/

/*!
    \fn void QComboBox::editTextChanged(const QString &text)

    This signal is emitted when the text in the combobox's line edit
    widget is changed. The new text is specified by \a text.
*/

/*!
    \fn QComboBox::InsertPolicy QComboBox::insertionPolicy() const
    \compat

    Use QComboBox::insertPolicy instead.
*/

/*!
    \fn void QComboBox::setInsertionPolicy(InsertPolicy policy)
    \compat

    Use QComboBox::insertPolicy instead.
*/

/*!
    \fn void QComboBox::setCurrentText(const QString &text)
    \compat

    Use setItemText() instead.

    \sa currentIndex()
*/

/*!
    \fn QString QComboBox::text(int index) const
    \compat

    Use itemText() instead.
*/

/*!
    \fn QPixmap QComboBox::pixmap(int index) const
    \compat

    Use itemIcon() instead.
*/

/*!
    \fn void QComboBox::insertStringList(const QStringList &list, int index)
    \compat

    Use insertItems() instead.
*/

/*!
    \fn void QComboBox::insertItem(const QString &text, int index)
    \compat
*/

/*!
    \fn void QComboBox::clearEdit()
    \compat

    Use clearEditText() instead.
*/


/*!
    \property QComboBox::frame
    \brief whether the combo box draws itself with a frame


    If enabled (the default) the combo box draws itself inside a
    frame, otherwise the combo box draws itself without any frame.
*/
bool QComboBox::hasFrame() const
{
    Q_D(const QComboBox);
    return d->frame;
}


void QComboBox::setFrame(bool enable)
{
    Q_D(QComboBox);
    d->frame = enable;
    update();
    updateGeometry();
}

/*!
    \property QComboBox::modelColumn
    \brief the column in the model that is visible
*/
int QComboBox::modelColumn() const
{
    Q_D(const QComboBox);
    return d->modelColumn;
}

void QComboBox::setModelColumn(int visibleColumn)
{
    Q_D(QComboBox);
    d->modelColumn = visibleColumn;
    QListView *lv = qobject_cast<QListView *>(d->viewContainer()->itemView());
    if (lv)
        lv->setModelColumn(visibleColumn);

#ifndef QT_NO_COMPLETER
    if (d->lineEdit && d->lineEdit->completer()
        && d->lineEdit->completer() == d->completer)
        d->lineEdit->completer()->setCompletionColumn(visibleColumn);
#endif
}

/*!
    \fn int QComboBox::currentItem() const

    Use currentIndex() instead.
*/

/*!
    \fn void QComboBox::setCurrentItem(int)

    Use setCurrentIndex(int) instead.
*/

/*!
    \fn void QComboBox::popup()

    Use showPopup() instead.
*/

/*!
    \fn void QComboBox::textChanged(const QString &text)

    Use the editTextChanged(const QString &text) signal instead.
*/

/*!
    \typedef QComboBox::Policy
    \compat

    Use QComboBox::InsertPolicy instead.
*/


#include "moc_qcombobox.cpp"
#endif // QT_NO_COMBOBOX

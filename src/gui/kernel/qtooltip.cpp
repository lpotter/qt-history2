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

#include <qapplication.h>
#include <qdesktopwidget.h>
#include <qevent.h>
#include <qhash.h>
#include <qlabel.h>
#include <qpointer.h>
#include <qstyle.h>
#include <qstyleoption.h>
#include <qstylepainter.h>
#include <qtimer.h>
#include <qtooltip.h>
#include <private/qeffects_p.h>
#include <qtextdocument.h>
#include <qdebug.h>

#ifndef QT_NO_TOOLTIP

/*!
    \class QToolTip

    \brief The QToolTip class provides tool tips (balloon help) for
    any widget.

    \ingroup helpsystem
    \mainclass

    The tip is a short piece of text reminding the user of the
    widget's function. It is drawn immediately below the given
    position in a distinctive black-on-yellow color combination. The
    tip can be any \l{QTextEdit}{rich text} formatted string.

    Rich text formatted tips implictely do word breaking, unless
    specified differently with \c{<p style='white-space:pre'>}.

    The simplest and most common way to set a widget's tooltip is by
    calling its QWidget::setToolTip() function.

    It is also possible to show different tool tips for different
    regions of a widget, by using a QHelpEvent of type
    QEvent::ToolTip. Intercept the help event in your widget's
    QWidget::event() function and call QToolTip::showText() with the
    text you want to display. The \l{widgets/tooltips}{Tooltips}
    example illustrates this technique.

    The default tooltip color and font can be customized with
    setPalette() and setFont().

    \sa QWidget::toolTip, QAction::toolTip, {Tooltips Example}
*/

class QTipLabel : public QLabel
{
    Q_OBJECT
public:
    QTipLabel(const QString& text, QWidget* parent);
    ~QTipLabel();
    static QTipLabel *instance;

    bool eventFilter(QObject *, QEvent *);

    QBasicTimer hideTimer, deleteTimer;

    void hideTip();
protected:
    void enterEvent(QEvent*){hideTip();}
    void timerEvent(QTimerEvent *e);
    void paintEvent(QPaintEvent *e);

};

QTipLabel *QTipLabel::instance = 0;

QTipLabel::QTipLabel(const QString& text, QWidget* parent)
    : QLabel(parent, Qt::ToolTip)
{
    delete instance;
    instance = this;
    setMargin(1 + style()->pixelMetric(QStyle::PM_ToolTipLabelFrameWidth, 0, this));
    setFrameStyle(QFrame::NoFrame);
    setAlignment(Qt::AlignLeft);
    setIndent(1);
    ensurePolished();
    setWordWrap(Qt::mightBeRichText(text));
    setText(text);

    QFontMetrics fm(font());
    QSize extra(1, 0);
    // Make it look good with the default ToolTip font on Mac, which has a small descent.
    if (fm.descent() == 2 && fm.ascent() >= 11)
        ++extra.rheight();

    resize(sizeHint() + extra);
    qApp->installEventFilter(this);
    hideTimer.start(10000, this);
    setWindowOpacity(style()->styleHint(QStyle::SH_ToolTipLabel_Opacity, 0, this) / 255.0);
    setPalette(QToolTip::palette());
}

void QTipLabel::paintEvent(QPaintEvent *ev)
{
    QStylePainter p(this);
    QStyleOptionFrame opt;
    opt.init(this);
    p.drawPrimitive(QStyle::PE_PanelTipLabel, opt);
    p.end();

    QLabel::paintEvent(ev);
}

QTipLabel::~QTipLabel()
{
    instance = 0;
}

void QTipLabel::hideTip()
{
    hide();
    // timer based deletion to prevent animation
    deleteTimer.start(250, this);
}

void QTipLabel::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == hideTimer.timerId())
        hideTip();
    else if (e->timerId() == deleteTimer.timerId())
        delete this;
}

bool QTipLabel::eventFilter(QObject *, QEvent *e)
{
    switch (e->type()) {
    case QEvent::KeyPress:
    case QEvent::KeyRelease: {
        int key = static_cast<QKeyEvent *>(e)->key();
        Qt::KeyboardModifiers mody = static_cast<QKeyEvent *>(e)->modifiers();

        if ((mody & Qt::KeyboardModifierMask)
            || (key == Qt::Key_Shift || key == Qt::Key_Control
                || key == Qt::Key_Alt || key == Qt::Key_Meta))
            break;
    }
    case QEvent::Leave:
    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::FocusIn:
    case QEvent::FocusOut:
    case QEvent::Wheel:
        hideTip();
    default:
        ;
    }
    return false;
}

/*!
    Shows \a text as a tool tip, at global position \a pos. The
    optional widget argument, \a w, is used to determine the
    appropriate screen on multi-head systems. If \a text is empty the
    tool tip is hidden. If the text is the same as the currently shown
    tooltip, the tip will \e not move. You can force moving by first
    hiding the tip with an empty text, and then showing the new tip at
    the new position.
*/
void QToolTip::showText(const QPoint &pos, const QString &text, QWidget *w)
{
    if (QTipLabel::instance && QTipLabel::instance->text() == text)
        return; /* this is NOT a bug, if the text doesn't change, you
                 don't want the tool tips to move. If you divide your
                 widget in different parts that show the same tool
                 tip, you must handle that yourself. Simple hide the
                 tip (by showing an empty string) and show the new
                 one. */

    if (text.isEmpty()) {
        if (QTipLabel::instance)
            QTipLabel::instance->hideTip();
        return;
    }

#ifndef QT_NO_EFFECTS
    bool preventAnimation = (QTipLabel::instance != 0);
#endif
    int scr;
    if (QApplication::desktop()->isVirtualDesktop())
        scr = QApplication::desktop()->screenNumber(pos);
    else
        scr = QApplication::desktop()->screenNumber(w);

#ifdef Q_WS_MAC
    QRect screen = QApplication::desktop()->availableGeometry(scr);
#else
    QRect screen = QApplication::desktop()->screenGeometry(scr);
#endif

    QLabel *label = new QTipLabel(text, QApplication::desktop()->screen(scr));

    QPoint p = pos;
    p += QPoint(2,
#ifdef Q_WS_WIN
                24
#else
                16
#endif
        );
    if (p.x() + label->width() > screen.x() + screen.width())
        p.rx() -= 4 + label->width();
    if (p.y() + label->height() > screen.y() + screen.height())
        p.ry() -= 24 + label->height();
    if (p.y() < screen.y())
        p.setY(screen.y());
    if (p.x() + label->width() > screen.x() + screen.width())
        p.setX(screen.x() + screen.width() - label->width());
    if (p.x() < screen.x())
        p.setX(screen.x());
    if (p.y() + label->height() > screen.y() + screen.height())
        p.setY(screen.y() + screen.height() - label->height());
    label->move(p);

#ifndef QT_NO_EFFECTS
    if ( QApplication::isEffectEnabled(Qt::UI_AnimateTooltip) == false || preventAnimation)
        label->show();
    else if (QApplication::isEffectEnabled(Qt::UI_FadeTooltip)) {
        qFadeEffect(label);
    }
    else
        qScrollEffect(label);
#else
    label->show();
#endif

}

/*!
    \fn void QToolTip::hideText()

    Hides the tool tip. This is the same as calling showText() with an
    empty string.

    \sa showText()
*/


Q_GLOBAL_STATIC_WITH_ARGS(QPalette, tooltip_palette, (Qt::black, QColor(255,255,220),
                                                      QColor(96,96,96), Qt::black, Qt::black,
                                                      Qt::black, QColor(255,255,220)))

/*!
    Returns the palette used to render tooltips.
*/
QPalette QToolTip::palette()
{
    return *tooltip_palette();
}

/*!
    Returns the font used to render tooltips.
*/
QFont QToolTip::font()
{
    return QApplication::font("QTipLabel");
}

/*!
    Sets the \a palette used to render tooltips.
*/
void QToolTip::setPalette(const QPalette &palette)
{
    *tooltip_palette() = palette;
}

/*!
    Sets the \a font used to render tooltips.
*/
void QToolTip::setFont(const QFont &font)
{
    QApplication::setFont(font, "QTipLabel");
}


/*!
    \fn void QToolTip::add(QWidget *widget, const QString &text)

    Use QWidget::setToolTip() instead.

    \oldcode
    tip->add(widget, text);
    \newcode
    widget->setToolTip(text);
    \endcode
*/

/*!
    \fn void QToolTip::add(QWidget *widget, const QRect &rect, const QString &text)

    Intercept the QEvent::ToolTip events in your widget's
    QWidget::event() function and call QToolTip::showText() with the
    text you want to display. The \l{widgets/tooltips}{Tooltips}
    example illustrates this technique.
*/

/*!
    \fn void QToolTip::remove(QWidget *widget)

    Use QWidget::setToolTip() instead.

    \oldcode
    tip->remove(widget);
    \newcode
    widget->setToolTip("");
    \endcode
*/

#include "qtooltip.moc"
#endif // QT_NO_TOOLTIP

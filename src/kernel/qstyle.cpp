/****************************************************************************
** $Id$
**
** Implementation of QStyle class
**
** Created : 981231
**
** Copyright (C) 1998-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/
#include "qstyle.h"
#ifndef QT_NO_STYLE
#include "qapplication.h"
#include "qnamespace.h"
#include "qpainter.h"
#include "qdrawutil.h" // for now
#include "qwidget.h"
#include "qimage.h"

#include <limits.h>


class QStylePrivate
{
public:
    QStylePrivate()
    {
    }
};

/*!
  \class QStyleOption qstyle.h
  \brief The QStyleOption class specifies optional parameters for QStyle functions.
  \preliminary
  \ingroup appearance

  Some QStyle functions take an optional argument specifying extra
  information that is required for a paritical primitive or control.
  So that the QStyle class can be extended, QStyleOption is use to
  give a variable-argument for these options.

  The QStyleOption class has constructors for each type of optional
  argument, and this set of constructors may be extended in future
  Qt releases. There are also corresponding access functions that
  return the optional arguments - these too may be extended.

  For each constructor, you should refer to the documentation of
  the QStyle functions to see the meaning of the arguments.

  When calling QStyle functions from your own widgets, you must only
  pass either the default QStyleOption, or the argument that QStyle
  is documented to accept. For example, if the function expects
  QStyleOption(QMenuItem*,int), you must not pass QStyleOption(QMenuItem*),
  and the optional integer would then be uninitialized.

  When making your own QStyle subclass, you must similarly only expect the
  default or documented argument - the other arguments will have
  uninitialized values.

  If you make your own QStyle subclasses and your own widgets, you can
  make a subclass of QStyleOption in order to pass additional arguments
  to your QStyle subclass (of course, you'll need to cast the
  "const QStyleOption&" argument to your subclass, so be sure your style
  has been called from your widget!
*/

/*!
  \fn QStyleOption::QStyleOption(StyleOptionDefault=Default)

  The default option. This can always be passed as the optional argument
  to QStyle functions.
*/

/*!
  \fn QStyleOption::QStyleOption(int, int)

  Pass two integers. For example, linewidth and midlinewidth.
*/
/*!
  \fn QStyleOption::QStyleOption(int, int, int, int)

  Pass four integers.
*/
/*!
  \fn QStyleOption::QStyleOption(QMenuItem*)

  Pass a menu item.
*/
/*!
  \fn QStyleOption::QStyleOption(QMenuItem*, int)

  Pass a menu item and an integer.
*/
/*!
  \fn QStyleOption::QStyleOption(QMenuItem*, int, int)

  Pass a menu item and two integers.
*/
/*!
  \fn QStyleOption::QStyleOption(const QColor&)

  Pass a color.
*/
/*!
  \fn QStyleOption::QStyleOption(QTab*)

  Pass a QTab.
*/
/*!
  \fn QStyleOption::QStyleOption(QListViewItem*)

  Pass a QListViewItem.
*/
/*!
  \fn QStyleOption::QStyleOption(Qt::ArrowType)

  Pass an Qt::ArrowType.
*/

/*!
  \fn bool QStyleOption::isDefault() const

  Returns whether the option was constructed with the
  default constructor.
*/

/*!
  \fn int QStyleOption::lineWidth() const

  Returns the line width if the appropriate constructor was
  called, else undefined.
*/
/*!
  \fn int QStyleOption::midLineWidth() const

  Returns the mid-line width if the appropriate constructor was
  called, else undefined.
*/
/*!
  \fn int QStyleOption::frameShape() const

  Returns a QFrame::Shape value if the appropriate constructor was
  called, else undefined.
*/
/*!
  \fn int QStyleOption::frameShadow() const

  Returns a QFrame::Shadow value if the appropriate constructor was
  called, else undefined.
*/

/*!
  \fn QMenuItem* QStyleOption::menuItem() const

  Returns a menu item if the appropriate constructor was
  called, else undefined.
*/
/*!
  \fn int QStyleOption::maxIconWidth() const

  Returns the maximum width of the menu item check area
  if the appropriate constructor was
  called, else undefined.
*/
/*!
  \fn int QStyleOption::tabWidth() const

  Returns the tab indent width if the appropriate constructor was
  called, else undefined.
*/

/*!
  \fn const QColor& QStyleOption::color() const

  Returns a color if the appropriate constructor was
  called, else undefined.
*/

/*!
  \fn QTab* QStyleOption::tab() const

  Returns a QTabBar tab if the appropriate constructor was
  called, else undefined.
*/

/*!
  \fn QListViewItem* QStyleOption::listViewItem() const

  Returns a QListView item if the appropriate constructor was
  called, else undefined.
*/

/*!
  \fn Qt::ArrowType QStyleOption::arrowType() const

  Returns an arrow type if the appropriate constructor was
  called, else undefined.
*/

/*!
  \class QStyle qstyle.h
  \brief The QStyle class specifies the look and feel of a GUI.
  \preliminary
  \ingroup appearance

    A large number of GUI elements are common to many widgets. The
    QStyle class allows the look of these elements to be modified
    across all widgets that use the QStyle functions.  It also
    provides two feel options: Motif and Windows.

  Although it is not possible to fully enumerate the look of graphic elements
  and the feel of widgets in a GUI, QStyle provides a considerable
  amount of control and customisability.

  In Qt 1.x the look and feel option for widgets was specified by a
  single value - the GUIStyle.  Starting with Qt 2.0, this notion has
  been expanded to allow the look to be specified by virtual drawing
  functions.

  Derived classes may reimplement some or all of the drawing functions
  to modify the look of all widgets that use those functions.

  Languages written from right to left (such as Arabic and Hebrew)
  usually also mirror the whole layout of widgets. If you design a
  style, you should take special care when drawing asymmetric elements
  to make sure that they also look correct in a mirrored layout. You can
  start your application with -reverse to check the mirrored layout.
  Also notice, that for a reversed layout, the light usually comes
  from top right instead of top left.

  The actual reverse layout is performed automatically when possible.
  However, for the sake of flexibility, the translation cannot be
  performed everywhere.  The documentation for each function in the
  QStyle API states whether the function expects/returns logical or
  screen coordinates.  Using logical coordinates (in ComplexControls,
  for example) provides great flexibility in controlling the look of a
  widget.  Use visualRect() when necessary to translate logical
  coordinates into screen coordinates for drawing.

  In Qt versions prior to 3.0 if you wanted a low level route into
  changing the appearance of a widget you would reimplement polish().
  With the new 3.0 style engine the recommended approach is to
  reimplement the draw functions, for example drawItem(),
  drawPrimitive(), drawControl(), drawControlMask(),
  drawComplexControl() and drawComplexControlMask(). Each of these
  functions is called with a range of parameters that provide
  information that you can use to determine how to draw them, e.g.
  style flags, rectangle, color group, etc.

    For information on changing elements of an existing style or
    creating your own style see the \link customstyles.html Style
    overview\endlink.
*/

/*!
  \enum Qt::GUIStyle

  \obsolete

  \value WindowsStyle
  \value MotifStyle
  \value MacStyle
  \value Win3Style
  \value PMStyle
*/

/*!
    \enum Qt::UIEffect

    \value UI_General
    \value UI_AnimateMenu
    \value UI_FadeMenu
    \value UI_AnimateCombo
    \value UI_AnimateTooltip
    \value UI_FadeTooltip

*/

/*!
  Constructs a QStyle.
*/
QStyle::QStyle()
{
    d = new QStylePrivate;
}

/*!
  Destroys the style and frees all allocated resources.
*/
QStyle::~QStyle()
{
    delete d;
    d = 0;
}

/*
  \fn GUIStyle QStyle::guiStyle() const
  \obsolete

  Returns an indicator to the additional "feel" component of a
  style. Current supported values are Qt::WindowsStyle and Qt::MotifStyle.
*/



/*!
  Initializes the appearance of a widget.

  This function is called for every widget at some point after it has
  been fully created but just \e before it is shown the very first
  time.

  Reasonable actions in this function might be to call
  QWidget::setBackgroundMode for the widget. An example of highly
  unreasonable use would be setting the geometry! Reimplementing this
  function gives you a backdoor through which you can change the
  appearance of a widget. With Qt 3.0's style engine you will rarely
  need to write your own polish(); instead reimplement drawItem(),
  drawPrimitive(), etc.

  The QWidget::inherits() function may provide enough information to
  allow class-specific customizations.  But be careful not to hard-code
  things too much because new QStyle subclasses will be expected to work
  reasonably with all current and \e future widgets.

  \sa unPolish()
*/
void QStyle::polish( QWidget*)
{
}

/*!
  Undoes the initialization of a widget's appearance.

  This function is the counterpart to polish. It is called for every
  polished widget when the style is dynamically changed. The former
  style has to unpolish its settings before the new style can polish
  them again.

  \sa polish()
*/
void QStyle::unPolish( QWidget*)
{
}


/*!
  \overload
  Late initialization of the QApplication object.

  \sa unPolish()
*/
void QStyle::polish( QApplication*)
{
}

/*!
  \overload
  Undoes the application polish.

  \sa polish()
*/
void QStyle::unPolish( QApplication*)
{
}

/*!
  \overload
  The style may have certain requirements for color palettes.  In this
  function it has the chance to change the palette according to these
  requirements.

  \sa QPalette, QApplication::setPalette()
*/
void QStyle::polish( QPalette&)
{
}

/*!
  Polishes the popup menu according to the GUI style. This usually means
  setting the mouse tracking (\l{QPopupMenu::setMouseTracking()}) and whether
  the menu is checkable by default (\l{QPopupMenu::setCheckable()}).
*/
void QStyle::polishPopupMenu( QPopupMenu *)
{
}

/*!
  Returns the appropriate area (see below) within rectangle \a r in
  which to draw the \a text or \a pixmap using painter \a p. If \a len
  is -1 (the default) all the \a text is drawn; otherwise only the
  first \a len characters of \a text are drawn. The text is aligned
  in accordance with the alignment \a flags (see \l{Qt::AlignmentFlags}).
    The \a enabled bool indicates whether or not the item is enabled.

    If \a r is larger than the area needed to render the \a
    text the rectangle that is returned will be offset within \a r in
    accordance with the alignment \a flags. For example if \a flags is
    \c AlignCenter the returned rectangle will be
    centered within \a r. If \a r is smaller than the area needed the
    rectangle that is returned will be \e larger than \a r (the
    smallest rectangle large enough to render the \a text or \a
    pixmap).

    By default, if both the text and the pixmap are not null, the
    the text is ignored.
*/
QRect QStyle::itemRect( QPainter *p, const QRect &r,
			int flags, bool enabled, const QPixmap *pixmap,
			const QString& text, int len ) const
{
    return qItemRect( p, (Qt::GUIStyle) styleHint(SH_GUIStyle),
		      r.x(), r.y(), r.width(), r.height(),
		      flags, enabled, pixmap, text, len );
}


/*!
  Draws the \a text or \a pixmap in rectangle \a r using painter \a p
  and color group \a g. The pen color is specified with \a penColor.
  The \a enabled bool indicates whether or not the item is enabled;
  when reimplementing this bool should influence how the item is
  drawn. If \a len is -1 (the default) all the \a text is drawn;
  otherwise only the first \a len characters of \a text are drawn. The
  text is aligned and wrapped according to the alignment \a flags (see
  \l{Qt::AlignmentFlags}).

    By default, if both the text and the pixmap are not null, the
    pixmap is drawn and the text is ignored.
*/
void QStyle::drawItem( QPainter *p, const QRect &r,
		       int flags, const QColorGroup &g, bool enabled,
		       const QPixmap *pixmap, const QString& text, int len,
		       const QColor* penColor ) const
{
    qDrawItem( p, (Qt::GUIStyle) styleHint(SH_GUIStyle),
	       r.x(), r.y(), r.width(), r.height(),
	       flags, g, enabled, pixmap, text, len, penColor );
}

/*!
  \enum QStyle::PrimitiveElement

  This enum represents the PrimitiveElements of a style.  A
  PrimitiveElement is a common GUI element, such as a checkbox
  indicator or pushbutton bevel.

  \value PE_ButtonCommand  button used to initiate an action, for
  example, a QPushButton.
    \value PE_ButtonDefault  this button is the default button, e.g.
    in a dialog.
  \value PE_ButtonBevel  generic button bevel.
  \value PE_ButtonTool  tool button, for example, a QToolButton.
  \value PE_ButtonDropDown  drop down button, for example, a tool
  button that displays a popup menu, for example, QPopupMenu.


  \value PE_FocusRect  generic focus indicator.


  \value PE_ArrowUp  up arrow.
  \value PE_ArrowDown  down arrow.
  \value PE_ArrowRight  right arrow.
  \value PE_ArrowLeft  left arrow.


  \value PE_SpinWidgetUp  up symbol for a spin widget, for example a
  QSpinBox.
  \value PE_SpinWidgetDown down symbol for a spin widget.
  \value PE_SpinWidgetPlus  increase symbol for a spin widget.
  \value PE_SpinWidgetMinus  decrease symbol for a spin widget.


  \value PE_Indicator  on/off indicator, for example, a QCheckBox.
  \value PE_IndicatorMask  bitmap mask for an indicator.
  \value PE_ExclusiveIndicator  exclusive on/off indicator, for example, a QRadioButton.
  \value PE_ExclusiveIndicatorMask  bitmap mask for an exclusive indicator.


  \value PE_DockWindowHandle  tear off handle for dock windows and toolbars, for example \l{QDockWindow}s and \l{QToolBar}s.
  \value PE_DockWindowSeparator  item separator for dock window and toolbar contents.
  \value PE_DockWindowResizeHandle  resize handle for dock windows.

  \value PE_Splitter  splitter handle; see also QSplitter.


  \value PE_Panel  generic panel frame; see also QFrame.
  \value PE_PanelPopup  panel frame for popup windows/menus; see also
  QPopupMenu.
  \value PE_PanelMenuBar  panel frame for menu bars.
  \value PE_PanelDockWindow  panel frame for dock windows and toolbars.

  \value PE_TabBarBase  area below tabs in a tab widget, for example,
  QTab.


  \value PE_HeaderSection  section of a list or table header; see also
  QHeader.
  \value PE_HeaderArrow arrow used to indicate sorting on a list or table
  header
  \value PE_StatusBarSection  section of a status bar; see also
  QStatusBar.


  \value PE_GroupBoxFrame  frame around a group box; see also
  QGroupBox.


  \value PE_Separator  generic separator.


  \value PE_SizeGrip  window resize handle; see also QSizeGrip.


  \value PE_CheckMark  generic check mark; see also QCheckBox.


  \value PE_ScrollBarAddLine  scrollbar line increase indicator (i.e. scroll down); see also QScrollBar.
  \value PE_ScrollBarSubLine  scrollbar line decrease indicator (i.e. scroll up).
  \value PE_ScrollBarAddPage  scolllbar page increase indicator (i.e. page down).
  \value PE_ScrollBarSubPage  scrollbar page decrease indicator (i.e. page up).
  \value PE_ScrollBarSlider  scrollbar slider
  \value PE_ScrollBarFirst  scrollbar first line indicator (i.e. home).
  \value PE_ScrollBarLast  scrollbar last line indicator (i.e. end).


  \value PE_ProgressBarChunk  section of a progress bar indicator; see
  also QProgressBar.

  \sa drawPrimitive()
*/
/*! \enum QStyle::SFlags
    \internal
*/
/*! \enum QStyle::SCFlags
    \internal
*/

/*!
  \enum QStyle::StyleFlags

  This enum represents flags for drawing PrimitiveElements.  Not all
  primitives use all of these flags.  Note that these flags may mean
  different things to different primitives. For an explanation of the
  relationship between primitives and their flags, as well as the
  different meanings of the flags, see the \link customstyles.html Style overview\endlink.

  \value Style_Default
  \value Style_Enabled
  \value Style_Raised
  \value Style_Sunken
  \value Style_Off
  \value Style_NoChange
  \value Style_On
  \value Style_Down
  \value Style_Horizontal
  \value Style_HasFocus
  \value Style_Top
  \value Style_Bottom
  \value Style_FocusAtBorder
  \value Style_AutoRaise
  \value Style_MouseOver
  \value Style_Up
  \value Style_Selected
  \value Style_HasFocus
  \value Style_Active
  \value Style_ButtonDefault

  \sa drawPrimitive()
*/

/*!
  \fn void QStyle::drawPrimitive( PrimitiveElement pe, QPainter *p, const QRect &r, const QColorGroup &cg, SFlags flags = Style_Default, const QStyleOption& opt = QStyleOption::Default ) const;

  Draws the style PrimitiveElement \a pe using the painter \a p in the
  area \a r.  Colors are used from the color group \a cg.

  The rect \a r should be in screen coordinates.

  The \a flags argument is used to control how the PrimitiveElement is drawn.
  Multiple flags can be OR'ed together.

  For example, a pressed button would be drawn with the flags \c
  Style_Enabled and \c Style_Down.

  The \a opt argument can be used to control how various
  PrimitiveElements are drawn. Note that \a opt may be the default value
  even for PrimitiveElements that make use of extra options. When \a opt
  is non-default, it is used as follows:

<center>
<table cellpadding=4 cellspacing=2 border=0>
<tr bgcolor=#A2C511>
<th>PrimitiveElement</th>
<th>Options</th>
<th>Notes</th>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top><b>PE_FocusRect</b></td>
<td valign=top>QStyleOption(const QColor& bg)</td>
<td valign=top>\e bg is the background color on which the focus rect is being drawn.</td>
</tr>
<tr bgcolor=#f0f0f0>
<td rowspan=2 valign=top><b>PE_Panel</b></td>
<td rowspan=2 valign=top>QStyleOption(int linewidth, int midlinewidth)
	<p>opt.\link QStyleOption::lineWidth() lineWidth()\endlink <p>opt.\link QStyleOption::midLineWidth() midLineWidth()\endlink</td>
<td valign=top>\e linewidth is the line width for drawing the panel.</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>\e midlinewidth is the mid-line width for drawing the panel.</td>
</tr>
<tr bgcolor=#d0d0d0>
<td rowspan=2 valign=top><b>PE_PanelPopup</b></td>
<td rowspan=2 valign=top>QStyleOption(int linewidth, int midlinewidth)
	<p>opt.\link QStyleOption::lineWidth() lineWidth()\endlink <p>opt.\link QStyleOption::midLineWidth() midLineWidth()\endlink</td>
<td valign=top>\e linewidth is the line width for drawing the panel.</td>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top>\e midlinewidth is the mid-line width for drawing the panel.</td>
</tr>
<tr bgcolor=#f0f0f0>
<td rowspan=2 valign=top><b>PE_PanelMenuBar</b></td>
<td rowspan=2 valign=top>QStyleOption(int linewidth, int midlinewidth)
	<p>opt.\link QStyleOption::lineWidth() lineWidth()\endlink <p>opt.\link QStyleOption::midLineWidth() midLineWidth()\endlink</td>
<td valign=top>\e linewidth is the line width for drawing the panel.</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>\e midlinewidth is the mid-line width for drawing the panel.</td>
</tr>
<tr bgcolor=#d0d0d0>
<td rowspan=2 valign=top><b>PE_PanelDockWindow</b></td>
<td rowspan=2 valign=top>QStyleOption(int linewidth, int midlinewidth)
	<p>opt.\link QStyleOption::lineWidth() lineWidth()\endlink <p>opt.\link QStyleOption::midLineWidth() midLineWidth()\endlink</td>
<td valign=top>\e linewidth is the line width for drawing the panel.</td>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top>\e midlinewidth is the mid-line width for drawing the panel.</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top rowspan=4><b>PE_GroupBoxFrame</b></td>
<td rowspan=4 valign=top>QStyleOption(int linewidth, int midlinewidth, int shape, int shadow)
	<p>opt.\link QStyleOption::lineWidth() lineWidth()\endlink <p>opt.\link QStyleOption::midLineWidth() midLineWidth()\endlink
	<p>opt.\link QStyleOption::frameShape() frameShape()\endlink <p>opt.\link QStyleOption::frameShadow() frameShadow()\endlink</td>
<td valign=top>\e linewidth is the line width for the group box.</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>\e midlinewidth is the mid-line width for the group box.</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>\e shape is the frame shape for the group box.  See QFrame.</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>\e shadow is the frame shadow for the group box.</td>
</tr>
</table>
</center>

  For all other \link QStyle::PrimitiveElement
  PrimitiveElements\endlink, \a opt is unused.

  \sa StyleFlags
*/

/*!
  \enum QStyle::ControlElement

  This enum represents a ControlElement.  A ControlElement is part of
  a widget that performs some action or display information to the user.


  \value CE_PushButton  the bevel and default indicator of a QPushButton.
  \value CE_PushButtonLabel  the label (iconset with text or pixmap) of a QPushButton.


  \value CE_CheckBox  the indicator of a QCheckBox.
  \value CE_CheckBoxLabel  the label (text or pixmap) of a QCheckBox.


  \value CE_RadioButton  the indicator of a QRadioButton.
  \value CE_RadioButtonLabel  the label (text or pixmap) of a QRadioButton.


  \value CE_TabBarTab  the tab within a QTabBar (a QTab).
  \value CE_TabBarLabel  the label within a QTab.


  \value CE_ProgressBarGroove  the groove where the progress indicator is drawn in
         a QProgressBar.
  \value CE_ProgressBarContents  the progress indicator of a QProgressBar.
  \value CE_ProgressBarLabel  the text label of a QProgressBar.


  \value CE_PopupMenuItem  a menu item in a QPopupMenu.


  \value CE_MenuBarItem  a menu item in a QMenuBar.

  \value CE_ToolButtonLabel a tool button's label.

  \sa drawControl()
*/

/*!
  \fn void QStyle::drawControl ( ControlElement element, QPainter *p, const QWidget *widget, const QRect &r, const QColorGroup &cg, SFlags how = Style_Default, const QStyleOption& opt = QStyleOption::Default ) const;

  Draws the ControlElement \a element using the painter \a p in the
  area \a r.  Colors are used from the color group \a cg.

  The rect \a r should be in screen coordinates.

  The \a how argument is used to control how the ControlElement is
  drawn.  Multiple flags can be OR'ed together.  See the table below for
  an explanation of which flags are used with the various ControlElements.

  The \a widget argument is a pointer to a QWidget or one of its
  subclasses.  The widget can be cast to the appropriate type based on
  the value of \a element. The \a opt argument can be used to pass
  extra information required when drawing the ControlElement.  Note
  that \a opt may be the default value even for ControlElements that can make use
  of the extra options.  See the table below for the appropriate \a
  widget and \a opt usage:

<center>
<table cellpadding=4 cellspacing=2 border=0>
<tr bgcolor=#A2C511>
<th>ControlElement<br>&amp; Widget Cast</th>
<th>Style Flags</th>
<th>Notes</th>
<th>Options</th>
<th>Notes</th>
</tr>
<tr bgcolor=#d0d0d0>
<td rowspan=6 valign=top><b>CE_PushButton</b><br>const QPushButton *</td>
<td valign=top>Style_Enabled</td>
<td valign=top>set if the button is enabled.</td>
<td rowspan=6 valign=top>unused</td>
<td rowspan=6 valign=top>&nbsp;</td>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top>Style_HasFocus</td>
<td valign=top>set if the button has input focus.</td>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top>Style_Raised
<td valign=top>set if the button is not down, not on and not flat.</td>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top>Style_On</td>
<td valign=top>set if the button is a toggle button and toggled on.</td>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top>Style_Down</td>
<td valign=top>set if the button is down (ie. mouse button or space pressed on
    the button).</td>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top>Style_ButtonDefault</td>
<td valign=top>set if the button a default button.</td>
</tr>
<tr bgcolor=#f0f0f0>
<td rowspan=6 valign=top><b>CE_PushButtonLabel</b><br>const QPushButton *</td>
<td valign=top>Style_Enabled</td>
<td valign=top>set if the button is enabled.</td>
<td rowspan=6 valign=top>unused</td>
<td rowspan=6 valign=top>&nbsp;</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>Style_HasFocus</td>
<td valign=top>set if the button has input focus.</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>Style_Raised
<td valign=top>set if the button is not down, not on and not flat.</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>Style_On</td>
<td valign=top>set if the button is a toggle button and toggled on.</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>Style_Down</td>
<td valign=top>set if the button is down (ie. mouse button or space pressed on
    the button).</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>Style_ButtonDefault</td>
<td valign=top>set if the button a default button.</td>
</tr>
<tr bgcolor=#d0d0d0>
<td rowspan=6 valign=top><b>CE_CheckBox</b><br>const QCheckBox *</td>
<td valign=top>Style_Enabled</td>
<td valign=top>set if the checkbox is enabled.</td>
<td rowspan=6 valign=top>unused</td>
<td rowspan=6 valign=top>&nbsp;</td>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top>Style_HasFocus</td>
<td valign=top>set if the checkbox has input focus.</td>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top>Style_On</td>
<td valign=top>set if the checkbox is checked.</td>
</tr>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top>Style_Off</td>
<td valign=top>set if the checkbox is not checked.</td>
</tr>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top>Style_NoChange</td>
<td valign=top>set if the checkbox is in the NoChange state.</td>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top>Style_Down</td>
<td valign=top>set if the checkbox is down (ie. mouse button or space pressed on
    the button).</td>
</tr>
<tr bgcolor=#f0f0f0>
<td rowspan=6 valign=top><b>CE_CheckboxLabel</b><br>const QCheckBox *</td>
<td valign=top>Style_Enabled</td>
<td valign=top>set if the checkbox is enabled.</td>
<td rowspan=6 valign=top>unused</td>
<td rowspan=6 valign=top>&nbsp;</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>Style_HasFocus</td>
<td valign=top>set if the checkbox has input focus.</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>Style_On</td>
<td valign=top>set if the checkbox is checked.</td>
</tr>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>Style_Off</td>
<td valign=top>set if the checkbox is not checked.</td>
</tr>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>Style_NoChange</td>
<td valign=top>set if the checkbox is in the NoChange state.</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>Style_Down</td>
<td valign=top>set if the checkbox is down (ie. mouse button or space pressed on
    the button).</td>
</tr>
<tr bgcolor=#d0d0d0>
<td rowspan=5 valign=top><b>CE_RadioButton</b><br>const QRadioButton *</td>
<td valign=top>Style_Enabled</td>
<td valign=top>set if the radiobutton is enabled.</td>
<td rowspan=5 valign=top>unused</td>
<td rowspan=5 valign=top>&nbsp;</td>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top>Style_HasFocus</td>
<td valign=top>set if the radiobutton has input focus.</td>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top>Style_On</td>
<td valign=top>set if the radiobutton is checked.</td>
</tr>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top>Style_Off</td>
<td valign=top>set if the radiobutton is not checked.</td>
</tr>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top>Style_Down</td>
<td valign=top>set if the radiobutton is down (ie. mouse button or space pressed on
    the button).</td>
</tr>
<tr bgcolor=#f0f0f0>
<td rowspan=5 valign=top><b>CE_RadioButtonLabel</b><br>const QRadioButton *</td>
<td valign=top>Style_Enabled</td>
<td valign=top>set if the radiobutton is enabled.</td>
<td rowspan=5 valign=top>unused</td>
<td rowspan=5 valign=top>&nbsp;</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>Style_HasFocus</td>
<td valign=top>set if the radiobutton has input focus.</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>Style_On</td>
<td valign=top>set if the radiobutton is checked.</td>
</tr>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>Style_Off</td>
<td valign=top>set if the radiobutton is not checked.</td>
</tr>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>Style_Down</td>
<td valign=top>set if the radiobutton is down (ie. mouse button or space pressed on
    the button).</td>
</tr>
<tr bgcolor=#d0d0d0>
<td rowspan=2 valign=top><b>CE_TabBarTab</b><br>const QTabBar *</td></td>
<td valign=top>Style_Enabled</td>
<td valign=top>set is the tabbar and tab is enabled.</td>
<td valign=top>QStyleOption(QTab *t)
	<p>opt.\link QStyleOption::tab() tab()\endlink</td>
<td valign=top>\e t is the QTab being drawn.</td>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top>Style_Selected</td>
<td valign=top>set is the tab is selected (ie. the current tab).</td>
<td valign=top>&nbsp;</td>
<td valign=top>&nbsp;</td>
</tr>
<tr bgcolor=#f0f0f0>
<td rowspan=2 valign=top><b>CE_TabBarLabel</b><br>const QTabBar *</td>
<td valign=top>Style_Enabled</td>
<td valign=top>set is the tabbar and tab is enabled.</td>
<td valign=top>QStyleOption(QTab *t)
        <p>opt.\link QStyleOption::tab() tab()\endlink</td>
<td valign=top>\e t is the QTab being drawn.</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>Style_HasFocus</td>
<td valign=top>set is the tab has input focus.</td>
<td valign=top>&nbsp;</td>
<td valign=top>&nbsp;</td>
</tr>
<tr bgcolor=#d0d0d0>
<td rowspan=2 valign=top><b>CE_ProgressBarGroove</b><br>const QProgressBar *</td>
<td valign=top>Style_Enabled</td>
<td valign=top>set is the progressbar is enabled.</td>
<td rowspan=2 valign=top>unused</td>
<td rowspan=2 valign=top>&nbsp;</td>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top>Style_HasFocus</td>
<td valign=top>set is the progressbar has input focus.</td>
</tr>
<tr bgcolor=#f0f0f0>
<td rowspan=2 valign=top><b>CE_ProgressBarContents</b><br>const QProgressBar *</td>
<td valign=top>Style_Enabled</td>
<td valign=top>set is the progressbar is enabled.</td>
<td rowspan=2 valign=top>unused</td>
<td rowspan=2 valign=top>&nbsp;</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>Style_HasFocus</td>
<td valign=top>set is the progressbar has input focus.</td>
</tr>
<tr bgcolor=#d0d0d0>
<td rowspan= 2 valign=top><b>CE_ProgressBarLabel</b><br>const QProgressBar *</td>
<td valign=top>Style_Enabled</td>
<td valign=top>set is the progressbar is enabled.</td>
<td rowspan=2 valign=top>unused</td>
<td rowspan=2 valign=top>&nbsp;</td>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top>Style_HasFocus</td>
<td valign=top>set is the progressbar has input focus.</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top rowspan=3><b>CE_PopupMenuItem</b><br>const QPopupMenu *</td>
<td valign=top>Style_Enabled</td>
<td valign=top>set if the menuitem is enabled</td>
<td rowspan=3 valign=top>QStyleOption(QMenuItem *mi, int tabwidth, int maxpmwidth)
	<p>opt.\link QStyleOption::menuItem() menuItem()\endlink
	<p>opt.\link QStyleOption::tabWidth() tabWidth()\endlink
	<p>opt.\link QStyleOption::maxIconWidth() maxIconWidth()\endlink</td>
<td valign=top>\e mi is the menu item being drawn.
QMenuItem is currently an internal class.</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>Style_Active</td>
<td valign=top>set if the menuitem is active (ie. the current item).</td>
<td valign=top>\e tabwidth is the width of the tab column where key accelerators are drawn.</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>Style_Down</td>
<td valign=top>set if the menuitem is down (ie. mouse button or space pressed).</td>
<td valign=top>\e maxpmwidth is the maximum width of the check column where checkmarks and iconsets
are drawn.</td>
</tr>
<tr bgcolor=#d0d0d0>
<td rowspan=4 valign=top><b>CE_MenuBarItem</b><br>const QMenuBar *</td>
<td valign=top>Style_Enabled</td>
<td valign=top>set if the menuitem is enabled</td>
<td valign=top>QStyleOption(QMenuItem *mi)
	<p>opt.\link QStyleOption::menuItem() menuItem()\endlink</td>
<td valign=top>\e mi is the menu item being drawn.</td>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top>Style_Active</td>
<td valign=top>set if the menuitem is active (ie. the current item).</td>
<td rowspan=3>&nbsp;</td>
<td rowspan=3>&nbsp;</td>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top>Style_Down</td>
<td valign=top>set if the menuitem is down (ie. mouse button or space pressed).</td>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top>Style_HasFocus</td>
<td valign=top>set if the menubar has input focus.</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top rowspan=7><b>CE_ToolButtonLabel</b><br>const QToolButton *</td>
<td valign=top>Style_Enabled</td>
<td valign=top>set if the toolbutton is enabled.</td>
<td valign=top>QStyleOption(ArrowType t)
	<p>opt.\link QStyleOption::arrowType() arrowType()\endlink</td>
<td valign=top>when the tool button only contains an arrow, \e t is the
    arrow's type.</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>Style_HasFocus</td>
<td valign=top>set if the toolbutton has input focus.</td>
<td valign=top>&nbsp;</td>
<td valign=top>&nbsp;</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>Style_Down</td>
<td valign=top>set if the toolbutton is down (ie. mouse button or space pressed).</td>
<td valign=top>&nbsp;</td>
<td valign=top>&nbsp;</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>Style_On</td>
<td valign=top>set if the toolbutton is a toggle button and is toggled on.</td>
<td valign=top rowspan=4>&nbsp;</td>
<td valign=top rowspan=4>&nbsp;</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>Style_AutoRaise</td>
<td valign=top>set if the toolbutton has auto-raise enabled.</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>Style_MouseOver</td>
<td valign=top>set if the mouse pointer is over the toolbutton.</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>Style_Raised</td>
<td valign=top>set if the button is not down, not on and doesn't contain the mouse
    when auto-raise is enabled.</td>
</tr>
</table>
</center>

  \sa ControlElement, StyleFlags
*/

/*!
  \fn void QStyle::drawControlMask( ControlElement element, QPainter *p, const QWidget *widget, const QRect &r, const QStyleOption& opt = QStyleOption::Default ) const;

  Draw a bitmask for the ControlElement \a element using the painter
  \a p in the area \r.  See drawControl() for an
  explanation of the use of the \a widget and \a opt arguments.

  The rect \a r should be in screen coordinates.

  \sa drawControl(), ControlElement
*/

/*!
  \enum QStyle::SubRect

  This enum represents a sub-area of a widget.  Style implementations
  would use these areas to draw the different parts of a widget.

  \value SR_PushButtonContents  area containing the label (iconset with text or pixmap).
  \value SR_PushButtonFocusRect  area for the focus rect (usually larger than the
         contents rect).

  \value SR_CheckBoxIndicator  area for the state indicator (e.g. check mark).
  \value SR_CheckBoxContents  area for the label (text or pixmap).
  \value SR_CheckBoxFocusRect  area for the focus indicator.


  \value SR_RadioButtonIndicator  area for the state indicator.
  \value SR_RadioButtonContents  area for the label.
  \value SR_RadioButtonFocusRect  area for the focus indicator.


  \value SR_ComboBoxFocusRect  area for the focus indicator.


  \value SR_SliderFocusRect  area for the focus indicator.


  \value SR_DockWindowHandleRect  area for the tear-off handle.


  \value SR_ProgressBarGroove  area for the groove.
  \value SR_ProgressBarContents  area for the progress indicator.
  \value SR_ProgressBarLabel  area for the text label.


  \value SR_ToolButtonContents area for the tool button's label.

  \sa subRect()
*/

/*!
  \fn QRect QStyle::subRect( SubRect subrect, const QWidget *widget ) const;

  Returns the sub-area \a subrect for the \a widget in logical
  coordinates.

  The \a widget argument is a pointer to a QWidget or one of its
  subclasses.  The widget can be cast to the appropriate type based on
  the value of \a subrect.  See the table below for the appropriate \a
  widget casts:

  <center>
  <table cellpadding=4 cellspacing=2 border=0>
    <tr bgcolor=#A2C511>
      <th>SubRect</th>
      <th>Widget Cast</th>
    </tr>
    <tr bgcolor=#d0d0d0>
      <td valign=top><b>SR_PushButtonContents</b></td>
      <td valign=top>const QPushButton *</td>
     </tr>
    <tr bgcolor=#f0f0f0>
      <td valign=top><b>SR_PushButtonFocusRect</b></td>
      <td valign=top>const QPushButton *</td>
     </tr>
    <tr bgcolor=#d0d0d0>
      <td valign=top><b>SR_CheckBoxIndicator</b></td>
      <td valign=top>const QCheckBox *</td>
     </tr>
    <tr bgcolor=#f0f0f0>
      <td valign=top><b>SR_CheckBoxContents</b></td>
      <td valign=top>const QCheckBox *</td>
    </tr>
    <tr bgcolor=#d0d0d0>
      <td valign=top><b>SR_CheckBoxFocusRect</b></td>
      <td valign=top>const QCheckBox *</td>
    </tr>
    <tr bgcolor=#f0f0f0>
      <td valign=top><b>SR_RadioButtonIndicator</b></td>
      <td valign=top>const QRadioButton *</td>
    </tr>
    <tr bgcolor=#d0d0d0>
      <td valign=top><b>SR_RadioButtonContents</b></td>
      <td valign=top>const QRadioButton *</td>
    </tr>
    <tr bgcolor=#f0f0f0>
      <td valign=top><b>SR_RadioButtonFocusRect</b></td>
      <td valign=top>const QRadioButton *</td>
    </tr>
    <tr bgcolor=#d0d0d0>
      <td valign=top><b>SR_ComboBoxFocusRect</b></td>
      <td valign=top>const QComboBox *</td>
    </tr>
    <tr bgcolor=#f0f0f0>
      <td valign=top><b>SR_DockWindowHandleRect</b></td>
      <td valign=top>const QWidget *</td>
    </tr>
    <tr bgcolor=#d0d0d0>
      <td valign=top><b>SR_ProgressBarGroove</b></td>
      <td valign=top>const QProgressBar *</td>
    </tr>
    <tr bgcolor=#f0f0f0>
      <td valign=top><b>SR_ProgressBarContents</b></td>
      <td valign=top>const QProgressBar *</td>
    </tr>
    <tr bgcolor=#d0d0d0>
      <td valign=top><b>SR_ProgressBarLabel</b></td>
      <td valign=top>const QProgressBar *</td>
    </tr>
  </table>
  </center>

  \e Note: the tear-off handle (SR_DockWindowHandleRect) for QDockWindow
  is a private class.  You can gain access to the QDockWindow by using
  the QWidget::parentWidget() function.  For example:

  \code
  if (! widget->parentWidget())
      return;
  const QDockWindow *dw = (const QDockWindow *) widget->parentWidget();
  \endcode

  \sa SubRect
*/

/*!
  \enum QStyle::ComplexControl

  This enum represents a ComplexControl. ComplexControls have
  different behaviour depending upon where the user clicks on them or
  which keys are pressed.

  \value CC_SpinWidget
  \value CC_ComboBox
  \value CC_ScrollBar
  \value CC_Slider
  \value CC_ToolButton
  \value CC_TitleBar
  \value CC_ListView

  \sa SubControl drawComplexControl()
*/

/*!
  \enum QStyle::SubControl

  This enum represents a SubControl within a ComplexControl.

  \value SC_None   special value that matches no other SubControl.


  \value SC_ScrollBarAddLine  scrollbar add line (i.e. down/right arrow); see also QScrollbar.
  \value SC_ScrollBarSubLine  scrollbar sub line (i.e. up/left arrow).
  \value SC_ScrollBarAddPage  scrollbar add page (i.e. page down).
  \value SC_ScrollBarSubPage  scrollbar sub page (i.e. page up).
  \value SC_ScrollBarFirst  scrollbar first line (i.e. home).
  \value SC_ScrollBarLast  scrollbar last line (i.e. end).
  \value SC_ScrollBarSlider  scrollbar slider handle.
  \value SC_ScrollBarGroove  special subcontrol which contains the area in which the
         slider handle may move.


  \value SC_SpinWidgetUp  spinwidget up/increase; see also QSpinBox.
  \value SC_SpinWidgetDown  spinwidget down/decrease.
  \value SC_SpinWidgetFrame  spinwidget frame.
  \value SC_SpinWidgetEditField  spinwidget edit field.
  \value SC_SpinWidgetButtonField  spinwidget button field.


  \value SC_ComboBoxEditField  combobox edit field; see also QComboBox.
  \value SC_ComboBoxArrow  combobox arrow
  \value SC_ComboBoxFrame combobox frame


  \value SC_SliderGroove  special subcontrol which contains the area in which the
         slider handle may move.
  \value SC_SliderHandle  slider handle.
  \value SC_SliderTickmarks  slider tickmarks.


  \value SC_ToolButton  tool button; see also QToolbutton.
  \value SC_ToolButtonMenu subcontrol for opening a popup menu in a tool button; see also QPopupMenu.


  \value SC_TitleBarSysMenu  system menu button (i.e. restore, close, etc.).
  \value SC_TitleBarMinButton  minimize button.
  \value SC_TitleBarMaxButton  maximize button.
  \value SC_TitleBarCloseButton  close button.
  \value SC_TitleBarLabel  window title label.
  \value SC_TitleBarNormalButton  normal (restore) button.
  \value SC_TitleBarShadeButton  shade button.
  \value SC_TitleBarUnshadeButton  unshade button.


  \value SC_ListView  (internal)
  \value SC_ListViewBranch  (internal)
  \value SC_ListViewExpand  expand item (i.e. show/hide child items).


  \value SC_All  special value that matches all SubControls.


  \sa ComplexControl
*/

/*!
  \fn void QStyle::drawComplexControl( ComplexControl control, QPainter *p, const QWidget *widget, const QRect &r, const QColorGroup &cg, SFlags how = Style_Default, SCFlags sub = SC_All, SCFlags subActive = SC_None, const QStyleOption& opt = QStyleOption::Default ) const;

  Draws the ComplexControl \a control using the painter \a p in the
  area \a r.  Colors are used from the color group \a cg.  The \a sub
  argument specifies which SubControls to draw.  Multiple SubControls
  can be OR'ed together.  The \a subActive argument specifies which
  SubControl is active.

  The rect \a r should be in logical coordinates.  Reimplementations
  of this function should use visualRect() to change the logical
  coordinates into screen coordinates when using drawPrimitive() and
  drawControl().

  The \a how argument is used to control how the ComplexControl is
  drawn.  Multiple flags can OR'ed together. See the table below for
  an explanation of which flags are used with the various ComplexControls.

  The \a widget argument is a pointer to a QWidget or one of its
  subclasses.  The widget can be cast to the appropriate type based on
  the value of \a control.  The \a opt argument can be used to pass
  extra information required when drawing the ComplexControl.  Note
  that \a opt may be the default value even for ComplexControls that
  can make use of the extra options.  See the table below for the appropriate
  \a widget and \a opt usage:

<center>
<table cellpadding=4 cellspacing=2 border=0>
<tr bgcolor=#A2C511>
<th>ComplexControl<br>&amp; Widget Cast</th>
<th>Style Flags</th>
<th>Notes</th>
<th>Options</th>
<th>Notes</th>
</tr>
<tr bgcolor=#d0d0d0>
<td rowspan=2 valign=top><b>CC_SpinWidget</b><br>const QSpinWidget *</td>
<td valign=top>Style_Enabled</td>
<td valign=top>set if the spinwidget is enabled.</td>
<td rowspan=2 valign=top>unused</td>
<td rowspan=2 valign=top>&nbsp;</td>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top>Style_HasFocus</td>
<td valign=top>set if the spinwidget has input focus.</td>
</tr>
<tr bgcolor=#f0f0f0>
<td rowspan=2 valign=top><b>CC_ComboBox</b><br>const QComboBox *</td>
<td valign=top>Style_Enabled</td>
<td valign=top>set if the combobox is enabled.</td>
<td rowspan=2 valign=top>unused</td>
<td rowspan=2 valign=top>&nbsp;</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>Style_HasFocus</td>
<td valign=top>set if the combobox has input focus.</td>
</tr>
<tr bgcolor=#d0d0d0>
<td rowspan=2 valign=top><b>CC_ScrollBar</b><br>const QScrollBar *</td>
<td valign=top>Style_Enabled</td>
<td valign=top>set if the scrollbar is enabled.</td>
<td valign=top>unused</td>
<td valign=top>&nbsp;</td>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top>Style_HasFocus</td>
<td valign=top>set if the scrollbar has input focus.</td>
<td valign=top>&nbsp;</td>
<td valign=top>&nbsp;</td>
</tr>
<tr bgcolor=#f0f0f0>
<td rowspan=2 valign=top><b>CC_Slider</b><br>const QSlider *</td>
<td valign=top>Style_Enabled</td>
<td valign=top>set if the slider is enabled.</td>
<td valign=top>unused</td>
<td valign=top>&nbsp;</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top>Style_HasFocus</td>
<td valign=top>set if the slider has input focus.</td>
<td valign=top>&nbsp;</td>
<td valign=top>&nbsp;</td>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top rowspan=7><b>CC_ToolButton</b><br>const QToolButton *</td>
<td valign=top>Style_Enabled</td>
<td valign=top>set if the toolbutton is enabled.</td>
<td valign=top>QStyleOption(ArrowType t)<p>opt.\link QStyleOption::arrowType() arrowType()\endlink</td>
<td valign=top>when the tool button only contains an arrow, \e t is the
    arrow's type.</td>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top>Style_HasFocus</td>
<td valign=top>set if the toolbutton has input focus.</td>
<td valign=top>&nbsp;</td>
<td valign=top>&nbsp;</td>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top>Style_Down</td>
<td valign=top>set if the toolbutton is down (ie. mouse button or space pressed).</td>
<td valign=top>&nbsp;</td>
<td valign=top>&nbsp;</td>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top>Style_On</td>
<td valign=top>set if the toolbutton is a toggle button and is toggled on.</td>
<td valign=top rowspan=4>&nbsp;</td>
<td valign=top rowspan=4>&nbsp;</td>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top>Style_AutoRaise</td>
<td valign=top>set if the toolbutton has auto-raise enabled.</td>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top>Style_MouseOver</td>
<td valign=top>set if the mouse pointer is over the toolbutton.</td>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top>Style_Raised</td>
<td valign=top>set if the button is not down, not on and doesn't contain the mouse
    when auto-raise is enabled.</td>
</tr>
<tr bgcolor=#f0f0f0>
<td valign=top><b>CC_TitleBar</b><br>const QWidget *</td>
<td valign=top>Style_Enabled</td>
<td valign=top>set if the titlebar is enabled.</td>
<td valign=top>unused</td>
<td valign=top>&nbsp;</td>
</tr>
<tr bgcolor=#d0d0d0>
<td valign=top><b>CC_ListView</b><br>const QListView *</td>
<td valign=top>Style_Enabled</td>
<td valign=top>set if the titlebar is enabled.</td>
<td valign=top>QStyleOption(QListViewItem *item)<p>opt.\link QStyleOption::listViewItem() listViewItem()\endlink</td>
<td valign=top>\e item is the item that needs branches drawn</td>
</tr>
</table>
</center>

  \sa ComplexControl, SubControl
*/

/*!
  \fn void QStyle::drawComplexControlMask( ComplexControl control, QPainter *p, const QWidget *widget, const QRect &r, const QStyleOption& opt = QStyleOption::Default ) const;

  Draw a bitmask for the ComplexControl \a control using the painter
  \a p in the area \a r.  See drawComplexControl() for an explanation
  of the use of the \a widget and \a opt arguments.

  The rect \a r should be in logical coordinates.  Reimplementations
  of this function should use visualRect() to change the logical
  corrdinates into screen coordinates when using drawPrimitive() and
  drawControl().

  \sa drawComplexControl() ComplexControl
*/

/*!
  \fn QRect QStyle::querySubControlMetrics( ComplexControl control, const QWidget *widget, SubControl subcontrol, const QStyleOption& opt = QStyleOption::Default ) const;

  Returns the rect for the SubControl \a subcontrol for \a widget in
  logical coordinates.

  The \a widget argument is a pointer to a QWidget or one of its
  subclasses.  The widget can be cast to the appropriate type based on
  the value of \a control. The \a opt argument can be used to pass
  extra information required when drawing the ComplexControl.  Note
  that \a opt may be the default value even for ComplexControls that can make use of
  the extra options. See drawComplexControl() for an explanation of the
  \a widget and \a opt arguments.

  \sa drawComplexControl(), ComplexControl, SubControl
*/

/*!
  \fn SubControl QStyle::querySubControl( ComplexControl control, const QWidget *widget, const QPoint &pos, const QStyleOption& opt = QStyleOption::Default ) const;

  Returns the SubControl for \a widget at the point \a pos.  The \a
  widget argument is a pointer to a QWidget or one of its subclasses.
  The widget can be casted to the appropriate type based on the value of
  \a control.  The \a opt argument can be used to pass extra
  information required when drawing the ComplexControl.  Note that \a
  opt may be the default value even for ComplexControls that can make use of the
  extra options. See drawComplexControl() for an explanation of the \a
  widget and \a opt arguments.

  Note that \a pos is passed in screen coordinates.  When using
  querySubControlMetrics() to check for hits and misses, use
  visualRect() to change the logical coordinates into screen
  coordinates.

  \sa drawComplexControl(), ComplexControl, SubControl, querySubControlMetrics()
*/

/*!
  \enum QStyle::PixelMetric

  This enum represents a PixelMetric.  A PixelMetric is a style dependent size
  represented as a single pixel value.

  \value PM_ButtonMargin  amount of whitespace between pushbutton labels and the frame.
  \value PM_ButtonDefaultIndicator  width of the default-button indicator frame.
  \value PM_MenuButtonIndicator  width of the menu button indicator
  proportional to the widget height.
  \value PM_ButtonShiftHorizontal  horizontal contents shift of a button when the
         button is down.
  \value PM_ButtonShiftVertical  vertical contents shift of a button when the
         button is down.


  \value PM_DefaultFrameWidth  default frame width, usually 2.
  \value PM_SpinBoxFrameWidth  frame width of a spin box.


  \value PM_MaximumDragDistance  Some feels require the scrollbar or
    other sliders to jump back to the original position when the mouse
    pointer is too far away while dragging.  A value of -1 disables this
    behavior.


  \value PM_ScrollBarExtent  width of a vertical scrollbar and the height of a
         horizontal scrollbar.
  \value PM_ScrollBarSliderMin the minimum height of a vertical scrollbar's slider
         and the minimum width of a horiztonal scrollbar slider.


  \value PM_SliderThickness  total slider thickness.
  \value PM_SliderControlThickness  thickness of the slider handle.
  \value PM_SliderLength length of the slider.
  \value PM_SliderTickmarkOffset the offset between the tickmarks and the slider.
  \value PM_SliderSpaceAvailable  the available space for the slider to move.


  \value PM_DockWindowSeparatorExtent  width of a separator in a horiztonal dock window
         and the height of a separator in a vertical dock window.
  \value PM_DockWindowHandleExtent  width of the handle in a horizontal dock window and
         the height of the handle in a vertical dock window.
  \value PM_DockWindowFrameWidth  frame width of a dock window.


  \value PM_MenuBarFrameWidth  frame width of a menubar.


  \value PM_TabBarTabOverlap number of pixels the tabs should overlap.
  \value PM_TabBarTabHSpace extra space added to the tab width.
  \value PM_TabBarTabVSpace extra space added to the tab height.
  \value PM_TabBarBaseHeight height of the area between the tab bar and the tab pages.
  \value PM_TabBarBaseOverlap number of pixels the tab bar overlaps the tab bar base.


  \value PM_ProgressBarChunkWidth  width of a chunk in a progress bar indicator.


  \value PM_SplitterWidth  width of a splitter.


  \value PM_IndicatorWidth  width of a check box indicator.
  \value PM_IndicatorHeight  height of a checkbox indicator.
  \value PM_ExclusiveIndicatorWidth  width of a radio button indicator.
  \value PM_ExclusiveIndicatorHeight  height of a radio button indicator.

  \sa pixelMetric()
*/

/*!
  \fn int QStyle::pixelMetric( PixelMetric metric, const QWidget *widget = 0 ) const;

  Returns the pixel metric for \a metric.  The \a widget argument is a
  pointer to a QWidget or one of its subclasses.  The widget can be
  cast to the appropriate type based on the value of \a metric.  Note
  that \a widget may be zero even for PixelMetrics that can make use
  of \a widget. See the table below for the appropriate \a widget
  casts:

  <center>
  <table cellpadding=4 cellspacing=2 border=0>
    <tr bgcolor=#A2C511>
      <th>PixelMetric</th>
      <th>Widget Cast</th>
    </tr>
    <tr bgcolor=#d0d0d0>
      <td valign=top>PM_SliderControlThickness</td>
      <td valign=top>const QSlider *</td>
    </tr>
    <tr bgcolor=#f0f0f0>
      <td valign=top>PM_SliderLength</td>
      <td valign=top>const QSlider *</td>
    </tr>
    <tr bgcolor=#d0d0d0>
      <td valign=top>PM_SliderTickmarkOffset</td>
      <td valign=top>const QSlider *</td>
    </tr>
    <tr bgcolor=#f0f0f0>
      <td valign=top>PM_SliderSpaceAvailable</td>
      <td valign=top>const QSlider *</td>
    </tr>
    <tr bgcolor=#d0d0d0>
      <td valign=top>PM_TabBarTabOverlap</td>
      <td valign=top>const QTabBar *</td>
    </tr>
    <tr bgcolor=#f0f0f0>
      <td valign=top>PM_TabBarTabHSpace</td>
      <td valign=top>const QTabBar *</td>
    </tr>
    <tr bgcolor=#d0d0d0>
      <td valign=top>PM_TabBarTabVSpace</td>
      <td valign=top>const QTabBar *</td>
    </tr>
    <tr bgcolor=#f0f0f0>
      <td valign=top>PM_TabBarBaseHeight</td>
      <td valign=top>const QTabBar *</td>
    </tr>
    <tr bgcolor=#d0d0d0>
      <td valign=top>PM_TabBarBaseOverlap</td>
      <td valign=top>const QTabBar *</td>
    </tr>
  </table>
  </center>

  \sa PixelMetric
*/

/*!
  \enum QStyle::ContentsType

  This enum represents a ContentsType.  It is used to calculate sizes
  for the contents of various widgets.

  \value CT_PushButton
  \value CT_CheckBox
  \value CT_RadioButton
  \value CT_ToolButton
  \value CT_ComboBox
  \value CT_Splitter
  \value CT_DockWindow
  \value CT_ProgressBar
  \value CT_PopupMenuItem

  \sa sizeFromContents()
*/

/*!
  \fn QSize QStyle::sizeFromContents( ContentsType contents, const QWidget *widget, const QSize &contentsSize, const QStyleOption& opt = QStyleOption::Default ) const;

  Returns the size of \a widget based on the contents size \a
  contentsSize.

  The \a widget argument is a pointer to a QWidget or one of its
  subclasses.  The widget can be cast to the appropriate type based on
  the value of \a contents. The \a opt argument can be used to pass
  extra information required when calculating the size.  Note that \a
  opt may be the default value even for ContentsTypes that can make use of the extra
  options.  See the table below for the appropriate \a widget and \a opt
  usage:

  <center>
  <table cellpadding=4 cellspacing=2 border=0>
    <tr bgcolor=#A2C511>
      <th>ContentsType</th>
      <th>Widget Cast</th>
      <th>Options</th>
      <th>Notes</th>
    </tr>
    <tr bgcolor=#d0d0d0>
      <td valign=top><b>CT_PushButton</b></td>
      <td valign=top>const QPushButton *</td>
      <td valign=top>unused</td>
      <td valign=top>&nbsp;</td>
    </tr>
    <tr bgcolor=#f0f0f0>
      <td valign=top><b>CT_CheckBox</b></td>
      <td valign=top>const QCheckBox *</td>
      <td valign=top>unused</td>
      <td valign=top>&nbsp;</td>
    </tr>
    <tr bgcolor=#d0d0d0>
      <td valign=top><b>CT_RadioButton</b></td>
      <td valign=top>const QRadioButton *</td>
      <td valign=top>unused</td>
      <td valign=top>&nbsp;</td>
    </tr>
    <tr bgcolor=#f0f0f0>
      <td valign=top><b>CT_ToolButton</b></td>
      <td valign=top>const QToolButton *</td>
      <td valign=top>unused</td>
      <td valign=top>&nbsp;</td>
    </tr>
    <tr bgcolor=#d0d0d0>
      <td valign=top><b>CT_ComboBox</b></td>
      <td valign=top>const QComboBox *</td>
      <td valign=top>unused</td>
      <td valign=top>&nbsp;</td>
    </tr>
    <tr bgcolor=#f0f0f0>
      <td valign=top><b>CT_Splitter</b></td>
      <td valign=top>const QSplitter *</td>
      <td valign=top>unused</td>
      <td valign=top>&nbsp;</td>
    </tr>
    <tr bgcolor=#d0d0d0>
      <td valign=top><b>CT_DockWindow</b></td>
      <td valign=top>const QDockWindow *</td>
      <td valign=top>unused</td>
      <td valign=top>&nbsp;</td>
    </tr>
    <tr bgcolor=#f0f0f0>
      <td valign=top><b>CT_ProgressBar</b></td>
      <td valign=top>const QProgressBar *</td>
      <td valign=top>unused</td>
      <td valign=top>&nbsp;</td>
    </tr>
    <tr bgcolor=#d0d0d0>
      <td valign=top><b>CT_PopupMenuItem</b></td>
      <td valign=top>const QPopupMenu *</td>
      <td valign=top>QStyleOption(QMenuItem *mi)
	    <p>opt.\link QStyleOption::menuItem() menuItem()\endlink</td>
      <td valign=top>\e mi is the menu item to use when calculating the size.
      QMenuItem is currently an internal class.</td>
    </tr>
  </table>
  </center>

  \sa ContentsType
*/

/*!
  \enum QStyle::StyleHint

  This enum represents a StyleHint.  A StyleHint is a general look
  and/or feel hint.

  \value SH_EtchDisabledText disabled text is "etched" like Windows.

  \value SH_GUIStyle the GUI style to use.

  \value SH_ScrollBar_BackgroundMode  the background mode for a
  QScrollBar.  Possible values are any of those in the \link
  Qt::BackgroundMode BackgroundMode\endlink enum.

  \value SH_ScrollBar_MiddleClickAbsolutePosition  a boolean value. If
  TRUE, middle clicking on a scrollbar causes the slider to jump to
  that position.  If FALSE, the middle clicking is ignored.

  \value SH_ScrollBar_ScrollWhenPointerLeavesControl  a boolean value.
  If TRUE, when clicking a scrollbar SubControl, holding the mouse
  button down and moving the pointer outside the SubControl, the
  scrollbar continues to scroll.  If FALSE, the scollbar stops
  scrolling when the pointer leaves the SubControl.


  \value SH_TabBar_Alignment  the alignment for tabs in a QTabWidget.
  Possible values are Qt::AlignLeft, Qt::AlignCenter and
  Qt::AlignRight.

    \value SH_Header_ArrowAlignment the placement of the sorting
    indicator may appear in list or table headers.
    Possible values are Qt::Left or Qt::Right.

    \value SH_Slider_SnapToValue  sliders snap to values while moving,
    like Windows

    \value SH_Slider_SloppyKeyEvents  key presses handled in a sloppy
    manner, i.e. left on a vertical slider subtracts a line.

    \value SH_ProgressDialog_CenterCancelButton  center button on
    progress dialogs, like Motif, otherwise right aligned.

    \value SH_ProgressDialog_TextLabelAlignment Qt::AlignmentFlags --
    text label alignment in progress dialogs; Center on windows,
    Auto|VCenter otherwise.

    \value SH_PrintDialog_RightAlignButtons  right align buttons in
    the print dialog, like Windows.

    \value SH_MainWindow_SpaceBelowMenuBar 1 or 2 pixel space between
    the menubar and the dockarea, like Windows.

    \value SH_FontDialog_SelectAssociatedText select the text in the
    line edit, or when selecting an item from the
    listbox, or when the line edit receives focus, like Windows.

    \value SH_PopupMenu_AllowActiveAndDisabled  allows disabled menu
    items to be active.

    \value SH_PopupMenu_SpaceActivatesItem  pressing Space activates
    the item, like Motif.

    \value SH_PopupMenu_SubMenuPopupDelay  the number of milliseconds
    to wait before opening a submenu; 256 on windows, 96 on Motif.

    \value SH_ScrollView_FrameOnlyAroundContents  whether scrollviews
    draw their frame only around contents (like Motif), or around
    contents, scrollbars and corner widgets (like Windows).

    \value SH_MenuBar_AltKeyNavigation  menubars items are navigable
    by pressing Alt, followed by using the arrow keys to select the
    desired item.

    \value SH_ComboBox_ListMouseTracking  mouse tracking in combobox
    dropdown lists.

    \value SH_PopupMenu_MouseTracking  mouse tracking in popup menus.

    \value SH_MenuBar_MouseTracking  mouse tracking in menubars.

    \value SH_ItemView_ChangeHighlightOnFocus  gray out selected items
    when losing focus.

  \sa styleHint()
*/

/*!
  \fn int QStyle::styleHint( StyleHint stylehint, const QWidget *widget = 0, QStyleHintReturn *returnData = 0 ) const;

  Returns the style hint \a stylehint for \a widget.  Currently, \a
  widget and \a returnData are unused, and are provided only for
  future development considerations.

  For an explanation of the return value see
  \link QStyle::StyleHint StyleHint\endlink.

  \sa StyleHint
*/

/*!
  \enum QStyle::StylePixmap

  This enum represents a StylePixmap.  A StylePixmap is a pixmap that
  can follow some existing GUI style or guideline.


  \value SP_TitleBarMinButton  minimize button on titlebars.  For example, in a
         QWorkspace.
  \value SP_TitleBarMaxButton  maximize button on titlebars.
  \value SP_TitleBarCloseButton  close button on titlebars.
  \value SP_TitleBarNormalButton  normal (restore) button on titlebars.
  \value SP_TitleBarShadeButton  shade button on titlebars.
  \value SP_TitleBarUnshadeButton  unshade button on titlebars.
  \value SP_MessageBoxInformation the 'information' icon.
  \value SP_MessageBoxWarning the 'warning' icon.
  \value SP_MessageBoxCritical the 'critical' icon.


  \value SP_DockWindowCloseButton  close button on dock windows;  see also QDockWindow.

  \sa stylePixmap()
*/

/*!
  \fn QPixmap QStyle::stylePixmap( StylePixmap stylepixmap, const QWidget *widget = 0, const QStyleOption& opt = QStyleOption::Default ) const;

  Returns a pixmap for \a stylepixmap.

  The \a opt argument can be used to pass extra information required
  when drawing the ControlElement.  Note that \a opt may be the
  default value even
  for StylePixmaps that can make use of the extra options.  Currently,
  the \a opt argument is unused.

  The \a widget argument is a pointer to a QWidget or one of its
  subclasses.  The widget can be cast to the appropriate type based on
  the value of \a stylepixmap. See the table below for the appropriate
  \a widget casts:

  <center>
  <table cellpadding=4 cellspacing=2 border=0>
    <tr bgcolor=#A2C511>
      <th>StylePixmap</th>
      <th>Widget Cast</th>
    </tr>
    <tr bgcolor=#d0d0d0>
      <td valign=top><b>SP_TitleBarMinButton</b></td>
      <td valign=top>const QWidget *</td>
    </tr>
    <tr bgcolor=#f0f0f0>
      <td valign=top><b>SP_TitleBarMaxButton</b></td>
      <td valign=top>const QWidget *</td>
    </tr>
    <tr bgcolor=#d0d0d0>
      <td valign=top><b>SP_TitleBarCloseButton</b></td>
      <td valign=top>const QWidget *</td>
    </tr>
    <tr bgcolor=#f0f0f0>
      <td valign=top><b>SP_TitleBarNormalButton</b></td>
      <td valign=top>const QWidget *</td>
    </tr>
    <tr bgcolor=#d0d0d0>
      <td valign=top><b>SP_TitleBarShadeButton</b></td>
      <td valign=top>const QWidget *</td>
    </tr>
    <tr bgcolor=#f0f0f0>
      <td valign=top><b>SP_TitleBarUnshadeButton</b></td>
      <td valign=top>const QWidget *</td>
    </tr>
    <tr bgcolor=#d0d0d0>
      <td valign=top><b>SP_DockWindowCloseButton</b></td>
      <td valign=top>const QDockWindow *</td>
    </tr>
  </table>
  </center>

  \sa StylePixmap
*/

/*!
  \fn QRect QStyle::visualRect( const QRect &logical, const QWidget *w );

  Returns the rect \a logical in screen coordinates.  The bounding
  rect for widget \a w is used to perform the translation.  This
  function is provided to aid style implementors in supporting
  right-to-left mode.

  \sa QApplication::reverseLayout()
*/
QRect QStyle::visualRect( const QRect &logical, const QWidget *w )
{
    QRect boundingRect = w->rect();
    QRect r = logical;
    if ( QApplication::reverseLayout() )
	r.moveBy( 2*(boundingRect.right() - logical.right()) +
		  logical.width() - boundingRect.width(), 0 );
    return r;
}

/*!
  \overload QRect QStyle::visualRect( const QRect &logical, const QRect &bounding );

  Returns the rect \a logical in screen coordinates.  The rect \a bounding
  is used to perform the translation.  This function is provided to aid style
  implementors in supporting right-to-left mode.

  \sa QApplication::reverseLayout()
*/
QRect QStyle::visualRect( const QRect &logical, const QRect &boundingRect )
{
    QRect r = logical;
    if ( QApplication::reverseLayout() )
	r.moveBy( 2*(boundingRect.right() - logical.right()) +
		  logical.width() - boundingRect.width(), 0 );
    return r;
}

/*!
  \fn int QStyle::defaultFrameWidth() const
  \obsolete
*/

/*!
  \fn void QStyle::tabbarMetrics( const QWidget *, int &, int &, int & ) const
  \obsolete
*/

/*!
  \fn QSize QStyle::scrollBarExtent() const
  \obsolete
*/

#endif // QT_NO_STYLE

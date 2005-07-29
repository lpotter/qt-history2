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

#include "qmenu.h"
#include "qhash.h"
#include "qapplication.h"
#include <private/qt_mac_p.h>
#include "qregexp.h"
#include "qmainwindow.h"
#include "qdockwidget.h"
#include "qtoolbar.h"
#include "qevent.h"
#include "qstyle.h"

#include <private/qapplication_p.h>
#include <private/qmenu_p.h>
#include <private/qmenubar_p.h>

/*****************************************************************************
  QMenu debug facilities
 *****************************************************************************/

/*****************************************************************************
  QMenu globals
 *****************************************************************************/
bool qt_mac_no_menubar_icons = false;
bool qt_mac_no_native_menubar = false;
bool qt_mac_no_menubar_merge = false;

static uint qt_mac_menu_static_cmd_id = 'QT00';
const UInt32 kMenuCreatorQt = 'cute';
enum {
    kMenuPropertyQAction = 'QAcT',
    kMenuPropertyQWidget = 'QWId',
    kMenuPropertyCausedQWidget = 'QCAU',
    kMenuPropertyMergeMenu = 'QApP',

    kHICommandAboutQt = 'AOQT'
};
struct {
    QPointer<QMenuBar> qmenubar;
    bool modal;
} qt_mac_current_menubar = { 0, false };

/*****************************************************************************
  Externals
 *****************************************************************************/
extern IconRef qt_mac_create_iconref(const QPixmap &px); //qpixmap_mac.cpp
extern QWidget * mac_keyboard_grabber; //qwidget_mac.cpp

/*****************************************************************************
  QMenu utility functions
 *****************************************************************************/
inline static QString qt_mac_no_ampersands(QString str) {
    for(int w = -1; (w=str.indexOf('&', w+1)) != -1;) {
        if(w < (int)str.length())
            str.remove(w, 1);
    }
    return str;
}

bool qt_mac_watchingAboutToShow(QMenu *menu)
{
    return menu && menu->receivers(SIGNAL(aboutToShow()));
}

static int qt_mac_CountMenuItems(MenuRef menu)
{
    if(menu) {
        int ret = 0;
        const int items = CountMenuItems(menu);
        for(int i = 0; i < items; i++) {
            MenuItemAttributes attr;
            if(GetMenuItemAttributes(menu, i+1, &attr) == noErr &&
               attr & kMenuItemAttrHidden)
                continue;
            ++ret;
        }
        return ret;
    }
    return 0;
}

//lookup a QMacMenuAction in a menu
static int qt_mac_menu_find_action(MenuRef menu, MenuCommand cmd)
{
    MenuItemIndex ret_idx;
    MenuRef ret_menu;
    if(GetIndMenuItemWithCommandID(menu, cmd, 1, &ret_menu, &ret_idx) == noErr) {
        if (ret_menu == menu)
            return (int)ret_idx;
    }
    return -1;
}
static int qt_mac_menu_find_action(MenuRef menu, QMacMenuAction *action)
{
    return qt_mac_menu_find_action(menu, action->command);
}

//enabling of commands
void qt_mac_command_set_enabled(MenuRef menu, UInt32 cmd, bool b)
{
    if(b) {
        EnableMenuCommand(menu, cmd);
        if(MenuRef dock_menu = GetApplicationDockTileMenu())
            EnableMenuCommand(dock_menu, cmd);
    } else {
        DisableMenuCommand(menu, cmd);
        if(MenuRef dock_menu = GetApplicationDockTileMenu())
            DisableMenuCommand(dock_menu, cmd);
    }
}

//toggling of modal state
void qt_mac_set_modal_state(MenuRef menu, bool on)
{
    for(int i = 1; i < CountMenuItems(menu); i++) {
        MenuRef submenu;
        GetMenuItemHierarchicalMenu(menu, i+1, &submenu);
        if(on)
            DisableMenuItem(submenu, 0);
        else
            EnableMenuItem(submenu, 0);
    }

    UInt32 commands[] = { kHICommandQuit, kHICommandPreferences, kHICommandAbout, kHICommandAboutQt, 0 };
    for(int c = 0; commands[c]; c++) {
        bool enabled = !on;
        if(enabled) {
            QMacMenuAction *action = 0;
            if(GetMenuCommandProperty(menu, commands[c], kMenuCreatorQt, kMenuPropertyQAction,
                                      sizeof(action), 0, &action) != noErr || !action) {
                if(commands[c] != kHICommandQuit)
                    enabled = false;
            } else {
                enabled = action->action ? action->action->isEnabled() : 0;
            }
        }
        qt_mac_command_set_enabled(menu, commands[c], enabled);
    }
}

void qt_mac_clear_menubar()
{
    MenuRef clear_menu = 0;
    if(CreateNewMenu(0, 0, &clear_menu) == noErr) {
        SetRootMenu(clear_menu);
        ReleaseMenu(clear_menu);
    } else {
        qWarning("Unknown error! %s:%d", __FILE__, __LINE__);
    }
    ClearMenuBar();
    qt_mac_command_set_enabled(0, kHICommandPreferences, false);
    InvalMenuBar();
}

static MenuCommand qt_mac_menu_merge_action(MenuRef merge, QMacMenuAction *action)
{
    if(qt_mac_no_menubar_merge || action->action->menu() || action->action->isSeparator())
        return 0;

    QString t = qt_mac_no_ampersands(action->action->text().toLower());
    int st = t.lastIndexOf('\t');
    if(st != -1)
        t.remove(st, t.length()-st);
    t.replace(QRegExp(QString::fromLatin1("\\.*$")), ""); //no ellipses
    //now the fun part
    MenuCommand ret = 0;
#define MENU_TRANSLATE(x) QCoreApplication::instance()->translate("QMenuBar", x)
    QString aboutString = MENU_TRANSLATE("About").toLower();
    if(t.startsWith(aboutString) || t.endsWith(aboutString)) {
        if(t.indexOf(QRegExp(QString::fromLatin1("qt$"), Qt::CaseInsensitive)) == -1)
            ret = kHICommandAbout;
        else
            ret = kHICommandAboutQt;
    } else if(t.startsWith(MENU_TRANSLATE("Config").toLower()) || t.startsWith(MENU_TRANSLATE("Preference").toLower()) ||
              t.startsWith(MENU_TRANSLATE("Options").toLower()) || t.startsWith(MENU_TRANSLATE("Setting").toLower()) ||
              t.startsWith(MENU_TRANSLATE("Setup").toLower())) {
        ret = kHICommandPreferences;
    } else if(t.startsWith(MENU_TRANSLATE("Quit").toLower()) || t.startsWith(MENU_TRANSLATE("Exit").toLower())) {
        ret = kHICommandQuit;
    }
#undef MENU_TRANSLATE
    QAction *cmd_action = 0;
    GetMenuCommandProperty(merge, ret, kMenuCreatorQt, kMenuPropertyQAction,
                           sizeof(cmd_action), 0, &cmd_action);
    if(cmd_action)
        return 0; //already taken
    return ret;
}

static bool qt_mac_auto_apple_menu(MenuCommand cmd)
{
    return (cmd == kHICommandPreferences || cmd == kHICommandQuit);
}

static QString qt_mac_menu_merge_text(MenuCommand cmd)
{
    QString ret;
    if(cmd == kHICommandAbout)
        ret = "About " + QString(qAppName());
    else if(cmd == kHICommandAboutQt)
        ret = "About Qt";
    else if(cmd == kHICommandPreferences)
        ret = "Preferences";
    else if(cmd == kHICommandQuit)
        ret = "Quit " + QString(qAppName());
    return ret;
}

static QKeySequence qt_mac_menu_merge_accel(MenuCommand cmd)
{
    QKeySequence ret;
    if(cmd == kHICommandPreferences)
        ret = QKeySequence(Qt::CTRL+Qt::Key_Comma);
    else if(cmd == kHICommandQuit)
        ret = QKeySequence(Qt::CTRL+Qt::Key_Q);
    return ret;
}

void Q_GUI_EXPORT qt_mac_set_menubar_icons(bool b) { qt_mac_no_menubar_icons = !b; }
void Q_GUI_EXPORT qt_mac_set_native_menubar(bool b) { qt_mac_no_native_menubar = !b; }
void Q_GUI_EXPORT qt_mac_set_menubar_merge(bool b) { qt_mac_no_menubar_merge = !b; }

bool qt_mac_activate_action(MenuRef menu, uint command, QAction::ActionEvent action_e, bool by_accel)
{
    //fire event
    QMacMenuAction *action = 0;
    if(GetMenuCommandProperty(menu, command, kMenuCreatorQt, kMenuPropertyQAction, sizeof(action), 0, &action) != noErr)
        return false;
    if(action_e == QAction::Trigger && by_accel && action->ignore_accel) //no, not a real accel (ie tab)
        return false;
    action->action->activate(action_e);

    //now walk up firing for each "caused" widget (like in the platform independant menu)
    while(menu) {
        //fire
        QWidget *widget = 0;
        GetMenuItemProperty(menu, 0, kMenuCreatorQt, kMenuPropertyQWidget, sizeof(widget), 0, &widget);
        if(QMenu *qmenu = ::qobject_cast<QMenu*>(widget)) {
            if(action_e == QAction::Trigger) {
                emit qmenu->triggered(action->action);
#ifdef QT3_SUPPORT
                emit qmenu->activated(qmenu->findIdForAction(action->action));
#endif
            } else if(action_e == QAction::Hover) {
                emit qmenu->hovered(action->action);
#ifdef QT3_SUPPORT
                emit qmenu->highlighted(qmenu->findIdForAction(action->action));
#endif
            }
        } else if(QMenuBar *qmenubar = ::qobject_cast<QMenuBar*>(widget)) {
            if(action_e == QAction::Trigger) {
                emit qmenubar->triggered(action->action);
#ifdef QT3_SUPPORT
                emit qmenubar->activated(qmenu->findIdForAction(action->action));
#endif
            } else if(action_e == QAction::Hover) {
                emit qmenubar->hovered(action->action);
#ifdef QT3_SUPPORT
                emit qmenubar->highlighted(qmenu->findIdForAction(action->action));
#endif
            }
            break; //nothing more..
        }

        //walk up
        QWidget *caused = 0;
        if(GetMenuItemProperty(menu, 0, kMenuCreatorQt, kMenuPropertyCausedQWidget, sizeof(caused), 0, &caused) != noErr)
            break;
        if(QMenu *qmenu2 = ::qobject_cast<QMenu*>(caused))
            menu = qmenu2->macMenu();
        else if(QMenuBar *qmenubar2 = ::qobject_cast<QMenuBar*>(caused))
            menu = qmenubar2->macMenu();
        else
            menu = 0;
    }
    if(action_e == QAction::Trigger)
        HiliteMenu(0);
    return true;
}

//handling of events for menurefs created by Qt..
static EventTypeSpec menu_events[] = {
    { kEventClassCommand, kEventCommandProcess },
    { kEventClassMenu, kEventMenuTargetItem },
    { kEventClassMenu, kEventMenuOpening },
    { kEventClassMenu, kEventMenuClosed },
};
OSStatus qt_mac_menu_event(EventHandlerCallRef er, EventRef event, void *)
{
    bool handled_event = true;
    UInt32 ekind = GetEventKind(event), eclass = GetEventClass(event);
    switch(eclass) {
    case kEventClassCommand:
        if(ekind == kEventCommandProcess) {
            UInt32 context;
            GetEventParameter(event, kEventParamMenuContext, typeUInt32,
                              0, sizeof(context), 0, &context);
            HICommand cmd;
            GetEventParameter(event, kEventParamDirectObject, typeHICommand,
                              0, sizeof(cmd), 0, &cmd);
            if(!mac_keyboard_grabber && (context & kMenuContextKeyMatching)) {
                QMacMenuAction *action = 0;
                if(GetMenuCommandProperty(cmd.menu.menuRef, cmd.commandID, kMenuCreatorQt,
                                          kMenuPropertyQAction, sizeof(action), 0, &action) == noErr) {
                    QWidget *widget = 0;
                    if (qApp->activePopupWidget())
                        widget = (qApp->activePopupWidget()->focusWidget() ?
                                  qApp->activePopupWidget()->focusWidget() : qApp->activePopupWidget());
                    else if(QApplicationPrivate::focus_widget)
                        widget = QApplicationPrivate::focus_widget;
                    if(widget) {
                        int key = action->action->shortcut();
                        QKeyEvent accel_ev(QEvent::ShortcutOverride, (key & (~Qt::KeyboardModifierMask)),
                                           Qt::KeyboardModifiers(key & Qt::KeyboardModifierMask));
                        accel_ev.ignore();
                        qt_sendSpontaneousEvent(widget, &accel_ev);
                        if(accel_ev.isAccepted()) {
                            handled_event = false;
                            break;
                        }
                    }
                }
            }
            handled_event = qt_mac_activate_action(cmd.menu.menuRef, cmd.commandID,
                                                   QAction::Trigger, context & kMenuContextKeyMatching);
        }
        break;
    case kEventClassMenu: {
        MenuRef menu;
        GetEventParameter(event, kEventParamDirectObject, typeMenuRef, NULL, sizeof(menu), NULL, &menu);
        if(ekind == kEventMenuTargetItem) {
            MenuCommand command;
            GetEventParameter(event, kEventParamMenuCommand, typeMenuCommand,
                              0, sizeof(command), 0, &command);
            handled_event = qt_mac_activate_action(menu, command, QAction::Hover, false);
        } else if(ekind == kEventMenuOpening || ekind == kEventMenuClosed) {
            MenuRef mr;
            GetEventParameter(event, kEventParamDirectObject, typeMenuRef,
                              0, sizeof(mr), 0, &mr);

            QWidget *widget = 0;
            if(GetMenuItemProperty(mr, 0, kMenuCreatorQt, kMenuPropertyQWidget, sizeof(widget), 0, &widget) == noErr) {
                if(QMenu *qmenu = ::qobject_cast<QMenu*>(widget)) {
                    handled_event = true;
                    if(ekind == kEventMenuOpening)
                        emit qmenu->aboutToShow();
#ifdef QT3_SUPPORT
                    else
                        emit qmenu->aboutToHide();
#endif
                }
            }
        } else {
            handled_event = false;
        }
        break; }
    default:
        handled_event = false;
        break;
    }
    if(!handled_event) //let the event go through
        return CallNextEventHandler(er, event);
    return noErr; //we eat the event
}
static EventHandlerRef mac_menu_event_handler = 0;
static EventHandlerUPP mac_menu_eventUPP = 0;
static void qt_mac_cleanup_menu_event()
{
    if(mac_menu_event_handler) {
        RemoveEventHandler(mac_menu_event_handler);
        mac_menu_event_handler = 0;
    }
    if(mac_menu_eventUPP) {
        DisposeEventHandlerUPP(mac_menu_eventUPP);
        mac_menu_eventUPP = 0;
    }
}
static inline void qt_mac_create_menu_event_handler()
{
    if(!mac_menu_event_handler) {
        mac_menu_eventUPP = NewEventHandlerUPP(qt_mac_menu_event);
        InstallEventHandler(GetApplicationEventTarget(), mac_menu_eventUPP,
                            GetEventTypeCount(menu_events), menu_events, 0,
                            &mac_menu_event_handler);
        qAddPostRoutine(qt_mac_cleanup_menu_event);
    }
}

//creation of the MenuRef
static MenuRef qt_mac_create_menu(QWidget *w)
{
    MenuRef ret = 0;
    if(CreateNewMenu(0, 0, &ret) == noErr) {
        qt_mac_create_menu_event_handler();
        SetMenuItemProperty(ret, 0, kMenuCreatorQt, kMenuPropertyQWidget, sizeof(w), &w);
    } else {
        qWarning("This really cannot happen!!");
    }
    return ret;
}


/*****************************************************************************
  QMenu bindings
 *****************************************************************************/
QMenuPrivate::QMacMenuPrivate::QMacMenuPrivate() : menu(0)
{
}

QMenuPrivate::QMacMenuPrivate::~QMacMenuPrivate()
{
    for(QList<QMacMenuAction*>::Iterator it = actionItems.begin(); it != actionItems.end(); ++it)
        delete (*it);
    if(menu)
        ReleaseMenu(menu);
}

void
QMenuPrivate::QMacMenuPrivate::addAction(QAction *a, QMacMenuAction *before)
{
    QMacMenuAction *action = new QMacMenuAction;
    action->action = a;
    action->command = qt_mac_menu_static_cmd_id++;
    action->ignore_accel = 0;
    action->merged = 0;
    action->menu = 0;
    addAction(action, before);
}

void
QMenuPrivate::QMacMenuPrivate::addAction(QMacMenuAction *action, QMacMenuAction *before)
{
    if(!action)
        return;
    int before_index = actionItems.indexOf(before);
    actionItems.insert(before_index, action);

    int index = qt_mac_menu_find_action(menu, action);
    action->menu = menu;
    /* I don't know if this is a bug or a feature, but when the action is considered a mergable action it
       will stay that way, until removed.. */
    if(!qt_mac_no_menubar_merge) {
        MenuRef merge = 0;
        GetMenuItemProperty(menu, 0, kMenuCreatorQt, kMenuPropertyMergeMenu,
                            sizeof(action->menu), 0, &merge);
        if(merge) {
            if(MenuCommand cmd = qt_mac_menu_merge_action(merge, action)) {
                action->merged = 1;
                action->menu = merge;
                action->command = cmd;
                if(qt_mac_auto_apple_menu(cmd))
                    index = 0; //no need
            }
        }
    }

    if(index == -1) {
        index = before_index;
        MenuItemAttributes attr = kMenuItemAttrAutoRepeat;
        if(before)
            InsertMenuItemTextWithCFString(action->menu, 0, before_index-1, attr, action->command);
        else
            AppendMenuItemTextWithCFString(action->menu, 0, attr, action->command, (MenuItemIndex*)&index);
        SetMenuCommandProperty(action->menu, action->command, kMenuCreatorQt, kMenuPropertyQAction,
                               sizeof(action), &action);
    } else {
        qt_mac_command_set_enabled(action->menu, action->command, !QApplicationPrivate::modalState());
        SetMenuCommandProperty(0, action->command, kMenuCreatorQt, kMenuPropertyQAction, sizeof(action), &action);
    }
    syncAction(action);
}

void
QMenuPrivate::QMacMenuPrivate::syncAction(QMacMenuAction *action)
{
    if(!action)
        return;
    const int index = qt_mac_menu_find_action(action->menu, action);
    if(index == -1)
        return;

    if(!action->action->isVisible()) {
        ChangeMenuItemAttributes(action->menu, index, kMenuItemAttrHidden, 0);
        return;
    }
    ChangeMenuItemAttributes(action->menu, index, 0, kMenuItemAttrHidden);

    if(action->action->isSeparator()) {
        for(int i = 0; i < actionItems.size(); ++i) {
            if(actionItems.at(i) == action) {
                bool hide = true;
                for(++i; i < actionItems.size(); ++i) {
                    QMacMenuAction *action = actionItems.at(i);
                    if(!action->merged && !action->action->isSeparator()) {
                        hide = false;
                        break;
                    }
                }
                if(hide)
                    ChangeMenuItemAttributes(action->menu, index, kMenuItemAttrHidden, 0);
                else
                    ChangeMenuItemAttributes(action->menu, index, 0, kMenuItemAttrHidden);
                break;
            }
        }
        ChangeMenuItemAttributes(action->menu, index, kMenuItemAttrSeparator, 0);
        return;
    }
    ChangeMenuItemAttributes(action->menu, index, 0, kMenuItemAttrSeparator);

    //find text (and accel)
    action->ignore_accel = 0;
    QString text = action->action->text();
    QKeySequence accel = action->action->shortcut();
    {
        int st = text.lastIndexOf('\t');
        if(st != -1) {
            action->ignore_accel = 1;
            accel = QKeySequence(text.right(text.length()-(st+1)));
            text.remove(st, text.length()-st);
        }
    }
    {
        QString cmd_text = qt_mac_menu_merge_text(action->command);
        if(!cmd_text.isNull()) {
            text = cmd_text;
            accel = qt_mac_menu_merge_accel(action->command);
        }
    }
    if(accel.count() > 1)
        text += " (****)"; //just to denote a multi stroke shortcut

    MenuItemDataRec data;
    memset(&data, '\0', sizeof(data));

    //text
    data.whichData |= kMenuItemDataCFString;
    QCFString cfstr(qt_mac_no_ampersands(text));
    data.cfText = cfstr;

    //enabled
    data.whichData |= kMenuItemDataEnabled;
    data.enabled = action->action->isEnabled();

    //icon
    data.whichData |= kMenuItemDataIconHandle;
    if(!action->action->icon().isNull() && !qt_mac_no_menubar_icons) {
        data.iconType = kMenuIconRefType;
        data.iconHandle = (Handle)qt_mac_create_iconref(action->action->icon().pixmap(22, QIcon::Normal));
    } else {
        data.iconType = kMenuNoIcon;
    }

    if(action->action->font().resolve()) { //font
        if(action->action->font().bold())
            data.style |= bold;
        if(action->action->font().underline())
            data.style |= underline;
        if(action->action->font().italic())
            data.style |= italic;
        if(data.style)
            data.whichData |= kMenuItemDataStyle;
        data.whichData |= kMenuItemDataFontID;
        ATSUFONDtoFontID(FMGetFontFamilyFromATSFontFamilyRef(
                             (ATSFontFamilyRef)action->action->font().handle()),
                         0, (ATSUFontID*)&data.fontID);
    }

    if(action->action->menu()) { //submenu
        data.whichData |= kMenuItemDataSubmenuHandle;
        data.submenuHandle = action->action->menu()->macMenu();
        QWidget *caused = 0;
        GetMenuItemProperty(action->menu, 0, kMenuCreatorQt, kMenuPropertyQWidget, sizeof(caused), 0, &caused);
        SetMenuItemProperty(data.submenuHandle, 0, kMenuCreatorQt, kMenuPropertyCausedQWidget, sizeof(caused), &caused);
    } else { //respect some other items
        //shortcuts
        if(accel.isEmpty()) {
            data.whichData |= kMenuItemDataCmdKeyModifiers;
            data.whichData |= kMenuItemDataCmdKeyGlyph;
        } else {
            int accel_key = accel[0];
            data.whichData |= kMenuItemDataCmdKeyModifiers;
            if((accel_key & Qt::CTRL) != Qt::CTRL)
                data.cmdKeyModifiers |= kMenuNoCommandModifier;
            if((accel_key & Qt::META) == Qt::META)
                data.cmdKeyModifiers |= kMenuControlModifier;
            if((accel_key & Qt::ALT) == Qt::ALT)
                data.cmdKeyModifiers |= kMenuOptionModifier;
            if((accel_key & Qt::SHIFT) == Qt::SHIFT)
                data.cmdKeyModifiers |= kMenuShiftModifier;

            accel_key &= ~(Qt::MODIFIER_MASK | Qt::UNICODE_ACCEL);
            if(accel_key == Qt::Key_Return)
                data.cmdKeyGlyph = kMenuReturnGlyph;
            else if(accel_key == Qt::Key_Enter)
                data.cmdKeyGlyph = kMenuEnterGlyph;
            else if(accel_key == Qt::Key_Tab)
                data.cmdKeyGlyph = kMenuTabRightGlyph;
            else if(accel_key == Qt::Key_Backspace)
                data.cmdKeyGlyph = kMenuDeleteLeftGlyph;
            else if(accel_key == Qt::Key_Delete)
                data.cmdKeyGlyph = kMenuDeleteRightGlyph;
            else if(accel_key == Qt::Key_Escape)
                data.cmdKeyGlyph = kMenuEscapeGlyph;
            else if(accel_key == Qt::Key_PageUp)
                data.cmdKeyGlyph = kMenuPageUpGlyph;
            else if(accel_key == Qt::Key_PageDown)
                data.cmdKeyGlyph = kMenuPageDownGlyph;
            else if(accel_key == Qt::Key_Up)
                data.cmdKeyGlyph = kMenuUpArrowGlyph;
            else if(accel_key == Qt::Key_Down)
                data.cmdKeyGlyph = kMenuDownArrowGlyph;
            else if(accel_key == Qt::Key_Left)
                data.cmdKeyGlyph = kMenuLeftArrowGlyph;
            else if(accel_key == Qt::Key_Right)
                data.cmdKeyGlyph = kMenuRightArrowGlyph;
            else if(accel_key == Qt::Key_CapsLock)
                data.cmdKeyGlyph = kMenuCapsLockGlyph;
            else if(accel_key >= Qt::Key_F1 && accel_key <= Qt::Key_F15)
                data.cmdKeyGlyph = (accel_key - Qt::Key_F1) + kMenuF1Glyph;
            else if(accel_key == Qt::Key_Home)
                data.cmdKeyGlyph = kMenuNorthwestArrowGlyph;
            else if(accel_key == Qt::Key_End)
                data.cmdKeyGlyph = kMenuSoutheastArrowGlyph;
            if(data.cmdKeyGlyph) {
                data.whichData |= kMenuItemDataCmdKeyGlyph;
            } else {
                data.whichData |= kMenuItemDataCmdKey;
                data.cmdKey = (UniChar)accel_key;
            }
        }
    }

    //mark glyph
    data.whichData |= kMenuItemDataMark;
    if(action->action->isChecked()) {
#if 0
        if(action->action->actionGroup() &&
           action->action->actionGroup()->isExclusive())
            data.mark = diamondMark;
        else
#endif
            data.mark = checkMark;
    } else {
        data.mark = noMark;
    }

    //actually set it
    SetMenuItemData(action->menu, action->command, true, &data);
}

void
QMenuPrivate::QMacMenuPrivate::removeAction(QMacMenuAction *action)
{
    if(!action)
        return;
    if(action->command == kHICommandQuit || action->command == kHICommandPreferences)
        qt_mac_command_set_enabled(action->menu, action->command, false);
    else
        DeleteMenuItem(action->menu, qt_mac_menu_find_action(action->menu, action));
    actionItems.removeAll(action);
}

MenuRef
QMenuPrivate::macMenu(MenuRef merge)
{
    Q_Q(QMenu);
    if(mac_menu && mac_menu->menu)
        return mac_menu->menu;
    if(!mac_menu)
        mac_menu = new QMacMenuPrivate;
    mac_menu->menu = qt_mac_create_menu(q);
    if(merge)
        SetMenuItemProperty(mac_menu->menu, 0, kMenuCreatorQt, kMenuPropertyMergeMenu, sizeof(merge), &merge);

    QList<QAction*> items = q->actions();
    for(int i = 0; i < items.count(); i++)
        mac_menu->addAction(items[i]);
    return mac_menu->menu;
}

/*!
    \internal

    This function will return the MenuRef used to create the native menubar
    bindings. This MenuRef may be referenced in the Menu Manager, or this
    can be used to create native dock menus.

    \warning This function is not portable.

    \sa QMenuBar::macMenu()
*/
MenuRef QMenu::macMenu(MenuRef merge) { return d_func()->macMenu(merge); }

/*****************************************************************************
  QMenuBar bindings
 *****************************************************************************/
typedef QHash<QWidget *, QMenuBar *> MenuBarHash;
Q_GLOBAL_STATIC(MenuBarHash, menubars)
static QMenuBar *fallback = 0;

QMenuBarPrivate::QMacMenuBarPrivate::QMacMenuBarPrivate() : menu(0), apple_menu(0)
{
}

QMenuBarPrivate::QMacMenuBarPrivate::~QMacMenuBarPrivate()
{
    for(QList<QMacMenuAction*>::Iterator it = actionItems.begin(); it != actionItems.end(); ++it)
        delete (*it);
    if(apple_menu)
        ReleaseMenu(apple_menu);
    if(menu)
        ReleaseMenu(menu);
}

void
QMenuBarPrivate::QMacMenuBarPrivate::addAction(QAction *a, QMacMenuAction *before)
{
    if(a->isSeparator())
        return;
    QMacMenuAction *action = new QMacMenuAction;
    action->action = a;
    action->ignore_accel = 1;
    action->merged = 0;
    action->menu = 0;
    action->command = qt_mac_menu_static_cmd_id++;
    addAction(action, before);
}

void
QMenuBarPrivate::QMacMenuBarPrivate::addAction(QMacMenuAction *action, QMacMenuAction *before)
{
    if(!action || !menu)
        return;
    if(!qt_mac_no_menubar_merge) {
        if(!apple_menu) { //handle the apple menu
            QWidget *widget = 0;
            GetMenuItemProperty(menu, 0, kMenuCreatorQt, kMenuPropertyQWidget, sizeof(widget), 0,
                                &widget);

            //create
            MenuItemIndex index;
            apple_menu = qt_mac_create_menu(widget);
            AppendMenuItemTextWithCFString(menu, 0, 0, 0, &index);

            // set it up
            SetMenuTitleWithCFString(apple_menu, QCFString(QString(QChar(0x14))));
            SetMenuItemHierarchicalMenu(menu, index, apple_menu);
            SetMenuItemProperty(apple_menu, 0, kMenuCreatorQt, kMenuPropertyQWidget, sizeof(widget),
                                &widget);
        }
    }

    const int before_index = actionItems.indexOf(before);
    actionItems.insert(before_index, action);

    action->menu = menu;
    MenuItemIndex index = before_index;
    if(before)
        InsertMenuItemTextWithCFString(action->menu, 0, before_index-1, 0, action->command);
    else
        AppendMenuItemTextWithCFString(action->menu, 0, 0, action->command, &index);
    SetMenuItemProperty(action->menu, index, kMenuCreatorQt, kMenuPropertyQAction, sizeof(action),
                        &action);
    syncAction(action);
}

void
QMenuBarPrivate::QMacMenuBarPrivate::syncAction(QMacMenuAction *action)
{
    if(!action || !menu)
        return;
    const int index = qt_mac_menu_find_action(action->menu, action);

    MenuRef submenu = 0;
    bool release_submenu = false;
    if(action->action->menu()) {
        if((submenu = action->action->menu()->macMenu(apple_menu))) {
            QWidget *caused = 0;
            GetMenuItemProperty(action->menu, 0, kMenuCreatorQt, kMenuPropertyQWidget, sizeof(caused), 0, &caused);
            SetMenuItemProperty(submenu, 0, kMenuCreatorQt, kMenuPropertyCausedQWidget, sizeof(caused), &caused);
        }
    } else if(!submenu) {
        release_submenu = true;
        CreateNewMenu(0, 0, &submenu);
    }
    if(submenu) {
        SetMenuItemHierarchicalMenu(action->menu, index, submenu);
        SetMenuTitleWithCFString(submenu, QCFString(qt_mac_no_ampersands(action->action->text())));
        bool visible = action->action->isVisible();
        if(visible && action->action->text() == QString(QChar(0x14)))
            visible = false;
        if(visible && action->action->menu() && !action->action->menu()->actions().isEmpty() &&
           !qt_mac_CountMenuItems(action->action->menu()->macMenu(apple_menu)) &&
           !qt_mac_watchingAboutToShow(action->action->menu()))
            visible = false;
        if(visible)
            ChangeMenuAttributes(submenu, 0, kMenuAttrHidden);
        else
            ChangeMenuAttributes(submenu, kMenuAttrHidden, 0);

        if(release_submenu) //no pointers to it
            ReleaseMenu(submenu);
    } else {
        qWarning("QMenu: No MenuRef created for popup menu!");
    }
}

void
QMenuBarPrivate::QMacMenuBarPrivate::removeAction(QMacMenuAction *action)
{
    if(!action || !menu)
        return;
    DeleteMenuItem(action->menu, qt_mac_menu_find_action(action->menu, action));
    actionItems.removeAll(action);
}

void
QMenuBarPrivate::macCreateMenuBar(QWidget *parent)
{
    Q_Q(QMenuBar);
    if(!qgetenv("QT_MAC_NO_NATIVE_MENUBAR").isNull())
        qt_mac_no_native_menubar = true;
    if(!qt_mac_no_native_menubar) {
        extern void qt_event_request_menubarupdate(); //qapplication_mac.cpp
        qt_event_request_menubarupdate();
        if(!parent && !fallback) {
            fallback = q;
            mac_menubar = new QMacMenuBarPrivate;
        } else if (parent && parent->isWindow()) {
            menubars()->insert(q->window(), q);
            mac_menubar = new QMacMenuBarPrivate;
        }
    }
}

void QMenuBarPrivate::macDestroyMenuBar()
{
    Q_Q(QMenuBar);
    if (fallback == q)
        fallback = 0;
    delete mac_menubar;
    QWidget *tlw = q->window();
    menubars()->remove(tlw);
    mac_menubar = 0;
}

MenuRef QMenuBarPrivate::macMenu()
{
    Q_Q(QMenuBar);
    if(!mac_menubar) {
        return 0;
    } else if(!mac_menubar->menu) {
        ProcessSerialNumber mine, front;
        if(GetCurrentProcess(&mine) == noErr && GetFrontProcess(&front) == noErr) {
            mac_menubar->menu = qt_mac_create_menu(q);

            QList<QAction*> items = q->actions();
            for(int i = 0; i < items.count(); i++)
                mac_menubar->addAction(items[i]);
        }
    }
    return mac_menubar->menu;
}

/*!
    \internal

    This function will return the MenuRef used to create the native menubar
    bindings. This MenuRef is then set as the root menu for the Menu
    Manager.

    \warning This function is not portable.

    \sa QMenu::macMenu()
*/
MenuRef QMenuBar::macMenu() {  return d_func()->macMenu(); }

/*!
  \internal

  This function will update the current menubar and set it as the
  active menubar in the Menu Manager.

  \warning This function is not portable.

  \sa QMenu::macMenu(), QMenuBar::macMenu()
*/
bool QMenuBar::macUpdateMenuBar()
{
    if(qt_mac_no_native_menubar) //nothing to be done..
        return true;

    QMenuBar *mb = 0;
    //find a menubar
    QWidget *w = qApp->activeWindow();
    if(!w) {
        WindowClass c;
        for(WindowPtr wp = FrontWindow(); wp; wp = GetNextWindow(wp)) {
            if(GetWindowClass(wp, &c))
                break;
            if(c == kOverlayWindowClass)
                continue;
            w = QWidget::find((WId)wp);
            break;
        }
    }
    if(!w) {
        QWidgetList tlws = QApplication::topLevelWidgets();
        for(int i = 0; i < tlws.size(); ++i) {
            QWidget *tlw = tlws.at(i);
            if((tlw->isVisible() && tlw->windowType() != Qt::Tool &&
                tlw->windowType() != Qt::Popup)) {
                w = tlw;
                break;
            }
        }
    }
    if(w) {
        mb = menubars()->value(w);
#ifndef QT_NO_MAINWINDOW
        QDockWidget *dw = qobject_cast<QDockWidget *>(w);
        if(!mb && dw) {
            QMainWindow *mw = qobject_cast<QMainWindow *>(dw->parentWidget());
            if (mw && (mb = menubars()->value(mw)))
                w = mw;
        }
#endif
        while(w && !mb)
            mb = menubars()->value((w = w->parentWidget()));
    }
    if(!mb && !(!w || (!(w->windowType() == Qt::Tool) && !(w->windowType() == Qt::Popup))))
        mb = fallback;
    //now set it
    bool ret = false;
    if(mb) {
        if(MenuRef menu = mb->macMenu()) {
            SetRootMenu(menu);
            if(mb != menubars()->value(qApp->activeModalWidget()))
                qt_mac_set_modal_state(menu, QApplicationPrivate::modalState());
        }
        qt_mac_current_menubar.qmenubar = mb;
        qt_mac_current_menubar.modal = QApplicationPrivate::modalState();
        ret = true;
    } else if(qt_mac_current_menubar.qmenubar) {
        const bool modal = QApplicationPrivate::modalState();
        if(modal != qt_mac_current_menubar.modal) {
            if(MenuRef menu = qt_mac_current_menubar.qmenubar->macMenu()) {
                SetRootMenu(menu);
                if(qt_mac_current_menubar.qmenubar != menubars()->value(qApp->activeModalWidget()))
                    qt_mac_set_modal_state(menu, QApplicationPrivate::modalState());
            }
            qt_mac_current_menubar.modal = modal;
        }
    }
    return ret;
}

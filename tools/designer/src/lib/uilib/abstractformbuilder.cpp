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

#include "abstractformbuilder.h"
#include "ui4.h"

#include <QtCore/QVariant>
#include <QtCore/QMetaProperty>
#include <QtCore/QDateTime>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>

#include <QtGui/QAction>
#include <QtGui/QActionGroup>
#include <QtGui/QComboBox>
#include <QtGui/QGridLayout>
#include <QtGui/QIcon>
#include <QtGui/QListWidget>
#include <QtGui/QMainWindow>
#include <QtGui/QPixmap>
#include <QtGui/QShortcut>
#include <QtGui/QStatusBar>
#include <QtGui/QTreeWidget>
#include <QtGui/QWidget>
#include <QtGui/QSplitter>

#include <QtXml/QDomDocument>

// containers
#include <QtGui/QToolBox>
#include <QtGui/QStackedWidget>
#include <QtGui/QTabWidget>
#include <QtGui/QToolBar>
#include <QtGui/QMenuBar>

#include <QtCore/qdebug.h>

#include <limits.h>

#ifdef QFORMINTERNAL_NAMESPACE
using namespace QFormInternal;
#endif

class QFriendlyLayout: public QLayout
{
public:
    inline QFriendlyLayout() { Q_ASSERT(0); }

#ifdef QFORMINTERNAL_NAMESPACE
    friend class QFormInternal::QAbstractFormBuilder;
#else
    friend class QAbstractFormBuilder;
#endif
};

class QAbstractFormBuilderGadget: public QWidget
{
    Q_OBJECT
    Q_PROPERTY(Qt::Orientation orientation READ fakeOrientation)
    Q_PROPERTY(QSizePolicy::Policy sizeType READ fakeSizeType)
    Q_PROPERTY(QPalette::ColorRole colorRole READ fakeColorRole)
    Q_PROPERTY(QPalette::ColorGroup colorGroup READ fakeColorGroup)
public:
    QAbstractFormBuilderGadget() { Q_ASSERT(0); }

    Qt::Orientation fakeOrientation() const     { Q_ASSERT(0); return Qt::Horizontal; }
    QSizePolicy::Policy fakeSizeType() const    { Q_ASSERT(0); return QSizePolicy::Expanding; }
    QPalette::ColorGroup fakeColorGroup() const { Q_ASSERT(0); return static_cast<QPalette::ColorGroup>(0); }
    QPalette::ColorRole fakeColorRole() const   { Q_ASSERT(0); return static_cast<QPalette::ColorRole>(0); }
};

#ifdef Q_WS_MAC
static struct {
    int key;
    const char* name;
} keyname[] = {
    { Qt::Key_Space,        QT_TRANSLATE_NOOP("QShortcut", "Space") },
    { Qt::Key_Escape,       QT_TRANSLATE_NOOP("QShortcut", "Esc") },
    { Qt::Key_Tab,          QT_TRANSLATE_NOOP("QShortcut", "Tab") },
    { Qt::Key_Backtab,      QT_TRANSLATE_NOOP("QShortcut", "Backtab") },
    { Qt::Key_Backspace,    QT_TRANSLATE_NOOP("QShortcut", "Backspace") },
    { Qt::Key_Return,       QT_TRANSLATE_NOOP("QShortcut", "Return") },
    { Qt::Key_Enter,        QT_TRANSLATE_NOOP("QShortcut", "Enter") },
    { Qt::Key_Insert,       QT_TRANSLATE_NOOP("QShortcut", "Ins") },
    { Qt::Key_Delete,       QT_TRANSLATE_NOOP("QShortcut", "Del") },
    { Qt::Key_Pause,        QT_TRANSLATE_NOOP("QShortcut", "Pause") },
    { Qt::Key_Print,        QT_TRANSLATE_NOOP("QShortcut", "Print") },
    { Qt::Key_SysReq,       QT_TRANSLATE_NOOP("QShortcut", "SysReq") },
    { Qt::Key_Home,         QT_TRANSLATE_NOOP("QShortcut", "Home") },
    { Qt::Key_End,          QT_TRANSLATE_NOOP("QShortcut", "End") },
    { Qt::Key_Left,         QT_TRANSLATE_NOOP("QShortcut", "Left") },
    { Qt::Key_Up,           QT_TRANSLATE_NOOP("QShortcut", "Up") },
    { Qt::Key_Right,        QT_TRANSLATE_NOOP("QShortcut", "Right") },
    { Qt::Key_Down,         QT_TRANSLATE_NOOP("QShortcut", "Down") },
    { Qt::Key_PageUp,       QT_TRANSLATE_NOOP("QShortcut", "PgUp") },
    { Qt::Key_PageDown,     QT_TRANSLATE_NOOP("QShortcut", "PgDown") },
    { Qt::Key_CapsLock,     QT_TRANSLATE_NOOP("QShortcut", "CapsLock") },
    { Qt::Key_NumLock,      QT_TRANSLATE_NOOP("QShortcut", "NumLock") },
    { Qt::Key_ScrollLock,   QT_TRANSLATE_NOOP("QShortcut", "ScrollLock") },
    { Qt::Key_Menu,         QT_TRANSLATE_NOOP("QShortcut", "Menu") },
    { Qt::Key_Help,         QT_TRANSLATE_NOOP("QShortcut", "Help") },

    // Multimedia keys
    { Qt::Key_Back,         QT_TRANSLATE_NOOP("QShortcut", "Back") },
    { Qt::Key_Forward,      QT_TRANSLATE_NOOP("QShortcut", "Forward") },
    { Qt::Key_Stop,         QT_TRANSLATE_NOOP("QShortcut", "Stop") },
    { Qt::Key_Refresh,      QT_TRANSLATE_NOOP("QShortcut", "Refresh") },
    { Qt::Key_VolumeDown,   QT_TRANSLATE_NOOP("QShortcut", "Volume Down") },
    { Qt::Key_VolumeMute,   QT_TRANSLATE_NOOP("QShortcut", "Volume Mute") },
    { Qt::Key_VolumeUp,     QT_TRANSLATE_NOOP("QShortcut", "Volume Up") },
    { Qt::Key_BassBoost,    QT_TRANSLATE_NOOP("QShortcut", "Bass Boost") },
    { Qt::Key_BassUp,       QT_TRANSLATE_NOOP("QShortcut", "Bass Up") },
    { Qt::Key_BassDown,     QT_TRANSLATE_NOOP("QShortcut", "Bass Down") },
    { Qt::Key_TrebleUp,     QT_TRANSLATE_NOOP("QShortcut", "Treble Up") },
    { Qt::Key_TrebleDown,   QT_TRANSLATE_NOOP("QShortcut", "Treble Down") },
    { Qt::Key_MediaPlay,    QT_TRANSLATE_NOOP("QShortcut", "Media Play") },
    { Qt::Key_MediaStop,    QT_TRANSLATE_NOOP("QShortcut", "Media Stop") },
    { Qt::Key_MediaPrevious,QT_TRANSLATE_NOOP("QShortcut", "Media Previous") },
    { Qt::Key_MediaNext,    QT_TRANSLATE_NOOP("QShortcut", "Media Next") },
    { Qt::Key_MediaRecord,  QT_TRANSLATE_NOOP("QShortcut", "Media Record") },
    { Qt::Key_HomePage,     QT_TRANSLATE_NOOP("QShortcut", "Home") },
    { Qt::Key_Favorites,    QT_TRANSLATE_NOOP("QShortcut", "Favorites") },
    { Qt::Key_Search,       QT_TRANSLATE_NOOP("QShortcut", "Search") },
    { Qt::Key_Standby,      QT_TRANSLATE_NOOP("QShortcut", "Standby") },
    { Qt::Key_OpenUrl,      QT_TRANSLATE_NOOP("QShortcut", "Open URL") },
    { Qt::Key_LaunchMail,   QT_TRANSLATE_NOOP("QShortcut", "Launch Mail") },
    { Qt::Key_LaunchMedia,  QT_TRANSLATE_NOOP("QShortcut", "Launch Media") },
    { Qt::Key_Launch0,      QT_TRANSLATE_NOOP("QShortcut", "Launch (0)") },
    { Qt::Key_Launch1,      QT_TRANSLATE_NOOP("QShortcut", "Launch (1)") },
    { Qt::Key_Launch2,      QT_TRANSLATE_NOOP("QShortcut", "Launch (2)") },
    { Qt::Key_Launch3,      QT_TRANSLATE_NOOP("QShortcut", "Launch (3)") },
    { Qt::Key_Launch4,      QT_TRANSLATE_NOOP("QShortcut", "Launch (4)") },
    { Qt::Key_Launch5,      QT_TRANSLATE_NOOP("QShortcut", "Launch (5)") },
    { Qt::Key_Launch6,      QT_TRANSLATE_NOOP("QShortcut", "Launch (6)") },
    { Qt::Key_Launch7,      QT_TRANSLATE_NOOP("QShortcut", "Launch (7)") },
    { Qt::Key_Launch8,      QT_TRANSLATE_NOOP("QShortcut", "Launch (8)") },
    { Qt::Key_Launch9,      QT_TRANSLATE_NOOP("QShortcut", "Launch (9)") },
    { Qt::Key_LaunchA,      QT_TRANSLATE_NOOP("QShortcut", "Launch (A)") },
    { Qt::Key_LaunchB,      QT_TRANSLATE_NOOP("QShortcut", "Launch (B)") },
    { Qt::Key_LaunchC,      QT_TRANSLATE_NOOP("QShortcut", "Launch (C)") },
    { Qt::Key_LaunchD,      QT_TRANSLATE_NOOP("QShortcut", "Launch (D)") },
    { Qt::Key_LaunchE,      QT_TRANSLATE_NOOP("QShortcut", "Launch (E)") },
    { Qt::Key_LaunchF,      QT_TRANSLATE_NOOP("QShortcut", "Launch (F)") },

    // --------------------------------------------------------------
    // More consistent namings
    { Qt::Key_Print,        QT_TRANSLATE_NOOP("QShortcut", "Print Screen") },
    { Qt::Key_PageUp,       QT_TRANSLATE_NOOP("QShortcut", "Page Up") },
    { Qt::Key_PageDown,     QT_TRANSLATE_NOOP("QShortcut", "Page Down") },
    { Qt::Key_CapsLock,     QT_TRANSLATE_NOOP("QShortcut", "Caps Lock") },
    { Qt::Key_NumLock,      QT_TRANSLATE_NOOP("QShortcut", "Num Lock") },
    { Qt::Key_NumLock,      QT_TRANSLATE_NOOP("QShortcut", "Number Lock") },
    { Qt::Key_ScrollLock,   QT_TRANSLATE_NOOP("QShortcut", "Scroll Lock") },
    { Qt::Key_Insert,       QT_TRANSLATE_NOOP("QShortcut", "Insert") },
    { Qt::Key_Delete,       QT_TRANSLATE_NOOP("QShortcut", "Delete") },
    { Qt::Key_Escape,       QT_TRANSLATE_NOOP("QShortcut", "Escape") },
    { Qt::Key_SysReq,       QT_TRANSLATE_NOOP("QShortcut", "System Request") },

    { 0, 0 }
};

static QString qkeysequence_encodeString(int key)
{
    QString s;
    if ((key & Qt::META) == Qt::META)
        s += QShortcut::tr("Meta");
    if ((key & Qt::CTRL) == Qt::CTRL) {
        if (!s.isEmpty())
            s += QShortcut::tr("+");
        s += QShortcut::tr("Ctrl");
    }
    if ((key & Qt::ALT) == Qt::ALT) {
        if (!s.isEmpty())
            s += QShortcut::tr("+");
        s += QShortcut::tr("Alt");
    }
    if ((key & Qt::SHIFT) == Qt::SHIFT) {
        if (!s.isEmpty())
            s += QShortcut::tr("+");
        s += QShortcut::tr("Shift");
    }


    key &= ~(Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier);
    QString p;

    if (key && key < Qt::Key_Escape) {
        if (key < 0x10000) {
            p = QChar(key & 0xffff).toUpper();
        } else {
            p = QChar((key-0x10000)/0x400+0xd800);
            p += QChar((key-0x10000)%400+0xdc00);
        }
    } else if (key >= Qt::Key_F1 && key <= Qt::Key_F35) {
        p = QShortcut::tr("F%1").arg(key - Qt::Key_F1 + 1);
    } else if (key > Qt::Key_Space && key <= Qt::Key_AsciiTilde) {
        p.sprintf("%c", key);
    } else if (key) {
        int i=0;
        while (keyname[i].name) {
            if (key == keyname[i].key) {
                p = QShortcut::tr(keyname[i].name);
                break;
            }
            ++i;
        }
        // If we can't find the actual translatable keyname,
        // fall back on the unicode representation of it...
        // Or else characters like Qt::Key_aring may not get displayed
        // (Really depends on you locale)
        if (!keyname[i].name) {
            if (key < 0x10000) {
                p = QChar(key & 0xffff).toUpper();
            } else {
                p = QChar((key-0x10000)/0x400+0xd800);
                p += QChar((key-0x10000)%400+0xdc00);
            }
        }
    }

    if (!s.isEmpty())
        s += QShortcut::tr("+");

    s += p;
    return s;
}

static QString platformNeutralKeySequence(const QKeySequence &ks)
{
    uint k;
    QString str;
    for (k = 0; k < ks.count(); ++k) {
        str += qkeysequence_encodeString(ks[k]) + ", ";
    }
    str.truncate(str.size() - 2);
    return str;
}
#endif


/*!
    \class QAbstractFormBuilder

    \brief The QAbstractFormBuilder class provides a default
    implementation for classes that create user interfaces at
    run-time.

    \inmodule QtDesigner

    QAbstractFormBuilder provides a standard interface and a default
    implementation for constructing forms from user interface
    files. It is not intended to be instantiated directly. Use the
    QFormBuilder class to create user interfaces from \c{.ui} files at
    run-time. For example:

    \code
        MyForm::MyForm(QWidget *parent)
            : QWidget(parent)
        {
            QFormBuilder builder;
            QFile file(":/forms/myWidget.ui");
            file.open(QFile::ReadOnly);
            QWidget *myWidget = builder.load(&file, this);
            file.close();

            QVBoxLayout *layout = new QVBoxLayout;
            layout->addWidget(myWidget);
            setLayout(layout);
        }
    \endcode

    To override certain aspects of the form builder's behavior,
    subclass QAbstractFormBuilder and reimplement the relevant virtual
    functions:

    \list
    \o load() handles reading of \c{.ui} format files from arbitrary
       QIODevices, and construction of widgets from the XML data
       they contain.
    \o save() handles saving of widget details in \c{.ui} format to
       arbitrary QIODevices.
    \o workingDirectory() and setWorkingDirectory() control the
       directory in which forms are held. The form builder looks for
       other resources on paths relative to this directory.
    \endlist

    For a complete example using QFormBuilder, see the \l
    {designer/calculatorbuilder}{Calculator Builder example} which
    shows how to create a user interface from a \QD form at runtime,
    using the QFormBuilder class.

    \sa QFormBuilder
*/

/*!
    Constructs a new form builder.*/
QAbstractFormBuilder::QAbstractFormBuilder()
{
    m_defaultMargin = INT_MIN;
    m_defaultSpacing = INT_MIN;
}

/*!
    Destroys the form builder.*/
QAbstractFormBuilder::~QAbstractFormBuilder()
{
}

/*!
    \fn QWidget *QAbstractFormBuilder::load(QIODevice *device, QWidget *parent)

    Loads an XML representation of a widget from the given \a device,
    and constructs a new widget with the specified \a parent.

    \sa save()
*/
QWidget *QAbstractFormBuilder::load(QIODevice *dev, QWidget *parentWidget)
{
    QDomDocument doc;
    if (!doc.setContent(dev))
        return 0;

    QDomElement root = doc.firstChild().toElement();
    DomUI ui;
    ui.read(root); /// ### check the result

    return create(&ui, parentWidget);
}

/*!
    \internal
*/
QWidget *QAbstractFormBuilder::create(DomUI *ui, QWidget *parentWidget)
{
    if (DomLayoutDefault *def = ui->elementLayoutDefault()) {
        m_defaultMargin = def->hasAttributeMargin() ? def->attributeMargin() : INT_MIN;
        m_defaultSpacing = def->hasAttributeSpacing() ? def->attributeSpacing() : INT_MIN;
    }

    DomWidget *ui_widget = ui->elementWidget();
    if (!ui_widget)
        return 0;

    createCustomWidgets(ui->elementCustomWidgets());

    if (QWidget *widget = create(ui_widget, parentWidget)) {
        createConnections(ui->elementConnections(), widget);
        createResources(ui->elementResources());
        applyTabStops(widget, ui->elementTabStops());
        reset();

        return widget;
    }

    return 0;
}

/*!
    \internal
*/
QWidget *QAbstractFormBuilder::create(DomWidget *ui_widget, QWidget *parentWidget)
{
    QWidget *w = createWidget(ui_widget->attributeClass(), parentWidget, ui_widget->attributeName());
    if (!w)
        return 0;

    applyProperties(w, ui_widget->elementProperty());

    foreach (DomAction *ui_action, ui_widget->elementAction()) {
        QAction *child_action = create(ui_action, w);
        Q_UNUSED( child_action );
    }

    foreach (DomActionGroup *ui_action_group, ui_widget->elementActionGroup()) {
        QActionGroup *child_action_group = create(ui_action_group, w);
        Q_UNUSED( child_action_group );
    }

    foreach (DomWidget *ui_child, ui_widget->elementWidget()) {
        QWidget *child_w = create(ui_child, w);
        Q_UNUSED( child_w );
    }

    foreach (DomLayout *ui_lay, ui_widget->elementLayout()) {
        QLayout *child_lay = create(ui_lay, 0, w);
        Q_UNUSED( child_lay );
    }

    foreach (DomActionRef *ui_action_ref, ui_widget->elementAddAction()) {
        QString name = ui_action_ref->attributeName();
        if (name == QLatin1String("separator")) {
            QAction *sep = new QAction(w);
            sep->setSeparator(true);
            w->addAction(sep);
            addMenuAction(sep);
        } else if (QAction *a = m_actions.value(name)) {
            w->addAction(a);
        } else if (QActionGroup *g = m_actionGroups.value(name)) {
            foreach (QAction *a, g->actions()) {
                w->addAction(a);
            }
        } else if (QMenu *menu = qFindChild<QMenu*>(w, name)) {
            QMenu *parentMenu = qobject_cast<QMenu*>(w);
            QMenuBar *parentMenuBar = qobject_cast<QMenuBar*>(w);

            menu->setParent(w, Qt::Popup);

            QAction *menuAction = 0;

            if (parentMenuBar)
                menuAction = parentMenuBar->addMenu(menu);
            else if (parentMenu)
                menuAction = parentMenu->addMenu(menu);

            if (menuAction) {
                menuAction->setObjectName(menu->objectName() + QLatin1String("Action"));
                addMenuAction(menuAction);
            }
        }
    }

    loadExtraInfo(ui_widget, w, parentWidget);
    addItem(ui_widget, w, parentWidget);

    return w;
}

/*!
    \internal
*/
QAction *QAbstractFormBuilder::create(DomAction *ui_action, QObject *parent)
{
    QAction *a = createAction(parent, ui_action->attributeName());
    if (!a)
        return 0;

    applyProperties(a, ui_action->elementProperty());
    return a;
}

/*!
    \internal
*/
QActionGroup *QAbstractFormBuilder::create(DomActionGroup *ui_action_group, QObject *parent)
{
    QActionGroup *a = createActionGroup(parent, ui_action_group->attributeName());
    if (!a)
        return 0;

    applyProperties(a, ui_action_group->elementProperty());

    foreach (DomAction *ui_action, ui_action_group->elementAction()) {
        QAction *child_action = create(ui_action, a);
        Q_UNUSED( child_action );
    }

    foreach (DomActionGroup *g, ui_action_group->elementActionGroup()) {
        QActionGroup *child_action_group = create(g, parent);
        Q_UNUSED( child_action_group );
    }

    return a;
}

/*!
    \internal
*/
bool QAbstractFormBuilder::addItem(DomWidget *ui_widget, QWidget *widget, QWidget *parentWidget)
{
    QHash<QString, DomProperty*> attributes = propertyMap(ui_widget->elementAttribute());

    QString title = QLatin1String("Page");
    if (DomProperty *ptitle = attributes.value(QLatin1String("title"))) {
        title = toString(ptitle->elementString());
    }

    QString label = QLatin1String(QLatin1String("Page"));
    if (DomProperty *plabel = attributes.value(QLatin1String("label"))) {
        label = toString(plabel->elementString());
    }

    // apply the toolbar's attributes
    if (QToolBar *toolBar = qobject_cast<QToolBar*>(widget)) {
        if (QMainWindow *mw = qobject_cast<QMainWindow*>(parentWidget)) {
            if (DomProperty *attr = attributes.value(QLatin1String("toolBarArea"))) {
                Qt::ToolBarArea area = static_cast<Qt::ToolBarArea>(attr->elementNumber());
                mw->addToolBar(area, toolBar);
            } else {
                mw->addToolBar(toolBar);
            }
        }
    }

    if (QTabWidget *tabWidget = qobject_cast<QTabWidget*>(parentWidget)) {
        widget->setParent(0);

        int tabIndex = tabWidget->count();
        tabWidget->addTab(widget, title);

        if (DomProperty *picon = attributes.value(QLatin1String("icon"))) {
            tabWidget->setTabIcon(tabIndex, qvariant_cast<QIcon>(toVariant(0, picon)));
        }

        if (DomProperty *ptoolTip = attributes.value(QLatin1String("toolTip"))) {
            tabWidget->setTabToolTip(tabIndex, toString(ptoolTip->elementString()));
        }

        return true;
    }

    if (QStackedWidget *stackedWidget = qobject_cast<QStackedWidget*>(parentWidget)) {
        stackedWidget->addWidget(widget);
        return true;
    } else if (QToolBox *toolBox = qobject_cast<QToolBox*>(parentWidget)) {
        toolBox->addItem(widget, label);
        return true;
    }

    return false;
}

/*!
    \internal
*/
void QAbstractFormBuilder::layoutInfo(DomLayout *ui_layout, QObject *parent, int *margin, int *spacing)
{
    QHash<QString, DomProperty*> properties = propertyMap(ui_layout->elementProperty());

    if (margin)
        *margin = properties.contains(QLatin1String("margin"))
            ? properties.value(QLatin1String("margin"))->elementNumber()
            : m_defaultMargin;

    if (spacing)
        *spacing = properties.contains(QLatin1String("spacing"))
            ? properties.value(QLatin1String("spacing"))->elementNumber()
            : m_defaultSpacing;

    if (margin && m_defaultMargin == INT_MIN) {
        Q_ASSERT(parent);
        if (qstrcmp(parent->metaObject()->className(), "QLayoutWidget") == 0)
            *margin = INT_MIN;
    }
}

/*!
    \internal
*/
QLayout *QAbstractFormBuilder::create(DomLayout *ui_layout, QLayout *parentLayout, QWidget *parentWidget)
{
    QObject *p = parentLayout;

    if (p == 0)
        p = parentWidget;

    Q_ASSERT(p != 0);

    bool tracking = false;

    if (p == parentWidget && parentWidget->layout()) {
        tracking = true;
        p = parentWidget->layout();
    }

    QLayout *layout = createLayout(ui_layout->attributeClass(), p, QString());

    if (layout == 0)
        return 0;

    if (tracking && layout->parent() == 0) {
        QBoxLayout *box = qobject_cast<QBoxLayout*>(parentWidget->layout());
        Q_ASSERT(box != 0); // only QBoxLayout is supported
        box->addLayout(layout);
    }

    int margin = INT_MIN, spacing = INT_MIN;
    layoutInfo(ui_layout, p, &margin, &spacing);

    if (margin != INT_MIN)
       layout->setMargin(margin);

    if (spacing != INT_MIN)
        layout->setSpacing(spacing);

    applyProperties(layout, ui_layout->elementProperty());

    foreach (DomLayoutItem *ui_item, ui_layout->elementItem()) {
        if (QLayoutItem *item = create(ui_item, layout, parentWidget)) {
            addItem(ui_item, item, layout);
        }
    }

    return layout;
}

/*!
    \internal
*/
bool QAbstractFormBuilder::addItem(DomLayoutItem *ui_item, QLayoutItem *item, QLayout *layout)
{
    if (item->widget()) {
        static_cast<QFriendlyLayout*>(layout)->addChildWidget(item->widget());
    } else if (item->layout()) {
        static_cast<QFriendlyLayout*>(layout)->addChildLayout(item->layout());
    } else if (item->spacerItem()) {
        // nothing to do
    } else {
        return false;
    }

    if (QGridLayout *grid = qobject_cast<QGridLayout*>(layout)) {
        int rowSpan = ui_item->hasAttributeRowSpan() ? ui_item->attributeRowSpan() : 1;
        int colSpan = ui_item->hasAttributeColSpan() ? ui_item->attributeColSpan() : 1;
        grid->addItem(item, ui_item->attributeRow(), ui_item->attributeColumn(),
                        rowSpan, colSpan, item->alignment());
    } else {
        layout->addItem(item);
    }

    return true;
}

/*!
    \internal
*/
QLayoutItem *QAbstractFormBuilder::create(DomLayoutItem *ui_layoutItem, QLayout *layout, QWidget *parentWidget)
{
    switch (ui_layoutItem->kind()) {
    case DomLayoutItem::Widget:
        return new QWidgetItem(create(ui_layoutItem->elementWidget(), parentWidget));

    case DomLayoutItem::Spacer: {
        QSize size(0, 0);
        QSizePolicy::Policy sizeType = QSizePolicy::Expanding;
        bool isVspacer = false;

        DomSpacer *ui_spacer = ui_layoutItem->elementSpacer();

        int e_index = QAbstractFormBuilderGadget::staticMetaObject.indexOfProperty("sizeType");
        Q_ASSERT(e_index != -1);

        QMetaEnum sizePolicy_enum = QAbstractFormBuilderGadget::staticMetaObject.property(e_index).enumerator();

        e_index = QAbstractFormBuilderGadget::staticMetaObject.indexOfProperty("orientation");
        Q_ASSERT(e_index != -1);

        QMetaEnum orientation_enum = QAbstractFormBuilderGadget::staticMetaObject.property(e_index).enumerator();

        foreach (DomProperty *p, ui_spacer->elementProperty()) {
            QVariant v = toVariant(&QAbstractFormBuilderGadget::staticMetaObject, p); // ### remove me
            if (v.isNull())
                continue;

            if (p->attributeName() == QLatin1String("sizeHint") && p->kind() == DomProperty::Size) {
                size = v.toSize();  // ###  remove me
            } else if (p->attributeName() == QLatin1String("sizeType") && p->kind() == DomProperty::Enum) {
                sizeType = static_cast<QSizePolicy::Policy>(sizePolicy_enum.keyToValue(p->elementEnum().toUtf8()));
            } else if (p->attributeName() == QLatin1String("orientation") && p->kind() == DomProperty::Enum) {
                Qt::Orientation o = static_cast<Qt::Orientation>(orientation_enum.keyToValue(p->elementEnum().toUtf8()));
                isVspacer = (o == Qt::Vertical);
            }
        }

        QSpacerItem *spacer = 0;
        if (isVspacer)
            spacer = new QSpacerItem(size.width(), size.height(), QSizePolicy::Minimum, sizeType);
        else
            spacer = new QSpacerItem(size.width(), size.height(), sizeType, QSizePolicy::Minimum);
        return spacer; }

    case DomLayoutItem::Layout:
        return create(ui_layoutItem->elementLayout(), layout, parentWidget);

    default:
        break;
    }

    return 0;
}

/*!
    \internal
*/
void QAbstractFormBuilder::applyProperties(QObject *o, const QList<DomProperty*> &properties)
{
    foreach (DomProperty *p, properties) {
        QVariant v = toVariant(o->metaObject(), p);
        if (!v.isNull())
            o->setProperty(p->attributeName().toUtf8(), v);
    }
}

/*!
    \internal
*/
QVariant QAbstractFormBuilder::toVariant(const QMetaObject *meta, DomProperty *p)
{
    QVariant v;

    switch(p->kind()) {
    case DomProperty::Bool: {
        v = toBool(p->elementBool());
    } break;

    case DomProperty::Cstring: {
        v = p->elementCstring();
    } break;

    case DomProperty::Point: {
        DomPoint *point = p->elementPoint();
        QPoint pt(point->elementX(), point->elementY());
        v = QVariant(pt);
    } break;

    case DomProperty::Size: {
        DomSize *size = p->elementSize();
        QSize sz(size->elementWidth(), size->elementHeight());
        v = QVariant(sz);
    } break;

    case DomProperty::Rect: {
        DomRect *rc = p->elementRect();
        QRect g(rc->elementX(), rc->elementY(), rc->elementWidth(), rc->elementHeight());
        v = QVariant(g);
    } break;

    case DomProperty::String: {
        int index = meta->indexOfProperty(p->attributeName().toUtf8());
        if (index != -1 && meta->property(index).type() == QVariant::KeySequence)
            v = qVariantFromValue(QKeySequence(p->elementString()->text()));
        else
            v = p->elementString()->text();
    } break;

    case DomProperty::Number: {
        v = p->elementNumber();
    } break;

    case DomProperty::Double: {
        v = p->elementDouble();
    } break;

    case DomProperty::Color: {
        DomColor *color = p->elementColor();
        QColor c(color->elementRed(), color->elementGreen(), color->elementBlue());
        v = qVariantFromValue(c);
    } break;

    case DomProperty::Font: {
        DomFont *font = p->elementFont();

        QFont f(font->elementFamily(), font->elementPointSize(), font->elementWeight(), font->elementItalic());
        f.setBold(font->elementBold());
        f.setUnderline(font->elementUnderline());
        f.setStrikeOut(font->elementStrikeOut());
        v = qVariantFromValue(f);
    } break;

    case DomProperty::Date: {
        DomDate *date = p->elementDate();

        QDate d(date->elementYear(), date->elementMonth(), date->elementDay());
        v = QVariant(d);
    } break;

    case DomProperty::Time: {
        DomTime *t = p->elementTime();

        QTime tm(t->elementHour(), t->elementMinute(), t->elementSecond());
        v = QVariant(tm);
    } break;

    case DomProperty::DateTime: {
        DomDateTime *dateTime = p->elementDateTime();
        QDate d(dateTime->elementYear(), dateTime->elementMonth(), dateTime->elementDay());
        QTime tm(dateTime->elementHour(), dateTime->elementMinute(), dateTime->elementSecond());

        QDateTime dt(d, tm);
        v = QVariant(dt);
    } break;

    case DomProperty::Pixmap:
    case DomProperty::IconSet: {
        DomResourcePixmap *resource = 0;
        if (p->kind() == DomProperty::IconSet)
            resource = p->elementIconSet();
        else
            resource = p->elementPixmap();

        if (resource != 0) {
            QString icon_path = resource->text();
            QString qrc_path = resource->attributeResource();

            if (icon_path.isEmpty()) {
                if (p->kind() == DomProperty::IconSet)
                    return QIcon();

                return QPixmap();
            }

            if (qrc_path.isEmpty())
                icon_path = workingDirectory().absoluteFilePath(icon_path);
            else
                qrc_path = workingDirectory().absoluteFilePath(qrc_path);

            if (p->kind() == DomProperty::IconSet) {
                QIcon icon = nameToIcon(icon_path, qrc_path);

                v = qVariantFromValue(icon);
            } else {
                QPixmap pixmap = nameToPixmap(icon_path, qrc_path);
                v = qVariantFromValue(pixmap);
            }
        }
    } break;

    case DomProperty::Palette: {
        DomPalette *dom = p->elementPalette();
        QPalette palette;

        if (dom->elementActive()) {
            palette.setCurrentColorGroup(QPalette::Active);
            setupColorGroup(palette, dom->elementActive());
        }

        if (dom->elementInactive()) {
            palette.setCurrentColorGroup(QPalette::Inactive);
            setupColorGroup(palette, dom->elementInactive());
        }

        if (dom->elementDisabled()) {
            palette.setCurrentColorGroup(QPalette::Disabled);
            setupColorGroup(palette, dom->elementDisabled());
        }

        palette.setCurrentColorGroup(QPalette::Active);
        v = qVariantFromValue(palette);
    } break;

    case DomProperty::Cursor: {
        v = qVariantFromValue(QCursor(static_cast<Qt::CursorShape>(p->elementCursor())));
    } break;

    case DomProperty::Set: {
        QByteArray pname = p->attributeName().toUtf8();
        int index = meta->indexOfProperty(pname);
        if (index == -1) {
            qWarning() << "property" << pname << "is not supported";
            break;
        }

        QMetaEnum e = meta->property(index).enumerator();
        Q_ASSERT(e.isFlag() == true);

        v = e.keysToValue(p->elementSet().toUtf8());
    } break;

    case DomProperty::Enum: {
        QByteArray pname = p->attributeName().toUtf8();
        int index = meta->indexOfProperty(pname);
        if (index == -1) {
            qWarning() << "property" << pname << "is not supported";
            break;
        }

        QMetaEnum e = meta->property(index).enumerator();
        v = e.keyToValue(p->elementEnum().toUtf8());
    } break;

    case DomProperty::SizePolicy: {
        DomSizePolicy *sizep = p->elementSizePolicy();

        QSizePolicy sizePolicy;
        sizePolicy.setHorizontalStretch(sizep->elementHorStretch());
        sizePolicy.setVerticalStretch(sizep->elementVerStretch());

        sizePolicy.setHorizontalPolicy((QSizePolicy::Policy) sizep->elementHSizeType());
        sizePolicy.setVerticalPolicy((QSizePolicy::Policy) sizep->elementVSizeType());
        v = qVariantFromValue(sizePolicy);
    } break;

    default:
        qWarning() << "QAbstractFormBuilder::toVariant:" << p->kind() << " not implemented yet!";
        break;
    }

    return v;
}

/*!
    \internal
*/
void QAbstractFormBuilder::setupColorGroup(QPalette &palette, DomColorGroup *group)
{
    QList<DomColor*> colors = group->elementColor();
    for (int role = 0; role < colors.size(); ++role) {
        DomColor *color = colors.at(role);
        QColor c(color->elementRed(), color->elementGreen(), color->elementBlue());
        palette.setColor(QPalette::ColorRole(role), c); // ### TODO: support the QPalette::ColorRole as string
    }
}

/*!
    \internal
*/
DomColorGroup *QAbstractFormBuilder::saveColorGroup(const QPalette &palette)
{
    DomColorGroup *group = new DomColorGroup();
    QList<DomColor*> colors;

    for (int role = QPalette::Foreground; role < QPalette::NColorRoles; ++role) {
        QColor c = palette.color(QPalette::ColorRole(role));

        DomColor *color = new DomColor();
        color->setElementRed(c.red());
        color->setElementGreen(c.green());
        color->setElementBlue(c.blue());
        colors.append(color);
    }

    group->setElementColor(colors);
    return group;
}

/*!
    \internal
*/
QWidget *QAbstractFormBuilder::createWidget(const QString &widgetName, QWidget *parentWidget, const QString &name)
{
    Q_UNUSED(widgetName);
    Q_UNUSED(parentWidget);
    Q_UNUSED(name);
    return 0;
}

/*!
    \internal
*/
QLayout *QAbstractFormBuilder::createLayout(const QString &layoutName, QObject *parent, const QString &name)
{
    Q_UNUSED(layoutName);
    Q_UNUSED(parent);
    Q_UNUSED(name);
    return 0;
}

/*!
    \internal
*/
QAction *QAbstractFormBuilder::createAction(QObject *parent, const QString &name)
{
    QAction *action = new QAction(parent);
    action->setObjectName(name);
    m_actions.insert(name, action);

    return action;
}

/*!
    \internal
*/
QActionGroup *QAbstractFormBuilder::createActionGroup(QObject *parent, const QString &name)
{
    QActionGroup *g = new QActionGroup(parent);
    g->setObjectName(name);
    m_actionGroups.insert(name, g);

    return g;
}

/*!
    \fn void QAbstractFormBuilder::save(QIODevice *device, QWidget *widget)

    Saves an XML representation of the given \a widget to the
    specified \a device in the standard \c{.ui} file format.

    \sa load()*/
void QAbstractFormBuilder::save(QIODevice *dev, QWidget *widget)
{
    DomWidget *ui_widget = createDom(widget, 0);
    Q_ASSERT( ui_widget != 0 );

    DomUI *ui = new DomUI();
    ui->setAttributeVersion(QLatin1String("4.0"));
    ui->setElementWidget(ui_widget);

    saveDom(ui, widget);

    QDomDocument doc;
    doc.appendChild(ui->write(doc));
    QByteArray bytes = doc.toString().toUtf8();
    dev->write(bytes, bytes.size());

    m_laidout.clear();

    delete ui;
}

/*!
    \internal
*/
void QAbstractFormBuilder::saveDom(DomUI *ui, QWidget *widget)
{
    ui->setElementClass(widget->objectName());

    if (DomConnections *ui_connections = saveConnections()) {
        ui->setElementConnections(ui_connections);
    }

    if (DomCustomWidgets *ui_customWidgets = saveCustomWidgets()) {
        ui->setElementCustomWidgets(ui_customWidgets);
    }

    if (DomTabStops *ui_tabStops = saveTabStops()) {
        ui->setElementTabStops(ui_tabStops);
    }

    if (DomResources *ui_resources = saveResources()) {
        ui->setElementResources(ui_resources);
    }
}

/*!
    \internal
*/
DomConnections *QAbstractFormBuilder::saveConnections()
{
    return new DomConnections;
}

/*!
    \internal
*/
DomWidget *QAbstractFormBuilder::createDom(QWidget *widget, DomWidget *ui_parentWidget, bool recursive)
{
    DomWidget *ui_widget = new DomWidget();
    ui_widget->setAttributeClass(QLatin1String(widget->metaObject()->className()));
    ui_widget->setElementProperty(computeProperties(widget));

    if (recursive) {
        if (QLayout *layout = widget->layout()) {
            if (DomLayout *ui_layout = createDom(layout, 0, ui_parentWidget)) {
                QList<DomLayout*> ui_layouts;
                ui_layouts.append(ui_layout);

                ui_widget->setElementLayout(ui_layouts);
            }
        }
    }

    // widgets, actions and action groups
    QList<DomWidget*> ui_widgets;
    QList<DomAction*> ui_actions;
    QList<DomActionGroup*> ui_action_groups;

    QList<QObject*> children;

    // splitters need to store their children in the order specified by child indexes,
    // not the order of the child list.
    if (QSplitter *splitter = qobject_cast<QSplitter*>(widget)) {
        for (int i = 0; i < splitter->count(); ++i)
            children.append(splitter->widget(i));
    } else {
        children = widget->children();
    }

    foreach (QObject *obj, children) {
        if (QWidget *childWidget = qobject_cast<QWidget*>(obj)) {
            if (m_laidout.contains(childWidget) || recursive == false)
                continue;

            if (DomWidget *ui_child = createDom(childWidget, ui_widget)) {
                ui_widgets.append(ui_child);
            }
        } else if (QAction *childAction = qobject_cast<QAction*>(obj)) {
            if (childAction->actionGroup() != 0) {
                // it will be added later.
                continue;
            }

            if (DomAction *ui_action = createDom(childAction)) {
                ui_actions.append(ui_action);
            }
        } else if (QActionGroup *childActionGroup = qobject_cast<QActionGroup*>(obj)) {
            if (DomActionGroup *ui_action_group = createDom(childActionGroup)) {
                ui_action_groups.append(ui_action_group);
            }
        }
    }

    // add-action
    QList<DomActionRef*> ui_action_refs;
    foreach (QAction *action, widget->actions()) {
        if (DomActionRef *ui_action_ref = createActionRefDom(action)) {
            ui_action_refs.append(ui_action_ref);
        }
    }

    if (recursive)
        ui_widget->setElementWidget(ui_widgets);

    ui_widget->setElementAction(ui_actions);
    ui_widget->setElementActionGroup(ui_action_groups);
    ui_widget->setElementAddAction(ui_action_refs);

    saveExtraInfo(widget, ui_widget, ui_parentWidget);

    return ui_widget;
}

/*!
    \internal
*/
DomActionRef *QAbstractFormBuilder::createActionRefDom(QAction *action)
{
    QString name = action->objectName();

    if (action->menu() != 0 && name.endsWith(QLatin1String("Action")))
        name = name.left(name.count() - 6);

    DomActionRef *ui_action_ref = new DomActionRef();
    if (action->isSeparator())
        ui_action_ref->setAttributeName(QLatin1String("separator"));
    else
        ui_action_ref->setAttributeName(name);

    return ui_action_ref;
}

/*!
    \internal
*/
DomLayout *QAbstractFormBuilder::createDom(QLayout *layout, DomLayout *ui_layout, DomWidget *ui_parentWidget)
{
    Q_UNUSED(ui_layout)
    DomLayout *lay = new DomLayout();
    lay->setAttributeClass(QLatin1String(layout->metaObject()->className()));
    lay->setElementProperty(computeProperties(layout));

    QList<DomLayoutItem*> ui_items;

    for (int idx=0; layout->itemAt(idx); ++idx) {
        QLayoutItem *item = layout->itemAt(idx);

        DomLayoutItem *ui_item = createDom(item, lay, ui_parentWidget);
        if (ui_item)
            ui_items.append(ui_item);
    }

    lay->setElementItem(ui_items);

    return lay;
}

/*!
    \internal
*/
DomLayoutItem *QAbstractFormBuilder::createDom(QLayoutItem *item, DomLayout *ui_layout, DomWidget *ui_parentWidget)
{
    DomLayoutItem *ui_item = new DomLayoutItem();

    if (item->widget())  {
        ui_item->setElementWidget(createDom(item->widget(), ui_parentWidget));
        m_laidout.insert(item->widget(), true);
    } else if (item->layout()) {
        ui_item->setElementLayout(createDom(item->layout(), ui_layout, ui_parentWidget));
    } else if (item->spacerItem()) {
        ui_item->setElementSpacer(createDom(item->spacerItem(), ui_layout, ui_parentWidget));
    }

    return ui_item;
}

/*!
    \internal
*/
DomSpacer *QAbstractFormBuilder::createDom(QSpacerItem *spacer, DomLayout *ui_layout, DomWidget *ui_parentWidget)
{
    Q_UNUSED(ui_layout);
    Q_UNUSED(ui_parentWidget);

    DomSpacer *ui_spacer = new DomSpacer();
    QList<DomProperty*> properties;

    DomProperty *prop = 0;

    // sizeHint property
    prop = new DomProperty();
    prop->setAttributeName(QLatin1String("sizeHint"));
    prop->setElementSize(new DomSize());
    prop->elementSize()->setElementWidth(spacer->sizeHint().width());
    prop->elementSize()->setElementHeight(spacer->sizeHint().height());
    properties.append(prop);

    // orientation property
    prop = new DomProperty(); // ### we don't implemented the case where expandingDirections() is both Vertical and Horizontal
    prop->setAttributeName(QLatin1String("orientation"));
    prop->setElementEnum((spacer->expandingDirections() & Qt::Horizontal) ? QLatin1String("Qt::Horizontal") : QLatin1String("Qt::Vertical"));
    properties.append(prop);

    ui_spacer->setElementProperty(properties);
    return ui_spacer;
}

/*!
    \internal
*/
DomProperty *QAbstractFormBuilder::createProperty(QObject *obj, const QString &pname, const QVariant &v)
{
    if (!checkProperty(obj, pname)) {
        return 0;
    }

    DomProperty *dom_prop = new DomProperty();

    const QMetaObject *meta = obj->metaObject();
    int pindex = meta->indexOfProperty(pname.toLatin1());
    if (pindex != -1) {
        QMetaProperty meta_property = meta->property(pindex);
        if (!meta_property.hasStdCppSet())
            dom_prop->setAttributeStdset(0);
    }

    switch (v.type()) {
        case QVariant::String: {
            DomString *str = new DomString();
            str->setText(v.toString());

            if (pname == QLatin1String("objectName"))
                str->setAttributeNotr(QLatin1String("true"));

            dom_prop->setElementString(str);
        } break;

        case QVariant::ByteArray: {
            dom_prop->setElementCstring(QString::fromUtf8(v.toByteArray()));
        } break;

        case QVariant::Int: {
            dom_prop->setElementNumber(v.toInt());
        } break;

        case QVariant::UInt: {
            dom_prop->setElementNumber(v.toUInt());
        } break;

        case QVariant::Double: {
            dom_prop->setElementDouble(v.toDouble());
        } break;

        case QVariant::Bool: {
            dom_prop->setElementBool(v.toBool() ? QLatin1String("true") : QLatin1String("false"));
        } break;

        case QVariant::Point: {
            DomPoint *pt = new DomPoint();
            QPoint point = v.toPoint();
            pt->setElementX(point.x());
            pt->setElementY(point.y());
            dom_prop->setElementPoint(pt);
        } break;

        case QVariant::Color: {
            DomColor *clr = new DomColor();
            QColor color = qvariant_cast<QColor>(v);
            clr->setElementRed(color.red());
            clr->setElementGreen(color.green());
            clr->setElementBlue(color.blue());
            dom_prop->setElementColor(clr);
        } break;

        case QVariant::Size: {
            DomSize *sz = new DomSize();
            QSize size = v.toSize();
            sz->setElementWidth(size.width());
            sz->setElementHeight(size.height());
            dom_prop->setElementSize(sz);
        } break;

        case QVariant::Rect: {
            DomRect *rc = new DomRect();
            QRect rect = v.toRect();
            rc->setElementX(rect.x());
            rc->setElementY(rect.y());
            rc->setElementWidth(rect.width());
            rc->setElementHeight(rect.height());
            dom_prop->setElementRect(rc);
        } break;

        case QVariant::Font: {
            DomFont *fnt = new DomFont();
            QFont font = qvariant_cast<QFont>(v);
            fnt->setElementBold(font.bold());
            fnt->setElementFamily(font.family());
            fnt->setElementItalic(font.italic());
            fnt->setElementPointSize(font.pointSize());
            fnt->setElementStrikeOut(font.strikeOut());
            fnt->setElementUnderline(font.underline());
            fnt->setElementWeight(font.weight());
            dom_prop->setElementFont(fnt);
        } break;

        case QVariant::Cursor: {
            dom_prop->setElementCursor(qvariant_cast<QCursor>(v).shape());
        } break;

        case QVariant::KeySequence: {
            DomString *s = new DomString();
#ifndef Q_WS_MAC
            s->setText(qvariant_cast<QKeySequence>(v));
#else
            s->setText(platformNeutralKeySequence(qvariant_cast<QKeySequence>(v)));
#endif
            dom_prop->setElementString(s);
        } break;

        case QVariant::Palette: {
            DomPalette *dom = new DomPalette();
            QPalette palette = qvariant_cast<QPalette>(v);

            palette.setCurrentColorGroup(QPalette::Active);
            dom->setElementActive(saveColorGroup(palette));

            palette.setCurrentColorGroup(QPalette::Inactive);
            dom->setElementInactive(saveColorGroup(palette));

            palette.setCurrentColorGroup(QPalette::Disabled);
            dom->setElementDisabled(saveColorGroup(palette));

            dom_prop->setElementPalette(dom);
        } break;

        case QVariant::SizePolicy: {
            DomSizePolicy *dom = new DomSizePolicy();
            QSizePolicy sizePolicy = qvariant_cast<QSizePolicy>(v);

            dom->setElementHorStretch(sizePolicy.horizontalStretch());
            dom->setElementVerStretch(sizePolicy.verticalStretch());

            dom->setElementHSizeType(sizePolicy.horizontalPolicy());
            dom->setElementVSizeType(sizePolicy.verticalPolicy());

            dom_prop->setElementSizePolicy(dom);
        } break;

        case QVariant::Time: {
            DomTime *dom = new DomTime();
            QTime time = qvariant_cast<QTime>(v);

            dom->setElementHour(time.hour());
            dom->setElementMinute(time.minute());
            dom->setElementSecond(time.second());

            dom_prop->setElementTime(dom);
        } break;

        case QVariant::Pixmap:
        case QVariant::Icon: {
            DomResourcePixmap *r = new DomResourcePixmap;
            QString icon_path;
            QString qrc_path;
            if (v.type() == QVariant::Icon) {
                QIcon icon = qvariant_cast<QIcon>(v);
                icon_path = iconToFilePath(icon);
                qrc_path = iconToQrcPath(icon);
            } else {
                QPixmap pixmap = qvariant_cast<QPixmap>(v);
                icon_path = pixmapToFilePath(pixmap);
                qrc_path = pixmapToQrcPath(pixmap);
            }

            if (qrc_path.isEmpty())
                icon_path = workingDirectory().relativeFilePath(icon_path);
            else
                qrc_path = workingDirectory().relativeFilePath(qrc_path);

            r->setText(icon_path);
            if (!qrc_path.isEmpty())
                r->setAttributeResource(qrc_path);

            if (v.type() == QVariant::Icon)
                dom_prop->setElementIconSet(r);
            else
                dom_prop->setElementPixmap(r);
        } break;

        default: {
            qWarning("support for property `%s' of type `%d' not implemented yet!!",
                pname.toUtf8().data(), v.type());
        } break;
    }

    dom_prop->setAttributeName(pname);
    // ### dom_prop->setAttributeStdset(true);

    if (dom_prop->kind() == DomProperty::Unknown) {
        delete dom_prop;
        dom_prop = 0;
    }

    return dom_prop;
}

/*!
    \internal
*/
QList<DomProperty*> QAbstractFormBuilder::computeProperties(QObject *obj)
{
    QList<DomProperty*> lst;

    const QMetaObject *meta = obj->metaObject();

    QHash<QByteArray, bool> properties;
    for(int i=0; i<meta->propertyCount(); ++i)
        properties.insert(meta->property(i).name(), true);

    QList<QByteArray> propertyNames = properties.keys();

    for(int i=0; i<propertyNames.size(); ++i) {
        QString pname = QString::fromUtf8(propertyNames.at(i));
        QMetaProperty prop = meta->property(meta->indexOfProperty(pname.toUtf8()));

        if (!prop.isWritable() || !checkProperty(obj, QLatin1String(prop.name())))
            continue;

        QVariant v = prop.read(obj);

        DomProperty *dom_prop = 0;
        if (v.type() == QVariant::Int) {
            dom_prop = new DomProperty();

            if (prop.isFlagType())
                qWarning("flags property not supported yet!!");

            if (prop.isEnumType()) {
                QString scope = QString::fromUtf8(prop.enumerator().scope());
                if (scope.size())
                    scope += QString::fromUtf8("::");
                QString e = QString::fromUtf8(prop.enumerator().valueToKey(v.toInt()));
                if (e.size())
                    dom_prop->setElementEnum(scope + e);
            } else
                dom_prop->setElementNumber(v.toInt());
            dom_prop->setAttributeName(pname);
        } else {
            dom_prop = createProperty(obj, pname, v);
        }

        if (!dom_prop || dom_prop->kind() == DomProperty::Unknown)
            delete dom_prop;
        else
            lst.append(dom_prop);
    }

    return lst;
}

/*!
    \internal
*/
bool QAbstractFormBuilder::toBool(const QString &str)
{
    return str.toLower() == QLatin1String("true");
}

/*!
    \internal
*/
QHash<QString, DomProperty*> QAbstractFormBuilder::propertyMap(const QList<DomProperty*> &properties)
{
    QHash<QString, DomProperty*> map;

    foreach (DomProperty *p, properties)
        map.insert(p->attributeName(), p);

    return map;
}

/*!
    \internal
*/
bool QAbstractFormBuilder::checkProperty(QObject *obj, const QString &prop) const
{
    Q_UNUSED(obj);
    Q_UNUSED(prop);

    return true;
}

/*!
    \internal
*/
QString QAbstractFormBuilder::toString(const DomString *str)
{
    return str ? str->text() : QString();
}

/*!
    \internal
*/
void QAbstractFormBuilder::applyTabStops(QWidget *widget, DomTabStops *tabStops)
{
    if (!tabStops)
        return;

    QWidget *lastWidget = 0;

    QStringList l = tabStops->elementTabStop();
    for (int i=0; i<l.size(); ++i) {
        QString name = l.at(i);

        QWidget *child = qFindChild<QWidget*>(widget, name);
        if (!child) {
            qWarning("'%s' isn't a valid widget\n", name.toUtf8().data());
            continue;
        }

        if (i == 0) {
            lastWidget = qFindChild<QWidget*>(widget, name);
            continue;
        } else if (!child || !lastWidget) {
            continue;
        }

        QWidget::setTabOrder(lastWidget, child);

        lastWidget = qFindChild<QWidget*>(widget, name);
    }
}

/*!
    \internal
*/
DomCustomWidgets *QAbstractFormBuilder::saveCustomWidgets()
{
    return 0;
}

/*!
    \internal
*/
DomTabStops *QAbstractFormBuilder::saveTabStops()
{
    return 0;
}

/*!
    \internal
*/
DomResources *QAbstractFormBuilder::saveResources()
{
    return 0;
}

/*!
    \internal
*/
void QAbstractFormBuilder::saveTreeWidgetExtraInfo(QTreeWidget *treeWidget, DomWidget *ui_widget, DomWidget *ui_parentWidget)
{
    Q_UNUSED(ui_parentWidget);

    QList<DomColumn*> columns;

    // save the header
    for (int c = 0; c<treeWidget->columnCount(); ++c) {
        DomColumn *column = new DomColumn;

        QList<DomProperty*> properties;

        // property text
        DomProperty *ptext = new DomProperty;
        DomString *str = new DomString;
        str->setText(treeWidget->headerItem()->text(c));
        ptext->setAttributeName(QLatin1String("text"));
        ptext->setElementString(str);
        properties.append(ptext);

        QIcon icon = treeWidget->headerItem()->icon(c);
        if (!icon.isNull()) {
            QString iconPath = iconToFilePath(icon);
            QString qrcPath = iconToQrcPath(icon);

            DomProperty *p = new DomProperty;

            DomResourcePixmap *pix = new DomResourcePixmap;
            if (!qrcPath.isEmpty())
                pix->setAttributeResource(qrcPath);

            pix->setText(iconPath);

            p->setAttributeName(QLatin1String("icon"));
            p->setElementIconSet(pix);

            properties.append(p);
        }

        column->setElementProperty(properties);
        columns.append(column);
    }

    ui_widget->setElementColumn(columns);
}

/*!
    \internal
*/
void QAbstractFormBuilder::saveListWidgetExtraInfo(QListWidget *listWidget, DomWidget *ui_widget, DomWidget *ui_parentWidget)
{
    Q_UNUSED(ui_parentWidget);

    QList<DomItem*> ui_items = ui_widget->elementItem();

    for (int i=0; i<listWidget->count(); ++i) {
        QListWidgetItem *item = listWidget->item(i);
        DomItem *ui_item = new DomItem();

        QList<DomProperty*> properties;

        // text
        DomString *str = new DomString;
        str->setText(item->text());

        DomProperty *p = 0;

        p = new DomProperty;
        p->setAttributeName(QLatin1String("text"));
        p->setElementString(str);
        properties.append(p);

        if (!item->icon().isNull()) {
            QString iconPath = iconToFilePath(item->icon());
            QString qrcPath = iconToQrcPath(item->icon());

            p = new DomProperty;

            DomResourcePixmap *pix = new DomResourcePixmap;
            if (!qrcPath.isEmpty())
                pix->setAttributeResource(qrcPath);

            pix->setText(iconPath);

            p->setAttributeName(QLatin1String("icon"));
            p->setElementIconSet(pix);

            properties.append(p);
        }

        ui_item->setElementProperty(properties);
        ui_items.append(ui_item);
    }

    ui_widget->setElementItem(ui_items);
}

/*!
    \internal
*/
void QAbstractFormBuilder::saveComboBoxExtraInfo(QComboBox *comboBox, DomWidget *ui_widget, DomWidget *ui_parentWidget)
{
    Q_UNUSED(ui_parentWidget);

    QList<DomItem*> ui_items = ui_widget->elementItem();

    for (int i=0; i<comboBox->count(); ++i) {
        DomItem *ui_item = new DomItem();

        QList<DomProperty*> properties;

        // text
        DomString *str = new DomString;
        str->setText(comboBox->itemText(i));

        DomProperty *p = 0;

        p = new DomProperty;
        p->setAttributeName(QLatin1String("text"));
        p->setElementString(str);
        properties.append(p);

        QIcon icon = comboBox->itemData(i).value<QIcon>();
        if (!icon.isNull()) {
            QString iconPath = iconToFilePath(icon);
            QString qrcPath = iconToQrcPath(icon);

            DomProperty *p = new DomProperty;

            DomResourcePixmap *pix = new DomResourcePixmap;
            if (!qrcPath.isEmpty())
                pix->setAttributeResource(qrcPath);

            pix->setText(iconPath);

            p->setAttributeName(QLatin1String("icon"));
            p->setElementIconSet(pix);

            properties.append(p);
        }

        ui_item->setElementProperty(properties);
        ui_items.append(ui_item);
    }

    ui_widget->setElementItem(ui_items);
}

/*!
    \internal
*/
void QAbstractFormBuilder::saveExtraInfo(QWidget *widget, DomWidget *ui_widget, DomWidget *ui_parentWidget)
{
    if (QListWidget *listWidget = qobject_cast<QListWidget*>(widget)) {
        saveListWidgetExtraInfo(listWidget, ui_widget, ui_parentWidget);
    } else if (QTreeWidget *treeWidget = qobject_cast<QTreeWidget*>(widget)) {
        saveTreeWidgetExtraInfo(treeWidget, ui_widget, ui_parentWidget);
    } else if (QComboBox *comboBox = qobject_cast<QComboBox*>(widget)) {
        saveComboBoxExtraInfo(comboBox, ui_widget, ui_parentWidget);
    }
}

/*!
    \internal
*/
void QAbstractFormBuilder::loadListWidgetExtraInfo(DomWidget *ui_widget, QListWidget *listWidget, QWidget *parentWidget)
{
    Q_UNUSED(parentWidget);

    foreach (DomItem *ui_item, ui_widget->elementItem()) {
        QHash<QString, DomProperty*> properties = propertyMap(ui_item->elementProperty());
        QListWidgetItem *item = new QListWidgetItem(listWidget);

        DomProperty *p = 0;

        p = properties.value(QLatin1String("text"));
        if (p && p->kind() == DomProperty::String) {
            item->setText(p->elementString()->text());
        }

        p = properties.value(QLatin1String("icon"));
        if (p && p->kind() == DomProperty::IconSet) {
            DomResourcePixmap *icon = p->elementIconSet();
            Q_ASSERT(icon != 0);
            QString iconPath = icon->text();
            QString qrcPath = icon->attributeResource();

            item->setIcon(nameToIcon(iconPath, qrcPath));
        }
    }

    DomProperty *currentRow = propertyMap(ui_widget->elementProperty()).value("currentRow");
    if (currentRow)
        listWidget->setCurrentRow(currentRow->elementNumber());
}

/*!
    \internal
*/
void QAbstractFormBuilder::loadTreeWidgetExtraInfo(DomWidget *ui_widget, QTreeWidget *treeWidget, QWidget *parentWidget)
{
    Q_UNUSED(parentWidget);

    QList<DomColumn*> columns = ui_widget->elementColumn();

    treeWidget->setColumnCount(columns.count());

    for (int i = 0; i<columns.count(); ++i) {
        DomColumn *c = columns.at(i);
        QHash<QString, DomProperty*> properties = propertyMap(c->elementProperty());

        DomProperty *ptext = properties.value(QLatin1String("text"));
        DomProperty *picon = properties.value(QLatin1String("icon"));

        if (ptext != 0 && ptext->elementString())
            treeWidget->headerItem()->setText(i, ptext->elementString()->text());

        if (picon && picon->kind() == DomProperty::IconSet) {
            DomResourcePixmap *icon = picon->elementIconSet();
            Q_ASSERT(icon != 0);
            QString iconPath = icon->text();
            QString qrcPath = icon->attributeResource();

            treeWidget->headerItem()->setIcon(i, nameToIcon(iconPath, qrcPath));
        }
    }
}

/*!
    \internal
*/
void QAbstractFormBuilder::loadComboBoxExtraInfo(DomWidget *ui_widget, QComboBox *comboBox, QWidget *parentWidget)
{
    Q_UNUSED(parentWidget);

    foreach (DomItem *ui_item, ui_widget->elementItem()) {
        QHash<QString, DomProperty*> properties = propertyMap(ui_item->elementProperty());
        QString text;
        QIcon icon;

        DomProperty *p = 0;

        p = properties.value(QLatin1String("text"));
        if (p && p->elementString()) {
            text = p->elementString()->text();
        }

        p = properties.value(QLatin1String("icon"));
        if (p && p->kind() == DomProperty::IconSet) {
            DomResourcePixmap *picon = p->elementIconSet();
            Q_ASSERT(picon != 0);
            QString iconPath = picon->text();
            QString qrcPath = picon->attributeResource();

            icon = nameToIcon(iconPath, qrcPath);
        }

        comboBox->addItem(icon, text);
        comboBox->setItemData((comboBox->count()-1), icon);
    }

    DomProperty *currentIndex = propertyMap(ui_widget->elementProperty()).value("currentIndex");
    if (currentIndex)
        comboBox->setCurrentIndex(currentIndex->elementNumber());
}

/*!
    \internal
*/
void QAbstractFormBuilder::loadExtraInfo(DomWidget *ui_widget, QWidget *widget, QWidget *parentWidget)
{
    if (QListWidget *listWidget = qobject_cast<QListWidget*>(widget)) {
        loadListWidgetExtraInfo(ui_widget, listWidget, parentWidget);
    } else if (QTreeWidget *treeWidget = qobject_cast<QTreeWidget*>(widget)) {
        loadTreeWidgetExtraInfo(ui_widget, treeWidget, parentWidget);
    } else if (QComboBox *comboBox = qobject_cast<QComboBox*>(widget)) {
        loadComboBoxExtraInfo(ui_widget, comboBox, parentWidget);
    }
}

/*!
    \internal
*/
QIcon QAbstractFormBuilder::nameToIcon(const QString &filePath, const QString &qrcPath)
{
    Q_UNUSED(qrcPath);
    QFileInfo fileInfo(workingDirectory(), filePath);
    return QIcon(fileInfo.absoluteFilePath());
}

/*!
    \internal
*/
QString QAbstractFormBuilder::iconToFilePath(const QIcon &pm) const
{
    Q_UNUSED(pm);
    return QString();
}

/*!
    \internal
*/
QString QAbstractFormBuilder::iconToQrcPath(const QIcon &pm) const
{
    Q_UNUSED(pm);
    return QString();
}

/*!
    \internal
*/
QPixmap QAbstractFormBuilder::nameToPixmap(const QString &filePath, const QString &qrcPath)
{
    Q_UNUSED(qrcPath);
    QFileInfo fileInfo(workingDirectory(), filePath);
    return QPixmap(fileInfo.absoluteFilePath());
}

/*!
    \internal
*/
QString QAbstractFormBuilder::pixmapToFilePath(const QPixmap &pm) const
{
    Q_UNUSED(pm);
    return QString();
}

/*!
    \internal
*/
QString QAbstractFormBuilder::pixmapToQrcPath(const QPixmap &pm) const
{
    Q_UNUSED(pm);
    return QString();
}

/*!
    Returns the current working directory of the form builder.

    \sa setWorkingDirectory() */
QDir QAbstractFormBuilder::workingDirectory() const
{
    return m_workingDirectory;
}

/*!
    Sets the current working directory of the form builder to the \a directory specified.

    \sa workingDirectory()*/
void QAbstractFormBuilder::setWorkingDirectory(const QDir &directory)
{
    m_workingDirectory = directory;
}

/*!
    \internal
*/
DomAction *QAbstractFormBuilder::createDom(QAction *action)
{
    if (action->parentWidget() == action->menu() || action->isSeparator())
        return 0;

    DomAction *ui_action = new DomAction;
    ui_action->setAttributeName(action->objectName());

    QList<DomProperty*> properties = computeProperties(action);
    ui_action->setElementProperty(properties);

    return ui_action;
}

/*!
    \internal
*/
DomActionGroup *QAbstractFormBuilder::createDom(QActionGroup *actionGroup)
{
    DomActionGroup *ui_action_group = new DomActionGroup;
    ui_action_group->setAttributeName(actionGroup->objectName());

    QList<DomProperty*> properties = computeProperties(actionGroup);
    ui_action_group->setElementProperty(properties);

    QList<DomAction*> ui_actions;

    foreach (QAction *action, actionGroup->actions()) {
        if (DomAction *ui_action = createDom(action)) {
            ui_actions.append(ui_action);
        }
    }

    ui_action_group->setElementAction(ui_actions);

    return ui_action_group;
}

/*!
    \internal
*/
void QAbstractFormBuilder::addMenuAction(QAction *action)
{
    Q_UNUSED(action);
}

/*!
    \internal
*/
void QAbstractFormBuilder::reset()
{
    m_laidout.clear();
    m_actions.clear();
    m_actionGroups.clear();
    m_defaultMargin = INT_MIN;
    m_defaultSpacing = INT_MIN;
}

/*!
    \fn void QAbstractFormBuilder::createConnections ( DomConnections *, QWidget * )
    \internal
*/

/*!
    \fn void QAbstractFormBuilder::createCustomWidgets ( DomCustomWidgets * )
    \internal
*/

/*!
    \fn void QAbstractFormBuilder::createResources ( DomResources * )
    \internal
*/

#include "abstractformbuilder.moc"

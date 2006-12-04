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

#include "propertyeditor.h"
#include "findicondialog_p.h"
#include "qpropertyeditor_model_p.h"
#include "qpropertyeditor_items_p.h"
#include "newdynamicpropertydialog.h"

// sdk
#include <QtDesigner/QtDesigner>
#include <QtDesigner/QExtensionManager>

// shared
#include <iconloader_p.h>
#include <qdesigner_utils_p.h>
#include <qdesigner_propertycommand_p.h>
#include <metadatabase_p.h>
#include "paletteeditorbutton.h"
#include <QtGui/QtGui>

#ifndef Q_MOC_RUN
using namespace qdesigner_internal;
#endif

IProperty *PropertyEditor::createSpecialProperty(const QVariant &value, const QString &name)
{
    Q_UNUSED(value);
    Q_UNUSED(name);

    return 0;
}

// ---------------------------------------------------------------------------------

namespace qdesigner_internal {

class IconProperty : public AbstractProperty<QIcon>
{
public:
    IconProperty(QDesignerFormEditorInterface *core, const QIcon &value, const QString &name);

    void setValue(const QVariant &value);
    QString toString() const;
    QVariant decoration() const;

    QWidget *createEditor(QWidget *parent, const QObject *target, const char *receiver) const;
    void updateEditorContents(QWidget *editor);
    void updateValue(QWidget *editor);
private:
    QDesignerFormEditorInterface *m_core;
};

class PixmapProperty : public AbstractProperty<QPixmap>
{
public:
    PixmapProperty(QDesignerFormEditorInterface *core, const QPixmap &pixmap, const QString &name);

    void setValue(const QVariant &value);
    QString toString() const;
    QVariant decoration() const;

    QWidget *createEditor(QWidget *parent, const QObject *target, const char *receiver) const;
    void updateEditorContents(QWidget *editor);
    void updateValue(QWidget *editor);
private:
    QDesignerFormEditorInterface *m_core;
};

class PaletteProperty : public AbstractProperty<QPalette>
{
public:
    PaletteProperty(QDesignerFormEditorInterface *core, const QPalette &value,
                QWidget *selectedWidget, const QString &name);

    void setValue(const QVariant &value);
    QString toString() const;

    QWidget *createEditor(QWidget *parent, const QObject *target, const char *receiver) const;
    void updateEditorContents(QWidget *editor);
    void updateValue(QWidget *editor);

private:
    QDesignerFormEditorInterface *m_core;
    QWidget *m_selectedWidget;
};

// This handles editing of pixmap and icon properties
class GraphicsPropertyEditor : public QWidget
{
    Q_OBJECT
public:
    GraphicsPropertyEditor(QDesignerFormEditorInterface *core, const QIcon &pm, QWidget *parent);
    GraphicsPropertyEditor(QDesignerFormEditorInterface *core, const QPixmap &pixmap, QWidget *parent);
    ~GraphicsPropertyEditor();

    void setIcon(const QIcon &pm);
    void setPixmap(const QPixmap &pm);
    QIcon icon() const { return m_mode == Icon ? m_icon : QIcon(); }
    QPixmap pixmap() const { return m_mode == Pixmap ? m_pixmap : QPixmap(); }

signals:
    void iconChanged(const QIcon &pm);
    void pixmapChanged(const QPixmap &pm);

private slots:
    void showDialog();
    void comboActivated(int idx);

private:
    void init();
    void populateCombo();
    int indexOfIcon(const QIcon &icon);
    int indexOfPixmap(const QPixmap &pixmap);

    enum Mode { Icon, Pixmap };
    const Mode m_mode;

    QDesignerFormEditorInterface *m_core;
    QComboBox *m_combo;
    QToolButton *m_button;
    QIcon m_icon;
    QPixmap m_pixmap;
};

GraphicsPropertyEditor::~GraphicsPropertyEditor()
{
}

void GraphicsPropertyEditor::init()
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);

    m_combo = new QComboBox(this);
    m_combo->setFrame(0);
    m_combo->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    m_combo->setEditable(false);
    layout->addWidget(m_combo);
    m_button = new QToolButton(this);
    m_button->setIcon(createIconSet(QLatin1String("fileopen.png")));
    m_button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
    m_button->setFixedWidth(20);
    layout->addWidget(m_button);
    connect(m_button, SIGNAL(clicked()), this, SLOT(showDialog()));
    connect(m_combo, SIGNAL(activated(int)), this, SLOT(comboActivated(int)));

    populateCombo();
}

void GraphicsPropertyEditor::comboActivated(int idx)
{
    if (m_mode == Icon) {
        setIcon(qvariant_cast<QIcon>(m_combo->itemData(idx)));
    } else {
        setPixmap(qvariant_cast<QPixmap>(m_combo->itemData(idx)));
    }
}

int GraphicsPropertyEditor::indexOfIcon(const QIcon &icon)
{
    if (m_mode == Pixmap)
        return -1;

    if (icon.isNull())
        return 0;

    for (int i = 1; i < m_combo->count(); ++i) {
        if (qvariant_cast<QIcon>(m_combo->itemData(i)).serialNumber() == icon.serialNumber())
            return i;
    }

    populateCombo();

    for (int i = 1; i < m_combo->count(); ++i) {
        if (qvariant_cast<QIcon>(m_combo->itemData(i)).serialNumber() == icon.serialNumber())
            return i;
    }

    return -1;
}

int GraphicsPropertyEditor::indexOfPixmap(const QPixmap &pixmap)
{
    if (m_mode == Icon)
        return -1;

    if (pixmap.isNull())
        return 0;

    for (int i = 1; i < m_combo->count(); ++i) {
        if (qvariant_cast<QPixmap>(m_combo->itemData(i)).serialNumber() == pixmap.serialNumber())
            return i;
    }

    populateCombo();

    for (int i = 1; i < m_combo->count(); ++i) {
        if (qvariant_cast<QPixmap>(m_combo->itemData(i)).serialNumber() == pixmap.serialNumber())
            return i;
    }

    return -1;
}

void GraphicsPropertyEditor::populateCombo()
{
    QDesignerFormWindowInterface *form = m_core->formWindowManager()->activeFormWindow();
    if (form == 0)
        return;
    const QStringList qrc_list = form->resourceFiles();

    m_combo->clear();

    QDesignerIconCacheInterface *cache = m_core->iconCache();
    if (m_mode == Icon) {
        m_combo->addItem(tr("<no icon>"));
        const QList<QIcon> icon_list = cache->iconList();
        foreach (QIcon icon, icon_list) {
            const QString qrc_path = cache->iconToQrcPath(icon);
            if (!qrc_path.isEmpty() && !qrc_list.contains(qrc_path))
                continue;
            m_combo->addItem(icon, QFileInfo(cache->iconToFilePath(icon)).fileName(),
                                QVariant(icon));
        }
    } else {
        m_combo->addItem(tr("<no pixmap>"));
        const QList<QPixmap> pixmap_list = cache->pixmapList();
        foreach (QPixmap pixmap, pixmap_list) {
            const QString qrc_path = cache->iconToQrcPath(pixmap);
            if (!qrc_path.isEmpty() && !qrc_list.contains(qrc_path))
                continue;
            m_combo->addItem(QIcon(pixmap),
                                QFileInfo(cache->pixmapToFilePath(pixmap)).fileName(),
                                QVariant(pixmap));
        }
    }
    const bool blocked = m_combo->blockSignals(true);
    m_combo->setCurrentIndex(0);
    m_combo->blockSignals(blocked);
}

GraphicsPropertyEditor::GraphicsPropertyEditor(QDesignerFormEditorInterface *core, const QIcon &pm,
                                                QWidget *parent)
    : QWidget(parent),
      m_mode(Icon),
      m_core(core)
{
    init();
    setIcon(pm);
}

GraphicsPropertyEditor::GraphicsPropertyEditor(QDesignerFormEditorInterface *core, const QPixmap &pm,
                                                QWidget *parent)
    : QWidget(parent),
      m_mode(Pixmap),
      m_core(core)
{
    init();
    setPixmap(pm);
}

void GraphicsPropertyEditor::showDialog()
{
    QDesignerFormWindowInterface *form = m_core->formWindowManager()->activeFormWindow();
    if (form == 0)
        return;

    QString file_path;
    QString qrc_path;

    if (m_mode == Icon && !m_icon.isNull()) {
        file_path = m_core->iconCache()->iconToFilePath(m_icon);
        qrc_path = m_core->iconCache()->iconToQrcPath(m_icon);
    } else if (!m_pixmap.isNull()) {
        file_path = m_core->iconCache()->pixmapToFilePath(m_pixmap);
        qrc_path = m_core->iconCache()->pixmapToQrcPath(m_pixmap);
    }

    FindIconDialog dialog(form, 0);
    dialog.setPaths(qrc_path, file_path);
    if (dialog.exec()) {
        file_path = dialog.filePath();
        qrc_path = dialog.qrcPath();
        if (!file_path.isEmpty()) {
            populateCombo();
            if (m_mode == Icon) {
                const QIcon icon = m_core->iconCache()->nameToIcon(file_path, qrc_path);
                populateCombo();
                setIcon(icon);
            } else {
                const QPixmap pixmap = m_core->iconCache()->nameToPixmap(file_path, qrc_path);
                populateCombo();
                setPixmap(pixmap);
            }
        }
    }
}

void GraphicsPropertyEditor::setIcon(const QIcon &pm)
{
    if (m_mode == Pixmap)
        return;

    if (pm.isNull() && m_icon.isNull())
        return;
    if (pm.serialNumber() == m_icon.serialNumber())
        return;

    m_icon = pm;

    const bool blocked = m_combo->blockSignals(true);
    m_combo->setCurrentIndex(indexOfIcon(m_icon));
    m_combo->blockSignals(blocked);

    emit iconChanged(m_icon);
}

void GraphicsPropertyEditor::setPixmap(const QPixmap &pm)
{
    if (m_mode == Icon)
        return;

    if (pm.isNull() && m_pixmap.isNull())
        return;
    if (pm.serialNumber() == m_pixmap.serialNumber())
        return;

    m_pixmap = pm;

    const bool blocked = m_combo->blockSignals(true);
    m_combo->setCurrentIndex(indexOfPixmap(m_pixmap));
    m_combo->blockSignals(blocked);

    emit pixmapChanged(m_pixmap);
}

}  // namespace qdesigner_internal

IconProperty::IconProperty(QDesignerFormEditorInterface *core, const QIcon &value, const QString &name)
    : AbstractProperty<QIcon>(value, name),
      m_core(core)
{
}

void IconProperty::setValue(const QVariant &value)
{
    m_value = qvariant_cast<QIcon>(value);
}

QString IconProperty::toString() const
{
    const QString path = m_core->iconCache()->iconToFilePath(m_value);
    return QFileInfo(path).fileName();
}

QVariant IconProperty::decoration() const
{
    static QIcon empty_icon;
    if (empty_icon.isNull())
        empty_icon = QIcon(QLatin1String(":/trolltech/formeditor/images/emptyicon.png"));

    if (m_value.isNull())
        return qVariantFromValue(empty_icon);
    return qVariantFromValue(m_value);
}

QWidget *IconProperty::createEditor(QWidget *parent, const QObject *target,
                                        const char *receiver) const
{
    GraphicsPropertyEditor *editor = new GraphicsPropertyEditor(m_core, m_value, parent);

    QObject::connect(editor, SIGNAL(iconChanged(QIcon)), target, receiver);

    return editor;
}

void IconProperty::updateEditorContents(QWidget *editor)
{
    if (GraphicsPropertyEditor *ed = qobject_cast<GraphicsPropertyEditor*>(editor)) {
        ed->setIcon(m_value);
    }
}

void IconProperty::updateValue(QWidget *editor)
{
    if (GraphicsPropertyEditor *ed = qobject_cast<GraphicsPropertyEditor*>(editor)) {
        const QIcon newValue = ed->icon();

        if (newValue.serialNumber() != m_value.serialNumber()) {
            m_value = newValue;
            setChanged(true);
        }
    }
}

PixmapProperty::PixmapProperty(QDesignerFormEditorInterface *core, const QPixmap &pixmap, const QString &name)
    : AbstractProperty<QPixmap>(pixmap, name),
      m_core(core)
{
}

void PixmapProperty::setValue(const QVariant &value)
{
    m_value = qvariant_cast<QPixmap>(value);
}

QString PixmapProperty::toString() const
{
    const QString path = m_core->iconCache()->pixmapToFilePath(m_value);
    return QFileInfo(path).fileName();
}

QVariant PixmapProperty::decoration() const
{
    static QIcon empty_icon;
    if (empty_icon.isNull())
        empty_icon = QIcon(QLatin1String(":/trolltech/formeditor/images/emptyicon.png"));

    if (m_value.isNull())
        return qVariantFromValue(empty_icon);
    return qVariantFromValue(QIcon(m_value));
}

QWidget *PixmapProperty::createEditor(QWidget *parent, const QObject *target, const char *receiver) const
{
    GraphicsPropertyEditor *editor = new GraphicsPropertyEditor(m_core, m_value, parent);

    QObject::connect(editor, SIGNAL(pixmapChanged(QPixmap)), target, receiver);

    return editor;
}

void PixmapProperty::updateEditorContents(QWidget *editor)
{
    if (GraphicsPropertyEditor *ed = qobject_cast<GraphicsPropertyEditor*>(editor)) {
        ed->setPixmap(m_value);
    }
}

void PixmapProperty::updateValue(QWidget *editor)
{
    if (GraphicsPropertyEditor *ed = qobject_cast<GraphicsPropertyEditor*>(editor)) {
        QPixmap newValue = ed->pixmap();

        if (newValue.serialNumber() != m_value.serialNumber()) {
            m_value = newValue;
            setChanged(true);
        }
    }
}

// -------------------------------------------------------------------------
PaletteProperty::PaletteProperty(QDesignerFormEditorInterface *core, const QPalette &value, QWidget *selectedWidget,
                const QString &name)
    : AbstractProperty<QPalette>(value, name),
      m_core(core),
      m_selectedWidget(selectedWidget)
{
}

void PaletteProperty::setValue(const QVariant &value)
{
    m_value = qvariant_cast<QPalette>(value);
    QPalette parentPalette = QPalette();
    if (m_selectedWidget) {
        if (m_selectedWidget->isWindow())
            parentPalette = QApplication::palette(m_selectedWidget);
        else {
            if (m_selectedWidget->parentWidget())
                parentPalette = m_selectedWidget->parentWidget()->palette();
        }
    }
    const uint mask = m_value.resolve();
    m_value = m_value.resolve(parentPalette);
    m_value.resolve(mask);
}

QString PaletteProperty::toString() const
{
    return QString(); // ### implement me
}

QWidget *PaletteProperty::createEditor(QWidget *parent, const QObject *target, const char *receiver) const
{
    PaletteEditorButton *btn = new PaletteEditorButton(m_core, m_value, m_selectedWidget, parent);
    QObject::connect(btn, SIGNAL(changed()), target, receiver);
    return btn;
}

void PaletteProperty::updateEditorContents(QWidget *editor)
{
    if (PaletteEditorButton *btn = qobject_cast<PaletteEditorButton*>(editor)) {
        btn->setPalette(m_value);
    }
}

void PaletteProperty::updateValue(QWidget *editor)
{
    if (PaletteEditorButton *btn = qobject_cast<PaletteEditorButton*>(editor)) {
        const QPalette newValue = btn->palette();

        if (newValue.resolve() != m_value.resolve() || newValue != m_value) {
            m_value = newValue;
            setChanged(true);
        }
    }
}


// -------------------------------------------------------------------------------------

struct Group
{
    QString name;
    QList<IProperty*> properties;

    inline Group() {}
    inline Group(const QString &n): name(n) {}

    inline bool operator == (const Group &other) const
    { return name == other.name; }
};


// A pair <ValidationMode, bool hasComment>.
typedef QPair<TextPropertyValidationMode, bool> StringPropertyParameters;
    
StringPropertyParameters textPropertyValidationMode(const QObject *object,const QString &pname,
                                                    QVariant::Type type, bool isMainContainer)   
{
    // Legacy: buddy comes along as ByteArray for some reason. Else we do not know.
    if (type == QVariant::ByteArray) {
        if (pname == QLatin1String("buddy"))
            return StringPropertyParameters(ValidationObjectName, false);
        return StringPropertyParameters(ValidationMultiLine, false);
    }
    // object name - no comment
    if (pname == QLatin1String("objectName")) {
        const TextPropertyValidationMode vm =  isMainContainer ? ValidationObjectNameScope : ValidationObjectName;
        return StringPropertyParameters(vm, false);
    }

    // Any names
    if (pname == QLatin1String("buddy") || pname.endsWith(QLatin1String("Name")))
        return StringPropertyParameters(ValidationObjectName, true);
        
    // Multi line?
    if (pname == QLatin1String("styleSheet")) 
        return StringPropertyParameters(ValidationStyleSheet, true);
    
    if (pname == QLatin1String("styleSheet")     || pname == QLatin1String("toolTip")   || 
        pname.endsWith(QLatin1String("ToolTip")) || pname == QLatin1String("whatsThis") ||
        pname == QLatin1String("iconText")       || pname == QLatin1String("windowIconText")  ||
        pname == QLatin1String("html")           || pname == QLatin1String("accessibleDescription"))
        return StringPropertyParameters(ValidationMultiLine, true);


    // text only if not Action, LineEdit
    if (pname == QLatin1String("text") && !(qobject_cast<const QAction *>(object) || qobject_cast<const QLineEdit *>(object)))
        return StringPropertyParameters(ValidationMultiLine, true);

    // default to single
    return StringPropertyParameters(ValidationSingleLine, true);    
}


// Create a string prop with proper validation mode
StringProperty* PropertyEditor::createStringProperty(QObject *object, const QString &pname, const QVariant &value, bool isMainContainer) const 
{
    const StringPropertyParameters params = textPropertyValidationMode(object, pname, value.type(), isMainContainer);
    // Does a meta DB entry exist - add comment
    const bool hasComment = params.second && metaDataBaseItem();
    const QString comment = hasComment ? propertyComment(m_core, object, pname) : QString();
    const QString stringValue = value.type() == QVariant::ByteArray ? QString::fromUtf8(value.toByteArray()) : value.toString();
    return new StringProperty(stringValue, pname, params.first, hasComment, comment );
}

QDesignerMetaDataBaseItemInterface* PropertyEditor::metaDataBaseItem() const 
{
    QObject *o = object();
    if (!o) 
        return 0;
    QDesignerMetaDataBaseInterface *db = core()->metaDataBase();
    if (!db) 
        return 0;
    return db->item(o);
}

void PropertyEditor::createPropertySheet(PropertyCollection *root, QObject *object)
{
    QList<Group> groups;

    QExtensionManager *m = m_core->extensionManager();
    QDesignerLanguageExtension *lang = qt_extension<QDesignerLanguageExtension*> (m, m_core);

    bool isMainContainer = false;
    if (QWidget *widget = qobject_cast<QWidget*>(object)) {
        if (QDesignerFormWindowInterface *fw = QDesignerFormWindowInterface::findFormWindow(widget)) {
            isMainContainer = (fw->mainContainer() == widget);
        }
    }
    m_prop_sheet = qobject_cast<QDesignerPropertySheetExtension*>(m->extension(object, Q_TYPEID(QDesignerPropertySheetExtension)));
    for (int i=0; i<m_prop_sheet->count(); ++i) {
        if (!m_prop_sheet->isVisible(i))
            continue;

        const QString pname = m_prop_sheet->propertyName(i);
        const QVariant value = m_prop_sheet->property(i);

        IProperty *p = 0;
        if (qVariantCanConvert<FlagType>(value)) {
            FlagType f = qvariant_cast<FlagType>(value);

            if (pname == QLatin1String("alignment")) {                                                
                    p = new AlignmentProperty(f.items, Qt::Alignment(f.value.toInt()), pname);                
            } else {
                if (lang) {
                    QMap<QString, QVariant> items;
                    QMapIterator<QString, QVariant> it (f.items);
                    while (it.hasNext()) {
                        it.next();
                        const QString id = lang->enumerator(it.key());
                        items.insert(id, it.value());
                    }
                    f.items = items;
                }

                p = new FlagsProperty(f.items, f.value.toInt(), pname);
            }
        } else if (qVariantCanConvert<EnumType>(value)) {
            EnumType e = qvariant_cast<EnumType>(value);

            if (lang) {
                QMap<QString, QVariant> items;
                QMapIterator<QString, QVariant> it (e.items);
                e.names.clear();
                while (it.hasNext()) {
                    it.next();
                    QString id = lang->enumerator(it.key());
                    items.insert(id, it.value());
                    e.names.append(id);
                }
                e.items = items;
            }

            p = new MapProperty(e.items, e.value, pname, e.names);
        }

        if (!p) {
            switch (value.type()) {
            case 0:
                p = createSpecialProperty(value, pname);
                break;
            case QVariant::Int:
                p = new IntProperty(value.toInt(), pname);
                break;
            case QVariant::UInt:
                p = new UIntProperty(value.toUInt(), pname);
                break;
            case QVariant::LongLong:
                p = new LongLongProperty(value.toLongLong(), pname);
                break;
            case QVariant::ULongLong:
                p = new ULongLongProperty(value.toULongLong(), pname);
                break;
            case QVariant::Double:
                p = new DoubleProperty(value.toDouble(), pname);
                break;
            case QVariant::Char:
                p = new CharProperty(value.toChar(), pname);
                break;
            case QVariant::Bool:
                p = new BoolProperty(value.toBool(), pname);
                break;
            case QVariant::ByteArray:
            case QVariant::String: 
                p = createStringProperty(object, pname, value, isMainContainer);
                break;
            case QVariant::Size:
                p = new SizeProperty(value.toSize(), pname);
                break;
            case QVariant::SizeF:
                p = new SizeFProperty(value.toSizeF(), pname);
                break;
            case QVariant::Point:
                p = new PointProperty(value.toPoint(), pname);
                break;
            case QVariant::PointF:
                p = new PointFProperty(value.toPointF(), pname);
                break;
            case QVariant::Rect:
                p = new RectProperty(value.toRect(), pname);
                break;
            case QVariant::RectF:
                p = new RectFProperty(value.toRectF(), pname);
                break;
            case QVariant::Icon:
                p = new IconProperty(m_core, qvariant_cast<QIcon>(value), pname);
                break;
            case QVariant::Pixmap:
                p = new PixmapProperty(m_core, qvariant_cast<QPixmap>(value), pname);
                break;
            case QVariant::Font:
                p = new FontProperty(qvariant_cast<QFont>(value), pname, qobject_cast<QWidget *>(object));
                break;
            case QVariant::Color:
                p = new ColorProperty(qvariant_cast<QColor>(value), pname);
                break;
            case QVariant::SizePolicy:
                p = new SizePolicyProperty(qvariant_cast<QSizePolicy>(value), pname);
                break;
            case QVariant::DateTime:
                p = new DateTimeProperty(value.toDateTime(), pname);
                break;
            case QVariant::Date:
                p = new DateProperty(value.toDate(), pname);
                break;
            case QVariant::Time:
                p = new TimeProperty(value.toTime(), pname);
                break;
            case QVariant::Cursor:
                p = new CursorProperty(qvariant_cast<QCursor>(value), pname);
                break;
            case QVariant::KeySequence:
                p = createStringProperty(object, pname, qvariant_cast<QKeySequence>(value), isMainContainer);
                break;
            case QVariant::Palette:
                p = new PaletteProperty(m_core, qvariant_cast<QPalette>(value),
                                qobject_cast<QWidget *>(object), pname);
                break;
            case QVariant::Url:
                p = new UrlProperty(value.toUrl(), pname);
                break;
            case QVariant::StringList:
                p = new StringListProperty(qvariant_cast<QStringList>(value), pname);
                break;
            default:
                // ### qWarning() << "property" << pname << "with type" << value.type() << "not supported yet!";
                break;
            } // end switch
        }

        if (p != 0) {
            p->setHasReset(m_prop_sheet->hasReset(i));
            p->setChanged(m_prop_sheet->isChanged(i));
            p->setDirty(false);

            const QString pgroup = m_prop_sheet->propertyGroup(i);
            int groupIndex = groups.indexOf(pgroup);
            if (groupIndex == -1) {
                groupIndex = groups.count();
                groups.append(Group(pgroup));
            }

            QList<IProperty*> &groupProperties = groups[groupIndex].properties;
            groupProperties.append(p);
        }
    }

    foreach (Group g, groups) {
        root->addProperty(new SeparatorProperty(QString(), g.name));
        foreach (IProperty *p, g.properties) {
            root->addProperty(p);
        }
    }
}

PropertyEditor::PropertyEditor(QDesignerFormEditorInterface *core,
            QWidget *parent, Qt::WindowFlags flags)
    : QDesignerPropertyEditor(parent, flags),
      m_core(core),
      m_editor(new QPropertyEditor(this)),
      m_properties(0),
      m_prop_sheet(0)
{
    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setMargin(0);
    lay->addWidget(m_editor);

    connect(m_editor, SIGNAL(propertyChanged(IProperty*)),
        this, SLOT(slotFirePropertyChanged(IProperty*)));
    connect(m_editor->editorModel(), SIGNAL(resetProperty(QString)),
                this, SLOT(slotResetProperty(QString)));

    m_editor->viewport()->installEventFilter(this);
}

PropertyEditor::~PropertyEditor()
{
}

bool PropertyEditor::isReadOnly() const
{
    return m_editor->isReadOnly();
}

void PropertyEditor::setReadOnly(bool readOnly)
{
    m_editor->setReadOnly(readOnly);
}

QDesignerFormEditorInterface *PropertyEditor::core() const
{
    return m_core;
}

IProperty *PropertyEditor::propertyByName(IProperty *p, const QString &name)
{
    if (p->propertyName() == name)
        return p;

    if (p->kind() == IProperty::Property_Group) {
        IPropertyGroup *g = static_cast<IPropertyGroup*>(p);
        for (int i=0; i<g->propertyCount(); ++i)
            if (IProperty *c = propertyByName(g->propertyAt(i), name))
                return c;
    }

    return 0;
}

void PropertyEditor::setPropertyValue(const QString &name, const QVariant &value, bool changed)
{
    if (isReadOnly())
        return;
    
    IProperty *p = propertyByName(m_editor->initialInput(), name);
    if (!p)
        return;

    if (p->value() != value) 
        p->setValue(value);
    
    p->setChanged(changed);
    p->setDirty(false);
    
    m_editor->editorModel()->refresh(p);
}

void PropertyEditor::setPropertyComment(const QString &name, const QString &value)
{
    if (isReadOnly())
        return;

    IProperty *parent = propertyByName(m_editor->initialInput(), name);
    if (!parent || parent->kind() != IProperty::Property_Group)
        return;
    
    AbstractPropertyGroup *parentGroup = static_cast<AbstractPropertyGroup *>(parent);
    
    if (parentGroup->propertyCount() != 1)
        return;
    
    IProperty *commentProperty = parentGroup->propertyAt(0);
    if (commentProperty->value().toString() != value)
        commentProperty->setValue(value);
    
    commentProperty->setDirty(false);

    m_editor->editorModel()->refresh(commentProperty);    
}

void PropertyEditor::slotFirePropertyChanged(IProperty *p)
{
    if (isReadOnly() || !object())
        return;

    // Comment or property
    if (p->parent() && p->propertyName() == QLatin1String("comment")) {
        const QString parentProperty = p->parent()->propertyName();
        emit propertyCommentChanged(parentProperty, p->value().toString());
    } else {
        emit propertyChanged(p->propertyName(), p->value());
    }
}

void PropertyEditor::clearDirty(IProperty *p)
{
    p->setDirty(false);

    if (p->kind() == IProperty::Property_Normal)
        return;

    IPropertyGroup *g = static_cast<IPropertyGroup*>(p);
    for (int i=0; i<g->propertyCount(); ++i)
        clearDirty(g->propertyAt(i));
}

void PropertyEditor::setObject(QObject *object)
{
    if (m_editor->initialInput())
        clearDirty(m_editor->initialInput());

    m_object = object;
    if (QAction *action = qobject_cast<QAction*>(m_object)) {
        if (action->menu())
            m_object = action->menu();
    }

    IPropertyGroup *old_properties = m_properties;
    m_properties = 0;
    m_prop_sheet = 0;

    if (m_object) {
        PropertyCollection *collection = new PropertyCollection(QLatin1String("<root>"));
        createPropertySheet(collection, object);
        m_properties = collection;
    }

    m_editor->setInitialInput(m_properties);

    delete old_properties;
}

void PropertyEditor::slotResetProperty(const QString &prop_name)
{
    QDesignerFormWindowInterface *form = m_core->formWindowManager()->activeFormWindow();
    if (form == 0) {
        qWarning("PropertyEditor::resetProperty(): widget does not belong to any form");
        return;
    }
    emit resetProperty(prop_name);
}

QString PropertyEditor::currentPropertyName() const
{
    const QModelIndex index = m_editor->selectionModel()->currentIndex();
    if (index.isValid()) {
        IProperty *property = static_cast<IProperty*>(index.internalPointer());

        while (property && property->isFake())
            property = property->parent();

        if (property)
            return property->propertyName();
    }

    return QString();
}

bool PropertyEditor::eventFilter(QObject *object, QEvent *event)
{
    bool res = QDesignerPropertyEditor::eventFilter(object, event);
    if (object != m_editor->viewport() || event->type() != QEvent::ContextMenu)
        return res;

    QContextMenuEvent *cme = (QContextMenuEvent *)event;

    QModelIndex idx = m_editor->indexAt(cme->pos());
    QPropertyEditorModel *model = m_editor->editorModel();
    IProperty *nonfake = model->privateData(idx);
    while (nonfake != 0 && nonfake->isFake())
        nonfake = nonfake->parent();

    QDesignerPropertySheetExtension *sheet = qt_extension<QDesignerPropertySheetExtension*>(m_core->extensionManager(), m_object);
    if (!sheet)
        return res;

    int index = -1;
    bool addEnabled = false;
    bool insertRemoveEnabled = false;
    if (sheet->dynamicPropertiesAllowed()) {
        addEnabled = true;
        if (nonfake) {
            int idx = sheet->indexOf(nonfake->propertyName());
            if (sheet->isDynamicProperty(idx)) {
                insertRemoveEnabled = true;
                index = idx;
            }
        }
    }

    QMenu menu(this);
    QAction *addAction = menu.addAction(tr("Add Dynamic Property..."));
    addAction->setEnabled(addEnabled);
    QAction *insertAction = menu.addAction(tr("Insert Dynamic Property..."));
    insertAction->setEnabled(insertRemoveEnabled);
    QAction *removeAction = menu.addAction(tr("Remove Dynamic Property"));
    removeAction->setEnabled(insertRemoveEnabled);
    QAction *result = menu.exec(cme->globalPos());

    if (result == removeAction && nonfake) {
        RemoveDynamicPropertyCommand *cmd = new RemoveDynamicPropertyCommand(m_core->formWindowManager()->activeFormWindow());
        cmd->init(m_object, nonfake->propertyName());
        m_core->formWindowManager()->activeFormWindow()->commandHistory()->push(cmd);
    } else if (result == addAction || result == insertAction) {
        NewDynamicPropertyDialog dlg(this);
        QStringList reservedNames;
        for (int i = 0; i < sheet->count(); i++)
            reservedNames.append(sheet->propertyName(i));
        dlg.setReservedNames(reservedNames);
        if (dlg.exec() == QDialog::Accepted) {
            QString newName = dlg.propertyName();
            QVariant newValue = dlg.propertyValue();

            InsertDynamicPropertyCommand *cmd = new InsertDynamicPropertyCommand(m_core->formWindowManager()->activeFormWindow());
            cmd->init(m_object, newName, newValue, result == insertAction ? index : -1);
            m_core->formWindowManager()->activeFormWindow()->commandHistory()->push(cmd);
        }
    }
    return res;
}


#include "propertyeditor.moc"

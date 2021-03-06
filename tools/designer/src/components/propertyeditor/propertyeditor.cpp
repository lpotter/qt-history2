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
#include "newdynamicpropertydialog.h"
#include "dynamicpropertysheet.h"
#include "shared_enums_p.h"

// sdk
#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerFormWindowManagerInterface>
#include <QtDesigner/QExtensionManager>
#include <QtDesigner/QDesignerPropertySheetExtension>
#include <QtDesigner/QDesignerWidgetDataBaseInterface>
// shared
#include <qdesigner_utils_p.h>
#include <qdesigner_propertycommand_p.h>
#include <metadatabase_p.h>

#include <QtGui/QAction>
#include <QtGui/QLineEdit>
#include <QtGui/QMenu>
#include <QtGui/QApplication>
#include <QtGui/QVBoxLayout>
#include <QtGui/QScrollArea>
#include <QtGui/QStackedWidget>
#include <QtGui/QToolBar>
#include <QtGui/QToolButton>
#include <QtGui/QActionGroup>
#include <QtGui/QLabel>

#include <QtCore/QSignalMapper>

#include "qttreepropertybrowser.h"
#include "qtbuttonpropertybrowser.h"
#include "qtvariantproperty.h"
#include "designerpropertymanager.h"
#include "qdesigner_propertysheet_p.h"

#include <iconloader_p.h>

#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

// ---------------------------------------------------------------------------------

namespace qdesigner_internal {

// A pair <ValidationMode, bool hasComment>.
typedef QPair<TextPropertyValidationMode, bool> StringPropertyParameters;

// Return a pair of validation mode and flag indicating whether property has a comment
// for textual properties.

StringPropertyParameters textPropertyValidationMode(const QObject *object,const QString &propertyName,
                                                    QVariant::Type type, bool isMainContainer)
{
    if (type == QVariant::ByteArray) {
        return StringPropertyParameters(ValidationMultiLine, false);
    }
    // object name - no comment
    if (propertyName == QLatin1String("objectName")) {
        const TextPropertyValidationMode vm =  isMainContainer ? ValidationObjectNameScope : ValidationObjectName;
        return StringPropertyParameters(vm, false);
    }

    // Accessibility. Both are texts the narrator reads
    if (propertyName == QLatin1String("accessibleDescription") || propertyName == QLatin1String("accessibleName"))
        return StringPropertyParameters(ValidationMultiLine, true);

    // Any names
    if (propertyName == QLatin1String("buddy") || propertyName.endsWith(QLatin1String("Name")))
        return StringPropertyParameters(ValidationObjectName, false);

    // Multi line?
    if (propertyName == QLatin1String("styleSheet"))
        return StringPropertyParameters(ValidationStyleSheet, false);

    if (propertyName == QLatin1String("styleSheet")     || propertyName == QLatin1String("toolTip")   ||
        propertyName.endsWith(QLatin1String("ToolTip")) || propertyName == QLatin1String("whatsThis") ||
        propertyName == QLatin1String("iconText")       || propertyName == QLatin1String("windowIconText")  ||
        propertyName == QLatin1String("html"))
        return StringPropertyParameters(ValidationMultiLine, true);


    // text only if not Action, LineEdit
    if (propertyName == QLatin1String("text") && !(qobject_cast<const QAction *>(object) || qobject_cast<const QLineEdit *>(object)))
        return StringPropertyParameters(ValidationMultiLine, true);

    // default to single
    return StringPropertyParameters(ValidationSingleLine, true);
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

void PropertyEditor::setupStringProperty(QtVariantProperty *property,
                const QString &propertyName, const QVariant &value, bool isMainContainer)
{
    const StringPropertyParameters params = textPropertyValidationMode(m_object, propertyName, value.type(), isMainContainer);
    // Does a meta DB entry exist - add comment
    const bool hasComment = params.second && metaDataBaseItem();
    const QString stringValue = value.type() == QVariant::ByteArray ? QString::fromUtf8(value.toByteArray()) : value.toString();
    property->setAttribute(QLatin1String("validationMode"), params.first);
    // assuming comment cannot appear or disappear for the same property in different object instance
    if (hasComment && !m_propertyToComment.contains(property)) {
        QtVariantProperty *commentProperty = m_propertyManager->addProperty(QVariant::String, tr("comment"));
        commentProperty->setToolTip(tr("comment"));
        commentProperty->setValue(propertyComment(m_core, m_object, propertyName));
        property->addSubProperty(commentProperty);
        m_propertyToComment[property] = commentProperty;
        m_commentToProperty[commentProperty] = property;
    }
}

void PropertyEditor::setupPaletteProperty(QtVariantProperty *property)
{
    QPalette value = qvariant_cast<QPalette>(property->value());
    QPalette superPalette = QPalette();
    QWidget *currentWidget = qobject_cast<QWidget *>(m_object);
    if (currentWidget) {
        if (currentWidget->isWindow())
            superPalette = QApplication::palette(currentWidget);
        else {
            if (currentWidget->parentWidget())
                superPalette = currentWidget->parentWidget()->palette();
        }
    }
    m_updatingBrowser = true;
    property->setAttribute(QLatin1String("superPalette"), superPalette);
    m_updatingBrowser = false;
}

PropertyEditor::PropertyEditor(QDesignerFormEditorInterface *core, QWidget *parent, Qt::WindowFlags flags)
    : QDesignerPropertyEditor(parent, flags), m_core(core), m_propertySheet(0)
{
    m_stackedWidget = new QStackedWidget(this);
    m_sorting = false;
    m_coloring = false;

    m_colors[0] =  QColor(255, 230, 191);
    m_colors[1] =  QColor(255, 255, 191);
    m_colors[2] =  QColor(191, 255, 191);
    m_colors[3] =  QColor(199, 255, 255);
    m_colors[4] =  QColor(234, 191, 255);
    m_colors[5] =  QColor(255, 191, 239);
    m_dynamicColor = QColor(191, 207, 255);
    m_layoutColor = QColor(255, 191, 191);

    m_dynamicGroup = 0;

    QToolBar *toolBar = new QToolBar(this);

    QActionGroup *actionGroup = new QActionGroup(this);
    m_treeAction = new QAction(tr("Tree View"), this);
    m_treeAction->setCheckable(true);
    m_treeAction->setIcon(createIconSet(QLatin1String("widgets/listview.png")));
    m_buttonAction = new QAction(tr("Drop Down Button View"), this);
    m_buttonAction->setCheckable(true);
    m_buttonAction->setIcon(createIconSet(QLatin1String("dropdownbutton.png")));

    actionGroup->addAction(m_treeAction);
    actionGroup->addAction(m_buttonAction);
    m_treeAction->setChecked(true);
    connect(actionGroup, SIGNAL(triggered(QAction *)),
                this, SLOT(slotViewTriggered(QAction *)));

    QWidget *classWidget = new QWidget(toolBar);
    classWidget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    m_classLabel = new QLabel(classWidget);
    QHBoxLayout *l = new QHBoxLayout(classWidget);
    l->setMargin(0);
    l->addWidget(m_classLabel);
    m_classLabel->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed));

    m_addDynamicAction = new QAction(tr("Add Dynamic Property..."), this);
    m_addDynamicAction->setIcon(createIconSet(QLatin1String("plus.png")));
    m_addDynamicAction->setEnabled(false);
    connect(m_addDynamicAction, SIGNAL(triggered()), this, SLOT(slotAddDynamicProperty()));
    QToolButton *removeButton = new QToolButton(toolBar);
    m_removeDynamicAction = new QAction(tr("Remove Dynamic Property"), this);
    m_removeDynamicAction->setIcon(createIconSet(QLatin1String("minus.png")));
    m_removeDynamicAction->setEnabled(false);
    m_removeDynamicMenu = new QMenu(this);
    m_removeDynamicAction->setMenu(m_removeDynamicMenu);
    removeButton->setDefaultAction(m_removeDynamicAction);
    removeButton->setPopupMode(QToolButton::InstantPopup);

    QToolButton *configureButton = new QToolButton(toolBar);
    QAction *configureAction = new QAction(tr("Configure Property Editor"), this);
    configureAction->setIcon(createIconSet(QLatin1String("widgets/toolbutton.png")));
    QMenu *configureMenu = new QMenu(this);
    configureAction->setMenu(configureMenu);
    configureButton->setDefaultAction(configureAction);
    configureButton->setPopupMode(QToolButton::InstantPopup);

    m_sortingAction = new QAction(createIconSet(QLatin1String("sort.png")), tr("Sorting"), this);
    m_sortingAction->setCheckable(true);
    connect(m_sortingAction, SIGNAL(toggled(bool)), this, SLOT(slotSorting(bool)));

    m_coloringAction = new QAction(createIconSet(QLatin1String("color.png")), tr("Color Groups"), this);
    m_coloringAction->setCheckable(true);
    connect(m_coloringAction, SIGNAL(toggled(bool)), this, SLOT(slotColoring(bool)));

    m_removeMapper = new QSignalMapper(this);
    connect(m_removeMapper, SIGNAL(mapped(const QString &)), this, SIGNAL(removeDynamicProperty(const QString &)));

    toolBar->addWidget(classWidget);
    toolBar->addAction(m_addDynamicAction);
    toolBar->addWidget(removeButton);
    toolBar->addSeparator();
    toolBar->addWidget(configureButton);

    configureMenu->addAction(m_sortingAction);
    configureMenu->addAction(m_coloringAction);
    configureMenu->addSeparator();
    configureMenu->addAction(m_treeAction);
    configureMenu->addAction(m_buttonAction);

    QScrollArea *buttonScroll = new QScrollArea(m_stackedWidget);
    m_buttonBrowser = new QtButtonPropertyBrowser(buttonScroll);
    buttonScroll->setWidgetResizable(true);
    buttonScroll->setWidget(m_buttonBrowser);
    m_buttonIndex = m_stackedWidget->addWidget(buttonScroll);

    m_treeBrowser = new QtTreePropertyBrowser(m_stackedWidget);
    m_treeBrowser->setRootIsDecorated(false);
    m_treeBrowser->setMarkPropertiesWithoutValue(true);
    m_treeBrowser->setResizeMode(QtTreePropertyBrowser::Interactive);
    m_treeIndex = m_stackedWidget->addWidget(m_treeBrowser);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(toolBar);
    layout->addWidget(m_stackedWidget);
    layout->setMargin(0);
    layout->setSpacing(0);

    m_propertyManager = new DesignerPropertyManager(m_core, this);
    DesignerEditorFactory *treeFactory = new DesignerEditorFactory(m_core, this);
    treeFactory->setSpacing(0);
    DesignerEditorFactory *groupFactory = new DesignerEditorFactory(m_core, this);
    QtVariantPropertyManager *variantManager = m_propertyManager;
    m_buttonBrowser->setFactoryForManager(variantManager, groupFactory);
    m_treeBrowser->setFactoryForManager(variantManager, treeFactory);

    m_stackedWidget->setCurrentIndex(m_treeIndex);
    m_currentBrowser = m_treeBrowser;

    connect(groupFactory, SIGNAL(resetProperty(QtProperty *)), this, SLOT(slotResetProperty(QtProperty *)));
    connect(treeFactory, SIGNAL(resetProperty(QtProperty *)), this, SLOT(slotResetProperty(QtProperty *)));
    connect(variantManager, SIGNAL(valueChanged(QtProperty *, const QVariant &)),
                this, SLOT(slotValueChanged(QtProperty *, const QVariant &)));

    m_updatingBrowser = false;
}

PropertyEditor::~PropertyEditor()
{

}

void PropertyEditor::setExpanded(QtBrowserItem *item, bool expanded)
{
    if (m_buttonBrowser == m_currentBrowser)
        m_buttonBrowser->setExpanded(item, expanded);
    else if (m_treeBrowser == m_currentBrowser)
        m_treeBrowser->setExpanded(item, expanded);
}

bool PropertyEditor::isExpanded(QtBrowserItem *item)
{
    if (m_buttonBrowser == m_currentBrowser)
        return m_buttonBrowser->isExpanded(item);
    else if (m_treeBrowser == m_currentBrowser)
        return m_treeBrowser->isExpanded(item);
    return false;
}

void PropertyEditor::storePropertiesExpansionState(const QList<QtBrowserItem *> &items)
{
    const QChar bar = QLatin1Char('|');
    QListIterator<QtBrowserItem *> itProperty(items);
    while (itProperty.hasNext()) {
        QtBrowserItem *propertyItem = itProperty.next();
        if (!propertyItem->children().empty()) {
            QtProperty *property = propertyItem->property();
            const QString propertyName = property->propertyName();
            const QMap<QtProperty *, QString>::const_iterator itGroup = m_propertyToGroup.constFind(property);
            if (itGroup != m_propertyToGroup.constEnd()) {
                QString key = itGroup.value();
                key += bar;
                key += propertyName;
                m_expansionState[key] = isExpanded(propertyItem);
            }
        }
    }
}

void PropertyEditor::storeExpansionState()
{
    const QList<QtBrowserItem *> items = m_currentBrowser->topLevelItems();
    if (m_sorting) {
        storePropertiesExpansionState(items);
    } else {
        QListIterator<QtBrowserItem *> itGroup(items);
        while (itGroup.hasNext()) {
            QtBrowserItem *item = itGroup.next();
            const QString groupName = item->property()->propertyName();
            QList<QtBrowserItem *> propertyItems = item->children();
            if (!propertyItems.empty())
                m_expansionState[groupName] = isExpanded(item);

            // properties stuff here
            storePropertiesExpansionState(propertyItems);
        }
    }
}

void PropertyEditor::collapseAll()
{
    QList<QtBrowserItem *> items = m_currentBrowser->topLevelItems();
    QListIterator<QtBrowserItem *> itGroup(items);
    while (itGroup.hasNext())
        setExpanded(itGroup.next(), false);
}

void PropertyEditor::applyPropertiesExpansionState(const QList<QtBrowserItem *> &items)
{
    const QChar bar = QLatin1Char('|');
    QListIterator<QtBrowserItem *> itProperty(items);
    while (itProperty.hasNext()) {
        const QMap<QString, bool>::const_iterator excend = m_expansionState.constEnd();
        QtBrowserItem *propertyItem = itProperty.next();
        QtProperty *property = propertyItem->property();
        const QString propertyName = property->propertyName();
        const QMap<QtProperty *, QString>::const_iterator itGroup = m_propertyToGroup.constFind(property);
        if (itGroup != m_propertyToGroup.constEnd()) {
            QString key = itGroup.value();
            key += bar;
            key += propertyName;
            const QMap<QString, bool>::const_iterator pit = m_expansionState.constFind(key);
            if (pit != excend)
                setExpanded(propertyItem, pit.value());
            else
                setExpanded(propertyItem, false);
        }
    }
}

void PropertyEditor::applyExpansionState()
{
    const QList<QtBrowserItem *> items = m_currentBrowser->topLevelItems();
    if (m_sorting) {
        applyPropertiesExpansionState(items);
    } else {
        QListIterator<QtBrowserItem *> itTopLevel(items);
        const QMap<QString, bool>::const_iterator excend = m_expansionState.constEnd();
        while (itTopLevel.hasNext()) {
            QtBrowserItem *item = itTopLevel.next();
            const QString groupName = item->property()->propertyName();
            const QMap<QString, bool>::const_iterator git = m_expansionState.constFind(groupName);
            if (git != excend)
                setExpanded(item, git.value());
            else
                setExpanded(item, true);
            // properties stuff here
            applyPropertiesExpansionState(item->children());
        }
    }
}

void PropertyEditor::clearView()
{
    m_currentBrowser->clear();
}

QColor PropertyEditor::propertyColor(QtProperty *property) const
{
    if (!m_coloring)
        return QColor();

    QtProperty *groupProperty = property;

    QMap<QtProperty *, QString>::ConstIterator itProp = m_propertyToGroup.constFind(property);
    if (itProp != m_propertyToGroup.constEnd())
        groupProperty = m_nameToGroup.value(itProp.value());

    const int groupIdx = m_groups.indexOf(groupProperty);
    if (groupIdx != -1) {
        if (groupProperty == m_dynamicGroup)
            return m_dynamicColor;
        if (isLayoutGroup(groupProperty))
            return m_layoutColor;
        return m_colors[groupIdx % m_colors.count()];
    }
    return QColor();
}

void PropertyEditor::fillView()
{
    if (m_sorting) {
        QMapIterator<QString, QtVariantProperty *> itProperty(m_nameToProperty);
        while (itProperty.hasNext()) {
            QtVariantProperty *property = itProperty.next().value();
            m_currentBrowser->addProperty(property);
        }
    } else {
        QListIterator<QtProperty *> itGroup(m_groups);
        while (itGroup.hasNext()) {
            QtProperty *group = itGroup.next();
            QtBrowserItem *item = m_currentBrowser->addProperty(group);
            if (m_currentBrowser == m_treeBrowser)
                m_treeBrowser->setBackgroundColor(item, propertyColor(group));
            group->setModified(m_currentBrowser == m_treeBrowser);
        }
    }
}

bool PropertyEditor::isLayoutGroup(QtProperty *group) const
{
    if (group->propertyName() == QLatin1String("Layout"))
        return true;
    return false;
}

void PropertyEditor::updateActionsState()
{
    m_coloringAction->setEnabled(m_treeAction->isChecked() && !m_sortingAction->isChecked());
}

void PropertyEditor::slotViewTriggered(QAction *action)
{
    storeExpansionState();
    collapseAll();
    const bool wasEnabled = updatesEnabled();
    setUpdatesEnabled(false);
    clearView();
    int idx = 0;
    if (action == m_treeAction) {
        m_currentBrowser = m_treeBrowser;
        idx = m_treeIndex;
    } else if (action == m_buttonAction) {
        m_currentBrowser = m_buttonBrowser;
        idx = m_buttonIndex;
    }
    fillView();
    m_stackedWidget->setCurrentIndex(idx);
    applyExpansionState();
    setUpdatesEnabled(wasEnabled);
    updateActionsState();
}

void PropertyEditor::slotSorting(bool sort)
{
    if (sort == m_sorting)
        return;

    storeExpansionState();
    m_sorting = sort;
    collapseAll();
    const bool wasEnabled = updatesEnabled();
    setUpdatesEnabled(false);
    clearView();
    m_treeBrowser->setRootIsDecorated(sort);
    fillView();
    applyExpansionState();
    setUpdatesEnabled(wasEnabled);
    updateActionsState();
}

void PropertyEditor::slotColoring(bool coloring)
{
    if (coloring == m_coloring)
        return;

    m_coloring = coloring;
    if (m_currentBrowser == m_treeBrowser) {
        QList<QtBrowserItem *> items = m_treeBrowser->topLevelItems();
        QListIterator<QtBrowserItem *> itItem(items);
        while (itItem.hasNext()) {
            QtBrowserItem *item = itItem.next();
            m_treeBrowser->setBackgroundColor(item, propertyColor(item->property()));
        }
    }
}

void PropertyEditor::slotAddDynamicProperty()
{
    if (!m_propertySheet)
        return;

    const QDesignerDynamicPropertySheetExtension *dynamicSheet =
            qt_extension<QDesignerDynamicPropertySheetExtension*>(m_core->extensionManager(), m_object);

    if (!dynamicSheet)
        return;

    NewDynamicPropertyDialog dlg(core()->dialogGui(), this);
    QStringList reservedNames;
    const int propertyCount = m_propertySheet->count();
    for (int i = 0; i < propertyCount; i++) {
        if (!dynamicSheet->isDynamicProperty(i) || m_propertySheet->isVisible(i))
            reservedNames.append(m_propertySheet->propertyName(i));
    }
    dlg.setReservedNames(reservedNames);
    if (dlg.exec() == QDialog::Accepted) {
        const QString newName = dlg.propertyName();
        const QVariant newValue = dlg.propertyValue();

        emit addDynamicProperty(newName, newValue);
    }
}

QDesignerFormEditorInterface *PropertyEditor::core() const
{
    return m_core;
}

bool PropertyEditor::isReadOnly() const
{
    return false;
}

void PropertyEditor::setReadOnly(bool /*readOnly*/)
{
    qDebug() << "PropertyEditor::setReadOnly() request";
}

void PropertyEditor::setPropertyValue(const QString &name, const QVariant &value, bool changed)
{
    const QMap<QString, QtVariantProperty*>::const_iterator it = m_nameToProperty.constFind(name);
    if (it == m_nameToProperty.constEnd())
        return;
    QtVariantProperty *property = it.value();
    updateBrowserValue(property, value);
    property->setModified(changed);
}

void PropertyEditor::setPropertyComment(const QString &name, const QString &value)
{
    const QMap<QString, QtVariantProperty*>::const_iterator it = m_nameToProperty.constFind(name);
    if (it == m_nameToProperty.constEnd())
        return;

    QtVariantProperty *property = it.value();
    const QMap<QtVariantProperty *, QtVariantProperty *>::const_iterator cit = m_propertyToComment.constFind(property);
    if (cit == m_propertyToComment.constEnd())
        return;
    QtVariantProperty *commentProperty = cit.value();
    updateBrowserValue(commentProperty, value);
}

void PropertyEditor::updatePropertySheet()
{
    if (!m_propertySheet)
        return;

    updateToolBarLabel();

    const int propertyCount = m_propertySheet->count();
    const  QMap<QString, QtVariantProperty*>::const_iterator npcend = m_nameToProperty.constEnd();
    for (int i = 0; i < propertyCount; ++i) {
        const QString propertyName = m_propertySheet->propertyName(i);
        const QMap<QString, QtVariantProperty*>::const_iterator it = m_nameToProperty.constFind(propertyName);
        if (it != npcend)
            updateBrowserValue(it.value(), m_propertySheet->property(i));
    }
}

void PropertyEditor::updateToolBarLabel()
{
    QString objectName;
    QString className;
    if (m_object) {
        objectName = m_object->objectName();
        className = realClassName(m_object);
    }

    m_classLabel->setText(tr("%1\n%2").arg(objectName).arg(className));
    m_classLabel->setToolTip(tr("Object: %1\nClass: %2").arg(objectName).arg(className));

}

void PropertyEditor::updateBrowserValue(QtVariantProperty *property, const QVariant &value)
{
    QVariant v = value;
    if (property->propertyType() == QtVariantPropertyManager::enumTypeId()) {
        const PropertySheetEnumValue e = qvariant_cast<PropertySheetEnumValue>(v);
        v = e.metaEnum.keys().indexOf(e.metaEnum.valueToKey(e.value));
    } else if (property->propertyType() == DesignerPropertyManager::designerFlagTypeId()) {
        const PropertySheetFlagValue f = qvariant_cast<PropertySheetFlagValue>(v);
        v = QVariant(f.value);
    } else if (property->propertyType() == DesignerPropertyManager::designerAlignmentTypeId()) {
        const PropertySheetFlagValue f = qvariant_cast<PropertySheetFlagValue>(v);
        v = QVariant(f.value);
    }
    QDesignerPropertySheet *sheet = qobject_cast<QDesignerPropertySheet*>(m_core->extensionManager()->extension(m_object, Q_TYPEID(QDesignerPropertySheetExtension)));
    if (sheet && m_propertyToGroup.contains(property)) { // don't do it for comments since property sheet doesn't keep them
        property->setEnabled(sheet->isEnabled(sheet->indexOf(property->propertyName())));
    }
    m_updatingBrowser = true;
    property->setValue(v);
    m_updatingBrowser = false;
}

int PropertyEditor::toBrowserType(const QVariant &value, const QString &propertyName) const
{
    if (qVariantCanConvert<PropertySheetFlagValue>(value)) {
        if (propertyName == QLatin1String("alignment"))
            return DesignerPropertyManager::designerAlignmentTypeId();
        return DesignerPropertyManager::designerFlagTypeId();
    }
    if (qVariantCanConvert<PropertySheetEnumValue>(value))
        return DesignerPropertyManager::enumTypeId();

    if (value.type() == QVariant::ByteArray)
        return QVariant::String;
    return value.userType();
}

QString PropertyEditor::realClassName(QObject *object) const
{
    if (!object)
        return 0;

    const QString qLayoutWidget = QLatin1String("QLayoutWidget");
    const QString designerPrefix = QLatin1String("QDesigner");

    QString className = QLatin1String(object->metaObject()->className());
    const QDesignerWidgetDataBaseInterface *db = core()->widgetDataBase();
    if (QDesignerWidgetDataBaseItemInterface *widgetItem = db->item(db->indexOfObject(object, true))) {
        className = widgetItem->name();

        if (object->isWidgetType() && className == qLayoutWidget
                && static_cast<QWidget*>(object)->layout()) {
            className = QLatin1String(static_cast<QWidget*>(object)->layout()->metaObject()->className());
        }
    }

    if (className.startsWith(designerPrefix))
        className.remove(1, designerPrefix.size() - 1);

    return className;
}

void PropertyEditor::setObject(QObject *object)
{
    m_object = object;
    m_removeDynamicMenu->clear();

    storeExpansionState();

    const bool wasEnabled = updatesEnabled();
    setUpdatesEnabled(false);

    updateToolBarLabel();

    QMap<QString, QtVariantProperty *> toRemove = m_nameToProperty;

    const QDesignerDynamicPropertySheetExtension *dynamicSheet =
            qt_extension<QDesignerDynamicPropertySheetExtension*>(m_core->extensionManager(), m_object);

    // list of properties to remove
    // remove them
    // traverse the sheet, in case property exists just set a value, otherwise - create it.

    QExtensionManager *m = m_core->extensionManager();

    m_propertySheet = qobject_cast<QDesignerPropertySheetExtension*>(m->extension(object, Q_TYPEID(QDesignerPropertySheetExtension)));
    if (m_propertySheet) {
        const int propertyCount = m_propertySheet->count();
        for (int i = 0; i < propertyCount; ++i) {
            if (!m_propertySheet->isVisible(i))
                continue;

            const QString propertyName = m_propertySheet->propertyName(i);
            if (m_propertySheet->indexOf(propertyName) != i)
                continue;
            const QString groupName = m_propertySheet->propertyGroup(i);
            const QMap<QString, QtVariantProperty *>::const_iterator rit = toRemove.constFind(propertyName);
            if (rit != toRemove.constEnd()) {
                QtVariantProperty *property = rit.value();
                if (m_propertyToGroup.value(property) == groupName && toBrowserType(m_propertySheet->property(i), propertyName) == property->propertyType())
                    toRemove.remove(propertyName);
            }
        }
    }

    QMapIterator<QString, QtVariantProperty *> itRemove(toRemove);
    while (itRemove.hasNext()) {
        itRemove.next();

        QtVariantProperty *property = itRemove.value();
        delete property;
        if (m_propertyToComment.contains(property)) {
            QtVariantProperty *commentProperty = m_propertyToComment.value(property);
            delete commentProperty;
            m_commentToProperty.remove(commentProperty);
            m_propertyToComment.remove(property);
        }
        m_nameToProperty.remove(itRemove.key());
        m_propertyToGroup.remove(property);
    }

    bool isMainContainer = false;
    if (QWidget *widget = qobject_cast<QWidget*>(object)) {
        if (QDesignerFormWindowInterface *fw = QDesignerFormWindowInterface::findFormWindow(widget)) {
            isMainContainer = (fw->mainContainer() == widget);
        }
    }
    QStringList dynamicProperties;
    m_groups.clear();

    if (m_propertySheet) {
        QtProperty *lastProperty = 0;
        QtProperty *lastGroup = 0;
        const int propertyCount = m_propertySheet->count();
        for (int i = 0; i < propertyCount; ++i) {
            if (!m_propertySheet->isVisible(i))
                continue;

            const QString propertyName = m_propertySheet->propertyName(i);
            if (m_propertySheet->indexOf(propertyName) != i)
                continue;
            const QVariant value = m_propertySheet->property(i);

            const int type = toBrowserType(value, propertyName);

            QtVariantProperty *property = 0;
            bool newProperty = false;
            if (m_nameToProperty.contains(propertyName)) {
                property = m_nameToProperty.value(propertyName);
            } else {
                property = m_propertyManager->addProperty(type, propertyName);
                if (property) {
                    property->setToolTip(propertyName);
                    newProperty = true;
                    if (type == DesignerPropertyManager::enumTypeId()) {
                        const PropertySheetEnumValue e = qvariant_cast<PropertySheetEnumValue>(value);
                        QStringList names;
                        QStringListIterator it(e.metaEnum.keys());
                        while (it.hasNext())
                            names.append(it.next());
                        m_updatingBrowser = true;
                        property->setAttribute(QLatin1String("enumNames"), names);
                        m_updatingBrowser = false;
                    } else if (type == DesignerPropertyManager::designerFlagTypeId()) {
                        const PropertySheetFlagValue f = qvariant_cast<PropertySheetFlagValue>(value);
                        QList<QPair<QString, uint> > flags;
                        QStringListIterator it(f.metaFlags.keys());
                        while (it.hasNext()) {
                            const QString name = it.next();
                            const uint val = f.metaFlags.keyToValue(name);
                            flags.append(qMakePair(name, val));
                        }
                        m_updatingBrowser = true;
                        QVariant v;
                        qVariantSetValue(v, flags);
                        property->setAttribute(QLatin1String("flags"), v);
                        m_updatingBrowser = false;
                    }
                }
            }

            if (property != 0) {
                bool dynamicProperty = false;
                if (dynamicSheet && dynamicSheet->isDynamicProperty(i)) {
                    dynamicProperties.append(propertyName);
                    dynamicProperty = true;
                }

                if (type == QVariant::String)
                    setupStringProperty(property, propertyName, value, isMainContainer);

                if (type == QVariant::Palette)
                    setupPaletteProperty(property);

                property->setAttribute(QLatin1String("resettable"), m_propertySheet->hasReset(i));

                const QString groupName = m_propertySheet->propertyGroup(i);
                QtVariantProperty *groupProperty = 0;

                if (newProperty) {
                    QMap<QString, QtVariantProperty*>::const_iterator itPrev = m_nameToProperty.insert(propertyName, property);
                    m_propertyToGroup[property] = groupName;
                    if (m_sorting) {
                        QtProperty *previous = 0;
                        if (itPrev != m_nameToProperty.constBegin())
                            previous = (--itPrev).value();
                        m_currentBrowser->insertProperty(property, previous);
                    }
                }
                const QMap<QString, QtVariantProperty*>::const_iterator gnit = m_nameToGroup.constFind(groupName);
                if (gnit != m_nameToGroup.constEnd()) {
                    groupProperty = gnit.value();
                } else {
                    groupProperty = m_propertyManager->addProperty(QtVariantPropertyManager::groupTypeId(), groupName);
                    groupProperty->setToolTip(groupName);
                    QtBrowserItem *item = 0;
                    if (!m_sorting)
                         item = m_currentBrowser->insertProperty(groupProperty, lastGroup);
                    m_nameToGroup[groupName] = groupProperty;
                    m_groups.append(groupProperty);
                    if (dynamicProperty)
                        m_dynamicGroup = groupProperty;
                    if (m_currentBrowser == m_treeBrowser && item) {
                        m_treeBrowser->setBackgroundColor(item, propertyColor(groupProperty));
                        groupProperty->setModified(true);
                    }
                }

                if (lastGroup != groupProperty) {
                    lastProperty = 0;
                    lastGroup = groupProperty;
                }
                if (!m_groups.contains(groupProperty))
                    m_groups.append(groupProperty);
                if (newProperty)
                    groupProperty->insertSubProperty(property, lastProperty);

                lastProperty = property;

                updateBrowserValue(property, value);
                const QMap<QtVariantProperty *, QtVariantProperty *>::const_iterator cit = m_propertyToComment.constFind(property);
                if (cit != m_propertyToComment.constEnd()) {
                    updateBrowserValue(cit.value(), propertyComment(m_core, m_object, propertyName));
                }
                property->setModified(m_propertySheet->isChanged(i));
            } else {
                qDebug() << "Property" << propertyName << "of type" << type << "not supported yet!";
            }
        }
    }
    QMap<QString, QtVariantProperty *> groups = m_nameToGroup;
    QMapIterator<QString, QtVariantProperty *> itGroup(groups);
    while (itGroup.hasNext()) {
        QtVariantProperty *groupProperty = itGroup.next().value();
        if (groupProperty->subProperties().empty()) {
            if (groupProperty == m_dynamicGroup)
                m_dynamicGroup = 0;
            delete groupProperty;
            m_nameToGroup.remove(itGroup.key());
        }
    }
    const bool addEnabled = dynamicSheet ? dynamicSheet->dynamicPropertiesAllowed() : false;
    const bool removeEnabled = addEnabled && dynamicProperties.count();
    m_addDynamicAction->setEnabled(addEnabled);
    m_removeDynamicAction->setEnabled(removeEnabled);
    QStringListIterator it(dynamicProperties);
    while (it.hasNext()) {
        const QString property = it.next();
        QAction *action = new QAction(property, m_removeDynamicMenu);
        connect(action, SIGNAL(triggered()), m_removeMapper, SLOT(map()));
        m_removeMapper->setMapping(action, property);
        m_removeDynamicMenu->addAction(action);
    }
    applyExpansionState();
    setUpdatesEnabled(wasEnabled);
}

QtBrowserItem *PropertyEditor::nonFakePropertyBrowserItem(QtBrowserItem *item) const
{
    // Top-level properties are QObject/QWidget groups, etc. Find first item property below
    // which should be nonfake
    const QList<QtBrowserItem *> topLevelItems = m_currentBrowser->topLevelItems();
    do {
        if (topLevelItems.contains(item->parent()))
            return item;
        item = item->parent();
    } while (item);
    return 0;
}

QString PropertyEditor::currentPropertyName() const
{
    if (QtBrowserItem *browserItem = m_currentBrowser->currentItem())
        if (QtBrowserItem *topLevelItem = nonFakePropertyBrowserItem(browserItem)) {
            return topLevelItem->property()->propertyName();
        }
    return QString();
}

void PropertyEditor::slotResetProperty(QtProperty *property)
{
    QDesignerFormWindowInterface *form = m_core->formWindowManager()->activeFormWindow();
    if (!form)
        return;

    if (m_propertyManager->resetFontSubProperty(property))
        return;

    if (!m_propertyToGroup.contains(property))
        return;

    emit resetProperty(property->propertyName());
}

void PropertyEditor::slotValueChanged(QtProperty *property, const QVariant &value)
{
    if (m_updatingBrowser)
        return;

    if (!m_propertySheet)
        return;

    QtVariantProperty *varProp = m_propertyManager->variantProperty(property);

    if (!varProp)
        return;

    if (m_commentToProperty.contains(varProp)) {
        QtVariantProperty *commentParentProperty = m_commentToProperty.value(varProp);
        emit propertyCommentChanged(commentParentProperty->propertyName(), value.toString());
        return;
    }

    if (!m_propertyToGroup.contains(property))
        return;

    if (varProp->propertyType() == QtVariantPropertyManager::enumTypeId()) {
        PropertySheetEnumValue e = qvariant_cast<PropertySheetEnumValue>(m_propertySheet->property(m_propertySheet->indexOf(property->propertyName())));
        const int val = value.toInt();
        const QString valName = varProp->attributeValue(QLatin1String("enumNames")).toStringList().at(val);
        bool ok = false;
        e.value = e.metaEnum.parseEnum(valName, &ok);
        Q_ASSERT(ok);
        QVariant v;
        qVariantSetValue(v, e);
        emit propertyChanged(property->propertyName(), v);
        return;
    }

    emit propertyChanged(property->propertyName(), value);
}

}

QT_END_NAMESPACE

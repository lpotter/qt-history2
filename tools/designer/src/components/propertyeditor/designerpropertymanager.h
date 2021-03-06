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

#ifndef DESIGNERPROPERTYMANAGER_H
#define DESIGNERPROPERTYMANAGER_H

#include "qtvariantproperty.h"
#include <QtCore/QUrl>

QT_BEGIN_NAMESPACE

typedef QPair<QString, uint> DesignerIntPair;
typedef QList<DesignerIntPair> DesignerFlagList;

class QDesignerFormEditorInterface;
class QLineEdit;
class QUrl;

namespace qdesigner_internal
{

class ResetWidget;

class TextEditor;
class PaletteEditorButton;
class GraphicsPropertyEditor;
class StringListEditorButton;

class ResetDecorator : public QObject
{
    Q_OBJECT
public:
    ResetDecorator(QObject *parent = 0) : QObject(parent), m_spacing(-1) {}
    ~ResetDecorator();

    void connectPropertyManager(QtAbstractPropertyManager *manager);
    QWidget *editor(QWidget *subEditor, bool resettable, QtAbstractPropertyManager *manager, QtProperty *property,
                QWidget *parent);
    void disconnectPropertyManager(QtAbstractPropertyManager *manager);
    void setSpacing(int spacing);
signals:
    void resetProperty(QtProperty *property);
private slots:
    void slotPropertyChanged(QtProperty *property);
    void slotEditorDestroyed(QObject *object);
private:
    QMap<QtProperty *, QList<ResetWidget *> > m_createdResetWidgets;
    QMap<ResetWidget *, QtProperty *> m_resetWidgetToProperty;
    int m_spacing;
};

class DesignerPropertyManager : public QtVariantPropertyManager
{
    Q_OBJECT
public:
    DesignerPropertyManager(QDesignerFormEditorInterface *core, QObject *parent = 0);

    virtual QStringList attributes(int propertyType) const;
    virtual int attributeType(int propertyType, const QString &attribute) const;

    virtual QVariant attributeValue(const QtProperty *property, const QString &attribute);
    virtual bool isPropertyTypeSupported(int propertyType) const;
    virtual QVariant value(const QtProperty *property) const;
    virtual int valueType(int propertyType) const;
    virtual QString valueText(const QtProperty *property) const;
    virtual QIcon valueIcon(const QtProperty *property) const;

    bool resetFontSubProperty(QtProperty *subProperty);

    static int designerFlagTypeId();
    static int designerFlagListTypeId();
    static int designerAlignmentTypeId();

public Q_SLOTS:
    virtual void setAttribute(QtProperty *property,
                const QString &attribute, const QVariant &value);
    virtual void setValue(QtProperty *property, const QVariant &value);
protected:
    virtual void initializeProperty(QtProperty *property);
    virtual void uninitializeProperty(QtProperty *property);
private Q_SLOTS:
    void slotValueChanged(QtProperty *property, const QVariant &value);
    void slotPropertyDestroyed(QtProperty *property);
private:
    typedef QMap<QtProperty *, bool> PropertyBoolMap;
    PropertyBoolMap m_resetMap;

    int bitCount(int mask) const;
    struct FlagData
    {
        FlagData() : val(0) {}
        uint val;
        DesignerFlagList flags;
        QList<uint> values;
    };
    typedef QMap<QtProperty *, FlagData> PropertyFlagDataMap;
    PropertyFlagDataMap m_flagValues;
    typedef  QMap<QtProperty *, QList<QtProperty *> > PropertyToPropertyListMap;
    PropertyToPropertyListMap m_propertyToFlags;
    QMap<QtProperty *, QtProperty *> m_flagToProperty;

    int alignToIndexH(uint align) const;
    int alignToIndexV(uint align) const;
    uint indexHToAlign(int idx) const;
    uint indexVToAlign(int idx) const;
    QString indexHToString(int idx) const;
    QString indexVToString(int idx) const;
    QMap<QtProperty *, uint> m_alignValues;
    QMap<QtProperty *, QtProperty *> m_propertyToAlignH;
    QMap<QtProperty *, QtProperty *> m_propertyToAlignV;
    QMap<QtProperty *, QtProperty *> m_alignHToProperty;
    QMap<QtProperty *, QtProperty *> m_alignVToProperty;

    int antialiasingToIndex(QFont::StyleStrategy antialias) const;
    QFont::StyleStrategy indexToAntialiasing(int idx) const;
    QString indexAntialiasingToString(int idx) const;
    QMap<QtProperty *, QtProperty *> m_propertyToAntialiasing;
    QMap<QtProperty *, QtProperty *> m_antialiasingToProperty;

    unsigned fontFlag(int idx) const;
    QMap<QtProperty *, QMap<int, QtProperty *> > m_propertyToFontSubProperties;
    QMap<QtProperty *, int> m_fontSubPropertyToFlag;
    QMap<QtProperty *, QtProperty *> m_fontSubPropertyToProperty;
    QtProperty *m_createdFontProperty;
    int m_lastSubFontIndex;

    struct PaletteData
    {
        QPalette val;
        QPalette superPalette;
    };
    typedef QMap<QtProperty *, PaletteData>  PropertyPaletteDataMap;
    PropertyPaletteDataMap m_paletteValues;

    QMap<QtProperty *, QIcon> m_iconValues;

    QMap<QtProperty *, QPixmap> m_pixmapValues;

    QMap<QtProperty *, uint> m_uintValues;
    QMap<QtProperty *, qlonglong> m_longLongValues;
    QMap<QtProperty *, qulonglong> m_uLongLongValues;
    QMap<QtProperty *, QUrl> m_urlValues;
    QMap<QtProperty *, QStringList> m_stringListValues;

    typedef QMap<QtProperty *, int>  PropertyIntMap;
    PropertyIntMap m_stringAttributes;

    bool m_changingSubValue;
    QDesignerFormEditorInterface *m_core;
};

class DesignerEditorFactory : public QtVariantEditorFactory
{
    Q_OBJECT
public:
    explicit DesignerEditorFactory(QDesignerFormEditorInterface *core, QObject *parent = 0);
    ~DesignerEditorFactory();
    void setSpacing(int spacing);
signals:
    void resetProperty(QtProperty *property);
protected:
    void connectPropertyManager(QtVariantPropertyManager *manager);
    QWidget *createEditor(QtVariantPropertyManager *manager, QtProperty *property,
                QWidget *parent);
    void disconnectPropertyManager(QtVariantPropertyManager *manager);
private slots:
    void slotEditorDestroyed(QObject *object);
    void slotAttributeChanged(QtProperty *property, const QString &attribute, const QVariant &value);
    void slotValueChanged(QtProperty *property, const QVariant &value);
    void slotStringTextChanged(const QString &value);
    void slotPaletteChanged(const QPalette &value);
    void slotIconChanged(const QIcon &value);
    void slotPixmapChanged(const QPixmap &value);
    void slotUintChanged(const QString &value);
    void slotLongLongChanged(const QString &value);
    void slotULongLongChanged(const QString &value);
    void slotUrlChanged(const QString &value);
    void slotStringListChanged(const QStringList &value);
private:

    ResetDecorator *m_resetDecorator;
    bool m_changingPropertyValue;
    QDesignerFormEditorInterface *m_core;

    int m_spacing;

    QMap<QtProperty *, QList<TextEditor *> >                m_stringPropertyToEditors;
    QMap<TextEditor *, QtProperty *>                        m_editorToStringProperty;
    QMap<QtProperty *, QList<PaletteEditorButton *> >       m_palettePropertyToEditors;
    QMap<PaletteEditorButton *, QtProperty *>               m_editorToPaletteProperty;
    QMap<QtProperty *, QList<GraphicsPropertyEditor *> >    m_iconPropertyToEditors;
    QMap<GraphicsPropertyEditor *, QtProperty *>            m_editorToIconProperty;
    QMap<QtProperty *, QList<GraphicsPropertyEditor *> >    m_pixmapPropertyToEditors;
    QMap<GraphicsPropertyEditor *, QtProperty *>            m_editorToPixmapProperty;
    QMap<QtProperty *, QList<QLineEdit *> >                 m_uintPropertyToEditors;
    QMap<QLineEdit *, QtProperty *>                         m_editorToUintProperty;
    QMap<QtProperty *, QList<QLineEdit *> >                 m_longLongPropertyToEditors;
    QMap<QLineEdit *, QtProperty *>                         m_editorToLongLongProperty;
    QMap<QtProperty *, QList<QLineEdit *> >                 m_uLongLongPropertyToEditors;
    QMap<QLineEdit *, QtProperty *>                         m_editorToULongLongProperty;
    QMap<QtProperty *, QList<QLineEdit *> >                 m_urlPropertyToEditors;
    QMap<QLineEdit *, QtProperty *>                         m_editorToUrlProperty;
    QMap<QtProperty *, QList<StringListEditorButton *> >    m_stringListPropertyToEditors;
    QMap<StringListEditorButton *, QtProperty *>            m_editorToStringListProperty;
};

}

QT_END_NAMESPACE

Q_DECLARE_METATYPE(DesignerIntPair)
Q_DECLARE_METATYPE(DesignerFlagList)

#endif


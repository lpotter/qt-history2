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

#ifndef WIDGETBOX_H
#define WIDGETBOX_H

#include "widgetbox_global.h"
#include <qdesigner_widgetbox_p.h>

QT_BEGIN_NAMESPACE

class QDesignerFormEditorInterface;
class QDesignerFormWindowInterface;

namespace qdesigner_internal {

class WidgetBoxTreeView;
class WidgetCollectionModel;
class Scratchpad;

class QT_WIDGETBOX_EXPORT WidgetBox : public QDesignerWidgetBox
{
    Q_OBJECT
public:
    explicit WidgetBox(QDesignerFormEditorInterface *core, QWidget *parent = 0, Qt::WindowFlags flags = 0);
    virtual ~WidgetBox();

    QDesignerFormEditorInterface *core() const;

    virtual int categoryCount() const;
    virtual Category category(int cat_idx) const;
    virtual void addCategory(const Category &cat);
    virtual void removeCategory(int cat_idx);

    virtual int widgetCount(int cat_idx) const;
    virtual Widget widget(int cat_idx, int wgt_idx) const;
    virtual void addWidget(int cat_idx, const Widget &wgt);
    virtual void removeWidget(int cat_idx, int wgt_idx);

    void dropWidgets(const QList<QDesignerDnDItemInterface*> &item_list, const QPoint &global_mouse_pos);

    virtual void setFileName(const QString &file_name);
    virtual QString fileName() const;
    virtual bool load();
    virtual bool save();

    virtual bool loadContents(const QString &contents);

protected:
    virtual void dragEnterEvent (QDragEnterEvent * event);
    virtual void dragMoveEvent(QDragMoveEvent * event);
    virtual void dropEvent (QDropEvent * event);

private slots:
    void handleMousePress(const QString &name, const QString &xml, bool custom, const QPoint &global_mouse_pos);

private:
    QDesignerFormEditorInterface *m_core;
    WidgetBoxTreeView *m_view;
};

}  // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // WIDGETBOX_H

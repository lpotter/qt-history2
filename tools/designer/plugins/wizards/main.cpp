 /**********************************************************************
** Copyright (C) 2000 Trolltech AS.  All rights reserved.
**
** This file is part of Qt Designer.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#include <qcleanuphandler.h>
#include <designerinterface.h>
#include <qfeatures.h>
#include <qwidget.h>
#include <templatewizardiface.h>
#ifndef QT_NO_SQL
#include "sqlformwizardimpl.h"
#endif
#include "mainwindowwizard.h"

class StandardTemplateWizardInterface : public TemplateWizardInterface
{
public:
    StandardTemplateWizardInterface();

    QRESULT queryInterface( const QUuid&, QUnknownInterface** );
    unsigned long addRef();
    unsigned long release();

    QStringList featureList() const;

    void setup( const QString &templ, QWidget *widget, DesignerFormWindow *fw, QUnknownInterface *aIface );

private:
    unsigned long ref;
    QUnknownInterface *appIface;

};

StandardTemplateWizardInterface::StandardTemplateWizardInterface()
    : ref( 0 ), appIface( 0 )
{
}

QStringList StandardTemplateWizardInterface::featureList() const
{
    QStringList list;
#ifndef QT_NO_SQL
    list << "QDataBrowser" << "QDesignerDataBrowser" << "QDataView" << \
	"QDesignerDataView" << "QDataTable";
#endif
    list << "QMainWindow";

    return list;
}

void StandardTemplateWizardInterface::setup( const QString &templ, QWidget *widget, DesignerFormWindow *fw, QUnknownInterface *aIface )
{
#ifndef QT_NO_SQL
    appIface = aIface;
    if ( templ == "QDesignerDataView" ||
	 templ == "QDesignerDataBrowser" ||
	 templ == "QDataView" ||
	 templ == "QDataBrowser" ||
	 templ == "QDataTable" ) {
	SqlFormWizard wizard( appIface, widget, 0, fw, 0, TRUE );
	wizard.exec();
    }
#endif
    if ( templ == "QMainWindow" ) {
	MainWindowWizardBase wizard( 0, 0, TRUE );
	wizard.setAppInterface( aIface, fw, widget );
	wizard.exec();
    }
}

QRESULT StandardTemplateWizardInterface::queryInterface( const QUuid& uuid, QUnknownInterface** iface )
{
    if ( uuid == IID_QUnknown )
	*iface = (QUnknownInterface*)this;
    else if ( uuid == IID_QFeatureList )
	*iface = (QFeatureListInterface*)this;
    else if ( uuid == IID_TemplateWizard )
	*iface = (TemplateWizardInterface*)this;

    if ( *iface )
	(*iface)->addRef();
}

unsigned long StandardTemplateWizardInterface::addRef()
{
    return ref++;
}

unsigned long StandardTemplateWizardInterface::release()
{
    if ( !--ref ) {
	delete this;
	return 0;
    }
    return ref;
}

Q_EXPORT_INTERFACE()
{
    Q_CREATE_INSTANCE( StandardTemplateWizardInterface )
}

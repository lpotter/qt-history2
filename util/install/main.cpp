#include <qapplication.h>
#include "setupwizardimpl.h"


int main( int argc, char** argv )
{
	QApplication* app = new QApplication( argc, argv );
	SetupWizardImpl* w = new SetupWizardImpl;

	w->app = app;
	w->show();

	app->setMainWidget( w );

	app->exec();

	w->stopProcesses();

	return 0;
}

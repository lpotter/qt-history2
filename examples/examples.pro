TEMPLATE    =	subdirs
SUBDIRS     =	aclock \
		action \
		addressbook \
		application \
		buttongroups \
		checklists \
		cursor \
		customlayout \
		dclock \
		dirview \
		dragdrop \
		drawdemo \
		drawlines \
		forever \
		guithreads \
		hello \
		helpviewer \
		i18n \
		layout \
		life \
		lineedits \
		listboxcombo \
		listbox \
		listviews \
		menu \
		movies \
		picture \
		pingpong \
		popup \
		process \
		progress \
		progressbar \
		qdir \
		qfd \
		qmag \
		qwerty \
		rangecontrols \
		richtext \
		rot13 \
		semaphores \
		scribble \
		scrollview \
		showimg \
		splitter \
		tabdialog \
		tetrix \
		themes \
		tictac \
		tooltip \
		validator \
		widgets \
		wizard \
		xform
!contains(QT_PRODUCT,qt-professional): SUBDIRS += demo

canvas:SUBDIRS +=   canvas
opengl:SUBDIRS +=   box \
		    gear \
		    glpixmap \
		    overlay \
		    sharedbox \
		    texture
nas:SUBDIRS += 	    sound
iconview:SUBDIRS += fileiconview \
		    iconview
network:SUBDIRS +=  clientserver/client \
		    clientserver/server \
		    ftpclient \
		    httpd \
		    mail \
		    networkprotocol
workspace:SUBDIRS+= mdi
table:SUBDIRS +=    statistics \
		    table
sql:SUBDIRS += sql

xml:SUBDIRS +=	xml/outliner \
		xml/tagreader \
		xml/tagreader-with-features

embedded:SUBDIRS += winmanager \
		notepad \
		kiosk \
		launcher

embedded:SUBDIRS -= showimg

win32:SUBDIRS += trayicon

X11DIRS	    =   biff \
		desktop

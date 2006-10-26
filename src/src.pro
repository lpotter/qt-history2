TEMPLATE = subdirs

# this order is important
unset(SRC_SUBDIRS)
win32:SRC_SUBDIRS += src_winmain
SRC_SUBDIRS += src_tools_moc src_tools_rcc src_tools_uic src_corelib src_xml src_gui src_sql src_network src_svg
!embedded:contains(QT_CONFIG, opengl): SRC_SUBDIRS += src_opengl
contains(QT_CONFIG, qt3support): SRC_SUBDIRS += src_qt3support
!cross_compile {
    contains(QT_CONFIG, qt3support): SRC_SUBDIRS += src_tools_uic3
}
win32:!contains(QT_EDITION, OpenSource|Console):SRC_SUBDIRS += src_activeqt src_tools_idc
SRC_SUBDIRS += src_plugins

src_winmain.subdir = $$QT_BUILD_TREE/src/winmain
src_tools_moc.subdir = $$QT_BUILD_TREE/src/tools/moc
src_tools_rcc.subdir = $$QT_BUILD_TREE/src/tools/rcc
src_tools_uic.subdir = $$QT_BUILD_TREE/src/tools/uic
src_corelib.subdir = $$QT_BUILD_TREE/src/corelib
src_xml.subdir = $$QT_BUILD_TREE/src/xml
src_gui.subdir = $$QT_BUILD_TREE/src/gui
src_sql.subdir = $$QT_BUILD_TREE/src/sql
src_network.subdir = $$QT_BUILD_TREE/src/network
src_svg.subdir = $$QT_BUILD_TREE/src/svg
src_opengl.subdir = $$QT_BUILD_TREE/src/opengl
src_qt3support.subdir = $$QT_BUILD_TREE/src/qt3support
src_tools_uic3.subdir = $$QT_BUILD_TREE/src/tools/uic3
src_activeqt.subdir = $$QT_BUILD_TREE/src/activeqt
src_tools_idc.subdir = $$QT_BUILD_TREE/src/tools/idc
src_plugins.subdir = $$QT_BUILD_TREE/src/plugins

#CONFIG += ordered
!ordered {
   src_corelib.depends = src_tools_moc src_tools_rcc
   src_gui.depends = src_corelib src_tools_uic
   src_xml.depends = src_corelib
   src_svg.depends = src_xml src_gui
   src_network.depends = src_corelib
   src_opengl.depends = src_gui
   src_sql.depends = src_corelib
   src_qt3support.depends = src_gui src_xml src_network src_sql
   src_tools_uic3.depends = src_qt3support src_xml
   src_tools_idc.depends = src_corelib
   src_tools_activeqt.depends = src_tools_idc src_gui
   src_plugins.depends = src_gui src_sql src_svg
   contains(QT_CONFIG, qt3support): src_plugins.depends += src_qt3support
}

# This creates a sub-src rule
sub_src_target.CONFIG = recursive
sub_src_target.recurse = $$SRC_SUBDIRS
sub_src_target.target = sub-src
sub_src_target.recurse_target =
QMAKE_EXTRA_TARGETS += sub_src_target

# This gives us a top level debug/release
EXTRA_DEBUG_TARGETS =
EXTRA_RELEASE_TARGETS =
for(subname, SRC_SUBDIRS) {
   subdir = $$subname
   !isEmpty($${subname}.subdir):subdir = $$eval($${subname}.subdir)
   subpro = $$subdir/$${basename(subdir)}.pro
   !exists($$subpro):next()
   subtarget = $$replace(subdir, [^A-Za-z0-9], _)
   isEqual($$list($$fromfile($$subpro, TEMPLATE)), lib) {
       #debug
       eval(debug-$${subtarget}.depends = $${subdir}/$(MAKEFILE) $$EXTRA_DEBUG_TARGETS)
       eval(debug-$${subtarget}.commands = (cd $$subdir && $(MAKE) -f $(MAKEFILE) debug))
       EXTRA_DEBUG_TARGETS += debug-$${subtarget}
       QMAKE_EXTRA_TARGETS += debug-$${subtarget}
       #release
       eval(release-$${subtarget}.depends = $${subdir}/$(MAKEFILE) $$EXTRA_RELEASE_TARGETS)
       eval(release-$${subtarget}.commands = (cd $$subdir && $(MAKE) -f $(MAKEFILE) release))
       EXTRA_RELEASE_TARGETS += release-$${subtarget}
       QMAKE_EXTRA_TARGETS += release-$${subtarget}
    } else { #do not have a real debug target/release
       #debug
       eval(debug-$${subtarget}.depends = $${subdir}/$(MAKEFILE) $$EXTRA_DEBUG_TARGETS)
       eval(debug-$${subtarget}.commands = (cd $$subdir && $(MAKE) -f $(MAKEFILE) first))
       EXTRA_DEBUG_TARGETS += debug-$${subtarget}
       QMAKE_EXTRA_TARGETS += debug-$${subtarget}
       #release
       eval(release-$${subtarget}.depends = $${subdir}/$(MAKEFILE) $$EXTRA_RELEASE_TARGETS)
       eval(release-$${subtarget}.commands = (cd $$subdir && $(MAKE) -f $(MAKEFILE) first))
       EXTRA_RELEASE_TARGETS += release-$${subtarget}
       QMAKE_EXTRA_TARGETS += release-$${subtarget}
   }
}
debug.depends = $$EXTRA_DEBUG_TARGETS
release.depends = $$EXTRA_RELEASE_TARGETS
QMAKE_EXTRA_TARGETS += debug release

SUBDIRS += $$SRC_SUBDIRS


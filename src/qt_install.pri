#always install the library
target.path=$$libs.path
INSTALLS += target

#headers
!isEmpty(QPRO_PWD) { #all .h files without a _
    INSTALL_HEADERS = 
    exists($(QTDIR)/include/$TARGET/$TARGET):INSTALL_HEADER += $(QTDIR)/include/$TARGET/$TARGET
    for(file, $$list($$files($$QPRO_PWD/*.h, true))) {
        file_base = $$basename(file)
        isEmpty($$list($$find(file_base, _))):INSTALL_HEADERS += $$file
    }
    !isEmpty(INSTALL_HEADERS) {
       flat_headers.files = $$INSTALL_HEADERS
       flat_headers.path = $$headers.path/Qt
       INSTALLS += flat_headers

       targ_headers.files = $$INSTALL_HEADERS
       targ_headers.path = $$headers.path/$$TARGET
       INSTALLS += targ_headers
    }
}

macx { #mac framework
    QtFramework = /System/Library/Frameworks/$${TARGET}.framework
    framework.path = $$QtFramework/Headers
    framework.extra += cp -rf $$target.path/$(TARGET) $(INSTALL_ROOT)/$$QtFramework/$${TARGET};
    framework.extra += cp -rf $$headers.files $(INSTALL_ROOT)/$$QtFramework/Headers;
    INSTALLS += framework
}

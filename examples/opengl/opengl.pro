TEMPLATE      = subdirs
SUBDIRS       = 2dpainting \
                grabber \
                hellogl \
                overpainting \
                pbuffers \
                samplebuffers \
                textures
		

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS opengl.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/opengl
INSTALLS += target sources

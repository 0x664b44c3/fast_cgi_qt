INCLUDEPATH+=$$PWD
QT+=network
HEADERS += \
	$$PWD/fastcgi.h \
	$$PWD/fastcgi_connection.h \
	$$PWD/fastcgi_listener.h \
	$$PWD/fastcgi_request.h \
	$$PWD/fastcgi_request_wrapper.h \
	$$PWD/fastcgi_types.h \
	$$PWD/fcgi.h

SOURCES += \
	$$PWD/fastcgi.cpp \
	$$PWD/fastcgi_connection.cpp \
	$$PWD/fastcgi_helpers.cpp \
	$$PWD/fastcgi_listener.cpp \
	$$PWD/fastcgi_request.cpp \
	$$PWD/fastcgi_request_wrapper.cpp

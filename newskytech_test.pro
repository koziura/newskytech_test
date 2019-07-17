TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

# Check if the sqlite.pri file exists
#! include($$PWD/share/sqlite/sqlite.pri) {
#	error("Couldn't find the sqlite.pri file!")
#}

SQLITE_PATH = $$PWD/../3rd_party/sqlite

INCLUDEPATH += \
	$${SQLITE_PATH}/inc

SOURCES += \
        main.c

LIBS += \
	-L$${SQLITE_PATH}/bin/static

LIBS += \
	-lpthread \
	-luuid \
	-ldl \
	-lsqlite

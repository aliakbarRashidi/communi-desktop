######################################################################
# Communi
######################################################################

TEMPLATE = lib
CONFIG += plugin static
TARGET = highlighterplugin

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets
    DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x000000
}

DESTDIR = ../../../plugins
DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD

HEADERS += $$PWD/highlighterplugin.h
SOURCES += $$PWD/highlighterplugin.cpp

include(../../config.pri)
include(../../libs/backend/backend.pri)
include(../../libs/tree/tree.pri)
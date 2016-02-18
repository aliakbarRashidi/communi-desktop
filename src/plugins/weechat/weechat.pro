######################################################################
# Communi
######################################################################

QT += websockets
TEMPLATE = lib
COMMUNI += core model util
CONFIG += communi_plugin

HEADERS += \
    $$PWD/weechatplugin.h \
    $$PWD/weechatprotocol.h \
    $$PWD/weechatbuffer.h \
    $$PWD/weechatmessage.h \
    $$PWD/weechatobject.h

SOURCES += \
    $$PWD/weechatplugin.cpp \
    $$PWD/weechatprotocol.cpp \
    $$PWD/weechatbuffer.cpp \
    $$PWD/weechatmessage.cpp \
    $$PWD/weechatobject.cpp

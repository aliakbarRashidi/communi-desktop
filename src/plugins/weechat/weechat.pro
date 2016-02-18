######################################################################
# Communi
######################################################################

TEMPLATE = lib
COMMUNI += core model util
CONFIG += communi_plugin

HEADERS += \
    $$PWD/weechatplugin.h \
    $$PWD/weechatprotocol.h

SOURCES += \
    $$PWD/weechatplugin.cpp \
    $$PWD/weechatprotocol.cpp

######################################################################
# Communi
######################################################################

DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD

HEADERS += $$PWD/systemmonitor.h
SOURCES += $$PWD/systemmonitor.cpp

mac {
    HEADERS += $$PWD/mac/Reachability.h
    OBJECTIVE_SOURCES += $$PWD/mac/Reachability.m
    OBJECTIVE_SOURCES += $$PWD/systemmonitor_mac.mm
    LIBS += -framework SystemConfiguration -framework AppKit
} else:win32 {
    DEFINES += _WIN32_WINNT=0x0600
    HEADERS += $$PWD/win/netlistmgr_util.h
    HEADERS += $$PWD/win/networkmonitor.h
    HEADERS += $$PWD/win/screenmonitor.h
    SOURCES += $$PWD/win/networkmonitor.cpp
    SOURCES += $$PWD/win/screenmonitor.cpp
    SOURCES += $$PWD/systemmonitor_win.cpp
    LIBS += -luuid -lole32 -loleaut32
} else:unix {
    QT += dbus
    SOURCES += $$PWD/systemmonitor_dbus.cpp
} else {
    error(unsupported platform)
}

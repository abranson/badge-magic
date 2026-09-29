QT += core gui quick dbus testlib
CONFIG += console testcase c++17
TEMPLATE = app
TARGET = tst_badgemagic
INCLUDEPATH += ../src
SOURCES += tst_badgemagic.cpp \
    ../src/badgeblemanager.cpp \
    ../src/badgeencoder.cpp \
    ../src/badgestore.cpp \
    ../src/badgepreviewitem.cpp
HEADERS += ../src/badgeblemanager.h ../src/badgepreviewitem.h

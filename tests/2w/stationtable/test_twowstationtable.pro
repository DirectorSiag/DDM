QT += core testlib
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = test_twowstationtable

INCLUDEPATH += \
    $$PWD/../../../src \
    $$PWD/../../../src/model \
    $$PWD/../../../src/model/2w

SOURCES += \
    test_twowstationtable.cpp

HEADERS += \
    $$PWD/../../../src/model/2w/twoWStationTable.h

RESOURCES += \
    resources_test.qrc

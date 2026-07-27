QT += core testlib

CONFIG += console c++17
CONFIG -= app_bundle

TEMPLATE = app
TARGET = test_borneocalculator

INCLUDEPATH += \
    $$PWD/../../../src \
    $$PWD/../../../src/model \
    $$PWD/../../../src/model/borneo

SOURCES += \
    $$PWD/../../../src/model/borneo/borneoCalculator.cpp \
    test_borneocalculator.cpp

HEADERS += \
    $$PWD/../../../src/model/borneo/borneoCalculator.h \
    $$PWD/../../../src/model/borneo/borneoSessionState.h

RESOURCES += \
    resources_test.qrc
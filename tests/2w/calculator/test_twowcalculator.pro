QT += core testlib
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = test_twowcalculator

INCLUDEPATH += \
    $$PWD/../../../src \
    $$PWD/../../../src/model \
    $$PWD/../../../src/model/2w \
    $$PWD/../../../src/model/utils

SOURCES += \
    $$PWD/../../../src/model/2w/twoWCalculator.cpp \
    $$PWD/../../../src/model/utils/RadarMath.cpp \
    test_twowcalculator.cpp

HEADERS += \
    $$PWD/../../../src/model/2w/twoWCalculator.h \
    $$PWD/../../../src/model/2w/twoWStationTable.h

RESOURCES += \
    resources_test.qrc

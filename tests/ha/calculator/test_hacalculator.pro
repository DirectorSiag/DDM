QT += core testlib
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = test_hacalculator

INCLUDEPATH += \
    $$PWD/../../../src \
    $$PWD/../../../src/model \
    $$PWD/../../../src/model/ha \
    $$PWD/../../../src/model/utils \
    $$PWD/../../../src/model/enums

SOURCES += \
    $$PWD/../../../src/model/ha/haCalculator.cpp \
    $$PWD/../../../src/model/utils/RadarMath.cpp \
    test_hacalculator.cpp

HEADERS += \
    $$PWD/../../../src/model/ha/haCalculator.h \
    $$PWD/../../../src/model/ha/haSessionState.h \
    $$PWD/../../../src/model/utils/RadarMath.h

RESOURCES += resources_test_calculator.qrc

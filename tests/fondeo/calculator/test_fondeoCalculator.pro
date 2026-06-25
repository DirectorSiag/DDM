QT += core testlib
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = test_fondeoCalculator

INCLUDEPATH += \
    $$PWD/../../../src \
    $$PWD/../../../src/model \
    $$PWD/../../../src/model/fondeo \
    $$PWD/../../../src/model/utils

SOURCES += \
    $$PWD/../../../src/model/fondeo/fondeoCalculator.cpp \
    $$PWD/../../../src/model/utils/RadarMath.cpp \
    test_fondeoCalculator.cpp

HEADERS += \
    $$PWD/../../../src/model/fondeo/fondeoCalculator.h \
    $$PWD/../../../src/model/fondeo/fondeoSessionState.h

RESOURCES += \
    resources_test.qrc

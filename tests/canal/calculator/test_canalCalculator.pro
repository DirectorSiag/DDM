QT += core testlib
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = test_canalCalculator

INCLUDEPATH += \
    $$PWD/../../../src \
    $$PWD/../../../src/model \
    $$PWD/../../../src/model/canal \
    $$PWD/../../../src/model/utils

SOURCES += \
    $$PWD/../../../src/model/canal/canalCalculator.cpp \
    $$PWD/../../../src/model/utils/RadarMath.cpp \
    test_canalCalculator.cpp

HEADERS += \
    $$PWD/../../../src/model/canal/canalCalculator.h \
    $$PWD/../../../src/model/canal/canalSessionState.h

RESOURCES += \
    resources_test.qrc

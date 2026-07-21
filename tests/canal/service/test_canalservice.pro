QT += core testlib
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = test_canalservice

INCLUDEPATH += \
    $$PWD/../../../src \
    $$PWD/../../../src/controller \
    $$PWD/../../../src/controller/services \
    $$PWD/../../../src/model \
    $$PWD/../../../src/model/canal \
    $$PWD/../../../src/model/entities \
    $$PWD/../../../src/model/utils

SOURCES += \
    $$PWD/../../../src/controller/services/canalService.cpp \
    $$PWD/../../../src/model/canal/canalCalculator.cpp \
    $$PWD/../../../src/model/utils/RadarMath.cpp \
    $$PWD/../../../src/model/entities/track.cpp \
    test_canalservice.cpp

HEADERS += \
    $$PWD/../../../src/controller/services/canalService.h \
    $$PWD/../../../src/model/canal/canalSessionState.h \
    $$PWD/../../../src/model/canal/canalCalculator.h \
    $$PWD/../../../src/model/commandContext.h \
    $$PWD/../../../src/model/entities/track.h \
    $$PWD/../../../src/model/enums/enums.h

RESOURCES += \
    resources_test.qrc

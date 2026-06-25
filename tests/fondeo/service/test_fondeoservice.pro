QT += core testlib
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = test_fondeoservice

INCLUDEPATH += \
    $$PWD/../../../src \
    $$PWD/../../../src/controller \
    $$PWD/../../../src/controller/services \
    $$PWD/../../../src/model \
    $$PWD/../../../src/model/fondeo \
    $$PWD/../../../src/model/entities \
    $$PWD/../../../src/model/utils

SOURCES += \
    $$PWD/../../../src/controller/services/fondeoService.cpp \
    $$PWD/../../../src/model/fondeo/fondeoCalculator.cpp \
    $$PWD/../../../src/model/utils/RadarMath.cpp \
    $$PWD/../../../src/model/entities/track.cpp \
    test_fondeoservice.cpp

HEADERS += \
    $$PWD/../../../src/controller/services/fondeoService.h \
    $$PWD/../../../src/model/fondeo/fondeoSessionState.h \
    $$PWD/../../../src/model/commandContext.h \
    $$PWD/../../../src/model/entities/track.h \
    $$PWD/../../../src/model/enums/enums.h


RESOURCES += \
    resources_test.qrc

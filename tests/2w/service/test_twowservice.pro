QT += core testlib
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = test_twowservice

INCLUDEPATH += \
    $$PWD/../../../src \
    $$PWD/../../../src/controller \
    $$PWD/../../../src/controller/services \
    $$PWD/../../../src/model \
    $$PWD/../../../src/model/2w \
    $$PWD/../../../src/model/entities \
    $$PWD/../../../src/model/utils

SOURCES += \
    $$PWD/../../../src/controller/services/TwoWService.cpp \
    $$PWD/../../../src/model/2w/twoWCalculator.cpp \
    $$PWD/../../../src/model/utils/RadarMath.cpp \
    $$PWD/../../../src/model/entities/track.cpp \
    test_twowservice.cpp

HEADERS += \
    $$PWD/../../../src/controller/services/TwoWService.h \
    $$PWD/../../../src/model/commandContext.h \
    $$PWD/../../../src/model/2w/twoWCalculator.h \
    $$PWD/../../../src/model/2w/twoWStationTable.h \
    $$PWD/../../../src/model/2w/twoWSessionState.h \
    $$PWD/../../../src/model/entities/track.h \
    $$PWD/../../../src/model/enums/enums.h

RESOURCES += \
    resources_test.qrc

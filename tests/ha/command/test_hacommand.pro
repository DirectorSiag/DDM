QT += core network testlib
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = test_hacommand

INCLUDEPATH += \
    $$PWD/../../../src \
    $$PWD/../../../src/view \
    $$PWD/../../../src/controller \
    $$PWD/../../../src/controller/commands \
    $$PWD/../../../src/controller/services \
    $$PWD/../../../src/model \
    $$PWD/../../../src/model/ha \
    $$PWD/../../../src/model/entities \
    $$PWD/../../../src/model/utils \
    $$PWD/../../../src/model/enums

SOURCES += \
    $$PWD/../../../src/controller/commands/haCommand.cpp \
    $$PWD/../../../src/controller/services/haService.cpp \
    $$PWD/../../../src/model/ha/haCalculator.cpp \
    $$PWD/../../../src/model/ha/haSessionTimer.cpp \
    $$PWD/../../../src/model/utils/RadarMath.cpp \
    $$PWD/../../../src/model/entities/track.cpp \
    $$PWD/../../../src/model/estacionamientocalculator.cpp \
    test_hacommand.cpp

HEADERS += \
    $$PWD/../../../src/controller/commands/haCommand.h \
    $$PWD/../../../src/controller/commands/iCommand.h \
    $$PWD/../../../src/controller/services/haService.h \
    $$PWD/../../../src/model/commandContext.h \
    $$PWD/../../../src/model/ha/haCalculator.h \
    $$PWD/../../../src/model/ha/haSessionState.h \
    $$PWD/../../../src/model/ha/haSessionTimer.h \
    $$PWD/../../../src/model/entities/track.h \
    $$PWD/../../../src/model/enums/enums.h \
    $$PWD/../../../src/model/utils/RadarMath.h \
    $$PWD/../../../src/view/CommandParser.h \
    $$PWD/../../../src/view/iInputParser.h

RESOURCES += resources_test_command.qrc

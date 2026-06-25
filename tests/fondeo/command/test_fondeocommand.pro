QT += core network testlib
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = test_fondeocommand

INCLUDEPATH += \
    $$PWD/../../../src \
    $$PWD/../../../src/view \
    $$PWD/../../../src/controller \
    $$PWD/../../../src/controller/commands \
    $$PWD/../../../src/controller/services \
    $$PWD/../../../src/model \
    $$PWD/../../../src/model/fondeo \
    $$PWD/../../../src/model/entities \
    $$PWD/../../../src/model/utils

SOURCES += \
    $$PWD/../../../src/controller/commanddispatcher.cpp \
    $$PWD/../../../src/controller/commands/fondeoCommand.cpp \
    $$PWD/../../../src/controller/services/fondeoService.cpp \
    $$PWD/../../../src/model/fondeo/fondeoCalculator.cpp \
    $$PWD/../../../src/model/utils/RadarMath.cpp \
    $$PWD/../../../src/model/entities/track.cpp \
    test_fondeocommand.cpp

HEADERS += \
    $$PWD/../../../src/controller/commands/fondeoCommand.h \
    $$PWD/../../../src/controller/services/fondeoService.h \
    $$PWD/../../../src/model/fondeo/fondeoSessionState.h \
    $$PWD/../../../src/model/commandContext.h \
    $$PWD/../../../src/view/iInputParser.h \
    $$PWD/../../../src/controller/commanddispatcher.h \
    $$PWD/../../../src/controller/commands/ICommand.h \
    $$PWD/../../../src/model/entities/track.h \
    $$PWD/../../../src/model/enums/enums.h \
    $$PWD/../../../src/view/CommandParser.h

RESOURCES += \
    resources_test.qrc \
    resources_test.qrc

DISTFILES +=
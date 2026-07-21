QT += core network testlib
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = test_textcommand
INCLUDEPATH += \
    $$PWD/../../../src \
    $$PWD/../../../src/view \
    $$PWD/../../../src/controller \
    $$PWD/../../../src/controller/commands \
    $$PWD/../../../src/controller/services \
    $$PWD/../../../src/model \
    $$PWD/../../../src/model/texto \
    $$PWD/../../../src/model/entities \
    $$PWD/../../../src/model/utils
SOURCES += \
    $$PWD/../../../src/controller/commanddispatcher.cpp \
    $$PWD/../../../src/controller/commands/textCommand.cpp \
    $$PWD/../../../src/controller/services/textService.cpp \
    $$PWD/../../../src/model/texto/textCalculator.cpp \
    $$PWD/../../../src/model/utils/RadarMath.cpp \
    $$PWD/../../../src/model/entities/track.cpp \
    test_textcommand.cpp
HEADERS += \
    $$PWD/../../../src/controller/commands/textCommand.h \
    $$PWD/../../../src/controller/services/textService.h \
    $$PWD/../../../src/model/texto/textSessionState.h \
    $$PWD/../../../src/model/texto/textLabel.h \
    $$PWD/../../../src/model/texto/textCalculator.h \
    $$PWD/../../../src/model/commandContext.h \
    $$PWD/../../../src/view/iInputParser.h \
    $$PWD/../../../src/controller/commanddispatcher.h \
    $$PWD/../../../src/controller/commands/ICommand.h \
    $$PWD/../../../src/model/entities/track.h \
    $$PWD/../../../src/model/enums/enums.h \
    $$PWD/../../../src/view/CommandParser.h
RESOURCES += \
    resources_test.qrc
DISTFILES +=
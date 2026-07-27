QT += core testlib

CONFIG += console c++17
CONFIG -= app_bundle

TEMPLATE = app
TARGET = test_borneocommand

INCLUDEPATH += \
    $$PWD/../../../src \
    $$PWD/../../../src/controller \
    $$PWD/../../../src/controller/commands \
    $$PWD/../../../src/controller/services \
    $$PWD/../../../src/model \
    $$PWD/../../../src/model/borneo \
    $$PWD/../../../src/view

SOURCES += \
    $$PWD/../../../src/controller/commands/borneoCommand.cpp \
    $$PWD/../../../src/controller/services/borneoService.cpp \
    $$PWD/../../../src/model/borneo/borneoCalculator.cpp \
    $$PWD/../../../src/model/borneo/buqueClaseCatalog.cpp \
    test_borneocommand.cpp

HEADERS += \
    $$PWD/../../../src/controller/commands/borneoCommand.h \
    $$PWD/../../../src/controller/services/borneoService.h \
    $$PWD/../../../src/model/borneo/borneoCalculator.h \
    $$PWD/../../../src/model/borneo/borneoSessionState.h \
    $$PWD/../../../src/model/borneo/buqueClaseCatalog.h \
    $$PWD/../../../src/model/commandContext.h \
    $$PWD/../../../src/controller/commands/iCommand.h \
    $$PWD/../../../src/view/CommandParser.h

RESOURCES += \
    resources_test.qrc
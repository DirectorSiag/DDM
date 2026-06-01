QT += core network testlib
CONFIG += console c++17
CONFIG -= app_bundle

TEMPLATE = app
TARGET = test_twowcommand

# Permitir que el test encuentre los headers del sistema principal (3 niveles arriba)
INCLUDEPATH += \
    $$PWD/../../../src \
    $$PWD/../../../src/controller \
    $$PWD/../../../src/controller/commands \
    $$PWD/../../../src/model \
    $$PWD/../../../src/model/2w \
    $$PWD/../../../src/model/entities \
    $$PWD/../../../src/model/utils

# Fuentes requeridas para compilar el comando y el despachador
SOURCES += \
    $$PWD/../../../src/controller/commandDispatcher.cpp \
    $$PWD/../../../src/view/CommandParser.cpp \
    $$PWD/../../../src/controller/commandRegistry.cpp \
    $$PWD/../../../src/controller/commands/TwoWCommand.cpp \
    $$PWD/../../../src/controller/services/TwoWService.cpp \
    $$PWD/../../../src/model/2w/twoWCalculator.cpp \
    $$PWD/../../../src/model/utils/RadarMath.cpp \
    $$PWD/../../../src/model/entities/track.cpp \
    test_twowcommand.cpp

HEADERS += \
    $$PWD/../../../src/controller/commands/TwoWCommand.h \
    $$PWD/../../../src/model/commandContext.h

# Archivo de recursos para el JSON de este test
RESOURCES += resources_test.qrc
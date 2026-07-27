QT += core testlib

CONFIG += console c++17
CONFIG -= app_bundle

TEMPLATE = app
TARGET = test_borneoservice

INCLUDEPATH += \
    $$PWD/../../../src \
    $$PWD/../../../src/controller \
    $$PWD/../../../src/controller/services \
    $$PWD/../../../src/model \
    $$PWD/../../../src/model/borneo

SOURCES += \
    $$PWD/../../../src/controller/services/borneoService.cpp \
    $$PWD/../../../src/model/borneo/borneoCalculator.cpp \
    $$PWD/../../../src/model/borneo/buqueClaseCatalog.cpp \
    test_borneoservice.cpp

HEADERS += \
    $$PWD/../../../src/controller/services/borneoService.h \
    $$PWD/../../../src/model/borneo/borneoCalculator.h \
    $$PWD/../../../src/model/borneo/borneoSessionState.h \
    $$PWD/../../../src/model/borneo/buqueClaseCatalog.h \
    $$PWD/../../../src/model/commandContext.h

RESOURCES += \
    resources_test.qrc
QT += core testlib

CONFIG += console c++17
CONFIG -= app_bundle

TEMPLATE = app
TARGET = test_buqueClaseCatalog

INCLUDEPATH += \
    $$PWD/../../../src \
    $$PWD/../../../src/model \
    $$PWD/../../../src/model/borneo

SOURCES += \
    $$PWD/../../../src/model/borneo/buqueClaseCatalog.cpp \
    test_buqueClaseCatalog.cpp

HEADERS += \
    $$PWD/../../../src/model/borneo/buqueClaseCatalog.h

RESOURCES += \
    resources_test.qrc
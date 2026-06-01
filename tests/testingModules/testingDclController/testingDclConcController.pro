QT += core testlib

CONFIG += console c++17 warn_on testcase

TEMPLATE = app
TARGET = testingDclConcController

INCLUDEPATH += \
        ../../../src \
        ../../../src/controller \
        ../../../src/model \
        ../../../src/model/decoders \
        ../../../src/model/network

RESOURCES += ../../../resources.qrc

SOURCES += \
        tst_dclconccontroller.cpp \
        ../../../src/controller/dclConcController.cpp \
        ../../../src/model/decoders/concDecoder.cpp

HEADERS += \
        ../../../src/controller/dclConcController.h \
        ../../../src/model/decoders/concDecoder.h \
        ../../../src/model/decoders/iDecoder.h \
        ../../../src/model/network/iTransport.h

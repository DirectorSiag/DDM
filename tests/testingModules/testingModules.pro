QT += core testlib

CONFIG += console c++17 warn_on testcase

TEMPLATE = app
TARGET = testingModules

INCLUDEPATH += \
        ../../src/model/decoders \
        ../../src/model

RESOURCES += ../../resources.qrc

SOURCES += \
        tst_decoder.cpp \
        ../../src/model/decoders/concDecoder.cpp

HEADERS += \
        ../../src/model/decoders/concDecoder.h \
        ../../src/model/decoders/iDecoder.h
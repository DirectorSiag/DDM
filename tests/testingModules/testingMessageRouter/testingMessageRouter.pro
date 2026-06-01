QT += core testlib

CONFIG += console c++17 warn_on testcase

TEMPLATE = app
TARGET = testingMessageRouter

INCLUDEPATH += \
        ../../../src \
        ../../../src/controller \
        ../../../src/controller/handlers \
        ../../../src/controller/json \
        ../../../src/controller/json/validators \
        ../../../src/model \
        ../../../src/model/decoders \
        ../../../src/model/entities \
        ../../../src/model/network

RESOURCES += ../../../resources.qrc

SOURCES += \
        tst_messagerouter.cpp \
        ../../../src/controller/messagerouter.cpp \
        ../../../src/controller/dclConcController.cpp \
        ../../../src/controller/handlers/linecommandhandler.cpp \
        ../../../src/controller/json/jsoncommandhandler.cpp \
        ../../../src/controller/json/jsonresponsebuilder.cpp \
        ../../../src/controller/json/jsonserializer.cpp \
        ../../../src/controller/json/validators/jsonvalidator.cpp \
        ../../../src/model/decoders/concDecoder.cpp \
        ../../../src/model/entities/cursorEntity.cpp

HEADERS += \
        ../../../src/controller/messagerouter.h \
        ../../../src/controller/dclConcController.h \
        ../../../src/controller/handlers/linecommandhandler.h \
        ../../../src/controller/json/jsoncommandhandler.h \
        ../../../src/controller/json/jsonresponsebuilder.h \
        ../../../src/controller/json/jsonserializer.h \
        ../../../src/controller/json/validators/jsonvalidator.h \
        ../../../src/model/commandContext.h \
        ../../../src/model/decoders/concDecoder.h \
        ../../../src/model/decoders/iDecoder.h \
        ../../../src/model/entities/cursorEntity.h \
        ../../../src/model/entities/track.h \
        ../../../src/model/network/iTransport.h

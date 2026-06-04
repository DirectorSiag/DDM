QT += core testlib

CONFIG += console c++17 warn_on testcase

TEMPLATE = app
TARGET = testingJsonCommandHandler

INCLUDEPATH += \
        ../../../src \
        ../../../src/controller \
        ../../../src/controller/handlers \
        ../../../src/controller/json \
        ../../../src/controller/json/validators \
        ../../../src/model \
        ../../../src/model/entities \
        ../../../src/model/network

SOURCES += \
        tst_jsoncommandhandler.cpp \
        ../../../src/controller/handlers/linecommandhandler.cpp \
        ../../../src/controller/json/jsoncommandhandler.cpp \
        ../../../src/controller/json/jsonresponsebuilder.cpp \
        ../../../src/controller/json/jsonserializer.cpp \
        ../../../src/controller/json/validators/jsonvalidator.cpp \
        ../../../src/model/entities/cursorEntity.cpp

HEADERS += \
        ../../../src/controller/handlers/linecommandhandler.h \
        ../../../src/controller/json/jsoncommandhandler.h \
        ../../../src/controller/json/jsonresponsebuilder.h \
        ../../../src/controller/json/jsonserializer.h \
        ../../../src/controller/json/validators/jsonvalidator.h \
        ../../../src/model/commandContext.h \
        ../../../src/model/entities/cursorEntity.h \
        ../../../src/model/network/iTransport.h

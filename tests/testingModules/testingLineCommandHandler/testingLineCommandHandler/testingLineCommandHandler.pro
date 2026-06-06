QT += core testlib
QT -= gui

CONFIG += qt console c++17 warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app
TARGET = testingLineCommandHandler

INCLUDEPATH += \
        ../../../../src \
        ../../../../src/controller \
        ../../../../src/controller/handlers \
        ../../../../src/controller/json \
        ../../../../src/controller/json/validators \
        ../../../../src/model \
        ../../../../src/model/entities \
        ../../../../src/model/network

SOURCES += \
        ../../../../src/controller/handlers/linecommandhandler.cpp \
        ../../../../src/controller/json/jsonresponsebuilder.cpp \
        ../../../../src/controller/json/jsonserializer.cpp \
        ../../../../src/controller/json/validators/jsonvalidator.cpp \
        ../../../../src/model/entities/cursorEntity.cpp \
        tst_linecommandhandler.cpp

HEADERS += \
        ../../../../src/controller/handlers/linecommandhandler.h \
        ../../../../src/controller/json/jsonresponsebuilder.h \
        ../../../../src/controller/json/jsonserializer.h \
        ../../../../src/controller/json/validators/jsonvalidator.h \
        ../../../../src/model/commandContext.h \
        ../../../../src/model/entities/cursorEntity.h \
        ../../../../src/model/network/iTransport.h

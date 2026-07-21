QT += core testlib
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = test_textservice
INCLUDEPATH += \
    $$PWD/../../../src \
    $$PWD/../../../src/controller \
    $$PWD/../../../src/controller/services \
    $$PWD/../../../src/model \
    $$PWD/../../../src/model/texto \
    $$PWD/../../../src/model/entities \
    $$PWD/../../../src/model/utils
SOURCES += \
    $$PWD/../../../src/controller/services/textService.cpp \
    $$PWD/../../../src/model/texto/textCalculator.cpp \
    $$PWD/../../../src/model/utils/RadarMath.cpp \
    $$PWD/../../../src/model/entities/track.cpp \
    test_textservice.cpp
HEADERS += \
    $$PWD/../../../src/controller/services/textService.h \
    $$PWD/../../../src/model/texto/textSessionState.h \
    $$PWD/../../../src/model/texto/textLabel.h \
    $$PWD/../../../src/model/texto/textCalculator.h \
    $$PWD/../../../src/model/commandContext.h \
    $$PWD/../../../src/model/entities/track.h \
    $$PWD/../../../src/model/enums/enums.h
RESOURCES += \
    resources_test.qrc
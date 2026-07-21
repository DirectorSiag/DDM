QT += core testlib
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = test_textCalculator
INCLUDEPATH += \
    $$PWD/../../../src \
    $$PWD/../../../src/model \
    $$PWD/../../../src/model/texto \
    $$PWD/../../../src/model/utils
SOURCES += \
    $$PWD/../../../src/model/texto/textCalculator.cpp \
    $$PWD/../../../src/model/utils/RadarMath.cpp \
    test_textCalculator.cpp
HEADERS += \
    $$PWD/../../../src/model/texto/textCalculator.h \
    $$PWD/../../../src/model/texto/textLabel.h
RESOURCES += \
    resources_test.qrc
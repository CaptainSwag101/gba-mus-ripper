TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CFLAGS += -std=gnu++11
QMAKE_CXXFLAGS += -m32 -Wall -fdata-sections -ffunction-sections -fmax-errors=5 -Os -s -fwhole-program
QMAKE_LFLAGS += -static

SOURCES += \
    gba_mus_ripper.cpp

HEADERS += \
    hex_string.hpp

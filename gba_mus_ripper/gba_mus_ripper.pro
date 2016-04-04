TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CXXFLAGS += -std=gnu++11 -m32 -Wall -fdata-sections -ffunction-sections -fmax-errors=5 -Os -s -fwhole-program -static

SOURCES += \
    gba_mus_ripper.cpp

HEADERS += \
    hex_string.hpp

DISTFILES += \
    goldensun_synth.raw \
    psg_data.raw

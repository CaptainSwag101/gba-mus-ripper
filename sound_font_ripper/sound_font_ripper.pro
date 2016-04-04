TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CXXFLAGS += -std=gnu++11 -m32 -Wall -fdata-sections -ffunction-sections -fmax-errors=5 -Os -s -fwhole-program -static

SOURCES += \
    sf2.cpp \
    sound_font_riper.cpp \
    gba_samples.cpp \
    gba_instr.cpp

HEADERS += \
    sf2.hpp \
    sf2_chunks.hpp \
    sf2_types.hpp \
    gba_samples.hpp \
    gba_instr.hpp \
    hex_string.hpp

DISTFILES += \
    goldensun_synth.raw \
    psg_data.raw

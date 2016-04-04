TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CFLAGS += -std=gnu++11
QMAKE_LFLAGS += -static

SOURCES += \
    sf2.cpp \
    gba_samples.cpp \
    gba_instr.cpp \
    sound_font_ripper.cpp

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

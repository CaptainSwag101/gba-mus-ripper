TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CFLAGS += -std=gnu++11
QMAKE_CXXFLAGS += -Wall -fdata-sections -ffunction-sections -fmax-errors=5 -O2 -s -fwhole-program
QMAKE_LFLAGS += -static

SOURCES += \
    song_ripper.cpp \
    midi.cpp \

HEADERS += \
    midi.hpp

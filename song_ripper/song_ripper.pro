TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CFLAGS += -std=gnu++11
QMAKE_LFLAGS += -static

SOURCES += \
    song_riper.cpp \
    midi.cpp

HEADERS += \
    midi.hpp

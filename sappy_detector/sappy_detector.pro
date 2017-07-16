TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CFLAGS += -std=c99
QMAKE_CXXFLAGS += -Wall -fdata-sections -ffunction-sections -fmax-errors=5 -O2 -s -fwhole-program
QMAKE_LFLAGS += -static

SOURCES += \
    sappy_detector.c

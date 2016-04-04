TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CXXFLAGS += -std=c99 -m32 -Wall -fdata-sections -ffunction-sections -fmax-errors=5 -Os -s -fwhole-program -static

SOURCES += \
    sappy_detector.c

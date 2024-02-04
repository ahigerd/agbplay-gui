TEMPLATE = app
QT = core gui widgets
CONFIG += c++17
OBJECTS_DIR = .build
MOC_DIR = .build
RCC_DIR = .build

HEADERS += src/PianoKeys.h   src/VUMeter.h   src/TrackHeader.h
SOURCES += src/PianoKeys.cpp src/VUMeter.cpp src/TrackHeader.cpp

HEADERS += src/TrackView.h   src/TrackList.h   src/PlayerWindow.h
SOURCES += src/TrackView.cpp src/TrackList.cpp src/PlayerWindow.cpp

SOURCES += src/main.cpp

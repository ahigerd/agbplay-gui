TEMPLATE = app
QT = core gui widgets
CONFIG += c++17
OBJECTS_DIR = .build
MOC_DIR = .build
RCC_DIR = .build
INCLUDEPATH += $${_PRO_FILE_PWD_}/src $${_PRO_FILE_PWD_}/agbplay/src
QMAKE_CXXFLAGS += -D_XOPEN_SOURCE=700 -Wall -Wextra -Wconversion -Wunreachable-code -Wno-float-conversion
CONFIG += debug
CONFIG -= release
LIBS += -lm -pthread
unix {
  CONFIG += link_pkgconfig
  PKGCONFIG += sndfile portaudio-2.0
}
else {
  LIBS += -lsndfile -lportaudio
}

RESOURCES += agbplay.qrc

GUI_CLASS += PianoKeys VUMeter TrackHeader TrackView TrackList
GUI_CLASS += RomView PlayerWindow SongModel Player UiUtils
for(F, GUI_CLASS) {
  HEADERS += src/$${F}.h
  SOURCES += src/$${F}.cpp
}

AGBPLAY += CGBChannel CGBPatterns Debug GameConfig PlayerContext
AGBPLAY += SequenceReader SoundMixer ReverbEffect LoudnessCalculator
AGBPLAY += SoundChannel Resampler Rom SoundData SongEntry Types Xcept
for(F, AGBPLAY) {
  HEADERS += agbplay/src/$${F}.h
  SOURCES += agbplay/src/$${F}.cpp
}

# files with the implementations replaced with Qt equivalents
HEADERS += agbplay/src/ConfigManager.h agbplay/src/OS.h
SOURCES += src/ConfigManager.cpp       src/OS.cpp

SOURCES += src/main.cpp

# If git commands can be run without errors, grab the commit hash as a version number
system(git log -1 --pretty=format:) {
  VERSION = 1.0.0-$$system(git log -1 --pretty=format:%h)
}
# Otherwise just use a dummy version number
else {
  VERSION = 1.0.0
}

DEFINES += AGBPLAY_VERSION=$${VERSION}

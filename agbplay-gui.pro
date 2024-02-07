TEMPLATE = app
QT = core gui widgets
CONFIG += c++17
OBJECTS_DIR = .build
MOC_DIR = .build
RCC_DIR = .build
INCLUDEPATH += $${_PRO_FILE_PWD_}/src $${_PRO_FILE_PWD_}/agbplay/src $${_PRO_FILE_PWD_}
QMAKE_CXXFLAGS += -D_XOPEN_SOURCE=700 -Wall -Wextra -Wconversion -Wunreachable-code -Wno-float-conversion
CONFIG += debug
CONFIG -= release debug_and_release
CONFIG += link_pkgconfig
packagesExist(portaudio-2.0) {
  PKGCONFIG += portaudio-2.0
}
else {
  LIBS += -lportaudio
}
win32 {
  CONFIG += static
  QMAKE_LFLAGS += -static-libgcc -static-libstdc++ -static
}

RESOURCES += agbplay.qrc

GUI_CLASS += PianoKeys VUMeter TrackHeader TrackView TrackList
GUI_CLASS += RomView PlayerWindow SongModel Player UiUtils
GUI_CLASS += PlayerControls
for(F, GUI_CLASS) {
  HEADERS += src/$${F}.h
  SOURCES += src/$${F}.cpp
}

AGBPLAY += CGBChannel CGBPatterns Debug GameConfig PlayerContext
AGBPLAY += SequenceReader SoundMixer ReverbEffect LoudnessCalculator
AGBPLAY += SoundChannel Resampler Rom SoundData SongEntry Types Xcept
AGBPLAY += Ringbuffer
for(F, AGBPLAY) {
  HEADERS += agbplay/src/$${F}.h
  SOURCES += agbplay/src/$${F}.cpp
}

# files with the implementations replaced with Qt equivalents
HEADERS += agbplay/src/ConfigManager.h agbplay/src/OS.h
SOURCES += src/ConfigManager.cpp       src/OS.cpp

SOURCES += src/main.cpp

VERSION = 1.0.0
# If git commands can be run without errors, grab the commit hash
system(git log -1 --pretty=format:) {
  BUILD_HASH = -$$system(git log -1 --pretty=format:%h)
}
else {
  BUILD_HASH =
}

DEFINES += AGBPLAY_VERSION=$${VERSION}$${BUILD_HASH}

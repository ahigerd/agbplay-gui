TEMPLATE = app
QT = core gui widgets
CONFIG += c++17
OBJECTS_DIR = .build
MOC_DIR = .build
RCC_DIR = .build
INCLUDEPATH += $${_PRO_FILE_PWD_}/src $${_PRO_FILE_PWD_}/agbplay/src
QMAKE_CXXFLAGS += -D_XOPEN_SOURCE=700 -Wall -Wextra -Wconversion -Wunreachable-code
LIBS += -lm -pthread
unix {
  CONFIG += link_pkgconfig
  PKGCONFIG += sndfile portaudio-2.0
}
else {
  LIBS += -lsndfile -lportaudio
}

RESOURCES += agbplay.qrc

HEADERS += src/PianoKeys.h   src/VUMeter.h   src/TrackHeader.h
SOURCES += src/PianoKeys.cpp src/VUMeter.cpp src/TrackHeader.cpp

HEADERS += src/TrackView.h   src/TrackList.h   src/PlayerWindow.h
SOURCES += src/TrackView.cpp src/TrackList.cpp src/PlayerWindow.cpp

HEADERS += src/RomView.h
SOURCES += src/RomView.cpp

# files with the implementations replaced with Qt equivalents
HEADERS += agbplay/src/ConfigManager.h agbplay/src/OS.h
SOURCES += src/ConfigManager.cpp       src/OS.cpp

AGBPLAY += CGBChannel CGBPatterns Debug GameConfig Resampler Rom
AGBPLAY += SoundData SongEntry Types Xcept
for(F, AGBPLAY) {
  HEADERS += agbplay/src/$${F}.h
  SOURCES += agbplay/src/$${F}.cpp
}

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

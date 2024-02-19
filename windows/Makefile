QT_CONFIG_OPTS := -nomake examples -nomake tests -no-dbus -no-feature-testlib -no-feature-concurrent -no-feature-sql -no-feature-network
QT_CONFIG_OPTS := $(QT_CONFIG_OPTS) -release -c++std c++17 -static -opengl desktop -opensource -confirm-license -prefix $(realpath .)
PA_CONFIG_OPTS := --disable-alsa --disable-external-libs --disable-mpeg --without-jack --without-oss --with-winapi=wmme,wasapi
PA_CONFIG_OPTS := $(PA_CONFIG_OPTS) --enable-static --disable-shared --disable-full-suite

ifdef CROSS
ifeq ($(CROSS),mingw32)
	XPLAT = i686-w64-mingw32-
	XPLAT_SUFFIX =
	PA_BUILD = mingw32
	PA_TARGET = i686-pc-mingw64
else ifeq ($(CROSS),mingw64)
	XPLAT = x86_64-w64-mingw32-
	XPLAT_SUFFIX =
	PA_BUILD = mingw64
	PA_TARGET = x86_64-pc-mingw64
else ifeq ($(CROSS),mingw32-posix)
	XPLAT = i686-w64-mingw32-
	XPLAT_SUFFIX = -posix
	PA_BUILD = mingw32
	PA_TARGET = i686-pc-mingw64
else ifeq ($(CROSS),mingw64-posix)
	XPLAT = x86_64-w64-mingw32-
	XPLAT_SUFFIX = -posix
	PA_BUILD = mingw64
	PA_TARGET = x86_64-pc-mingw64
else
$(error Unknown CROSS, expected mingw32 or mingw64)
endif
	QT_CONFIG_OPTS := $(QT_CONFIG_OPTS) -xplatform win32-g++ -device-option CROSS_COMPILE=$(XPLAT) --hostprefix=$(realpath .)/cross
	PA_CONFIG_OPTS := $(PA_CONFIG_OPTS) --build=$(PA_BUILD) CC=$(XPLAT)gcc$(XPLAT_SUFFIX) CXX=$(XPLAT)g++$(XPLAT_SUFFIX) --target=$(PA_TARGET) --host=$(PA_TARGET) --prefix=$(realpath .)
	CROSS_QMAKE := PKG_CONFIG=$(XPLAT)pkg-config QMAKE_CXX=$(XPLAT)g++$(XPLAT_SUFFIX) QMAKE_LINK=$(XPLAT)g++$(XPLAT_SUFFIX)
else ifneq ($(OS),Windows_NT)
$(error Cross-compilation requires CROSS=mingw32 or CROSS=mingw64)
else
	PA_CONFIG_OPTS := $(PA_CONFIG_OPTS) --prefix=$(realpath .)
	CROSS_QMAKE :=
endif

all: agbplay-gui.exe

qt-static: lib/libQt5Widgets.a

qtbase/configure:
	git clone --depth=1 --single-branch --branch=5.15 https://github.com/qt/qtbase

qtbase/.config.notes: qtbase/configure
	+cd qtbase && ./configure $(QT_CONFIG_OPTS)

lib/libQt5Widgets.a: qtbase/.config.notes
	+$(MAKE) -C qtbase

portaudio-static: lib/libportaudio.a

portaudio/configure:
	git clone --depth=1 --single-branch --branch=v19.7.0 https://github.com/PortAudio/portaudio

lib/libportaudio.a: portaudio/configure
	+cd portaudio && ./configure $(PA_CONFIG_OPTS)
	+$(MAKE) -C portaudio
	+$(MAKE) -C portaudio install

Makefile.agbplay-gui: lib/libportaudio.a lib/libQt5Widgets.a ../agbplay-gui.pro
	mkdir -p .build
	./qtbase/bin/qmake -o Makefile.agbplay-gui .. $(CROSS_QMAKE) PA_ROOT=.

agbplay-gui.exe: Makefile.agbplay-gui lib/libportaudio.a lib/libQt5Widgets.a FORCE
	mkdir -p .build
	+$(MAKE) -f Makefile.agbplay-gui

buildclean:
	-cd qtbase && git clean -dxf
	-cd qtbase && git clean -dXf
	-cd portaudio && git clean -dxf
	-cd portaudio && git clean -dXf
	-rm -rf .build bin lib include pkgconfig mkspecs cross doc plugins
	-rm -f Makefile.agbplay-gui
	-rm -f agbplay-gui.exe agbplay-gui_plugin_import.cpp agbplay-gui_resource.rc

FORCE:

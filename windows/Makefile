QT_CONFIG_OPTS := -nomake examples -nomake tests -release -c++std c++17 -static -no-dbus -opengl desktop -opensource -confirm-license -hostprefix $(realpath .)
PA_CONFIG_OPTS := --disable-alsa --disable-external-libs --disable-mpeg --without-jack --without-oss --with-winapi=wmme,wasapi
PA_CONFIG_OPTS := $(PA_CONFIG_OPTS) --enable-static --disable-shared --disable-full-suite --prefix=$(realpath .)

ifdef CROSS
	ifeq ($(CROSS),mingw32)
		XPLAT = i686-w64-mingw32-
		PA_BUILD = mingw32
		PA_TARGET = i686-pc-mingw64
	else ifeq ($(CROSS),mingw64)
		XPLAT = x86_64-w64-mingw32-
		PA_BUILD = mingw64
		PA_TARGET = x86_64-pc-mingw64
	else
		$(error Unknown CROSS, expected mingw32 or mingw64)
	endif
	QT_CONFIG_OPTS := $(QT_CONFIG_OPTS) -xplatform win32-g++ -device-option CROSS_COMPILE=$(XPLAT)
	HOST_TRIPLET := $(shell ./portaudio/config.guess)
	PA_CONFIG_OPTS := $(PA_CONFIG_OPTS) --build=$(PA_BUILD) CC=$(XPLAT)gcc-posix CXX=$(XPLAT)g++-posix --target=$(PA_TARGET) --host=$(HOST_TRIPLET)
endif

all: agbplay-gui.exe

qt-static: lib/libQtWidgets5.a

qtbase:
	git clone --depth=1 --single-branch --branch=5.15 https://github.com/qt/qtbase

qtbase/Makefile: qtbase
	+cd qtbase && ./configure $(QT_CONFIG_OPTS)

qtbase/lib/libQtWidgets5.a: qtbase/Makefile
	+$(MAKE) -C qtbase

lib/libQtWidgets5.a: qtbase/lib/libQtWidgets5.a
	+$(MAKE) -C qtbase install

portaudio-static: lib/libportaudio.a

portaudio:
	git clone --depth=1 --single-branch --branch=v19.7.0 https://github.com/PortAudio/portaudio

portaudio/Makefile: portaudio
	+cd portaudio && ./configure $(PA_CONFIG_OPTS)

portaudio/lib/.libs/libportaudio.a: portaudio/Makefile
	+$(MAKE) -C portaudio

lib/libportaudio.a: portaudio/lib/.libs/libportaudio.a
	+$(MAKE) -C portaudio install

Makefile.agbplay-gui: lib/libportaudio.a lib/libQtWidgets5.a ../agbplay-gui.pro
	PKG_CONFIG_PATH=$(realpath ./lib/pkgconfig) ./bin/qmake -o Makefile.agbplay-gui ..

agbplay-gui.exe: Makefile.agbplay-gui lib/libportaudio.a lib/libQtWidgets5.a
	+$(MAKE) -f Makefile.agbplay-gui

buildclean:
	cd qtbase && git clean -dxf
	cd qtbase && git clean -dXf
	cd portaudio && git clean -dxf
	cd portaudio && git clean -dXf

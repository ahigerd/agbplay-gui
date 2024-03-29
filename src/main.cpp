#include <QApplication>
#include <QMessageBox>
#include <iostream>
#include <cstring>
#include <cstdio>
#include <clocale>
#include <portaudio.h>

#include "PlayerWindow.h"
#include "Player.h"
#include "Debug.h"
#include "Xcept.h"
#include "ConfigManager.h"

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)
#define AGBPLAY_VERSION_STRING STRINGIFY(AGBPLAY_VERSION)

int main(int argc, char** argv)
{
  QCoreApplication::setApplicationName("agbplay");
  QCoreApplication::setApplicationVersion(AGBPLAY_VERSION_STRING);
  QCoreApplication::setOrganizationName("ipatix");
  QCoreApplication::setOrganizationDomain("ipatix.agbplay");

  QApplication app(argc, argv);
  app.setWindowIcon(QIcon(":/logo.png"));

  if (!Debug::open("/dev/stderr") && !Debug::open(nullptr)) {
    QMessageBox::critical(nullptr, "agbplay-gui", PlayerWindow::tr("Debug Init failed"));
    return EXIT_FAILURE;
  }

  setlocale(LC_ALL, "");
  if (Pa_Initialize() != paNoError) {
    QMessageBox::critical(nullptr, "agbplay-gui", PlayerWindow::tr("Couldn't init portaudio"));
    return EXIT_FAILURE;
  }

  int result;
  /* scope */ {
    Player player;
    try {
      player.detectHostApi();
    } catch (std::exception& e) {
      QMessageBox::critical(nullptr, "agbplay-gui", e.what());
      return EXIT_FAILURE;
    }

    std::cout << "Loading Config..." << std::endl;
    ConfigManager::Instance().Load();

    PlayerWindow wgui(&player);
    wgui.show();

    QStringList args = app.arguments();
    if (args.length() > 1) {
      wgui.openRom(args[1]);
    }

    result = app.exec();
  }

  if (Pa_Terminate() != paNoError)
      std::cerr << "Error while terminating portaudio" << std::endl;
  Debug::close();
  return result;
}

#include <QApplication>
#include <QMessageBox>
#include <iostream>
#include <cstring>
#include <cstdio>
#include <clocale>
#include <portaudio.h>

#include "PlayerWindow.h"
//#include "SoundData.h"
#include "Debug.h"
#include "Xcept.h"
#include "ConfigManager.h"

int main(int argc, char** argv)
{
  QApplication app(argc, argv);
  app.setWindowIcon(QIcon(":/logo.png"));

  if (!Debug::open("/dev/stderr")) {
    QMessageBox::critical(nullptr, "agbplay-gui", PlayerWindow::tr("Debug Init failed"));
    return EXIT_FAILURE;
  }

  setlocale(LC_ALL, "");
  if (Pa_Initialize() != paNoError) {
    QMessageBox::critical(nullptr, "agbplay-gui", PlayerWindow::tr("Couldn't init portaudio"));
    return EXIT_FAILURE;
  }

  std::cout << "Loading Config..." << std::endl;
  ConfigManager::Instance().Load();

  PlayerWindow wgui;
  wgui.show();

  int result = app.exec();

  if (Pa_Terminate() != paNoError)
      std::cerr << "Error while terminating portaudio" << std::endl;
  Debug::close();
  return result;
}

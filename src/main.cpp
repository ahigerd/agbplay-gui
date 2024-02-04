#include <QApplication>
#include "PlayerWindow.h"

int main(int argc, char** argv)
{
  QApplication app(argc, argv);
  PlayerWindow w;
  w.show();
  return app.exec();
}

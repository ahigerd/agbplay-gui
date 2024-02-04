#pragma once

#include <QMainWindow>
class TrackList;

class PlayerWindow : public QMainWindow
{
Q_OBJECT
public:
  PlayerWindow(QWidget* parent = nullptr);

private:
  TrackList* trackList;
};

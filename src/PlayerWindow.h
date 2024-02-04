#pragma once

#include <QMainWindow>
class TrackList;
class QStandardItemModel;
class QTreeView;
class QLabel;

class PlayerWindow : public QMainWindow
{
Q_OBJECT
public:
  PlayerWindow(QWidget* parent = nullptr);

private:
  QLabel* makeTitle();
  QTreeView* makeView(const QString& label, QStandardItemModel* model);

  TrackList* trackList;
  QStandardItemModel* songs;
  QStandardItemModel* playlist;
};

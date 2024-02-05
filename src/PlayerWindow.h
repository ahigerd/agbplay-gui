#pragma once

#include <QMainWindow>
class TrackList;
class QStandardItemModel;
class QTreeView;
class QLabel;
class QPlainTextEdit;
class RomView;

class PlayerWindow : public QMainWindow
{
Q_OBJECT
public:
  PlayerWindow(QWidget* parent = nullptr);

public slots:
  void openRom();
  void about();

private:
  QLayout* makeTop();
  QLayout* makeLeft();
  QLayout* makeRight();
  void makeMenu();

  QLabel* makeTitle();
  QTreeView* makeView(const QString& label, QStandardItemModel* model);

  TrackList* trackList;
  QTreeView* songList;
  QTreeView* playlistView;
  RomView* romView;
  QPlainTextEdit* log;

  QStandardItemModel* songs;
  QStandardItemModel* playlist;
};

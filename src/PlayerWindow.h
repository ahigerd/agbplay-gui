#pragma once

#include <QMainWindow>
class TrackList;
class SongModel;
class QAbstractItemModel;
class QStandardItemModel;
class QTreeView;
class QLabel;
class QPlainTextEdit;
class SongTable;
class RomView;
class Rom;

class PlayerWindow : public QMainWindow
{
Q_OBJECT
public:
  PlayerWindow(QWidget* parent = nullptr);

public slots:
  void openRom();
  void openRom(const QString& path);
  void about();

signals:
  void romUpdated(Rom*);
  void songTableUpdated(SongTable*);

private:
  QLayout* makeTop();
  QLayout* makeLeft();
  QLayout* makeRight();
  void makeMenu();

  QLabel* makeTitle();
  QTreeView* makeView(QAbstractItemModel* model);

  TrackList* trackList;
  QTreeView* songList;
  QTreeView* playlistView;
  RomView* romView;
  QPlainTextEdit* log;

  SongModel* songs;
  QStandardItemModel* playlist;
};

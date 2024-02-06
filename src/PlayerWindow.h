#pragma once

#include <QMainWindow>
#include <memory>
#include "PlayerContext.h"
class TrackList;
class SongModel;
class QAbstractItemModel;
class QStandardItemModel;
class QTreeView;
class QLabel;
class QPlainTextEdit;
class VUMeter;
class VUState;
class SongTable;
class Player;
class RomView;
class Rom;

class PlayerWindow : public QMainWindow
{
Q_OBJECT
public:
  PlayerWindow(Player* player, QWidget* parent = nullptr);

public slots:
  void openRom();
  void openRom(const QString& path);
  void about();

signals:
  void romUpdated(Rom*);

protected:
  void closeEvent(QCloseEvent*);

private slots:
  void selectSong(const QModelIndex& index);
  void updateVU(PlayerContext*, VUState* vu);

private:
  QLayout* makeTop();
  QLayout* makeLeft();
  QLayout* makeRight();
  void makeMenu();

  QLabel* makeTitle();
  QTreeView* makeView(QAbstractItemModel* model);

  VUMeter* masterVU;
  TrackList* trackList;
  QTreeView* songList;
  QTreeView* playlistView;
  RomView* romView;
  QPlainTextEdit* log;

  SongModel* songs;
  QStandardItemModel* playlist;
  Player* player;
};

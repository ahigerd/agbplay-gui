#pragma once

#include <QMainWindow>
#include <QItemSelection>
#include <memory>
#include "PlayerContext.h"
class TrackList;
class SongModel;
class PlaylistModel;
class QAbstractItemModel;
class QTreeView;
class QLabel;
class QPlainTextEdit;
class QProgressBar;
class VUMeter;
class VUState;
class SongTable;
class Player;
class PlayerControls;
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
  void clearRecents();
  void openRecent(QAction* action);
  void songListMenu(const QPoint& pos);
  void playlistDirty(bool dirty);
  void clearOtherSelection(const QItemSelection& sel);

  void promptForExport();
  void promptForExportChannels();
  void promptForExportAll();
  void promptForExportPlaylist();
  void exportStarted(const QString& path);
  void exportFinished(const QString& path);
  void exportError(const QString& message);
  void exportCancelled();
  void playbackError(const QString& message);

private:
  QLayout* makeTop();
  QLayout* makeLeft();
  QLayout* makeRight();
  void makeMenu();
  QLabel* makeTitle();
  QTreeView* makeView(QAbstractItemModel* model);

  void fillRecents();
  void addRecent(const QString& path);
  void logMessage(const QString& message);
  void promptForExport(const QModelIndexList& items, bool split = false);
  void updateExportProgress();
  QModelIndexList selectedIndexes() const;

  VUMeter* masterVU;
  TrackList* trackList;
  QTreeView* songList;
  QTreeView* playlistView;
  RomView* romView;
  QPlainTextEdit* log;
  QWidget* progressPanel;
  QProgressBar* exportProgress;

  SongModel* songs;
  PlaylistModel* playlist;
  Player* player;
  PlayerControls* controls;
  QMenu* recentsMenu;
  QAction* saveAction;
  QAction* exportAction;
  QAction* exportChannelsAction;
  QAction* exportAllAction;
  QAction* exportPlaylistAction;

  bool playlistIsDirty;
};

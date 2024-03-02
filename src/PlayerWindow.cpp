#include "PlayerWindow.h"
#include "TrackList.h"
#include "VUMeter.h"
#include "RomView.h"
#include "Rom.h"
#include "ConfigManager.h"
#include "SongModel.h"
#include "Player.h"
#include "PlayerControls.h"
#include "PlaylistModel.h"
#include "UiUtils.h"
#include <QApplication>
#include <QBoxLayout>
#include <QGridLayout>
#include <QTreeView>
#include <QLabel>
#include <QFont>
#include <QFontDatabase>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QPlainTextEdit>
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QSettings>
#include <QProgressBar>
#include <QPushButton>

PlayerWindow::PlayerWindow(Player* player, QWidget* parent)
: QMainWindow(parent), player(player), playlistIsDirty(false)
{
  setWindowTitle("agbplay");
  songs = player->songModel();
  playlist = new PlaylistModel(songs);

  QWidget* base = new QWidget(this);
  setCentralWidget(base);

  QVBoxLayout* vbox = new QVBoxLayout(base);
  vbox->addLayout(makeTop(), 0);

  QHBoxLayout* hbox = new QHBoxLayout;
  hbox->addLayout(makeLeft(), 1);
  hbox->addLayout(makeRight(), 4);
  vbox->addLayout(hbox, 1);

  setMenuBar(new QMenuBar(this));
  makeMenu();

  QObject::connect(this, SIGNAL(romUpdated(Rom*)), romView, SLOT(updateRom(Rom*)));
  QObject::connect(player, SIGNAL(songTableUpdated(SongTable*)), romView, SLOT(updateSongTable(SongTable*)));
  QObject::connect(songList, SIGNAL(activated(QModelIndex)), this, SLOT(selectSong(QModelIndex)));
  QObject::connect(songList, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectSong(QModelIndex)));
  QObject::connect(songList->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(clearOtherSelection(QItemSelection)));
  QObject::connect(playlistView, SIGNAL(activated(QModelIndex)), this, SLOT(selectSong(QModelIndex)));
  QObject::connect(playlistView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectSong(QModelIndex)));
  QObject::connect(playlistView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(clearOtherSelection(QItemSelection)));
  QObject::connect(player, SIGNAL(songChanged(PlayerContext*,quint32,QString)), trackList, SLOT(selectSong(PlayerContext*,quint32,QString)));
  QObject::connect(player, SIGNAL(songChanged(PlayerContext*,quint32,QString)), songs, SLOT(songChanged(PlayerContext*,quint32)));
  QObject::connect(player, SIGNAL(songChanged(PlayerContext*,quint32,QString)), controls, SLOT(songChanged(PlayerContext*)));
  QObject::connect(player, SIGNAL(updated(PlayerContext*,VUState*)), trackList, SLOT(update(PlayerContext*,VUState*)));
  QObject::connect(player, SIGNAL(updated(PlayerContext*,VUState*)), this, SLOT(updateVU(PlayerContext*,VUState*)));
  QObject::connect(trackList, SIGNAL(muteToggled(int,bool)), player, SLOT(setMute(int,bool)));
  QObject::connect(controls, SIGNAL(togglePlay()), player, SLOT(togglePlay()));
  QObject::connect(controls, SIGNAL(play()), player, SLOT(play()));
  QObject::connect(controls, SIGNAL(pause()), player, SLOT(pause()));
  QObject::connect(controls, SIGNAL(stop()), player, SLOT(stop()));
  QObject::connect(player, SIGNAL(stateChanged(bool,bool)), controls, SLOT(updateState(bool,bool)));
  QObject::connect(player, SIGNAL(stateChanged(bool,bool)), songs, SLOT(stateChanged(bool,bool)));
  QObject::connect(recentsMenu, SIGNAL(triggered(QAction*)), this, SLOT(openRecent(QAction*)));
  QObject::connect(playlist, SIGNAL(playlistDirty(bool)), this, SLOT(playlistDirty(bool)));
  QObject::connect(player, SIGNAL(exportStarted(QString)), this, SLOT(exportStarted(QString)));
  QObject::connect(player, SIGNAL(exportFinished(QString)), this, SLOT(exportFinished(QString)));
  QObject::connect(player, SIGNAL(exportError(QString)), this, SLOT(exportError(QString)));
  QObject::connect(player, SIGNAL(exportCancelled()), this, SLOT(exportCancelled()));
  QObject::connect(player, SIGNAL(playbackError(QString)), this, SLOT(playbackError(QString)));
}

QLayout* PlayerWindow::makeTop()
{
  QHBoxLayout* layout = new QHBoxLayout;

  layout->addWidget(makeTitle(), 0);

  masterVU = new VUMeter(this);
  masterVU->setStereoLayout(Qt::Vertical);
  layout->addWidget(masterVU, 1);

  return layout;
}

QLayout* PlayerWindow::makeLeft()
{
  QVBoxLayout* layout = new QVBoxLayout;

  songList = makeView(songs);
  songList->setDragDropMode(QAbstractItemView::DragOnly);
  layout->addWidget(songList);

  playlistView = makeView(playlist);
  playlistView->setDragEnabled(true);
  playlistView->setDropIndicatorShown(true);
  playlistView->setAcceptDrops(true);
  playlistView->setDragDropMode(QAbstractItemView::DragDrop);
  layout->addWidget(playlistView);

  return layout;
}

QLayout* PlayerWindow::makeRight()
{
  QGridLayout* grid = new QGridLayout;
  grid->setRowStretch(0, 1);
  grid->setColumnStretch(0, 1);

  grid->addWidget(trackList = new TrackList(this), 0, 0, 2, 1);
  grid->addWidget(romView = new RomView(this), 0, 1);
  grid->addWidget(controls = new PlayerControls(this), 1, 1);
  grid->addWidget(log = new QPlainTextEdit(this), 2, 0, 1, 2);

  progressPanel = new QWidget(this);
  QHBoxLayout* hbox = new QHBoxLayout(progressPanel);;
  hbox->setContentsMargins(0, 0, 0, 0);
  hbox->addWidget(new QLabel(tr("Exporting:"), this), 0);
  hbox->addWidget(exportProgress = new QProgressBar(this), 1);
  exportProgress->setFormat("%v / %m");
  QPushButton* abort = new QPushButton(tr("Abort"), this);
  hbox->addWidget(abort, 0);
  grid->addWidget(progressPanel, 3, 0, 1, 2);
  progressPanel->hide();

  log->setReadOnly(true);
  log->setMaximumHeight(100);

  QObject::connect(abort, SIGNAL(clicked()), player, SLOT(cancelExport()));

  return grid;
}

void PlayerWindow::makeMenu()
{
  QMenuBar* mb = menuBar();
  QMenu* fileMenu = mb->addMenu(tr("&File"));
  fileMenu->addAction(tr("&Open ROM..."), this, SLOT(openRom()), QKeySequence::Open);
  recentsMenu = fileMenu->addMenu(tr("Open &Recent"));
  fillRecents();
  fileMenu->addSeparator();
  saveAction = fileMenu->addAction(tr("&Save Playlist"), playlist, SLOT(save()), QKeySequence::Save);
  fileMenu->addSeparator();
  exportAction = fileMenu->addAction(tr("&Export Selected..."), this, SLOT(promptForExport()), Qt::CTRL | Qt::Key_E);
  exportChannelsAction = fileMenu->addAction(tr("Export &Channels for Selected..."), this, SLOT(promptForExportChannels()));
  exportAllAction = fileMenu->addAction(tr("Export &All..."), this, SLOT(promptForExportAll()));
  exportPlaylistAction = fileMenu->addAction(tr("Export Tracks in &Playlist..."), this, SLOT(promptForExportPlaylist()));
  fileMenu->addSeparator();
  fileMenu->addAction(tr("E&xit"), qApp, SLOT(quit()));

  QMenu* controlMenu = mb->addMenu(tr("&Control"));
  controlMenu->addAction(controls->toggleAction());
  controlMenu->addAction(controls->playAction());
  controlMenu->addAction(controls->pauseAction());
  controlMenu->addAction(controls->stopAction());

  QMenu* helpMenu = mb->addMenu(tr("&Help"));
  helpMenu->addAction(tr("&About..."), this, SLOT(about()));
  helpMenu->addAction(tr("About &Qt..."), qApp, SLOT(aboutQt()));

  saveAction->setEnabled(false);
  exportAction->setEnabled(false);
  exportChannelsAction->setEnabled(false);
  exportAllAction->setEnabled(false);
  exportPlaylistAction->setEnabled(false);
}

QLabel* PlayerWindow::makeTitle()
{
  static const char* agbplayTitle =
    R"(           _         _           )" "\n"
    R"( __ _ __ _| |__ _ __| |__ _ _  _ )" "\n"
    R"(/ _` / _` | '_ \ '_ \ / _` | || |)" "\n"
    R"(\__,_\__, |_.__/ .__/_\__,_|\_, |)" "\n"
    R"(     |___/     |_|          |__/ )" "\n";

  QLabel* title = new QLabel(agbplayTitle, this);
  title->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  QFont font(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  font.setStyleHint(QFont::Monospace);
  font.setPointSize(10);
  font.setBold(true);
  title->setFont(font);
  return title;
}

QTreeView* PlayerWindow::makeView(QAbstractItemModel* model)
{
  QTreeView* view = new QTreeView(this);
  view->setRootIsDecorated(false);
  view->header()->resizeSection(0, 150);
  view->setModel(model);
  view->setSelectionMode(QAbstractItemView::ExtendedSelection);
  view->setEditTriggers(QAbstractItemView::EditKeyPressed | QAbstractItemView::SelectedClicked);
  view->setContextMenuPolicy(Qt::CustomContextMenu);
  QObject::connect(view, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(songListMenu(QPoint)));
  return view;
}

void PlayerWindow::openRom()
{
  QSettings settings;
  QStringList recents = settings.value("recentFiles").toStringList();
  QString lastPath;
  if (!recents.isEmpty()) {
    lastPath = QFileInfo(recents.first()).absolutePath();
  }

  QString path = QFileDialog::getOpenFileName(
    this,
    tr("Open GBA ROM File"),
    lastPath,
    QStringLiteral("%1 (*.gba);;%2 (*)").arg(tr("GBA ROM files")).arg(tr("All files"))
  );
  if (path.isEmpty()) {
    return;
  }
  openRom(path);
}

void PlayerWindow::openRom(const QString& path)
{
  Rom* rom;
  try {
    rom = player->openRom(path);
  } catch (std::exception& e) {
    player->openRom(QString());
    QMessageBox::warning(nullptr, "agbplay", e.what());
    setWindowFilePath(QString());
    setWindowTitle("agbplay");
    return;
  }

  addRecent(path);
  setWindowFilePath(path);
  setWindowTitle(QStringLiteral("agbplay - %1").arg(QFileInfo(path).fileName()));
  emit romUpdated(rom);

  songList->setCurrentIndex(songs->index(0, 0));
  saveAction->setEnabled(true);
  exportAction->setEnabled(true);
  exportChannelsAction->setEnabled(true);
  exportAllAction->setEnabled(true);
  exportPlaylistAction->setEnabled(playlist->rowCount() > 0);
}

void PlayerWindow::about()
{
  QFile about(":/about.html");
  about.open(QIODevice::ReadOnly | QIODevice::Text);
  QMessageBox::about(
    this,
    tr("agbplay-gui (%1)").arg(qApp->applicationVersion()),
    QString::fromUtf8(about.readAll())
  );
}

void PlayerWindow::selectSong(const QModelIndex& index)
{
  QModelIndex songIndex, playlistIndex;
  if (index.model() == songs) {
    songIndex = index;
    playlistIndex = playlist->mapFromSource(index);
  } else {
    playlistIndex = index;
    songIndex = playlist->mapToSource(index);
  }
  try {
    player->selectSong(songIndex.row());
    QTimer::singleShot(0, player, SLOT(play()));
  } catch (std::exception& e) {
    QMessageBox::warning(nullptr, "agbplay", e.what());
    return;
  }
  songList->scrollTo(songIndex);
  if (playlistIndex.isValid()) {
    playlistView->scrollTo(playlistIndex);
  }
}

void PlayerWindow::closeEvent(QCloseEvent*)
{
  if (playlistIsDirty) {
    auto choice = QMessageBox::question(this, "agbplay", tr("Do you want to save your changes to the playlist?"));
    if (choice == QMessageBox::Yes) {
      playlist->save();
    }
  }
  player->stop();
}

void PlayerWindow::updateVU(PlayerContext*, VUState* vu)
{
  masterVU->setLeft(vu->master.left);
  masterVU->setRight(vu->master.right);
}

void PlayerWindow::fillRecents()
{
  QSettings settings;
  QStringList recents = settings.value("recentFiles").toStringList();

  recentsMenu->clear();
  if (recents.isEmpty()) {
    recentsMenu->setEnabled(false);
    return;
  }
  recentsMenu->setEnabled(true);
  int i = 1;
  for (const QString& path : recents) {
    QAction* action = recentsMenu->addAction(QStringLiteral("&%1 - %2").arg(i).arg(QFileInfo(path).fileName()));
    action->setData(path);
  }
  recentsMenu->addSeparator();
  recentsMenu->addAction(tr("&Clear Recent"), this, SLOT(clearRecents()));
}

void PlayerWindow::addRecent(const QString& path)
{
  QSettings settings;
  QStringList recents = settings.value("recentFiles").toStringList();
  recents.removeAll(path);
  recents.insert(0, path);
  while (recents.length() > 4) {
    recents.removeLast();
  }

  settings.setValue("recentFiles", recents);
  fillRecents();
}

void PlayerWindow::clearRecents()
{
  QSettings settings;
  settings.remove("recentFiles");
  fillRecents();
}

void PlayerWindow::openRecent(QAction* action)
{
  QString path = action->data().toString();
  if (!path.isEmpty()) {
    openRom(path);
  }
}

void PlayerWindow::clearOtherSelection(const QItemSelection& sel)
{
  exportAction->setEnabled(playlistView->selectionModel()->hasSelection() || songList->selectionModel()->hasSelection());
  exportChannelsAction->setEnabled(exportAction->isEnabled());

  if (sel.isEmpty()) {
    return;
  }
  QItemSelectionModel* view = qobject_cast<QItemSelectionModel*>(sender());
  if (view == songList->selectionModel()) {
    playlistView->clearSelection();
  } else if (view == playlistView->selectionModel()) {
    songList->clearSelection();
  }
}

void PlayerWindow::songListMenu(const QPoint& pos)
{
  QTreeView* view = qobject_cast<QTreeView*>(sender());
  if (!view) {
    return;
  }

  QModelIndexList items = view->selectionModel()->selectedIndexes();
  if (items.isEmpty()) {
    QModelIndex item = view->indexAt(pos);
    if (item.isValid()) {
      items << item;
    } else {
      return;
    }
  }

  QAction play(style()->standardIcon(QStyle::SP_MediaPlay, nullptr, this), PlayerControls::tr("&Play"));
  QAction enqueue(tr("&Add to Playlist"));
  QAction remove(tr("&Remove from Playlist"));
  QAction rename(tr("Re&name"));
  QAction exportTrack(tr("&Export..."));
  QAction exportChannels(tr("Export &Channels..."));

  QList<QAction*> actions;
  if (items.length() == 1) {
    actions << &play;
  }
  if (view == songList) {
    actions << &enqueue;
  }
  if (view == playlistView) {
    actions << &remove;
  }
  if (items.length() == 1) {
    actions << &rename;
  }
  actions << &exportTrack << &exportChannels;

  QAction* action = QMenu::exec(actions, view->mapToGlobal(pos), actions.first(), view);
  if (action == &play) {
    selectSong(items[0]);
  } else if (action == &enqueue) {
    int end = playlist->rowCount();
    playlist->append(items);
    playlistView->clearSelection();
    for (int i = items.length() - 1; i >= 0; --i) {
      playlistView->selectionModel()->select(playlist->index(end + i), QItemSelectionModel::Select);
    }
    playlistView->scrollTo(playlist->index(end + items.length() - 1), QAbstractItemView::EnsureVisible);
    playlistView->selectionModel()->setCurrentIndex(playlist->index(end), QItemSelectionModel::NoUpdate);
  } else if (action == &remove) {
    playlist->remove(items);
    view->clearSelection();
    view->selectionModel()->setCurrentIndex(view->indexAt(pos), QItemSelectionModel::NoUpdate);
  } else if (action == &rename) {
    view->edit(items[0]);
  } else if (action == &exportTrack) {
    promptForExport();
  } else if (action == &exportChannels) {
    promptForExportChannels();
  }
}

void PlayerWindow::playlistDirty(bool dirty)
{
  playlistIsDirty = dirty;
  exportPlaylistAction->setEnabled(playlist->rowCount() > 0);
}

QModelIndexList PlayerWindow::selectedIndexes() const
{
  QModelIndexList items = songList->selectionModel()->selectedIndexes();

  if (items.isEmpty()) {
    for (const QModelIndex& idx : playlistView->selectionModel()->selectedIndexes()) {
      items << playlist->mapToSource(idx);
    }
  }

  return items;
}

void PlayerWindow::promptForExport()
{
  promptForExport(selectedIndexes());
}

void PlayerWindow::promptForExportChannels()
{
  promptForExport(selectedIndexes(), true);
}

void PlayerWindow::promptForExport(const QModelIndexList& items, bool split)
{
  QSettings settings;
  QString lastExportPath = settings.value("lastExport").toString();

  if (items.length() == 1 && !split) {
    QModelIndex idx = items.first();
    QString name = idx.data(Qt::EditRole).toString();
    QString prefix = fixedNumber(idx.row(), 4);
    if (name.isEmpty()) {
      name = QStringLiteral("%1.wav").arg(prefix);
    } else {
      name = QStringLiteral("%1 - %2.wav").arg(prefix).arg(name);
    }
    QString path = QFileDialog::getSaveFileName(
      this,
      tr("Export track to file"),
      QDir(lastExportPath).absoluteFilePath(name),
      QStringLiteral("%1 (*.wav);;%2 (*)").arg(tr("Wave audio files")).arg(tr("All files"))
    );
    if (!path.isEmpty()) {
      settings.setValue("lastExport", QFileInfo(path).absolutePath());
      exportProgress->setRange(0, 0);
      exportProgress->setValue(0);
      progressPanel->show();
      player->exportToWave(path, idx.row());
    }
  } else {
    QString path = QFileDialog::getExistingDirectory(
      this,
      split ? tr("Export tracks into directory") : tr("Export channels into directory"),
      lastExportPath
    );
    if (!path.isEmpty()) {
      settings.setValue("lastExport", path);
      QList<int> tracks;
      for (const QModelIndex& idx : items) {
        tracks << idx.row();
      }
      exportProgress->setRange(0, items.length());
      exportProgress->setValue(0);
      progressPanel->show();
      player->exportToWave(QDir(path), tracks, split);
    }
  }
}

void PlayerWindow::promptForExportAll()
{
  QModelIndexList items;
  for (int i = 0; i < songs->rowCount(); i++) {
    items << songs->index(i, 0);
  }
  promptForExport(items);
}

void PlayerWindow::promptForExportPlaylist()
{
  QModelIndexList items;
  for (int i = 0; i < playlist->rowCount(); i++) {
    items << playlist->mapToSource(playlist->index(i, 0));
  }
  promptForExport(items);
}

void PlayerWindow::exportStarted(const QString& path)
{
  logMessage(tr("Exporting to %1...").arg(path));
}

void PlayerWindow::exportFinished(const QString& path)
{
  logMessage(tr("Finished exporting %1.").arg(path));
  updateExportProgress();
}

void PlayerWindow::exportError(const QString& message)
{
  logMessage(tr("Error while exporting: %1").arg(message));
}

void PlayerWindow::exportCancelled()
{
  logMessage(tr("Export cancelled."));
  progressPanel->hide();
}

void PlayerWindow::playbackError(const QString& message)
{
  logMessage(tr("Error while playing: %1").arg(message));
}

void PlayerWindow::logMessage(const QString& message)
{
  log->appendPlainText(message);
}

void PlayerWindow::updateExportProgress()
{
  int max = exportProgress->maximum();
  int val = exportProgress->value() + 1;
  if (val >= max) {
    progressPanel->hide();
  } else {
    exportProgress->setValue(val);
  }
}

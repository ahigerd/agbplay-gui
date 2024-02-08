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
#include <QtDebug>

PlayerWindow::PlayerWindow(Player* player, QWidget* parent)
: QMainWindow(parent), player(player)
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
  QObject::connect(playlistView, SIGNAL(activated(QModelIndex)), this, SLOT(selectSong(QModelIndex)));
  QObject::connect(playlistView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectSong(QModelIndex)));
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

  log->setReadOnly(true);
  log->setMaximumHeight(100);

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
  fileMenu->addAction(tr("&Save Playlist"), playlist, SLOT(save()), QKeySequence::Save);
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
  return view;
}

void PlayerWindow::openRom()
{
  QString path = QFileDialog::getOpenFileName(
    this,
    tr("Open GBA ROM File"),
    QString(),
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
    QMessageBox::warning(nullptr, "agbplay-gui", e.what());
    setWindowFilePath(QString());
    setWindowTitle("agbplay");
    return;
  }

  addRecent(path);
  setWindowFilePath(path);
  setWindowTitle(QStringLiteral("agbplay - %1").arg(QFileInfo(path).fileName()));
  emit romUpdated(rom);

  songList->setCurrentIndex(songs->index(0, 0));
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
    QMessageBox::warning(nullptr, "agbplay-gui", e.what());
    return;
  }
  songList->scrollTo(songIndex);
  if (playlistIndex.isValid()) {
    playlistView->scrollTo(playlistIndex);
  }
}

void PlayerWindow::closeEvent(QCloseEvent*)
{
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

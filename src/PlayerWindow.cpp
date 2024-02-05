#include "PlayerWindow.h"
#include "TrackList.h"
#include "VUMeter.h"
#include "RomView.h"
#include "Rom.h"
#include "ConfigManager.h"
#include "SongModel.h"
#include <QApplication>
#include <QBoxLayout>
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
#include <QFileDialog>
#include <QSettings>

PlayerWindow::PlayerWindow(QWidget* parent)
: QMainWindow(parent)
{
  setWindowTitle("agbplay");
  songs = new SongModel(this);

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
  QObject::connect(songs, SIGNAL(songTableUpdated(SongTable*)), this, SIGNAL(songTableUpdated(SongTable*)));
  QObject::connect(this, SIGNAL(songTableUpdated(SongTable*)), romView, SLOT(updateSongTable(SongTable*)));
}

QLayout* PlayerWindow::makeTop()
{
  QHBoxLayout* layout = new QHBoxLayout;

  layout->addWidget(makeTitle(), 0);

  VUMeter* vu = new VUMeter(this);
  vu->setStereoLayout(Qt::Vertical);
  layout->addWidget(vu, 1);

  return layout;
}

QLayout* PlayerWindow::makeLeft()
{
  QVBoxLayout* layout = new QVBoxLayout;

  songList = makeView(songs);
  layout->addWidget(songList);

  playlist = new QStandardItemModel(this);
  playlist->setHorizontalHeaderLabels({ tr("Playlist") });
  playlistView = makeView(playlist);
  layout->addWidget(playlistView);

  return layout;
}

QLayout* PlayerWindow::makeRight()
{
  QVBoxLayout* vbox = new QVBoxLayout;
  QHBoxLayout* hbox = new QHBoxLayout;
  hbox->addWidget(trackList = new TrackList(this), 1);
  hbox->addWidget(romView = new RomView(this), 0);
  vbox->addLayout(hbox, 1);

  log = new QPlainTextEdit(this);
  log->setReadOnly(true);
  log->setMaximumHeight(100);
  vbox->addWidget(log, 0);

  return vbox;
}

void PlayerWindow::makeMenu()
{
  QMenuBar* mb = menuBar();
  QMenu* fileMenu = mb->addMenu(tr("&File"));
  fileMenu->addAction(tr("&Open ROM..."), this, SLOT(openRom()));
  fileMenu->addSeparator();
  fileMenu->addAction(tr("E&xit"), qApp, SLOT(quit()));

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
    Rom::CreateInstance(qPrintable(path));
    rom = &Rom::Instance();
    ConfigManager::Instance().SetGameCode(rom->GetROMCode());
    songs->openRom(rom);
  } catch (std::exception& e) {
    QMessageBox::warning(nullptr, "agbplay-gui", e.what());
    setWindowFilePath(QString());
    setWindowTitle("agbplay");
    return;
  }

  setWindowFilePath(path);
  setWindowTitle(QStringLiteral("agbplay - %1").arg(QFileInfo(path).fileName()));
  emit romUpdated(rom);
}

void PlayerWindow::about()
{
  QMessageBox::about(
    this,
    tr("agbplay-gui (%1)").arg(qApp->applicationVersion()),
    tr(
      "<b>agbplay</b> is a music player for GBA ROMs that use "
      "the MusicPlayer2000 (mp2k/m4a/\"Sappy\") sound engine."
    ));
}

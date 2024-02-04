#include "PlayerWindow.h"
#include "TrackList.h"
#include "VUMeter.h"
#include <QBoxLayout>
#include <QGroupBox>
#include <QTreeView>
#include <QLabel>
#include <QFont>
#include <QFontInfo>
#include <QFontDatabase>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QPlainTextEdit>

PlayerWindow::PlayerWindow(QWidget* parent)
: QMainWindow(parent)
{
  QWidget* base = new QWidget(this);
  setCentralWidget(base);

  QVBoxLayout* vbox = new QVBoxLayout(base);
  QHBoxLayout* header = new QHBoxLayout;
  QHBoxLayout* hbox = new QHBoxLayout;
  vbox->addLayout(header, 0);
  vbox->addLayout(hbox, 1);

  header->addWidget(makeTitle(), 0);

  VUMeter* vu = new VUMeter(this);
  vu->setStereoLayout(Qt::Vertical);
  header->addWidget(vu, 1);

  QVBoxLayout* vboxLeft = new QVBoxLayout;
  QVBoxLayout* vboxRight = new QVBoxLayout;
  hbox->addLayout(vboxLeft, 1);
  hbox->addLayout(vboxRight, 4);

  songs = new QStandardItemModel(this);
  QTreeView* songList = makeView(tr("Songs"), songs);
  songs->appendRow(new QStandardItem("0000"));
  vboxLeft->addWidget(songList);

  playlist = new QStandardItemModel(this);
  QTreeView* playlistView = makeView(tr("Playlist"), playlist);
  vboxLeft->addWidget(playlistView);

  QHBoxLayout* hboxRight = new QHBoxLayout;
  vboxRight->addLayout(hboxRight, 1);

  trackList = new TrackList(base);
  hboxRight->addWidget(trackList, 1);

  QLabel* romInfo = new QLabel(base);
  romInfo->setText(QStringLiteral("<b>ROM Name:</b>\nTest\n\n<b>ROM Code</b>\nTEST\n\n<b>Songtable Offset:</b>\n0x00000000\n\n<b>Number of Songs:</b>\n0").replace("\n", "<br>"));
  romInfo->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  hboxRight->addWidget(romInfo, 0);

  QPlainTextEdit* log = new QPlainTextEdit(base);
  log->setReadOnly(true);
  log->setMaximumHeight(100);
  vboxRight->addWidget(log, 0);
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

QTreeView* PlayerWindow::makeView(const QString& label, QStandardItemModel* model)
{
  QTreeView* view = new QTreeView(this);
  view->setRootIsDecorated(false);
  model->setHorizontalHeaderLabels({ label });
  //model->setHeaderData(0, Qt::Horizontal, QSize(150, 20), Qt::SizeHintRole);
  view->header()->resizeSection(0, 150);
  view->setModel(model);
  return view;
}

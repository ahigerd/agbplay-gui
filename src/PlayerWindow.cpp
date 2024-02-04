#include "PlayerWindow.h"
#include "TrackList.h"
#include "VUMeter.h"
#include <QBoxLayout>
#include <QGroupBox>
#include <QListView>
#include <QLabel>
#include <QFont>

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

  QFont titleFont = font();
  titleFont.setPointSize(40);
  QLabel* title = new QLabel("agbplay", base);
  title->setFont(titleFont);
  header->addWidget(title, 0);

  VUMeter* vu = new VUMeter(this);
  vu->setStereoLayout(Qt::Vertical);
  header->addWidget(vu, 1);

  QVBoxLayout* vboxLeft = new QVBoxLayout;
  QVBoxLayout* vboxRight = new QVBoxLayout;
  hbox->addLayout(vboxLeft, 0);
  hbox->addLayout(vboxRight, 1);

  QGroupBox* songGroup = new QGroupBox(tr("Songs"));
  QVBoxLayout* songLayout = new QVBoxLayout(songGroup);
  songLayout->setContentsMargins(0, 0, 0, 0);
  songLayout->addWidget(new QListView(songGroup));
  songGroup->setFixedWidth(150);
  vboxLeft->addWidget(songGroup);

  QGroupBox* playlistGroup = new QGroupBox(tr("Playlist"));
  QVBoxLayout* playlistLayout = new QVBoxLayout(playlistGroup);
  playlistLayout->setContentsMargins(0, 0, 0, 0);
  playlistLayout->addWidget(new QListView(playlistGroup));
  playlistGroup->setFixedWidth(150);
  vboxLeft->addWidget(playlistGroup);

  QHBoxLayout* hboxRight = new QHBoxLayout;
  vboxRight->addLayout(hboxRight, 1);

  trackList = new TrackList(base);
  hboxRight->addWidget(trackList, 1);

  QLabel* romInfo = new QLabel(base);
  romInfo->setText("ROM Name:\nTest\n\nROM Code\nTEST\n\nSongtable Offset:\n0x00000000\n\nNumber of Songs:\n0");
  romInfo->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  hboxRight->addWidget(romInfo, 0);

  QListView* log = new QListView(base);
  log->setMaximumHeight(100);
  vboxRight->addWidget(log, 0);
}

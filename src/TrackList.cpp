#include "TrackList.h"
#include "TrackHeader.h"
#include "TrackView.h"
#include "PlayerContext.h"
#include "VUMeter.h"
#include "UiUtils.h"
#include <QVBoxLayout>
#include <QScrollBar>
#include <QLabel>
#include <QEvent>

TrackList::TrackList(QWidget* parent)
: QScrollArea(parent)
{
  base = new QWidget(this);
  setWidget(base);
  setWidgetResizable(true);

  trackLayout = new QVBoxLayout(base);
  trackLayout->setContentsMargins(0, 0, 0, 0);
  header = new TrackHeader(this);
  header->setTrackName("");

  // populate a dummy entry for geometry
  TrackView* v = new TrackView(header, 0, base);
  tracks << v;
  trackLayout->addWidget(v, 0);
  trackLayout->addStretch(1);

  header->setGeometry(1, 1, width() - 2, header->sizeHint().height());
  setViewportMargins(0, header->sizeHint().height() + 1, 0, 0);

  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setSizeAdjustPolicy(QScrollArea::AdjustToContentsOnFirstShow);
  setMaximumWidth(v->maximumWidth() + verticalScrollBar()->sizeHint().width());
}

QSize TrackList::sizeHint() const
{
  return QSize(
    base->sizeHint().width() + verticalScrollBar()->sizeHint().width() + 20,
    base->sizeHint().height() + 4
  );
}

void TrackList::showEvent(QShowEvent* e)
{
  QScrollArea::showEvent(e);
  setMinimumWidth(sizeHint().width());
  setMinimumHeight(base->sizeHint().height() + 4);
}

void TrackList::resizeEvent(QResizeEvent* e)
{
  QScrollArea::resizeEvent(e);
  header->resize(width() - 2, viewportMargins().top() - 1);
}

void TrackList::selectSong(PlayerContext* ctx, quint32 addr, const QString& title)
{
  qDeleteAll(tracks);
  tracks.clear();

  if (ctx) {
    header->setTrackName(QStringLiteral("[%1] %2").arg(formatAddress(addr)).arg(title));

    int numTracks = int(ctx->seq.tracks.size());
    for (int i = 0; i < numTracks; i++) {
      TrackView* t = new TrackView(header, i, this);
      tracks << t;
      trackLayout->insertWidget(i, t);
      QObject::connect(t, SIGNAL(muteToggled(int,bool)), this, SLOT(onMuteToggled(int,bool)));
      QObject::connect(t, SIGNAL(soloToggled(int,bool)), this, SLOT(soloToggled(int,bool)));
    }

    update(ctx, nullptr);
  } else {
    header->setTrackName(QString());
  }
}

void TrackList::update(PlayerContext* ctx, VUState* vu)
{
  int numTracks = tracks.size();
  if (vu) {
    for (int i = 0; i < numTracks; i++) {
      tracks[i]->update(ctx, vu->track[i].left, vu->track[i].right);
    }
  } else {
    for (int i = 0; i < numTracks; i++) {
      tracks[i]->update(ctx, 0, 0);
    }
  }
}

void TrackList::onMuteToggled(int track, bool on)
{
  emit muteToggled(track, on);
  if (!on) {
    int numTracks = tracks.length();
    for (int i = 0; i < numTracks; i++) {
      tracks[i]->clearSolo();
    }
  }
}

void TrackList::soloToggled(int track, bool on)
{
  int numTracks = tracks.length();
  for (int i = 0; i < numTracks; i++) {
    emit muteToggled(i, on ? (i != track) : false);
  }
}

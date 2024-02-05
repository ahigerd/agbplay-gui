#include "TrackList.h"
#include "TrackHeader.h"
#include "TrackView.h"
#include "PlayerContext.h"
#include <QVBoxLayout>
#include <QScrollBar>
#include <QLabel>
#include <QEvent>
#include <QtDebug>

TrackList::TrackList(QWidget* parent)
: QScrollArea(parent)
{
  base = new QWidget(this);
  setWidget(base);
  setWidgetResizable(true);

  trackLayout = new QVBoxLayout(base);
  trackLayout->setContentsMargins(0, 0, 0, 0);
  header = new TrackHeader(this);
  header->setTrackName("Test Track");

  // populate a dummy entry for geometry
  TrackView* v = new TrackView(header, 0, base);
  tracks << v;
  trackLayout->addWidget(v, 0);
  trackLayout->addStretch(1);

  header->setGeometry(0, 0, width(), header->sizeHint().height());
  setViewportMargins(0, header->sizeHint().height(), 0, 0);

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
  header->resize(width(), viewportMargins().top());
}

void TrackList::selectSong(PlayerContext* ctx)
{
  qDeleteAll(tracks);
  tracks.clear();

  int numTracks = int(ctx->seq.tracks.size());
  for (int i = 0; i < numTracks; i++) {
    TrackView* t = new TrackView(header, i, this);
    tracks << t;
    trackLayout->insertWidget(i, t);
  }
}

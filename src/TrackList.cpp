#include "TrackList.h"
#include "TrackHeader.h"
#include "TrackView.h"
#include <QStylePainter>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QLabel>
#include <QEvent>
#include <QtDebug>
#include <QStyleOptionHeader>

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
  trackLayout->addWidget(v, 0);
  trackLayout->addStretch(1);

  header->setGeometry(0, 0, width(), header->sizeHint().height());
  setViewportMargins(0, header->sizeHint().height()/* + margins.top() + margins.bottom()*/, 0, 0);

  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setSizeAdjustPolicy(QScrollArea::AdjustToContentsOnFirstShow);
}

void TrackList::showEvent(QShowEvent* e)
{
  QScrollArea::showEvent(e);
  setMinimumWidth(sizeHint().width());
  setMinimumHeight(viewportMargins().top() + viewportMargins().bottom() + base->sizeHint().height() + 4);

  for (int i = 1; i < 16; i++) {
    TrackView* v = new TrackView(header, 0, base);
    trackLayout->insertWidget(i, v, 0);
  }
}

void TrackList::resizeEvent(QResizeEvent* e)
{
  QScrollArea::resizeEvent(e);
  header->resize(width(), viewportMargins().top());
}

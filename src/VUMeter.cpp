#include "VUMeter.h"
#include <QPainter>
#include <QLinearGradient>

VUState::VUState()
: masterLoudness(10.0f)
{
  // initializers only
}

void VUState::setTrackCount(int numTracks)
{
  loudness.clear();
  for (int i = 0; i < numTracks; i++) {
    loudness.emplace_back(5.0f);
  }
  track = std::vector<sample>(numTracks);
}

void VUState::reset()
{
  masterLoudness.Reset();
  for (LoudnessCalculator& c : loudness) {
    c.Reset();
  }
}

void VUState::update()
{
  masterLoudness.GetLoudness(master.left, master.right);
  std::size_t numTracks = loudness.size();
  for (std::size_t i = 0; i < numTracks; i++) {
    sample& level = track[i];
    loudness[i].GetLoudness(level.left, level.right);
  }
}

VUMeter::VUMeter(QWidget* parent)
: QWidget(parent), leftLevel(0), rightLevel(0), muted(false), stereo(Qt::Horizontal)
{
  // initializers only
}

void VUMeter::setStereoLayout(Qt::Orientation a)
{
  stereo = a;
  update();
}

void VUMeter::setLeft(double v)
{
  leftLevel = v > 1.0 ? 1.0 : v;
  update();
}

void VUMeter::setRight(double v)
{
  rightLevel = v > 1.0 ? 1.0 : v;
  update();
}

void VUMeter::setMute(bool m)
{
  if (m != muted) {
    muted = m;
    resizeEvent(nullptr);
    update();
  }
}

void VUMeter::paintEvent(QPaintEvent*)
{
  QPainter p(this);
  int w = width();
  int h = height() - 2;
  p.fillRect(rect(), Qt::black);

  if (stereo == Qt::Horizontal) {
    int span = w / 2 - 6;

    double l = span * leftLevel;
    p.fillRect(span - l, 1, l, h, leftGradient);

    double r = span * rightLevel;
    p.fillRect(w - span, 1, r, h, rightGradient);

    int barWidth = w - span * 2;
    p.fillRect(span + 3, 1, barWidth - 7, h, Qt::green);
  } else {
    int barHeight = h / 2 - 1;
    p.fillRect(1, 1, w * leftLevel, barHeight, rightGradient);
    p.fillRect(1, h - barHeight + 1, w * rightLevel, barHeight, rightGradient);
  }
}

void VUMeter::resizeEvent(QResizeEvent*)
{
  int v = muted ? 128 : 255;
  double w = width();
  double channelWidth = stereo == Qt::Horizontal ? w / 2 : 0;

  QLinearGradient left(0, 0, channelWidth, 0);
  left.setColorAt(0, QColor(v, 0, 0));
  left.setColorAt(0.25, QColor(v, v, 0));
  left.setColorAt(0.75, QColor(0, v, 0));
  left.setColorAt(1, QColor(0, v, 0));
  leftGradient = left;

  QLinearGradient right(channelWidth, 0, w, 0);
  right.setColorAt(1, QColor(v, 0, 0));
  right.setColorAt(0.25, QColor(0, v, 0));
  right.setColorAt(0.75, QColor(v, v, 0));
  right.setColorAt(0, QColor(0, v, 0));
  rightGradient = right;
}

#include "VUMeter.h"
#include <QPainter>
#include <QLinearGradient>

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
  leftLevel = v;
  update();
}

void VUMeter::setRight(double v)
{
  rightLevel = v;
  update();
}

void VUMeter::setMute(bool m)
{
  muted = m;
  resizeEvent(nullptr);
  update();
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

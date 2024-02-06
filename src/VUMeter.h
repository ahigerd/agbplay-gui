#pragma once

#include <QWidget>
#include <QBrush>
#include "LoudnessCalculator.h"

struct VUState
{
  VUState();

  void setTrackCount(int tracks);
  void reset();
  void update();

  sample master;
  std::vector<sample> track;

  LoudnessCalculator masterLoudness;
  std::vector<LoudnessCalculator> loudness;
};

class VUMeter : public QWidget
{
Q_OBJECT
public:
  VUMeter(QWidget* parent = nullptr);

public slots:
  void setStereoLayout(Qt::Orientation a);
  void setMute(bool m);
  void setLeft(double v);
  void setRight(double v);

protected:
  void paintEvent(QPaintEvent*);
  void resizeEvent(QResizeEvent*);

private:
  double leftLevel, rightLevel;
  bool muted;
  Qt::Orientation stereo;
  QBrush leftGradient, rightGradient;
};


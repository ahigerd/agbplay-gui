#pragma once

#include <QWidget>
class TrackHeader;
class QLabel;
class QCheckBox;
class PianoKeys;
class VUMeter;

class TrackView : public QWidget
{
Q_OBJECT
public:
  TrackView(TrackHeader* header, int index, QWidget* parent = nullptr);

  int headerWidth() const;
  QSize sizeHint() const;

protected:
  void resizeEvent(QResizeEvent*);

private:
  QWidget* leftPanel;
  QLabel* trackNumber;
  QCheckBox* mute;
  QCheckBox* solo;
  QLabel* location;
  QLabel* delay;
  QLabel* program;
  QLabel* pan;
  QLabel* volume;
  QLabel* mod;
  QLabel* pitch;
  PianoKeys* keys;
  VUMeter* vu;
};

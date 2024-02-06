#pragma once

#include <QWidget>
#include "LoudnessCalculator.h"
class TrackHeader;
class QLabel;
class QCheckBox;
class PianoKeys;
class VUMeter;
class PlayerContext;

class TrackView : public QWidget
{
Q_OBJECT
public:
  TrackView(TrackHeader* header, int index, QWidget* parent = nullptr);

  int headerWidth() const;
  QSize sizeHint() const;

  void update(PlayerContext* ctx);
  void clearSolo();

signals:
  void muteToggled(int track, bool on);
  void soloToggled(int track, bool on);

private slots:
  void setMute(bool);
  void setSolo(bool);

protected:
  void resizeEvent(QResizeEvent*);

private:
  LoudnessCalculator loudness;

  int trackIdx;
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

  bool muteUpdated;
};

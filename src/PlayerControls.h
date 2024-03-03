#pragma once

#include <QWidget>
#include <QStyle>
class QToolButton;
class QSlider;
class QMenu;
class QAction;
class PlayerContext;

class PlayerControls : public QWidget
{
Q_OBJECT
public:
  PlayerControls(QWidget* parent = nullptr);

  QAction* toggleAction() const;
  QAction* playAction() const;
  QAction* pauseAction() const;
  QAction* stopAction() const;

  double speedMultiplier() const;

  bool eventFilter(QObject* obj, QEvent* event);

public slots:
  void songChanged(PlayerContext*);
  void updateState(bool isPlaying, bool isPaused);

signals:
  void togglePlay();
  void play();
  void pause();
  void stop();
  void setSpeed(double multiplier);

private slots:
  void showSpeedMenu(const QPoint& pos);
  void setSpeedByAction(QAction* action);
  void speedSliderChanged(int value);

private:
  QToolButton* makeButton(QStyle::StandardPixmap icon, const QString& text, const char* slot = nullptr);
  void updateSpeed();
  double speedMultiplier(int value) const;

  QAction* toggle;
  QToolButton* playButton;
  QToolButton* pauseButton;
  QToolButton* stopButton;
  QSlider* speedSlider;
  QMenu* speedMenu;

  bool trackLoaded;
};

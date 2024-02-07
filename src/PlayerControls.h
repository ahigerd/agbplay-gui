#pragma once

#include <QWidget>
#include <QStyle>
class QToolButton;
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

public slots:
  void songChanged(PlayerContext*);
  void updateState(bool isPlaying, bool isPaused);

signals:
  void togglePlay();
  void play();
  void pause();
  void stop();

private:
  QToolButton* makeButton(QStyle::StandardPixmap icon, const QString& text, const char* slot = nullptr);

  QAction* toggle;
  QToolButton* playButton;
  QToolButton* pauseButton;
  QToolButton* stopButton;

  bool trackLoaded;
};

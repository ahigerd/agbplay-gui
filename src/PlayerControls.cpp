#include "PlayerControls.h"
#include <QToolButton>
#include <QKeySequence>
#include <QHBoxLayout>
#include <QAction>

PlayerControls::PlayerControls(QWidget* parent)
: QWidget(parent), trackLoaded(false)
{
  QHBoxLayout* layout = new QHBoxLayout(this);

  toggle = new QAction(tr("Play/Pause"), this);
  toggle->setShortcut(Qt::Key_Space);
  toggle->setEnabled(false);
  QObject::connect(toggle, SIGNAL(triggered(bool)), this, SIGNAL(togglePlay()));

  playButton = makeButton(QStyle::SP_MediaPlay, tr("&Play"), SIGNAL(play()));
  pauseButton = makeButton(QStyle::SP_MediaPause, tr("P&ause"), SIGNAL(pause()));
  stopButton = makeButton(QStyle::SP_MediaStop, tr("&Stop"), SIGNAL(stop()));
  stopAction()->setShortcut(Qt::Key_Escape);

  layout->addStretch(1);
  layout->addWidget(playButton);
  layout->addWidget(pauseButton);
  layout->addWidget(stopButton);
  layout->addStretch(1);
}

QAction* PlayerControls::toggleAction() const
{
  return toggle;
}

QAction* PlayerControls::playAction() const
{
  return playButton->defaultAction();
}

QAction* PlayerControls::pauseAction() const
{
  return pauseButton->defaultAction();
}

QAction* PlayerControls::stopAction() const
{
  return stopButton->defaultAction();
}

QToolButton* PlayerControls::makeButton(QStyle::StandardPixmap icon, const QString& text, const char* slot)
{
  QToolButton* btn = new QToolButton(this);
  QAction* action = new QAction(style()->standardIcon(icon, nullptr, this), text, this);
  action->setEnabled(false);
  btn->setDefaultAction(action);
  if (slot) {
    QObject::connect(action, SIGNAL(triggered(bool)), this, slot);
  }
  return btn;
}

void PlayerControls::songChanged(PlayerContext* ctx)
{
  trackLoaded = !!ctx;
  updateState(false, false);
}

void PlayerControls::updateState(bool isPlaying, bool)
{
  toggle->setEnabled(trackLoaded);
  playAction()->setEnabled(trackLoaded);
  pauseAction()->setEnabled(isPlaying);
  stopAction()->setEnabled(isPlaying);
}


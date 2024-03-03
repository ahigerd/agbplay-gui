#include "PlayerControls.h"
#include <QToolButton>
#include <QSlider>
#include <QLabel>
#include <QMenu>
#include <QKeySequence>
#include <QBoxLayout>
#include <QMouseEvent>
#include <QAction>
#include <cmath>

static const QPair<QString, int> speedPresets[] = {
  { "1/16x", -34 },
  { "1/8x", -26 },
  { "1/4x", -18 },
  { "1/2x", -10 },
  { "1x", 0 },
  { "2x", 10 },
  { "4x", 18 },
  { "8x", 26 },
  { "16x", 34 },
};

PlayerControls::PlayerControls(QWidget* parent)
: QWidget(parent), trackLoaded(false)
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  QHBoxLayout* hbox = new QHBoxLayout;

  toggle = new QAction(tr("Play/Pause"), this);
  toggle->setShortcut(Qt::Key_Space);
  toggle->setEnabled(false);
  QObject::connect(toggle, SIGNAL(triggered(bool)), this, SIGNAL(togglePlay()));

  playButton = makeButton(QStyle::SP_MediaPlay, tr("&Play"), SIGNAL(play()));
  pauseButton = makeButton(QStyle::SP_MediaPause, tr("P&ause"), SIGNAL(pause()));
  stopButton = makeButton(QStyle::SP_MediaStop, tr("&Stop"), SIGNAL(stop()));
  stopAction()->setShortcut(Qt::Key_Escape);

  hbox->addStretch(1);
  hbox->addWidget(playButton);
  hbox->addWidget(pauseButton);
  hbox->addWidget(stopButton);
  hbox->addStretch(1);

  QHBoxLayout* speed = new QHBoxLayout;
  QLabel* speedLabel = new QLabel(tr("&Speed:"), this);
  speedSlider = new QSlider(Qt::Horizontal, this);
  speedLabel->setBuddy(speedSlider);
  speedSlider->setRange(-34, 34);
  speedSlider->setTickPosition(QSlider::TicksBothSides);
  speedSlider->setTickInterval(34);
  speedSlider->setTracking(true);
  speedSlider->setPageStep(10);
  speedSlider->setContextMenuPolicy(Qt::CustomContextMenu);
  speed->addWidget(speedLabel, 0);
  speed->addWidget(speedSlider, 1);

  layout->addStretch(1);
  layout->addLayout(hbox);
  layout->addLayout(speed);

  speedMenu = new QMenu(speedSlider);
  for (const auto& pair : speedPresets) {
    QAction* action = new QAction(pair.first, speedSlider);
    action->setData(pair.second);
    action->setCheckable(true);
    speedMenu->addAction(action);
  }
  speedSlider->installEventFilter(this);
  updateSpeed();
  QObject::connect(speedSlider, SIGNAL(valueChanged(int)), this, SLOT(speedSliderChanged(int)));
  QObject::connect(speedSlider, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showSpeedMenu(QPoint)));
  QObject::connect(speedMenu, SIGNAL(triggered(QAction*)), this, SLOT(setSpeedByAction(QAction*)));
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

void PlayerControls::speedSliderChanged(int value)
{
  if (value >= -2 && value <= 2) {
    speedSlider->setValue(0);
    speedSlider->setPageStep(10);
    updateSpeed();
    return;
  }
  speedSlider->setPageStep(8);
  updateSpeed();
}

void PlayerControls::setSpeedByAction(QAction* action)
{
  int value = action->data().toInt();
  speedSlider->setValue(value);
  speedSliderChanged(value);
}

void PlayerControls::updateSpeed()
{
  double mult = speedMultiplier();
  if (mult < 1) {
    speedSlider->setToolTip(QStringLiteral("1/%L1x").arg(1.0 / mult, 0, 'g', 2));
  } else {
    speedSlider->setToolTip(QStringLiteral("%L1x").arg(mult, 0, 'g', 2));
  }
  for (QAction* action : speedMenu->actions()) {
    action->setChecked(std::abs(speedMultiplier(action->data().toInt()) - mult) < 0.001);
  }
  emit setSpeed(mult);
}

double PlayerControls::speedMultiplier() const
{
  return speedMultiplier(speedSlider->value());
}

double PlayerControls::speedMultiplier(int value) const
{
  if (value < 0) {
    value += 2;
  } else if (value > 0) {
    value -= 2;
  }
  // should range from -4.0 to +4.0
  double mag = value / 8.0;
  // should range from 1/16x to 16x
  return std::exp2(mag);
}

bool PlayerControls::eventFilter(QObject* obj, QEvent* event)
{
  if (obj == speedSlider) {
    if (event->type() == QEvent::MouseButtonDblClick) {
      QMouseEvent* me = static_cast<QMouseEvent*>(event);
      if (me->button() == Qt::LeftButton) {
        speedSliderChanged(0);
        return true;
      }
    }
  }
  return false;
}

void PlayerControls::showSpeedMenu(const QPoint& pos)
{
  speedMenu->exec(speedSlider->mapToGlobal(pos));
}

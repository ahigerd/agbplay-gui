#include "TrackView.h"
#include "TrackHeader.h"
#include "PianoKeys.h"
#include "VUMeter.h"
#include "PlayerContext.h"
#include "UiUtils.h"
#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>

#define addLabel(name, text) \
  name = new QLabel(text, leftPanel); \
  name->setGeometry(header->name); \
  name->setAlignment(Qt::AlignCenter);

TrackView::TrackView(TrackHeader* header, int index, QWidget* parent)
: QWidget(parent), loudness(0.5f), trackIdx(index), muteUpdated(false)
{
  QGridLayout* mainLayout = new QGridLayout(this);
  mainLayout->setContentsMargins(0, 0, 2, 0);
  mainLayout->setVerticalSpacing(1);
  mainLayout->setColumnStretch(0, 0);
  mainLayout->setColumnStretch(1, 1);

  leftPanel = new QWidget(this);
  keys = new PianoKeys(this);
  vu = new VUMeter(this);

  leftPanel->setFixedSize(header->sizeHint());
  mainLayout->addWidget(leftPanel, 0, 0, 2, 1);
  mainLayout->addWidget(keys, 0, 1);
  mainLayout->addWidget(vu, 1, 1);

  addLabel(trackNumber, "00");
  mute = new QCheckBox(TrackHeader::tr("M"), leftPanel);
  mute->setGeometry(header->mute);
  solo = new QCheckBox(TrackHeader::tr("S"), leftPanel);
  solo->setGeometry(header->solo);
  addLabel(location, "0x00000000");
  addLabel(delay, "W00");
  addLabel(program, "127");
  addLabel(pan, "+0");
  addLabel(volume, "100");
  addLabel(mod, "0");
  addLabel(pitch, "+0");

  trackNumber->setText(fixedNumber(index, 2));

  setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
  setMinimumWidth(sizeHint().width());
  setMaximumWidth(leftPanel->sizeHint().width() + keys->maximumSize().width());

  QObject::connect(mute, SIGNAL(toggled(bool)), vu, SLOT(setMute(bool)));
  QObject::connect(mute, SIGNAL(clicked(bool)), this, SLOT(setMute(bool)));
  QObject::connect(solo, SIGNAL(clicked(bool)), this, SLOT(setSolo(bool)));
}

int TrackView::headerWidth() const
{
  return leftPanel->sizeHint().width();
}

void TrackView::resizeEvent(QResizeEvent* e)
{
  QWidget::resizeEvent(e);
  vu->resize(PianoKeys::preferredWidth(keys->width()), vu->height());
}

QSize TrackView::sizeHint() const
{
  return QSize(453 + leftPanel->sizeHint().width(), leftPanel->sizeHint().height());
}

void TrackView::update(PlayerContext* ctx)
{
  const auto& track = ctx->seq.tracks[trackIdx];
  location->setText(formatAddress(track.pos));
  volume->setText(QString::number(track.vol));
  mod->setText(QString::number(track.mod));
  program->setText(QString::number(track.prog));
  pitch->setText(signedNumber(track.pitch));
  pan->setText(signedNumber(track.pan));

  int delayValue = std::max(0, int(track.delay));
  delay->setText("W" + fixedNumber(delayValue, 2));

  for (int i = 0; i < 128; i++) {
    keys->setNoteOn(i, track.activeNotes[i]);
  }

  float leftLevel, rightLevel;
  loudness.GetLoudness(leftLevel, rightLevel);
  vu->setLeft(leftLevel * 3);
  vu->setRight(rightLevel * 3);
  vu->setMute(track.muted);
  mute->setChecked(track.muted);
  if (track.muted) {
    solo->setChecked(false);
  }
}

void TrackView::setMute(bool on)
{
  emit muteToggled(trackIdx, on);
}

void TrackView::setSolo(bool on)
{
  emit soloToggled(trackIdx, on);
}

void TrackView::clearSolo()
{
  solo->setChecked(false);
}

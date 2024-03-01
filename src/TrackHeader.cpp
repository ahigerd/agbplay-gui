#include "TrackHeader.h"
#include <QLabel>
#include <QCheckBox>
#include <QStyle>
#include <QStyleOptionHeader>
#include <QStylePainter>

TrackHeader::Label::Label(const QRect& rect, const QString& text, int pos)
: rect(&rect), text(text), section(QStyleOptionHeader::Middle)
{
  if (pos < 0) {
    section = QStyleOptionHeader::Beginning;
  } else if (pos > 0) {
    section = QStyleOptionHeader::End;
  }
}

TrackHeader::TrackHeader(QWidget* parent)
: QWidget(parent)
{
  QCheckBox muteCheck(tr("M"), this);
  lineHeight = muteCheck.sizeHint().height();
  mute = calcRect(muteCheck, tr("Mute"));
  lineHeight = mute.height();

  QStyleOptionHeader opt;
  opt.initFrom(this);
  opt.state = QStyle::State_None | QStyle::State_Raised | QStyle::State_Horizontal | QStyle::State_Active | QStyle::State_Enabled;
  opt.orientation = Qt::Horizontal;
  opt.section = 1;
  opt.text = "X";
  opt.position = QStyleOptionHeader::Middle;
  opt.rect = QRect(0, 0, 50, lineHeight);
  QSize headerSize = style()->sizeFromContents(QStyle::CT_HeaderSection, &opt, opt.rect.size(), this);
  if (headerSize.height() > lineHeight) {
    lineHeight = headerSize.height();
    mute.setHeight(lineHeight);
  }

  trackNumber = calcRect(QLabel("00"), tr("Track"));
  solo = calcRect(QCheckBox(tr("S")), tr("Solo"));
  location = calcRect(QLabel("0x01234567"), tr("Location"));
  delay = calcRect(QLabel("W00"), tr("Delay"));
  program = calcRect(QLabel("127"), tr("Prog"), 1);
  pan = calcRect(QLabel("+127"), tr("Pan"), 1);
  volume = calcRect(QLabel("100"), tr("Vol"), 1);
  mod = calcRect(QLabel("100"), tr("Mod"), 1);
  pitch = calcRect(QLabel("+32767"), tr("Pitch"), 1);

  int muteWidth = mute.width();
  int soloWidth = solo.width();
  int progWidth = program.width();
  int panWidth = pan.width();
  int groupWidth = muteWidth > soloWidth ? muteWidth : soloWidth;
  if (progWidth > groupWidth) {
    groupWidth = progWidth;
  }
  if (panWidth > groupWidth) {
    groupWidth = panWidth;
  }
  mute.setWidth(groupWidth);
  solo.setWidth(groupWidth);
  program.setWidth(groupWidth);
  pan.setWidth(groupWidth);

  int locWidth = (location.width() & ~1) + 2;
  int volWidth = volume.width();
  int modWidth = mod.width();
  int halfWidth = modWidth > volWidth ? modWidth : volWidth;
  if (locWidth < halfWidth * 2) {
    locWidth = halfWidth * 2;
  } else {
    halfWidth = locWidth / 2;
  }
  location.setWidth(locWidth);
  volume.setWidth(halfWidth);
  mod.setWidth(halfWidth);

  int delayWidth = delay.width();
  int pitchWidth = pitch.width();
  if (delayWidth > pitchWidth) {
    pitch.setWidth(delayWidth);
  } else {
    delay.setWidth(pitchWidth);
  }

  mute.moveLeft(trackNumber.right() + 1);
  program.moveLeft(trackNumber.right() + 1);
  solo.moveLeft(mute.right() + 1);
  pan.moveLeft(mute.right() + 1);
  location.moveLeft(solo.right() + 1);
  volume.moveLeft(solo.right() + 1);
  mod.moveLeft(volume.right() + 1);
  delay.moveLeft(location.right() + 1);
  pitch.moveLeft(location.right() + 1);

  filler = QRect(0, program.top(), trackNumber.width(), delay.height());

  labels
    << Label(trackNumber, tr("Track"), -1)
    << Label(mute, tr("Mute"))
    << Label(solo, tr("Solo"))
    << Label(location, tr("Location"))
    << Label(delay, tr("Delay"), 1)
    << Label(filler, QString())
    << Label(program, tr("Prog"), -1)
    << Label(pan, tr("Pan"))
    << Label(volume, tr("Vol"))
    << Label(mod, tr("Mod"))
    << Label(pitch, tr("Pitch"), 1);
}

void TrackHeader::setTrackName(const QString& name)
{
  trackName = name;
  update();
}

QRect TrackHeader::calcRect(const QWidget& widget, const QString& header, int line)
{
  int w = widget.sizeHint().width();
  QLabel label(header, this);
  if (label.sizeHint().width() > w) {
    w = label.sizeHint().width();
  }

  QStyleOptionHeader opt;
  opt.initFrom(this);
  opt.state = QStyle::State_None | QStyle::State_Raised | QStyle::State_Horizontal | QStyle::State_Active | QStyle::State_Enabled;
  opt.orientation = Qt::Horizontal;
  opt.section = 1;
  opt.text = header;
  opt.position = QStyleOptionHeader::Middle;
  opt.rect = QRect(0, 0, w, lineHeight);

  QSize size = style()->sizeFromContents(QStyle::CT_HeaderSection, &opt, opt.rect.size(), this);
  if (size.width() > w) {
    w = size.width();
  }
  w += style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing, nullptr, this);

  return QRect(0, line * lineHeight, w, lineHeight);
}

QSize TrackHeader::sizeHint() const
{
  return QSize(pitch.right() + 1, pitch.bottom());
}

void TrackHeader::paintEvent(QPaintEvent*)
{
  QStylePainter p(this);
  p.save();

  QStyleOptionHeader opt;
  opt.initFrom(this);
  opt.state = QStyle::State_None | QStyle::State_Raised | QStyle::State_Horizontal | QStyle::State_Active | QStyle::State_Enabled;
  opt.rect = rect();
  p.drawControl(QStyle::CE_Header, opt);

  opt.textAlignment = Qt::AlignCenter;
  for (const auto& label : labels) {
    opt.rect = label.rect->adjusted(0, 0, 0, -1);
    opt.text = label.text;
    opt.section = label.section;
    p.drawControl(label.text.isEmpty() ? QStyle::CE_HeaderEmptyArea : QStyle::CE_Header, opt);
  }

  p.restore();
  QRect titleRect(delay.topRight(), rect().bottomRight());
  p.setPen(palette().text().color());
  p.drawText(titleRect.adjusted(4, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, trackName);
}

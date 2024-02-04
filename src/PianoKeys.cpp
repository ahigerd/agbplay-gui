#include "PianoKeys.h"
#include <algorithm>
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>

static const int numWhiteKeys = 7 * 10 + 5;
static const bool hasBlack[] = { true, true, false, true, true, true, false };
static const bool isBlack[] = { false, true, false, true, false, false, true, false, true, false, true, false };
static const int posInOctave[] = { 0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6 };
static const int posToNote[] = { 0, 2, 4, 5, 7, 9, 11 };

PianoKeys::PianoKeys(QWidget* parent)
: QWidget(parent)
{
  setMinimumSize(numWhiteKeys * 3 + 1, 8);
  setSizeIncrement(75, 2);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

  for (int i = 0; i < 10; i++) {
    activeKeys[qrand() % 128] = true;
  }
}

int PianoKeys::preferredWidth(int maxWidth)
{
  int keyWidth = maxWidth / numWhiteKeys;
  if (keyWidth < 3) {
    keyWidth = 3;
  }
  return keyWidth * numWhiteKeys + 1;
}

void PianoKeys::noteOn(int noteNumber)
{
  activeKeys[noteNumber] = true;
}

void PianoKeys::noteOff(int noteNumber)
{
  activeKeys[noteNumber] = false;
}

QRect PianoKeys::keyRect(int noteNum) const
{
  int octave = noteNum / 12;
  int note = noteNum % 12;
  int x = (octave * 7 + posInOctave[note]) * whiteWidth;
  if (isBlack[note]) {
    return QRect(x + blackOffset, 0, blackWidth, blackHeight);
  } else {
    return QRect(x, 0, whiteWidth, whiteHeight);
  }
}

void PianoKeys::resizeEvent(QResizeEvent*)
{
  whiteWidth = width() / numWhiteKeys;
  blackWidth = 2 * whiteWidth / 3;
  blackOffset = 2 * whiteWidth / 3;
  whiteHeight = height() - 1;
  blackHeight = whiteHeight / 2;
}

int PianoKeys::noteAt(const QPoint& pos) const
{
  int x = pos.x() / whiteWidth;
  return (x / 7) * 12 + posToNote[x % 7];
}

void PianoKeys::paintEvent(QPaintEvent* e)
{
  int left = std::clamp(noteAt(e->rect().topLeft()) - 1, 0, 126);
  int right = std::clamp(noteAt(e->rect().bottomRight()) + 1, left + 1, 127);

  QPainter p(this);
  QPalette pal = palette();
  QBrush white = pal.base();
  QBrush altWhite = pal.alternateBase();
  QBrush black = pal.shadow();
  QBrush light = pal.highlight();
  QBrush blackLight(pal.color(QPalette::Highlight).darker(120));
  QColor lightFrame(pal.color(QPalette::Highlight).darker(150));

  // First draw the white keys
  for (int i = left; i <= right; i++) {
    int degree = i % 12;
    if (isBlack[degree]) {
      continue;
    }
    QRect r = keyRect(i);
    if (activeKeys[i]) {
      p.fillRect(r, light);
      p.setPen(lightFrame);
    } else {
      p.fillRect(r, degree == 0 ? altWhite : white);
      p.setPen(QPalette::Text);
    }
    p.drawRect(r);
  }

  // Then draw the black keys on top
  for (int i = left; i <= right; i++) {
    int degree = i % 12;
    if (!isBlack[degree]) {
      continue;
    }
    QRect r = keyRect(i);
    if (activeKeys[i]) {
      p.fillRect(r, blackLight);
      p.setPen(lightFrame);
    } else {
      p.fillRect(r, black);
      p.setPen(QPalette::Text);
    }
    p.drawRect(r);
  }
}

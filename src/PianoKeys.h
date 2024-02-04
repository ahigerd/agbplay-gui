#pragma once

#include <QWidget>
#include <bitset>

class PianoKeys : public QWidget
{
Q_OBJECT
public:
  PianoKeys(QWidget* parent = nullptr);

  static int preferredWidth(int maxWidth);

public slots:
  void noteOn(int noteNumber);
  void noteOff(int noteNumber);

protected:
  void resizeEvent(QResizeEvent*);
  void paintEvent(QPaintEvent*);

private:
  QRect keyRect(int noteNum) const;
  int noteAt(const QPoint& pos) const;

  int keyboardLeft;
  int whiteWidth;
  int whiteHeight;
  int blackOffset;
  int blackWidth;
  int blackHeight;

  std::bitset<128> activeKeys;
};

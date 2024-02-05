#pragma once

#include <QWidget>
#include <QList>
#include <QPair>
#include <QStyleOptionHeader>
class QLabel;
class QCheckBox;

class TrackHeader : public QWidget
{
Q_OBJECT
public:
  TrackHeader(QWidget* parent = nullptr);

  void setTrackName(const QString& name);

  QRect trackNumber;
  QRect mute;
  QRect solo;
  QRect location;
  QRect delay;
  QRect program;
  QRect pan;
  QRect volume;
  QRect mod;
  QRect pitch;

  QSize sizeHint() const;

protected:
  void paintEvent(QPaintEvent*);

private:
  QRect calcRect(const QWidget& widget, const QString& header, int line = 0);

  QRect filler;

  struct Label {
    Label(const QRect& rect, const QString& text, int pos = 0);
    const QRect* rect;
    QString text;
    QStyleOptionHeader::SectionPosition section;
  };
  QList<Label> labels;

  int lineHeight;
  QString trackName;
};

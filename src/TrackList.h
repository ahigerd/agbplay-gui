#pragma once

#include <QScrollArea>
#include <QModelIndex>
#include <QVector>
class QVBoxLayout;
class TrackView;
class TrackHeader;
class PlayerContext;

class TrackList : public QScrollArea
{
Q_OBJECT
public:
  TrackList(QWidget* parent = nullptr);

  QSize sizeHint() const;

  void selectSong(PlayerContext* ctx);

protected:
  void showEvent(QShowEvent*);
  void resizeEvent(QResizeEvent*);

private:
  TrackHeader* header;
  QWidget* base;
  QVBoxLayout* trackLayout;
  QVector<TrackView*> tracks;
};

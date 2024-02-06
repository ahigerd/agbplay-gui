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

signals:
  void muteToggled(int track, bool on);

public slots:
  void selectSong(PlayerContext* ctx, quint32 addr, const QString& title);
  void update(PlayerContext* ctx);

private slots:
  void onMuteToggled(int track, bool on);
  void soloToggled(int track, bool on);

protected:
  void showEvent(QShowEvent*);
  void resizeEvent(QResizeEvent*);

private:
  TrackHeader* header;
  QWidget* base;
  QVBoxLayout* trackLayout;
  QVector<TrackView*> tracks;
};

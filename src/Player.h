#pragma once

#include <QObject>
#include <QTimer>
#include <memory>
#include "PlayerContext.h"
#include "SoundData.h"
class SongModel;
class Rom;

class Player : public QObject
{
Q_OBJECT
public:
  Player(QObject* parent = nullptr);

  Rom* openRom(const QString& path);
  SongModel* songModel() const;
  void selectSong(int index);

signals:
  void songTableUpdated(SongTable* table);
  void songChanged(PlayerContext* context, quint32 addr, const QString& name);
  void updated(PlayerContext* context);

public slots:
  void setMute(int trackIdx, bool on);

  void play();
  void pause();
  void stop();

private slots:
  void update();

private:
  QTimer timer, updateThrottle;
  std::unique_ptr<PlayerContext> ctx;
  std::unique_ptr<SongTable> songTable;
  SongModel* model;
};

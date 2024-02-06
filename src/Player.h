#pragma once

#include <QObject>
#include <QTimer>
#include <QThread>
#include <memory>
#include <atomic>
#include <portaudio.h>
#include "PlayerContext.h"
#include "LoudnessCalculator.h"
#include "SoundData.h"
#include "Ringbuffer.h"
#include "VUMeter.h"
class SongModel;
class Rom;

class Player : public QObject
{
Q_OBJECT
friend class PlayerThread;
public:
  Player(QObject* parent = nullptr);
  ~Player();

  void detectHostApi();

  Rom* openRom(const QString& path);
  SongModel* songModel() const;
  void selectSong(int index);

signals:
  void songTableUpdated(SongTable* table);
  void songChanged(PlayerContext* context, quint32 addr, const QString& name);
  void updated(PlayerContext* context, VUState* vu);

public slots:
  void setMute(int trackIdx, bool on);

  void play();
  void pause();
  void stop();

private slots:
  void update();
  void playbackDone();

private:
  static int audioCallback(const void*, void*, size_t, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void*);
  int audioCallback(sample* output, size_t frames);

  PaStreamParameters outputStreamParameters;
#if __has_include(<pa_win_wasapi.h>)
  PaWasapiStreamInfo wasapiStreamInfo;
#endif

  QTimer timer, updateThrottle;

  std::unique_ptr<PlayerContext> ctx;
  std::unique_ptr<SongTable> songTable;
  std::unique_ptr<QThread> playerThread;
  SongModel* model;

  enum class State : int {
    RESTART, PLAYING, PAUSED, TERMINATED, SHUTDOWN
  };
  std::atomic<State> playerState;

  PaStream* audioStream;
  uint32_t speedFactor;
  Ringbuffer rBuf;

  VUState vuState;
  std::vector<bool> mutedTracks;
};
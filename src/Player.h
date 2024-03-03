#pragma once

#include <QObject>
#include <QTimer>
#include <QThread>
#include <QDir>
#include <memory>
#include <atomic>
#include <portaudio.h>
#if __has_include(<pa_win_wasapi.h>)
#include <pa_win_wasapi.h>
#endif
#include "PlayerContext.h"
#include "LoudnessCalculator.h"
#include "SoundData.h"
#include "Ringbuffer.h"
#include "VUMeter.h"
class SongModel;
class Rom;

struct ExportItem {
  QString outputPath;
  quint32 trackAddr;
  bool splitTracks;
};

class Player : public QObject
{
Q_OBJECT
friend class AudioThread;
friend class PlayerThread;
friend class ExportThread;
public:
  Player(QObject* parent = nullptr);
  ~Player();

  void detectHostApi();

  Rom* openRom(const QString& path);
  SongModel* songModel() const;
  void selectSong(int index);

  bool exportToWave(const QString& filename, int track);
  bool exportToWave(const QDir& path, const QList<int>& tracks, bool split);

signals:
  void threadError(const QString& message);
  void songTableUpdated(SongTable* table);
  void songChanged(PlayerContext* context, quint32 addr, const QString& name);
  void updated(PlayerContext* context, VUState* vu);
  void stateChanged(bool isPlaying, bool isPaused);
  void exportStarted(const QString& path);
  void exportFinished(const QString& path);
  void exportError(const QString& message);
  void playbackError(const QString& message);
  void exportCancelled();

public slots:
  void setMute(int trackIdx, bool on);
  void setSpeed(double mult);

  void play();
  void pause();
  void stop();
  void togglePlay();

  void cancelExport();

private slots:
  void update();
  void playbackDone();
  void exportDone();

private:
  enum class State : int {
    RESTART, PLAYING, PAUSED, TERMINATED, SHUTDOWN
  };

  static int audioCallback(const void*, void*, unsigned long, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void*);
  int audioCallback(sample* output, size_t frames);
  void setState(State state);

  PaStreamParameters outputStreamParameters;
#if __has_include(<pa_win_wasapi.h>)
  PaWasapiStreamInfo wasapiStreamInfo;
#endif

  QTimer timer, updateThrottle;

  std::unique_ptr<PlayerContext> ctx;
  std::unique_ptr<SongTable> songTable;
  std::unique_ptr<QThread> playerThread;
  std::unique_ptr<QThread> exportThread;
  SongModel* model;

  std::atomic<State> playerState;
  std::atomic<bool> abortExport;

  PaStream* audioStream;
  uint32_t speedFactor;
  Ringbuffer rBuf;

  VUState vuState;
  std::vector<bool> mutedTracks;
  QList<ExportItem> exportQueue;
};

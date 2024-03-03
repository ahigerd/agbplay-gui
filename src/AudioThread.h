#pragma once

#include <QThread>
#include <vector>
#include "Player.h"
#include "Types.h"
class RiffWriter;

class AudioThread : public QThread
{
public:
  using State = Player::State;

  ~AudioThread();

protected:
  AudioThread(Player* player, const QString& name, PlayerContext* ctx);

  bool process();
  void prepare(quint32 addr);
  virtual void prepareBuffers() = 0;
  virtual void processTrack(std::size_t index, std::vector<sample>& samples, bool mute) = 0;
  virtual void outputBuffers() = 0;

  Player* player;
  PlayerContext* ctx;
  std::size_t samplesPerBuffer;
  std::vector<std::vector<sample>> trackAudio;
};

class PlayerThread : public AudioThread
{
public:
  PlayerThread(Player* player);
  ~PlayerThread();

protected:
  virtual void run() override;

  virtual void prepareBuffers() override;
  virtual void processTrack(std::size_t index, std::vector<sample>& samples, bool mute) override;
  virtual void outputBuffers() override;

private:
  void runStream();
  void restart();
  void play();

  std::vector<sample> silence, masterAudio;
};

class ExportThread : public AudioThread
{
public:
  ExportThread(Player* player);
  ~ExportThread();

protected:
  virtual void run() override;

  virtual void prepareBuffers() override;
  virtual void processTrack(std::size_t index, std::vector<sample>& samples, bool mute) override;
  virtual void outputBuffers() override;

private:
  void pad(RiffWriter* riff, std::uint32_t samples) const;

  std::unique_ptr<RiffWriter> riff;
  std::vector<std::unique_ptr<RiffWriter>> riffs;
  std::vector<std::int16_t> masterLeft, masterRight, silence;

  bool exportTracks;
};

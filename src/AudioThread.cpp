#include "AudioThread.h"
#include "ConfigManager.h"
#include "Xcept.h"
#include "Debug.h"
#include "RiffWriter.h"
#include <QDir>

AudioThread::AudioThread(Player* player, const QString& name, PlayerContext* ctx)
: QThread(player),
  player(player),
  ctx(ctx),
  samplesPerBuffer(ctx->mixer.GetSamplesPerBuffer())
{
  setObjectName(name);
  setTerminationEnabled(true);
}

AudioThread::~AudioThread()
{
}

void AudioThread::prepare(quint32 addr)
{
  ctx->InitSong(addr);
  uint8_t numTracks = static_cast<uint8_t>(ctx->seq.tracks.size());
  trackAudio.resize(numTracks);
  for (auto& buffer : trackAudio) {
    std::fill(buffer.begin(), buffer.end(), sample{0.0f, 0.0f});
    buffer.resize(samplesPerBuffer, sample{0.0f, 0.0f});
  }
}

bool AudioThread::process()
{
  prepareBuffers();
  // render audio buffers for tracks
  ctx->Process(trackAudio);
  for (size_t i = 0; i < trackAudio.size(); i++) {
    processTrack(i, trackAudio[i], ctx->seq.tracks[i].muted);
  }
  outputBuffers();
  return ctx->HasEnded();
}

PlayerThread::PlayerThread(Player* player)
: AudioThread(player, "mixer thread", player->ctx.get()),
  silence(samplesPerBuffer, sample{0.0f, 0.0f}),
  masterAudio(samplesPerBuffer, sample{0.0f, 0.0f})
{
  PaError err = Pa_StartStream(player->audioStream);
  if (err != paNoError) {
    throw Xcept("Pa_StartStream(): unable to start stream: %s", Pa_GetErrorText(err));
  }
  player->setState(State::PLAYING);
}

PlayerThread::~PlayerThread()
{
}

void PlayerThread::run()
{
  try {
    runStream();
    // reset song state after it has finished
    prepare(ctx->seq.GetSongHeaderPos());
  } catch (std::exception& e) {
    Debug::print("FATAL ERROR on streaming thread: %s", e.what());
    emit player->playbackError(e.what());
  }
  Pa_StopStream(player->audioStream);
  player->vuState.reset();
  // flush buffer
  player->rBuf.Clear();
  player->playerState = State::TERMINATED;
}

void PlayerThread::runStream()
{
  while (true) {
    switch (player->playerState) {
      case State::SHUTDOWN:
      case State::TERMINATED:
        return;
      case State::RESTART:
        restart();
        [[fallthrough]];
      case State::PLAYING:
        if (process()) {
          player->setState(State::SHUTDOWN);
          return;
        }
        break;
      case State::PAUSED:
        player->rBuf.Put(silence.data(), silence.size());
        break;
      default:
        throw Xcept("Internal PlayerInterface error: %d", (int)player->playerState.load());
    }
  }
}

void PlayerThread::restart()
{
  prepare(ctx->seq.GetSongHeaderPos());
  player->setState(State::PLAYING);
}

void PlayerThread::prepareBuffers()
{
  fill(masterAudio.begin(), masterAudio.end(), sample{0.0f, 0.0f});
}

void PlayerThread::processTrack(std::size_t index, std::vector<sample>& samples, bool mute)
{
  player->vuState.loudness[index].CalcLoudness(samples.data(), samplesPerBuffer);
  if (mute) {
    return;
  }

  for (size_t j = 0; j < samplesPerBuffer; j++) {
    masterAudio[j].left += samples[j].left;
    masterAudio[j].right += samples[j].right;
  }
}

void PlayerThread::outputBuffers()
{
  player->rBuf.Put(masterAudio.data(), masterAudio.size());
  player->vuState.masterLoudness.CalcLoudness(masterAudio.data(), samplesPerBuffer);
  player->vuState.update();
}

static GameConfig& cfg() {
  return ConfigManager::Instance().GetCfg();
}

ExportThread::ExportThread(Player* player)
: AudioThread(player, "export thread", new PlayerContext(
    ConfigManager::Instance().GetMaxLoopsPlaylist(),
    cfg().GetTrackLimit(),
    EnginePars(
      cfg().GetPCMVol(),
      cfg().GetEngineRev(),
      cfg().GetEngineFreq()
    )
  )),
  masterLeft(samplesPerBuffer, 0),
  masterRight(samplesPerBuffer, 0),
  silence(samplesPerBuffer, 0)
{
  player->abortExport = false;
}

ExportThread::~ExportThread()
{
  if (ctx) {
    delete ctx;
  }
}

void ExportThread::prepareBuffers()
{
  if (!exportTracks) {
    std::fill(masterLeft.begin(), masterLeft.end(), 0);
    std::fill(masterRight.begin(), masterRight.end(), 0);
  }
}

void ExportThread::processTrack(std::size_t index, std::vector<sample>& samples, bool)
{
  if (exportTracks) {
    for (size_t j = 0; j < samplesPerBuffer; j++) {
      masterLeft[j] = samples[j].left * 32767;
      masterRight[j] = samples[j].right * 32767;
    }
    riffs[index]->write(masterLeft, masterRight);
  } else {
    for (size_t j = 0; j < samplesPerBuffer; j++) {
      masterLeft[j] += samples[j].left * 32767;
      masterRight[j] += samples[j].right * 32767;
    }
  }
}

void ExportThread::outputBuffers()
{
  if (!exportTracks) {
    riff->write(masterLeft, masterRight);
  }
}

void ExportThread::pad(RiffWriter* riff, std::uint32_t samples) const
{
  while (samples > samplesPerBuffer) {
    riff->write(silence, silence);
    samples -= samplesPerBuffer;
  }
  if (samples > 0) {
    std::vector<std::int16_t> shortSilence(samples, 0);
    riff->write(shortSilence, shortSilence);
  }
}

void ExportThread::run()
{
  std::uint32_t padStart = ConfigManager::Instance().GetPadSecondsStart() * ctx->mixer.GetSampleRate();
  std::uint32_t padEnd = ConfigManager::Instance().GetPadSecondsEnd() * ctx->mixer.GetSampleRate();
  while (!player->exportQueue.isEmpty() && !player->abortExport) {
    auto item = player->exportQueue.takeFirst();
    exportTracks = item.splitTracks;
    try {
      prepare(item.trackAddr);
      if (exportTracks) {
        int numTracks = trackAudio.size();
        QDir dir(item.outputPath);
        if (!dir.mkpath(".")) {
          throw Xcept("Unable to create directory %s", qPrintable(item.outputPath));
        }
        riffs.clear();
        for (int i = 0; i < numTracks; i++) {
          RiffWriter* riff = new RiffWriter(ctx->mixer.GetSampleRate(), true);
          riffs.emplace_back(riff);
          QString filename = dir.absoluteFilePath(QStringLiteral("%1.wav").arg(i));
          bool ok = riff->open(filename);
          if (!ok) {
            riffs.clear();
            throw Xcept("Unable to open %s", qPrintable(filename));
          }
          pad(riff, padStart);
        }
      } else {
        riff.reset(new RiffWriter(ctx->mixer.GetSampleRate(), true));
        bool ok = riff->open(item.outputPath);
        if (!ok) {
          riff.reset();
          throw Xcept("Unable to open file");
        }
        pad(riff.get(), padStart);
      }
      emit player->exportStarted(item.outputPath);
      while (!player->abortExport) {
        if (process()) {
          break;
        }
      }
      if (exportTracks) {
        for (auto& riff : riffs) {
          pad(riff.get(), padEnd);
          riff->close();
        }
      } else {
        pad(riff.get(), padEnd);
        riff->close();
      }
      if (player->abortExport) {
        break;
      } else {
        emit player->exportFinished(item.outputPath);
      }
    } catch (std::exception& e) {
      emit player->exportError(e.what());
    }
  }
  if (player->abortExport) {
    emit player->exportCancelled();
  }
}


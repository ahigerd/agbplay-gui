#include "Player.h"
#include "AudioThread.h"
#include "ConfigManager.h"
#include "Rom.h"
#include "SongModel.h"
#include "UiUtils.h"
#include "Debug.h"
#include "RiffWriter.h"

// first portaudio hostapi has highest priority, last hostapi has lowest
// if none are available, the default one is selected.
// they are also the ones which are known to work
static const std::vector<PaHostApiTypeId> hostApiPriority = {
  // Unix
  paJACK,
  paALSA,
  // Windows
  paWASAPI,
  paMME,
  // Mac OS
  paCoreAudio,
  paSoundManager,
};

void Player::detectHostApi()
{
  outputStreamParameters.channelCount = 2;    // stereo
  outputStreamParameters.sampleFormat = paFloat32;

  // init host api
  std::vector<PaHostApiTypeId> hostApiPrioritiesWithFallback = hostApiPriority;
  const PaHostApiIndex defaultHostApiIndex = Pa_GetDefaultHostApi();
  if (defaultHostApiIndex < 0)
    throw Xcept("Pa_GetDefaultHostApi(): No host API avilable: %s", Pa_GetErrorText(defaultHostApiIndex));
  const PaHostApiInfo *defaultHostApiInfo = Pa_GetHostApiInfo(defaultHostApiIndex);
  if (defaultHostApiInfo == nullptr)
    throw Xcept("Pa_GetHostApiInfo(): failed with valid index");
  const auto f = std::find(hostApiPrioritiesWithFallback.begin(), hostApiPrioritiesWithFallback.end(), defaultHostApiInfo->type);
  if (f == hostApiPrioritiesWithFallback.end())
    hostApiPrioritiesWithFallback.push_back(defaultHostApiInfo->type);

  for (const auto apiType : hostApiPrioritiesWithFallback) {
    const PaHostApiIndex hostApiIndex = Pa_HostApiTypeIdToHostApiIndex(apiType);
    // prioritized host api available ?
    if (hostApiIndex < 0)
      continue;

    const PaHostApiInfo *apiInfo = Pa_GetHostApiInfo(hostApiIndex);
    if (apiInfo == nullptr)
      throw Xcept("Pa_GetHostApiInfo with valid index failed");
    const PaDeviceIndex deviceIndex = apiInfo->defaultOutputDevice;

    const PaDeviceInfo *devInfo = Pa_GetDeviceInfo(deviceIndex);
    if (devInfo == nullptr)
      throw Xcept("Pa_GetDeviceInfo(): failed with valid index");

    outputStreamParameters.device = deviceIndex;
    outputStreamParameters.suggestedLatency = devInfo->defaultLowOutputLatency;
    outputStreamParameters.hostApiSpecificStreamInfo = nullptr;

#if __has_include(<pa_win_wasapi.h>)
    if (apiType == paWASAPI) {
      memset(&wasapiStreamInfo, 0, sizeof(wasapiStreamInfo));
      wasapiStreamInfo.size = sizeof(wasapiStreamInfo);
      wasapiStreamInfo.hostApiType = paWASAPI;
      wasapiStreamInfo.version = 1;
      wasapiStreamInfo.flags = paWinWasapiAutoConvert;
    }
#endif

    PaError err = Pa_OpenStream(&audioStream, nullptr, &outputStreamParameters, STREAM_SAMPLERATE, paFramesPerBufferUnspecified, paNoFlag, audioCallback, this);
    if (err != paNoError) {
      Debug::print("Pa_OpenStream(): unable to open stream with host API %s: %s", apiInfo->name, Pa_GetErrorText(err));
      continue;
    }

    err = Pa_StartStream(audioStream);
    if (err != paNoError) {
      Debug::print("Pa_StartStream(): unable to start stream for Host API %s: %s", apiInfo->name, Pa_GetErrorText(err));
      err = Pa_CloseStream(audioStream);
      if (err != paNoError) {
        Debug::print("Pa_CloseStream(): unable to close stream for Host API %s: %s", apiInfo->name, Pa_GetErrorText(err));
      }
      audioStream = nullptr;
      continue;
    }
    Pa_StopStream(audioStream);
    return;
  }

  throw Xcept("Unable to initialize sound output: Host API could not be initialized");
}

Player::Player(QObject* parent)
: QObject(parent), ctx(nullptr), playerState(State::TERMINATED), audioStream(nullptr),
  speedFactor(64), rBuf(STREAM_BUF_SIZE)
{
  detectHostApi();

  model = new SongModel(this);

  timer.setTimerType(Qt::PreciseTimer);
  timer.setSingleShot(false);
  timer.setInterval(1000/30);
  QObject::connect(&timer, SIGNAL(timeout()), &updateThrottle, SLOT(start()));

  updateThrottle.setSingleShot(true);
  updateThrottle.setInterval(0);
  QObject::connect(&updateThrottle, SIGNAL(timeout()), this, SLOT(update()));
}

Player::~Player()
{
  if (audioStream) {
    Pa_StopStream(audioStream);
    PaError err = Pa_CloseStream(audioStream);
    if (err != paNoError) {
      Debug::print("Pa_CloseStream: %s", Pa_GetErrorText(err));
    }
  }
}

Rom* Player::openRom(const QString& path)
{
  stop();
  if (path.isEmpty()) {
    ctx.reset();
    return nullptr;
  }

  Rom::CreateInstance(qPrintable(path));
  Rom* rom = &Rom::Instance();
  ConfigManager::Instance().SetGameCode(rom->GetROMCode());
  const auto& cfg = ConfigManager::Instance().GetCfg();

  ctx = std::make_unique<PlayerContext>(
    ConfigManager::Instance().GetMaxLoopsPlaylist(),
    cfg.GetTrackLimit(),
    EnginePars(cfg.GetPCMVol(), cfg.GetEngineRev(), cfg.GetEngineFreq())
  );

  songTable.reset(new SongTable());
  model->setSongTable(songTable.get());
  emit songTableUpdated(songTable.get());

  selectSong(0);
  return rom;
}

SongModel* Player::songModel() const
{
  return model;
}

void Player::selectSong(int index)
{
  stop();
  QModelIndex idx = model->index(index, 0);
  std::uint32_t addr = model->songAddress(idx);
  ctx->InitSong(addr);

  vuState.setTrackCount(int(ctx->seq.tracks.size()));

  emit songChanged(ctx.get(), addr, idx.data(Qt::DisplayRole).toString());
}

void Player::play()
{
  if (!ctx) {
    return;
  }
  try {
    if (!playerThread) {
      playerThread.reset(new PlayerThread(this));
      QObject::connect(playerThread.get(), SIGNAL(finished()), this, SLOT(playbackDone()), Qt::QueuedConnection);
      playerThread->start();
    } else {
      State state = playerState;
      if (state == State::PLAYING) {
        setState(State::RESTART);
      } else if (state == State::PAUSED) {
        setState(State::PLAYING);
      }
    }
  } catch (std::exception& e) {
    Debug::print(e.what());
    emit threadError(tr("An error occurred while preparing to play:\n\n%1").arg(e.what()));
    return;
  }
  timer.start();
}

void Player::pause()
{
  timer.stop();
  if (!ctx || !playerThread) {
    return;
  }
  State state = playerState;
  if (state == State::PLAYING) {
    setState(State::PAUSED);
  } else if (state == State::PAUSED) {
    setState(State::PLAYING);
    timer.start();
  }
}

void Player::stop()
{
  timer.stop();
  if (!ctx) {
    return;
  }
  while (playerState == State::RESTART) {
    QThread::msleep(5);
  }
  if (playerState != State::TERMINATED) {
    setState(State::SHUTDOWN);
  }
  while (playerState != State::TERMINATED) {
    QThread::msleep(5);
  }
}

void Player::playbackDone()
{
  playerThread.reset();
  setState(State::TERMINATED);
  vuState.reset();
  update();
}

void Player::togglePlay()
{
  if (playerState == State::TERMINATED) {
    play();
  } else {
    pause();
  }
}

void Player::setState(Player::State state)
{
  playerState = state;
  emit stateChanged(state == State::RESTART || state == State::PLAYING || state == State::PAUSED, state == State::PAUSED);
}

void Player::update()
{
  emit updated(ctx.get(), &vuState);
}

void Player::setMute(int trackIdx, bool on)
{
  auto& track = ctx->seq.tracks[trackIdx];
  if (track.muted != on) {
    track.muted = on;
    updateThrottle.start();
  }
}

int Player::audioCallback(const void*, void* output, unsigned long frames, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* self)
{
  return reinterpret_cast<Player*>(self)->audioCallback(reinterpret_cast<sample*>(output), frames);
}

int Player::audioCallback(sample* output, size_t frames)
{
  rBuf.Take(output, frames);
  return 0;
}

bool Player::exportToWave(const QString& filename, int track)
{
  if (!ctx || !exportQueue.isEmpty() || exportThread) {
    return false;
  }
  try {
    QModelIndex idx = model->index(track, 0);
    quint32 addr = model->songAddress(idx);
    ExportItem item;
    item.outputPath = filename;
    item.trackAddr = addr;
    item.splitTracks = false;
    exportQueue << item;
    exportThread.reset(new ExportThread(this));
    QObject::connect(exportThread.get(), SIGNAL(finished()), this, SLOT(exportDone()), Qt::QueuedConnection);
    exportThread->start();
  } catch (std::exception& e) {
    Debug::print(e.what());
    emit threadError(tr("An error occurred while preparing to export:\n\n%1").arg(e.what()));
    return false;
  }
  return true;
}

bool Player::exportToWave(const QDir& path, const QList<int>& tracks, bool split)
{
  if (!ctx || !exportQueue.isEmpty() || exportThread) {
    return false;
  }
  try {
    for (int track : tracks) {
      QModelIndex idx = model->index(track, 0);
      quint32 addr = model->songAddress(idx);
      QString name = model->data(idx, Qt::EditRole).toString();
      if (!split || tracks.length() > 1) {
        QString prefix = fixedNumber(track, 4);
        if (name.isEmpty()) {
          name = prefix;
        } else {
          name = QStringLiteral("%1 - %2").arg(prefix).arg(name);
        }
        if (!split) {
          name = name + ".wav";
        }
      }
      ExportItem item;
      item.outputPath = path.absoluteFilePath(name);
      if (split && !item.outputPath.endsWith(path.separator())) {
        item.outputPath += path.separator();
      }
      item.trackAddr = addr;
      item.splitTracks = split;
      exportQueue << item;
    }
    exportThread.reset(new ExportThread(this));
    QObject::connect(exportThread.get(), SIGNAL(finished()), this, SLOT(exportDone()), Qt::QueuedConnection);
    exportThread->start();
  } catch (std::exception& e) {
    Debug::print(e.what());
    emit threadError(tr("An error occurred while preparing to export:\n\n%1").arg(e.what()));
    return false;
  }
  return true;
}

void Player::exportDone()
{
  exportThread.reset();
  exportQueue.clear();
}

void Player::cancelExport()
{
  abortExport = true;
}

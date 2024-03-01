#include "Player.h"
#include "ConfigManager.h"
#include "Rom.h"
#include "SongModel.h"
#include "UiUtils.h"
#include "Debug.h"
#include "RiffWriter.h"

class PlayerThread : public QThread
{
public:
  using State = Player::State;

  Player* player;
  PlayerContext* ctx;
  std::atomic<State>& playerState;
  std::size_t samplesPerBuffer;
  std::vector<sample> silence, masterAudio;
  std::vector<std::vector<sample>> trackAudio;
  VUState& vuState;

  PlayerThread(Player* player)
  : QThread(player), player(player), ctx(player->ctx.get()), playerState(player->playerState),
    samplesPerBuffer(ctx->mixer.GetSamplesPerBuffer()),
    silence(samplesPerBuffer, sample{0.0f, 0.0f}),
    masterAudio(samplesPerBuffer, sample{0.0f, 0.0f}),
    vuState(player->vuState)
  {
    setObjectName("mixer thread");

    PaError err = Pa_StartStream(player->audioStream);
    if (err != paNoError) {
      throw Xcept("Pa_StartStream(): unable to start stream: %s", Pa_GetErrorText(err));
    }
    player->setState(State::PLAYING);

    setTerminationEnabled(true);
  }

  ~PlayerThread()
  {
  }

  void restart()
  {
    ctx->InitSong(ctx->seq.GetSongHeaderPos());
    player->setState(State::PLAYING);
  }

  void play()
  {
    // clear high level mixing buffer
    fill(masterAudio.begin(), masterAudio.end(), sample{0.0f, 0.0f});
    // render audio buffers for tracks
    ctx->Process(trackAudio);
    for (size_t i = 0; i < trackAudio.size(); i++) {
      assert(trackAudio[i].size() == masterAudio.size());

      vuState.loudness[i].CalcLoudness(trackAudio[i].data(), samplesPerBuffer);
      if (ctx->seq.tracks[i].muted)
        continue;

      for (size_t j = 0; j < samplesPerBuffer; j++) {
        masterAudio[j].left += trackAudio[i][j].left;
        masterAudio[j].right += trackAudio[i][j].right;
      }
    }
    // blocking write to audio buffer
    player->rBuf.Put(masterAudio.data(), masterAudio.size());
    vuState.masterLoudness.CalcLoudness(masterAudio.data(), samplesPerBuffer);
    vuState.update();
    if (ctx->HasEnded()) {
      player->setState(State::SHUTDOWN);
    }
  }

  void runStream()
  {
    while (true) {
      switch (playerState) {
        case State::SHUTDOWN:
        case State::TERMINATED:
          return;
        case State::RESTART:
          restart();
          [[fallthrough]];
        case State::PLAYING:
          play();
          break;
        case State::PAUSED:
          player->rBuf.Put(silence.data(), silence.size());
          break;
        default:
          throw Xcept("Internal PlayerInterface error: %d", (int)playerState.load());
      }
    }
  }

  void run()
  {
    try {
      runStream();
      // reset song state after it has finished
      ctx->InitSong(ctx->seq.GetSongHeaderPos());
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
};

class ExportThread : public QThread
{
public:
  Player* player;
  PlayerContext ctx;
  std::size_t samplesPerBuffer;
  std::vector<std::int16_t> masterLeft, masterRight;
  std::vector<std::vector<sample>> trackAudio;
  QList<QPair<QString, quint32>>& exportQueue;

  static GameConfig& cfg() {
    return ConfigManager::Instance().GetCfg();
  }

  ExportThread(Player* player)
  : QThread(player),
    player(player),
    ctx(
      ConfigManager::Instance().GetMaxLoopsPlaylist(),
      cfg().GetTrackLimit(),
      EnginePars(
        cfg().GetPCMVol(),
        cfg().GetEngineRev(),
        cfg().GetEngineFreq()
      )
    ),
    samplesPerBuffer(ctx.mixer.GetSamplesPerBuffer()),
    masterLeft(samplesPerBuffer, 0),
    masterRight(samplesPerBuffer, 0),
    exportQueue(player->exportQueue)
  {
    setObjectName("export thread");
    setTerminationEnabled(true);
    player->abortExport = false;
  }

  ~ExportThread()
  {
  }

  void run()
  {
    while (!exportQueue.isEmpty() && !player->abortExport) {
      auto item = exportQueue.takeFirst();
      try {
        RiffWriter riff(ctx.mixer.GetSampleRate(), true);
        bool ok = riff.open(item.first);
        if (!ok) {
          throw Xcept("Unable to open file");
        }
        emit player->exportStarted(item.first);
        ctx.InitSong(item.second);
        while (!ctx.HasEnded() && !player->abortExport) {
          // clear high level mixing buffer
          fill(masterLeft.begin(), masterLeft.end(), 0);
          fill(masterRight.begin(), masterRight.end(), 0);
          // render audio buffers for tracks
          ctx.Process(trackAudio);
          for (size_t i = 0; i < trackAudio.size(); i++) {
            for (size_t j = 0; j < samplesPerBuffer; j++) {
              masterLeft[j] += trackAudio[i][j].left * 32767;
              masterRight[j] += trackAudio[i][j].right * 32767;
            }
          }
          // write to file
          riff.write(masterLeft, masterRight);
        }
        riff.close();
        if (player->abortExport) {
          break;
        } else {
          emit player->exportFinished(item.first);
        }
      } catch (std::exception& e) {
        emit player->exportError(e.what());
      }
    }
    if (player->abortExport) {
      emit player->exportCancelled();
    }
  }
};

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
    exportQueue << qMakePair(filename, addr);
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

bool Player::exportToWave(const QDir& path, const QList<int>& tracks)
{
  if (!ctx || !exportQueue.isEmpty() || exportThread) {
    return false;
  }
  try {
    for (int track : tracks) {
      QModelIndex idx = model->index(track, 0);
      quint32 addr = model->songAddress(idx);
      QString name = model->data(idx, Qt::EditRole).toString();
      QString prefix = fixedNumber(track, 4);
      if (name.isEmpty()) {
        name = QStringLiteral("%1.wav").arg(prefix);
      } else {
        name = QStringLiteral("%1 - %2.wav").arg(prefix).arg(name);
      }
      exportQueue << qMakePair(path.absoluteFilePath(name), addr);
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

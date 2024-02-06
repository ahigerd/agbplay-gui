#include "Player.h"
#include "ConfigManager.h"
#include "Rom.h"
#include "SongModel.h"
#include "UiUtils.h"

Player::Player(QObject* parent)
: QObject(parent), ctx(nullptr)
{
  model = new SongModel(this);

  timer.setTimerType(Qt::PreciseTimer);
  timer.setSingleShot(false);
  timer.setInterval(1000/60);
  QObject::connect(&timer, SIGNAL(timeout()), &updateThrottle, SLOT(start()));

  updateThrottle.setSingleShot(true);
  updateThrottle.setInterval(0);
  QObject::connect(&updateThrottle, SIGNAL(timeout()), this, SLOT(update()));
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
  std::uint32_t addr = model->songAddress(model->index(index, 0));
  ctx->InitSong(addr);
  emit songChanged(ctx.get(), addr, fixedNumber(index, 4));
}

void Player::play()
{
  if (!ctx) {
    return;
  }
  timer.start();
}

void Player::pause()
{
  timer.stop();
  if (!ctx) {
    return;
  }
}

void Player::stop()
{
  timer.stop();
  if (!ctx) {
    return;
  }
}

void Player::update()
{
  emit updated(ctx.get());
}

void Player::setMute(int trackIdx, bool on)
{
  auto& track = ctx->seq.tracks[trackIdx];
  if (track.muted != on) {
    track.muted = on;
    updateThrottle.start();
  }
}

#include <fstream>
#include <regex>
#include <cstring>
#include <iostream>
#include <cstdlib>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QFile>
#include <QDir>

#include "ConfigManager.h"
#include "Util.h"
#include "Xcept.h"
#include "Debug.h"
#include "OS.h"

#ifdef Q_OS_WIN
#define CONFIG_PATH QStandardPaths::AppDataLocation
#else
#define CONFIG_PATH QStandardPaths::ConfigLocation
#endif

ConfigManager& ConfigManager::Instance()
{
    static ConfigManager cm;
    return cm;
}

GameConfig& ConfigManager::GetCfg()
{
    if (curCfg)
        return *curCfg;
    else
        throw Xcept("Trying to get the game config without setting the game code");
}

const GameConfig& ConfigManager::GetCfg() const
{
    if (curCfg)
        return *curCfg;
    else
        throw Xcept("Trying to get the game config without setting the game code");
}

void ConfigManager::SetGameCode(const std::string& gameCode)
{
    for (GameConfig& config : configs)
    {
        const auto &gameCodesToCheck = config.GetGameCodes();
        if (std::find(gameCodesToCheck.begin(), gameCodesToCheck.end(), gameCode) != gameCodesToCheck.end()) {
            curCfg = &config;
            return;
        }
    }
    configs.emplace_back(gameCode);
    curCfg = &configs.back();
}

void ConfigManager::Load()
{
  /* Parse things from config file.
   * If the config file in home directory is not found,
   * try reading it from /etc/agbplay/agbplay.json.
   * If this isn't found either, use an empty config file. */
  QJsonObject root;
  QDir localDir(QStandardPaths::writableLocation(CONFIG_PATH));
  QString localPath = localDir.absoluteFilePath("agbplay.json");
  QString configPath = QStandardPaths::locate(CONFIG_PATH, "agbplay.json");
#ifdef Q_OS_WIN
  // On Windows, AppDataLocation refers to AppData/Roaming/agbplay-gui
  // But agbplay CLI looks in AppData/Roaming
  // If someone happens to already have a file there, honor it
  if (configPath.isEmpty()) {
    localDir.cdUp();
    if (localDir.exists("agbplay.json")) {
      configPath = localPath = localDir.absoluteFilePath("agbplay.json");;
    }
  }
#endif
  if (configPath.isEmpty()) {
    configPath = ":/agbplay.json";
    Debug::print("No configuration file found. Loading from defaults.");
  } else {
    if (configPath == localPath) {
      Debug::print("User local configuration found!");
    } else {
      Debug::print("Global configuration found!");
    }
  }
  QFile f(configPath);
  if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
    throw Xcept("Can't read file: %s", qPrintable(configPath));
  }
  QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
  root = doc.object();

  if (root["id"].toString() != "agbplay")
    throw Xcept("Bad JSON ID: %s", qPrintable(root["id"].toString()));

  // output directory used for saving rendered sogs
  if (root.contains("wav-output-dir")) {
    confWavOutputDir = root["wav-output-dir"].toString().toStdString();
  } else {
    confWavOutputDir = QDir(QStandardPaths::writableLocation(QStandardPaths::MusicLocation)).absoluteFilePath("agbplay").toStdString();
  }

  // CGB channel polyphony configuration
  confCgbPolyphony = str2cgbPoly(root.value("cgb-polyphony").toString("mono-strict").toStdString());

  // Loop configuration
  maxLoopsPlaylist = static_cast<int8_t>(root.value("max-loops-playlist").toInt(1));
  maxLoopsExport = static_cast<int8_t>(root.value("max-loops-export").toInt(1));

  // Silence padding
  padSecondsStart = root.value("pad-seconds-start").toDouble();
  padSecondsEnd = root.value("pad-seconds-end").toDouble();

  for (const QJsonValue& playlistValue : root["playlists"].toArray()) {
    QJsonObject playlist = playlistValue.toObject();
    // parse games
    std::vector<std::string> games;
    for (const QJsonValue& game : playlist["games"].toArray())
      games.emplace_back(game.toString().toStdString());
    configs.emplace_back(games);

    // parse other parameters
    configs.back().SetPCMVol(uint8_t(std::clamp<int>(playlist.value("pcm-master-volume").toInt(15), 0, 15)));
    configs.back().SetEngineFreq(uint8_t(std::clamp<int>(playlist.value("pcm-samplerate").toInt(4), 0, 15)));
    configs.back().SetEngineRev(uint8_t(std::clamp<int>(playlist.value("pcm-reverb-level").toInt(0), 0, 255)));
    configs.back().SetRevBufSize(uint16_t(playlist.value("pcm-reverb-buffer-len").toDouble(0x630)));
    configs.back().SetRevType(str2rev(playlist.value("pcm-reverb-type").toString("normal").toStdString()));
    configs.back().SetResType(str2res(playlist.value("pcm-resampling-algo").toString("linear").toStdString()));
    configs.back().SetResTypeFixed(str2res(playlist.value("pcm-fixed-rate-resampling-algo").toString("linear").toStdString()));
    configs.back().SetTrackLimit(uint8_t(std::clamp<int>(playlist.value("song-track-limit").toInt(16), 0, 16)));
    configs.back().SetAccurateCh3Volume(playlist.value("accurate-ch3-volume").toBool());
    configs.back().SetAccurateCh3Quantization(playlist.value("accurate-ch3-quantization").toBool());
    configs.back().SetSimulateCGBSustainBug(playlist.value("simulate-cgb-sustain-bug").toBool());

    for (const QJsonValue& songValue : playlist["songs"].toArray()) {
      QJsonObject song = songValue.toObject();
      configs.back().GetGameEntries().emplace_back(
          song.value("name").toString("?").toStdString(),
          static_cast<uint16_t>(song.value("index").toInt()));
    }
  }
}

void ConfigManager::Save()
{
    QJsonArray playlists;
    for (GameConfig& cfg : configs)
    {
        QJsonObject playlist;
        playlist["pcm-master-volume"] = static_cast<int>(cfg.GetPCMVol());
        playlist["pcm-samplerate"] = static_cast<int>(cfg.GetEngineFreq());
        playlist["pcm-reverb-level"] = static_cast<int>(cfg.GetEngineRev());
        playlist["pcm-reverb-buffer-len"] = static_cast<int>(cfg.GetRevBufSize());
        playlist["pcm-reverb-type"] = QString::fromStdString(rev2str(cfg.GetRevType()));
        playlist["pcm-resampling-algo"] = QString::fromStdString(res2str(cfg.GetResType()));
        playlist["pcm-fixed-rate-resampling-algo"] = QString::fromStdString(res2str(cfg.GetResTypeFixed()));
        playlist["song-track-limit"] = static_cast<int>(cfg.GetTrackLimit());
        playlist["accurate-ch3-volume"] = cfg.GetAccurateCh3Volume();
        playlist["accurate-ch3-quantization"] = cfg.GetAccurateCh3Quantization();
        playlist["simulate-cgb-sustain-bug"] = cfg.GetSimulateCGBSustainBug();

        QJsonArray games;
        for (const std::string& code : cfg.GetGameCodes())
            games.append(QString::fromStdString(code));
        playlist["games"] = games;

        QJsonArray songs;
        for (SongEntry entr : cfg.GetGameEntries()) {
            QJsonObject song;
            song["index"] = entr.GetUID();
            song["name"] = QString::fromStdString(entr.name);
            songs.append(song);
        }
        playlist["songs"] = songs;
        playlists.append(playlist);
    }

    QJsonObject root;
    root["id"] = "agbplay";
    root["wav-output-dir"] = QString::fromStdString(confWavOutputDir.string());
    root["cgb-polyphony"] = QString::fromStdString(cgbPoly2str(confCgbPolyphony));
    root["playlists"] = playlists;
    root["max-loops-playlist"] = maxLoopsPlaylist;
    root["max-loops-export"] = maxLoopsExport;
    root["pad-seconds-start"] = padSecondsStart;
    root["pad-seconds-end"] = padSecondsEnd;

    QDir localDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    localDir.mkpath(".");
    QString localPath = localDir.absoluteFilePath("agbplay.json");
    QFile jsonFile(localPath);
    // XXX: QFileDevice::FileError isn't declared with Q_ENUM so we can't get a string description
    if (!jsonFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        throw Xcept("Error while writing agbplay.json: %s", QString::number(jsonFile.error()).toStdString());

    QJsonDocument doc(root);
    jsonFile.write(doc.toJson());
    jsonFile.write("\n");

    Debug::print("Configuration/Playlist saved!");
}

const std::filesystem::path& ConfigManager::GetWavOutputDir()
{
    return confWavOutputDir;
}

CGBPolyphony ConfigManager::GetCgbPolyphony() const
{
    return confCgbPolyphony;
}

int8_t ConfigManager::GetMaxLoopsPlaylist() const
{
    return maxLoopsPlaylist < -1 ? 0 : maxLoopsPlaylist;
}

int8_t ConfigManager::GetMaxLoopsExport() const
{
    return maxLoopsExport < 0 ? 0 : maxLoopsExport;
}

double ConfigManager::GetPadSecondsStart() const
{
    return padSecondsStart;
}

double ConfigManager::GetPadSecondsEnd() const
{
    return padSecondsEnd;
}

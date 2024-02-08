#include "SongModel.h"
#include "SoundData.h"
#include "UiUtils.h"
#include "ConfigManager.h"
#include "SongEntry.h"
#include <QApplication>
#include <QMimeData>
#include <QStyle>
#include <QPalette>
#include <QImage>

SongModel::SongModel(QObject* parent)
: QAbstractListModel(parent), songTable(nullptr), activeSong(-1), isPlaying(false), isPaused(false)
{
  // initializers only
}

void SongModel::setSongTable(SongTable* table)
{
  beginResetModel();
  activeSong = -1;
  songTable = table;

  titles.clear();
  std::size_t numSongs = songTable->GetNumSongs();
  for (std::size_t i = 0; i < numSongs; i++) {
    titles << QString();
  }

  auto entries = ConfigManager::Instance().GetCfg().GetGameEntries();
  for (const auto& entry : entries) {
    titles[entry.GetUID()] = QString::fromStdString(entry.GetName());
  }

  endResetModel();
}

int SongModel::rowCount(const QModelIndex& parent) const
{
  if (!songTable || parent.isValid()) {
    return 0;
  }
  return int(songTable->GetNumSongs());
}

QVariant SongModel::data(const QModelIndex& index, int role) const
{
  if (role == Qt::EditRole) {
    return titles[index.row()];
  } else if (role == Qt::DisplayRole) {
    return QStringLiteral("[%1] %2").arg(fixedNumber(index.row(), 4)).arg(titles[index.row()]);
  } else if (role == Qt::ForegroundRole) {
    if (activeSong == index.row()) {
      return qApp->style()->standardPalette().buttonText();
    }
  } else if (role == Qt::BackgroundRole) {
    if (activeSong == index.row()) {
      return qApp->style()->standardPalette().button();
    }
  } else if (role == Qt::DecorationRole) {
    if (activeSong == index.row()) {
      if (isPaused) {
        return qApp->style()->standardIcon(QStyle::SP_MediaPause);
      } else if (isPlaying) {
        return qApp->style()->standardIcon(QStyle::SP_MediaPlay);
      } else {
        return qApp->style()->standardIcon(QStyle::SP_MediaStop);
      }
    } else if (blankIcon.isNull()) {
      QPixmap px = qApp->style()->standardPixmap(QStyle::SP_MediaPlay);
      QImage blank(px.width(), px.height(), QImage::Format_ARGB32);
      blank.fill(0);
      blankIcon.addPixmap(QPixmap::fromImage(blank));
    }
    return blankIcon;
  }
  return QVariant();
}

bool SongModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  if (role != Qt::EditRole) {
    return false;
  }
  titles[index.row()] = value.toString();
  emit dataChanged(index, index);
  return true;
}

QVariant SongModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (section == 0 && orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    return tr("Songs");
  }
  return QAbstractListModel::headerData(section, orientation, role);
}

std::uint32_t SongModel::songAddress(const QModelIndex& index) const
{
  if (!songTable || !index.isValid() || index.parent().isValid()) {
    return 0;
  }
  return std::uint32_t(songTable->GetPosOfSong(std::uint16_t(index.row())));
}

int SongModel::findByAddress(quint32 addr) const
{
  int ct = rowCount();
  for (int i = 0; i < ct; i++) {
    if (songTable->GetPosOfSong(std::uint16_t(i)) == addr) {
      return i;
    }
  }
  return -1;
}

void SongModel::songChanged(PlayerContext*, quint32 addr)
{
  int oldActiveSong = activeSong;
  activeSong = findByAddress(addr);
  if (oldActiveSong == activeSong) {
    return;
  }
  if (oldActiveSong >= 0) {
    QModelIndex idx = index(oldActiveSong, 0);
    emit dataChanged(idx, idx);
  }
  if (activeSong >= 0) {
    QModelIndex idx = index(activeSong, 0);
    emit dataChanged(idx, idx);
  }
}

Qt::ItemFlags SongModel::flags(const QModelIndex& index) const
{
  if (!index.isValid()) {
    return Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
  }
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled;
}

Qt::DropActions SongModel::supportedDragActions() const
{
  return Qt::LinkAction;
}

QMimeData* SongModel::mimeData(const QModelIndexList& idxs) const
{
  if (idxs.isEmpty()) {
    return nullptr;
  }
  QMimeData* data = new QMimeData();
  QStringList content;
  for (const QModelIndex& idx : idxs) {
    content << QString::number(idx.row());
  }
  data->setData("agbplay/tracklist", content.join(",").toUtf8());
  return data;
}

void SongModel::stateChanged(bool isPlaying, bool isPaused)
{
  this->isPlaying = isPlaying;
  this->isPaused = isPaused;
  emit dataChanged(index(activeSong, 0), index(activeSong, 0));
}

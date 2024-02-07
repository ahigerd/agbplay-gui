#include "SongModel.h"
#include "SoundData.h"
#include "UiUtils.h"
#include <QApplication>
#include <QStyle>
#include <QPalette>

SongModel::SongModel(QObject* parent)
: QAbstractListModel(parent), songTable(nullptr), activeSong(-1)
{
  // initializers only
}

void SongModel::setSongTable(SongTable* table)
{
  beginResetModel();
  activeSong = -1;
  songTable = table;
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
  if (role == Qt::DisplayRole) {
    return fixedNumber(index.row(), 4);
  } else if (role == Qt::ForegroundRole) {
    if (activeSong == index.row()) {
      return qApp->style()->standardPalette().buttonText();
    }
  } else if (role == Qt::BackgroundRole) {
    if (activeSong == index.row()) {
      return qApp->style()->standardPalette().button();
    }
  }
  return QVariant();
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

void SongModel::songChanged(PlayerContext*, quint32 addr)
{
  int ct = rowCount();
  int oldActiveSong = activeSong;
  activeSong = -1;
  for (int i = 0; i < ct; i++) {
    if (songTable->GetPosOfSong(std::uint16_t(i)) == addr) {
      activeSong = i;
      break;
    }
  }
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

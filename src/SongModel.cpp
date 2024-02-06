#include "SongModel.h"
#include "SoundData.h"
#include "UiUtils.h"

SongModel::SongModel(QObject* parent)
: QAbstractListModel(parent), songTable(nullptr)
{
  // initializers only
}

void SongModel::setSongTable(SongTable* table)
{
  beginResetModel();
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

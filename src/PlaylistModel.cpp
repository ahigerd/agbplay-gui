#include "PlaylistModel.h"
#include "SongModel.h"
#include "ConfigManager.h"
#include <QtDebug>

PlaylistModel::PlaylistModel(SongModel* source)
: QAbstractProxyModel(source)
{
  setSourceModel(source);

  QObject::connect(source, SIGNAL(modelReset()), this, SLOT(reload()));
  QObject::connect(source, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(onDataChanged(QModelIndex,QModelIndex)));
}

void PlaylistModel::reload()
{
  trackOrder.clear();
  auto entries = ConfigManager::Instance().GetCfg().GetGameEntries();
  for (const auto& entry : entries) {
    trackIndex[entry.GetUID()] = trackOrder.length();
    trackOrder << entry.GetUID();
  }
}

int PlaylistModel::rowCount(const QModelIndex& parent) const
{
  if (parent.isValid()) {
    return 0;
  }
  return trackOrder.length();
}

int PlaylistModel::columnCount(const QModelIndex& parent) const
{
  if (parent.isValid()) {
    return 0;
  }
  return sourceModel()->columnCount();
}

QModelIndex PlaylistModel::index(int row, int col, const QModelIndex& parent) const
{
  if (row < 0 || row >= trackOrder.length() || col < 0 || col >= sourceModel()->columnCount() || parent.isValid()) {
    return QModelIndex();
  }
  return createIndex(row, col, trackOrder[row]);
}

QModelIndex PlaylistModel::parent(const QModelIndex&) const
{
  return QModelIndex();
}

QModelIndex PlaylistModel::mapFromSource(const QModelIndex& idx) const
{
  int pos = trackIndex.value(idx.row(), -1);
  if (pos < 0) {
    return QModelIndex();
  }
  return index(pos, idx.column());
}

QModelIndex PlaylistModel::mapToSource(const QModelIndex& idx) const
{
  int row = idx.row();
  if (row < 0 || row >= trackOrder.length()) {
    return QModelIndex();
  }
  return sourceModel()->index(trackOrder[row], idx.column());
}

QVariant PlaylistModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (section == 0 && orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    return tr("Playlist");
  }
  return sourceModel()->headerData(section, orientation, role);
}

void PlaylistModel::onDataChanged(const QModelIndex& start, const QModelIndex& end)
{
  int min = trackOrder.length() - 1;
  int max = 0;
  int sRow = start.row();
  int eRow = end.row();
  if (eRow < sRow) {
    sRow = eRow;
    eRow = start.row();
  }
  for (int i = sRow; i <= eRow; i++) {
    int pos = trackIndex.value(i, -1);
    if (pos < 0) {
      continue;
    }
    if (pos > max) {
      max = pos;
    }
    if (pos < min) {
      min = pos;
    }
  }
  if (min <= max) {
    emit dataChanged(index(min, 0), index(max, columnCount() - 1));
  }
}

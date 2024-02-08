#include "PlaylistModel.h"
#include "SongModel.h"
#include "ConfigManager.h"
#include <QMimeData>
#include <algorithm>

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

Qt::DropActions PlaylistModel::supportedDragActions() const
{
  return Qt::MoveAction;
}

Qt::DropActions PlaylistModel::supportedDropActions() const
{
  return Qt::MoveAction | Qt::LinkAction;
}

QStringList PlaylistModel::mimeTypes() const
{
  return QStringList() << "agbplay/tracklist";
}

QMimeData* PlaylistModel::mimeData(const QModelIndexList& idxs) const
{
  if (idxs.isEmpty()) {
    return nullptr;
  }
  QMimeData* data = new QMimeData();
  QStringList content;
  for (const QModelIndex& idx : idxs) {
    content << QStringLiteral("@%1").arg(idx.row());
  }
  data->setData("agbplay/tracklist", content.join(",").toUtf8());
  return data;
}

bool PlaylistModel::canDropMimeData(const QMimeData* data, Qt::DropAction, int, int, const QModelIndex&) const
{
  return data->formats().contains("agbplay/tracklist");
}

bool PlaylistModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int beforeRow, int, const QModelIndex&)
{
  QStringList tracks = QString::fromUtf8(data->data("agbplay/tracklist")).split(",");
  int ct = tracks.length();
  int minPos = beforeRow, maxPos = beforeRow + ct;
  if (action == Qt::MoveAction) {
    QList<int> toRemove;
    // First, collect the items that will be removed and the bounds that will be affected
    for (const QString& item : tracks) {
      int pos = item.section('@', 1, 1).toInt();
      toRemove << pos;
      if (pos <= beforeRow) {
        --beforeRow;
      }
      if (pos < minPos) {
        minPos = pos;
      }
      if (pos > maxPos) {
        maxPos = pos;
      }
    }

    // Create a list of indexes to operate on; the data will be updated later
    emit layoutAboutToBeChanged(QList<QPersistentModelIndex>(), QAbstractItemModel::VerticalSortHint);
    QModelIndexList layoutBefore, layoutAfter;
    for (int i = minPos; i <= maxPos; i++) {
      layoutBefore << index(i);
    }

    // Remove the items being moved, from last to first
    QList<int> toRemoveSorted = toRemove;
    std::sort(toRemoveSorted.begin(), toRemoveSorted.end(), std::greater<int>());
    for (int pos : toRemoveSorted) {
      layoutBefore.removeAt(pos - minPos);
    }

    // Insert the items being moved into the new locations
    for (int i = 0; i < ct; i++) {
      layoutBefore.insert(beforeRow + i - minPos, index(toRemove[i]));
    }

    // Update the data and any persistent model indexes
    for (int i = minPos; i <= maxPos; i++) {
      quintptr id = layoutBefore[i - minPos].internalId();
      layoutAfter << createIndex(i, 0, id);
      trackOrder[i] = int(id);
    }
    changePersistentIndexList(layoutBefore, layoutAfter);
  } else {
    // Insertion is much more simple
    QList<int> toInsert;
    for (const QString& item : tracks) {
      toInsert << item.toInt();
    }
    beginInsertRows(QModelIndex(), beforeRow, beforeRow + ct - 1);
    for (int i = 0; i < ct; i++) {
      trackOrder.insert(beforeRow + i, toInsert[i]);
    }
  }

  // Update the mapping index
  trackIndex.clear();
  for (int i = 0; i < trackOrder.length(); i++) {
    trackIndex[trackOrder[i]] = i;
  }

  // Notify views of changes
  if (action == Qt::MoveAction) {
    emit layoutChanged(QList<QPersistentModelIndex>(), QAbstractItemModel::VerticalSortHint);
  } else {
    endInsertRows();
  }
  return true;
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

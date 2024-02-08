#pragma once

#include <QAbstractProxyModel>
#include <QHash>
#include <memory>
class SongModel;

class PlaylistModel : public QAbstractProxyModel
{
Q_OBJECT
public:
  PlaylistModel(SongModel* source);

  int rowCount(const QModelIndex& parent = QModelIndex()) const;
  int columnCount(const QModelIndex& parent = QModelIndex()) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

  QModelIndex index(int row, int col = 0, const QModelIndex& parent = QModelIndex()) const;
  QModelIndex parent(const QModelIndex& idx) const;
  QModelIndex mapFromSource(const QModelIndex& idx) const;
  QModelIndex mapToSource(const QModelIndex& idx) const;

private slots:
  void reload();
  void onDataChanged(const QModelIndex& start, const QModelIndex& end);

private:
  QList<int> trackOrder;
  QHash<int, int> trackIndex;
};

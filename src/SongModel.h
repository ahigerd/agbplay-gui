#pragma once

#include <QAbstractListModel>
#include <QIcon>
#include <memory>
class SongTable;
class PlayerContext;

class SongModel : public QAbstractListModel
{
Q_OBJECT
public:
  SongModel(QObject* parent = nullptr);

  int rowCount(const QModelIndex& parent = QModelIndex()) const;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
  bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::DisplayRole);
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
  Qt::ItemFlags flags(const QModelIndex& idx) const;
  Qt::DropActions supportedDragActions() const;
  QMimeData* mimeData(const QModelIndexList& idxs) const;

  std::uint32_t songAddress(const QModelIndex& index) const;

signals:
  void playlistDirty(bool dirty = true);

public slots:
  void setSongTable(SongTable* table);
  void songChanged(PlayerContext*, quint32 addr);
  void stateChanged(bool isPlaying, bool isPaused);

protected:
  int findByAddress(quint32 addr) const;

private:
  SongTable* songTable;
  int activeSong;
  bool isPlaying, isPaused;
  QStringList titles;
  mutable QIcon blankIcon;
};

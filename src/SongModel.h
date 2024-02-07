#pragma once

#include <QAbstractListModel>
#include <memory>
class Rom;
class SongTable;
class PlayerContext;

class SongModel : public QAbstractListModel
{
Q_OBJECT
public:
  SongModel(QObject* parent = nullptr);

  int rowCount(const QModelIndex& parent = QModelIndex()) const;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

  std::uint32_t songAddress(const QModelIndex& index) const;

public slots:
  void setSongTable(SongTable* table);
  void songChanged(PlayerContext*, quint32 addr);

private:
  SongTable* songTable;
  int activeSong;
};

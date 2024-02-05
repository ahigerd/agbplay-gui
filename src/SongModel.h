#pragma once

#include <QAbstractListModel>
#include <memory>
#include "SoundData.h"
class Rom;

class SongModel : public QAbstractListModel
{
Q_OBJECT
public:
  SongModel(QObject* parent = nullptr);

  int rowCount(const QModelIndex& parent = QModelIndex()) const;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

  void openRom(Rom*);

signals:
  void songTableUpdated(SongTable*);

private:
  std::unique_ptr<SongTable> songTable;
};

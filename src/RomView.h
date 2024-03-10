#pragma once

#include <QWidget>
class QLabel;
class QComboBox;
class Rom;
class SongTable;

class RomView : public QWidget
{
Q_OBJECT
public:
  RomView(QWidget* parent = nullptr);

public slots:
  void updateRom(Rom* rom);
  void songTablesFound(const std::vector<quint32>& addrs);
  void updateSongTable(SongTable* table);

signals:
  void songTableSelected(quint32 addr);

private slots:
  void onSongTableSelected();

private:
  QLabel* addLabel(const QString& title);

  QLabel* romName;
  QLabel* romCode;
  QLabel* tablePos;
  QComboBox* tableSelector;
  QLabel* numSongs;
};

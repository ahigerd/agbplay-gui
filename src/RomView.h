#pragma once

#include <QWidget>
class QLabel;
class Rom;
class SongTable;

class RomView : public QWidget
{
Q_OBJECT
public:
  RomView(QWidget* parent = nullptr);

public slots:
  void updateRom(Rom* rom);
  void updateSongTable(SongTable* table);

private:
  QLabel* addLabel(const QString& title);

  QLabel* romName;
  QLabel* romCode;
  QLabel* tablePos;
  QLabel* numSongs;
};

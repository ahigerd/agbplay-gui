#include "RomView.h"
#include "Rom.h"
#include "SoundData.h"
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

RomView::RomView(QWidget* parent)
: QWidget(parent)
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  romName = addLabel(tr("ROM Name:"));
  romCode = addLabel(tr("ROM Code:"));
  tablePos = addLabel(tr("Songtable Offset:"));
  numSongs = addLabel(tr("Number of Songs:"));
  layout->addStretch(1);
}

QLabel* RomView::addLabel(const QString& title)
{
  QGroupBox* box = new QGroupBox(title, this);
  QVBoxLayout* boxLayout = new QVBoxLayout(box);
  QLabel* label = new QLabel(box);
  boxLayout->setContentsMargins(0, 0, 0, 0);
  boxLayout->addWidget(label);
  static_cast<QVBoxLayout*>(layout())->addWidget(box, 0);
  return label;
}

void RomView::updateRom(Rom* rom)
{
  if (!rom) {
    romName->setText("");
    romCode->setText("");
  } else {
    romName->setText(QString::fromStdString(rom->ReadString(0xA0, 12)));
    romCode->setText(QString::fromStdString(rom->GetROMCode()));
  }
}

void RomView::updateSongTable(SongTable* table)
{
  if (!table) {
    tablePos->setText("");
    numSongs->setText("");
  } else {
    tablePos->setText("0x" + QString::number(table->GetSongTablePos(), 16));
    numSongs->setText(QString::number(table->GetNumSongs()));
  }
}

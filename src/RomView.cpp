#include "RomView.h"
#include "Rom.h"
#include "SoundData.h"
#include <QGroupBox>
#include <QLabel>
#include <QComboBox>
#include <QVBoxLayout>

RomView::RomView(QWidget* parent)
: QWidget(parent)
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  romName = addLabel(tr("ROM Name:"));
  romCode = addLabel(tr("ROM Code:"));

  QGroupBox* box = new QGroupBox(tr("Songtable Offset:"), this);
  QVBoxLayout* boxLayout = new QVBoxLayout(box);
  tablePos = new QLabel(box);
  tableSelector = new QComboBox(box);
  boxLayout->setContentsMargins(0, 0, 0, 0);
  boxLayout->addWidget(tablePos);
  boxLayout->addWidget(tableSelector);
  tableSelector->hide();
  layout->addWidget(box, 0);

  numSongs = addLabel(tr("Number of Songs:"));
  layout->addStretch(1);

  QObject::connect(tableSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(onSongTableSelected()));
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

void RomView::songTablesFound(const std::vector<quint32>& addrs)
{
  tableSelector->blockSignals(true);
  tableSelector->clear();

  for (quint32 addr : addrs) {
    tableSelector->addItem("0x" + QString::number(addr, 16), QVariant::fromValue(addr));
  }

  tableSelector->setVisible(addrs.size() > 1);
  tablePos->setVisible(addrs.size() <= 1);
  tableSelector->blockSignals(false);
}

void RomView::updateSongTable(SongTable* table)
{
  if (!table) {
    tablePos->setText("");
    numSongs->setText("");
    tableSelector->hide();
    tablePos->show();
  } else {
    auto addr = table->GetSongTablePos();
    tablePos->setText("0x" + QString::number(addr, 16));
    int index = tableSelector->findData(QVariant::fromValue(addr));
    tableSelector->blockSignals(true);
    tableSelector->setCurrentIndex(index);
    tableSelector->blockSignals(false);
    numSongs->setText(QString::number(table->GetNumSongs()));
  }
}

void RomView::onSongTableSelected()
{
  emit songTableSelected(tableSelector->currentData().value<quint32>());
}

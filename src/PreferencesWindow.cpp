#include "PreferencesWindow.h"
#include "ConfigManager.h"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>

PreferencesWindow::PreferencesWindow(QWidget* parent)
: QDialog(parent)
{
  setWindowTitle(tr("agbplay Preferences"));

  ConfigManager& cfg = ConfigManager::Instance();
  QGridLayout* layout = new QGridLayout(this);
  layout->setColumnStretch(1, 1);

  QLabel* lblCgbPolyphony = new QLabel(tr("&CGB Polyphony:"), this);
  layout->addWidget(lblCgbPolyphony, 0, 0);
  layout->addWidget(cgbPolyphony = new QComboBox(this), 0, 1, 1, 2);
  lblCgbPolyphony->setBuddy(cgbPolyphony);
  cgbPolyphony->addItem(tr("Strict (Default)"), int(CGBPolyphony::MONO_STRICT));
  cgbPolyphony->addItem(tr("Smooth"), int(CGBPolyphony::MONO_SMOOTH));
  cgbPolyphony->addItem(tr("Polyphonic"), int(CGBPolyphony::POLY));
  cgbPolyphony->setCurrentIndex(int(cfg.GetCgbPolyphony()));

  QLabel* lblMaxLoopsPlaylist = new QLabel(tr("&Loops during playback:"), this);
  layout->addWidget(lblMaxLoopsPlaylist, 1, 0);
  layout->addWidget(maxLoopsPlaylist = new QSpinBox(this), 1, 1, 1, 2);
  lblMaxLoopsPlaylist->setBuddy(maxLoopsPlaylist);
  maxLoopsPlaylist->setMinimum(1);

  loopInfinitely = new QCheckBox(tr("Loop &infinitely"), this);
  if (cfg.GetMaxLoopsPlaylist() < 0) {
    maxLoopsPlaylist->setValue(1);
    loopInfinitely->setChecked(true);
    maxLoopsPlaylist->setEnabled(false);
  } else {
    maxLoopsPlaylist->setValue(cfg.GetMaxLoopsPlaylist());
  }
  layout->addWidget(loopInfinitely, 2, 1, 1, 2);

  QLabel* lblMaxLoopsExport = new QLabel(tr("L&oops during export:"), this);
  layout->addWidget(lblMaxLoopsExport, 3, 0);
  layout->addWidget(maxLoopsExport = new QSpinBox(this), 3, 1, 1, 2);
  lblMaxLoopsExport->setBuddy(maxLoopsExport);
  maxLoopsExport->setValue(cfg.GetMaxLoopsExport());
  maxLoopsExport->setMinimum(1);

  QLabel* lblPadSecondsStart = new QLabel(tr("Add silence to &start of export:"), this);
  layout->addWidget(lblPadSecondsStart, 4, 0);
  layout->addWidget(padSecondsStart = new QDoubleSpinBox(this), 4, 1);
  layout->addWidget(new QLabel(tr("sec"), this), 4, 2);
  lblPadSecondsStart->setBuddy(padSecondsStart);
  padSecondsStart->setValue(cfg.GetPadSecondsStart());
  padSecondsStart->setMinimum(0);

  QLabel* lblPadSecondsEnd = new QLabel(tr("Add silence to &end of export:"), this);
  layout->addWidget(lblPadSecondsEnd, 5, 0);
  layout->addWidget(padSecondsEnd = new QDoubleSpinBox(this), 5, 1);
  layout->addWidget(new QLabel(tr("sec"), this), 5, 2);
  lblPadSecondsEnd->setBuddy(padSecondsEnd);
  padSecondsEnd->setValue(cfg.GetPadSecondsEnd());
  padSecondsEnd->setMinimum(0);

  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  layout->addWidget(buttons, 6, 0, 1, 3);

  QObject::connect(loopInfinitely, SIGNAL(clicked()), this, SLOT(updateEnabled()));
  QObject::connect(buttons, SIGNAL(accepted()), this, SLOT(save()));
  QObject::connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
}

void PreferencesWindow::save()
{
  ConfigManager& cfg = ConfigManager::Instance();

  cfg.SetCgbPolyphony(CGBPolyphony(cgbPolyphony->currentData().toInt()));
  if (loopInfinitely->isChecked()) {
    cfg.SetMaxLoopsPlaylist(-1);
  } else {
    cfg.SetMaxLoopsPlaylist(maxLoopsPlaylist->value());
  }
  cfg.SetMaxLoopsExport(maxLoopsExport->value());
  cfg.SetPadSecondsStart(padSecondsStart->value());
  cfg.SetPadSecondsEnd(padSecondsEnd->value());

  cfg.Save();
  accept();
}

void PreferencesWindow::updateEnabled()
{
  maxLoopsPlaylist->setEnabled(!loopInfinitely->isChecked());
}

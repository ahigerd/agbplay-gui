#pragma once

#include <QDialog>
class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;

class PreferencesWindow : public QDialog
{
Q_OBJECT
public:
  PreferencesWindow(QWidget* parent = nullptr);

private slots:
  void updateEnabled();
  void save();

private:
  QComboBox* cgbPolyphony;
  QSpinBox* maxLoopsPlaylist;
  QCheckBox* loopInfinitely;
  QSpinBox* maxLoopsExport;
  QDoubleSpinBox* padSecondsStart;
  QDoubleSpinBox* padSecondsEnd;
};

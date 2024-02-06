#include "UiUtils.h"

QString signedNumber(int number)
{
  if (number <= 0) {
    return QString::number(number);
  } else {
    return "+" + QString::number(number);
  }
}

QString fixedNumber(int number, int digits)
{
  return QString::number(number).rightJustified(digits, '0');
}

QString formatAddress(std::uint32_t addr)
{
  return "0x" + QString::number(addr, 16).rightJustified(8, '0');
}

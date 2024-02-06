#pragma once

#include <QString>

QString signedNumber(int number);
QString fixedNumber(int number, int digits);
QString formatAddress(std::uint32_t addr);

template <typename T>
inline QString formatAddress(T addr)
{
  return formatAddress(std::uint32_t(addr));
}

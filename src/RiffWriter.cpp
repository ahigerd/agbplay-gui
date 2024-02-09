#include "RiffWriter.h"

template <typename T>
static void writeLE(std::ostream& stream, T data)
{
  char bytes[sizeof(T)];
  for (std::size_t i = 0; i < sizeof(T); i++) {
    bytes[i] = char(data & 0xFF);
    data = T(data >> 8);
  }
  stream.write(bytes, sizeof(T));
}

RiffWriter::RiffWriter(uint32_t sampleRate, bool stereo, uint32_t size)
: sampleRate(sampleRate), size(size), stereo(stereo), rewriteSize(!size)
{
  // initializers only
}

RiffWriter::~RiffWriter()
{
  close();
}

bool RiffWriter::open(const std::string& filename)
{
  file.open(filename, std::ios::out | std::ios::trunc | std::ios::binary);
  if (!file.is_open() || !file) {
    return false;
  }
  file.write("RIFF", 4);
  writeLE<uint32_t>(file, size ? size + 36 : 0xFFFFFFFF);
  file.write("WAVEfmt \x10\0\0\0\1\0", 14);
  writeLE<uint16_t>(file, stereo ? 2 : 1);
  writeLE<uint32_t>(file, sampleRate);
  writeLE<uint32_t>(file, sampleRate * 2 * (stereo ? 2 : 1));
  writeLE<uint16_t>(file, stereo ? 4 : 2);
  file.write("\x10\0data", 6);
  writeLE<uint32_t>(file, size ? size : 0xFFFFFFFF);
  return true;
}

void RiffWriter::write(const uint8_t* data, size_t length)
{
  if (rewriteSize) {
    size += std::uint32_t(length);
  }
  file.write(reinterpret_cast<const char*>(data), length);
}

void RiffWriter::write(const std::vector<int16_t>& data)
{
  std::size_t words = data.size();
  if (rewriteSize) {
    size += std::uint32_t(words * 2);
  }
  for (std::size_t i = 0; i < words; i++) {
    writeLE(file, data[i]);
  }
}

void RiffWriter::write(const std::vector<int16_t>& left, const std::vector<int16_t>& right)
{
  std::size_t leftWords = left.size(), rightWords = right.size();
  std::size_t words = leftWords < rightWords ? rightWords : leftWords;
  if (rewriteSize) {
    size += std::uint32_t(words * 4);
  }
  for (std::size_t i = 0; i < words; i++) {
    writeLE<int16_t>(file, i < leftWords ? left[i] : 0);
    writeLE<int16_t>(file, i < rightWords ? right[i] : 0);
  }
}

void RiffWriter::close()
{
  if (!file.is_open() || !file) {
    return;
  }
  if (rewriteSize) {
    file.seekp(4, std::ios::beg);
    if (!file.fail()) {
      writeLE(file, size + 36);
      file.seekp(32, std::ios::cur);
      writeLE(file, size);
    }
  }
  file.close();
}

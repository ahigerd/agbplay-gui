#pragma once

#include <fstream>
#include <string>
#include <vector>
#include <cstdint>

class RiffWriter
{
public:
  RiffWriter(uint32_t sampleRate, bool stereo, uint32_t sizeInBytes = 0);
  ~RiffWriter();

  bool open(const std::string& filename);
  void write(const uint8_t* data, size_t length);
  inline void write(const int8_t* data, size_t length)
    { write(reinterpret_cast<const uint8_t*>(data), length); }
  inline void write(const std::vector<int8_t>& data)
    { write(data.data(), data.size()); }
  inline void write(const std::vector<uint8_t>& data)
    { write(data.data(), data.size()); }
  void write(const std::vector<int16_t>& data);
  void write(const std::vector<int16_t>& left, const std::vector<int16_t>& right);
  void close();

private:
  std::ofstream file;
  uint32_t sampleRate, size;
  bool stereo, rewriteSize;
};

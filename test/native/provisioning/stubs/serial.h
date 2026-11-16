#pragma once
#include <deque>
#include <vector>
#include <Stream.h>
class TestSerial : public Stream {
public:
  std::deque<uint8_t> input;
  std::vector<uint8_t> output;
  int available() override { return int(input.size()); }
  int read() override {
    if (input.empty())
      return -1;
    auto b = input.front();
    input.pop_front();
    return b;
  }
  size_t write(const uint8_t *bytes, size_t n) override {
    output.insert(output.end(), bytes, bytes + n);
    return n;
  }
};

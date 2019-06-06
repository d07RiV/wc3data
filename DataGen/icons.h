#pragma once

#include "image/image.h"

class ImageStorage {
public:
  ImageStorage(int width, int height, int rows, int cols)
    : width_(width)
    , height_(height)
    , rows_(rows)
    , cols_(cols)
  {}
  ~ImageStorage() {
    flush();
  }

  void add(uint64 hash, Image image);

  void flush();

  void writeHashes(File file) {
    file.write(hashes_.data(), hashes_.size() * sizeof(uint64));
  }

private:
  int width_;
  int height_;
  int rows_;
  int cols_;
  uint32 id_ = 1000;
  Image current_;
  int x_, y_;
  std::vector<uint64> hashes_;
};

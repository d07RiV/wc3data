#include "icons.h"
#include "rmpq/common.h"
#include "hash.h"

void ImageStorage::add(uint64 hash, Image image) {
  hashes_.push_back(hash);
  image = image.resize(width_, height_);
  if (!current_) {
    current_ = Image(width_ * cols_, height_ * rows_);
    x_ = 0;
    y_ = 0;
  }
  current_.blt(x_ * width_, y_ * height_, image);
  x_ += 1;
  if (x_ >= cols_) {
    x_ = 0;
    y_ += 1;
  }
  if (y_ >= rows_) {
    flush();
  }
}

void ImageStorage::flush() {
  if (current_) {
    current_.write(fmtstring("icons%u.png", id_++));
    current_ = Image();
  }
}

#pragma once

#include <unordered_map>
#include "utils/file.h"

class SLKFile {
public:
  SLKFile(File file);

  bool valid() const
  {
    return height_ > 0;
  }

  size_t rows() const
  {
    return height_ - 1;
  }
  size_t cols() const
  {
    return width_;
  }
  char const* columnName(int i) const
  {
    return buffer_.data() + table_[i];
  }
  char const* item(int i, int j) const
  {
    return buffer_.data() + table_[(i + 1) * width_ + j];
  }
  bool has(int i, int j) const
  {
    return table_[(i + 1) * width_ + j] != 0;
  }
  int columnIndex(std::string const& name) const
  {
    auto it = cols_.find(name);
    return it == cols_.end() ? -1 : it->second;
  }

  void csv(File out) const;

private:
  std::unordered_map<std::string, int> cols_;
  std::string buffer_;
  std::vector<size_t> table_;
  size_t width_;
  size_t height_;
};

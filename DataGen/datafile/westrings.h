#pragma once

#include "utils/file.h"
#include <unordered_map>

class WEStrings {
public:
  void merge(File file);

  char const* get(std::string const& str) const {
    auto it = strings_.find(str);
    return it == strings_.end() ? nullptr : buffer_.data() + it->second;
  }

private:
  std::string buffer_;
  std::unordered_map<std::string, int> strings_;
};

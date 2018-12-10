#pragma once

#include <string>
#include <unordered_map>
#include "utils/types.h"
#include "utils/file.h"

class StringLib {
public:
  StringLib() {}
  StringLib(File file);
  StringLib(StringLib&&) = default;

  StringLib& operator=(StringLib&&) = default;

  void write(File file);

  char const* get(uint32 key);
  void add(uint32 key, char const* value);
  void add(uint32 key, std::string const& value);

private:
  std::string buffer_;
  std::unordered_map<uint32, uint32> index_;
};

#pragma once

#include "utils/types.h"
#include <string>

namespace __impl {
  inline constexpr uint32 chr(char c) {
    return uint8(c);
  }
}

inline constexpr uint32 idFromString(char const* str) {
  using namespace __impl;
  return str[0] ? (chr(str[0]) << 24 | chr(str[1]) << 16 | chr(str[2]) << 8 | chr(str[3])) : 0;
}

inline uint32 idFromString(std::string const& str) {
  return idFromString(str.c_str());
}

std::string idToString(uint32 id);

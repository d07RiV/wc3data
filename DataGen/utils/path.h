#pragma once

#include <string>

namespace path {
  std::string name(std::string const& path);
  std::string title(std::string const& path);
  std::string path(std::string const& path);
  std::string ext(std::string const& path);
#ifndef NO_SYSTEM
  std::string root();
#endif
#ifdef _MSC_VER
  static const char sep = '\\';
#else
  static const char sep = '/';
#endif
}

std::string operator / (std::string const& lhs, std::string const& rhs);

#include "path.h"
#include "file.h"
#include "common.h"
using namespace std;

string path::name(string const& path) {
  int pos = path.length();
  while (pos > 0 && path[pos - 1] != '/' && path[pos - 1] != '\\') {
    --pos;
  }
  return path.substr(pos);
}
string path::title(string const& path) {
  size_t pos = path.length();
  size_t dot = path.length();
  while (pos && path[pos - 1] != '/' && path[pos - 1] != '\\') {
    --pos;
    if (path[pos] == '.' && dot == path.length()) {
      dot = pos;
    }
  }
  if (dot == pos) {
    return path.substr(pos);
  } else {
    return path.substr(pos, dot - pos);
  }
}
string path::path(string const& path) {
  int pos = path.length();
  while (pos > 0 && path[pos - 1] != '/' && path[pos - 1] != '\\') {
    --pos;
  }
  return path.substr(0, pos ? pos - 1 : 0);
}
string path::ext(string const& path) {
  size_t pos = path.length();
  size_t dot = path.length();
  while (pos && path[pos - 1] != '/' && path[pos - 1] != '\\') {
    --pos;
    if (path[pos] == '.' && dot == path.length()) {
      dot = pos;
    }
  }
  if (dot == pos) {
    return "";
  } else {
    return path.substr(dot);
  }
}

#ifdef _MSC_VER
#include <windows.h>
#endif
#ifndef NO_SYSTEM
string path::root() {
  static string rp;
  if (rp.empty()) {
    char buffer[512];
#ifdef _MSC_VER
    GetModuleFileName(GetModuleHandle(NULL), buffer, sizeof buffer);
#else
    readlink("/proc/self/exe", buffer, sizeof buffer);
#endif
    rp = path(buffer);
#ifdef _DEBUG
    rp = "G:\\rivsoft\\wc3\\DataGen\\work";
#endif
    rp = "C:\\Projects\\wc3data\\DataGen\\work";
    SetCurrentDirectory(rp.c_str());
  }
  return rp;
}
#endif
string operator / (string const& lhs, string const& rhs) {
  if (lhs.empty() || rhs.empty()) return lhs + rhs;
  bool left = (lhs.back() == '\\' || lhs.back() == '/');
  bool right = (rhs.front() == '\\' || rhs.front() == '/');
  if (left && right) {
    string res = lhs;
    res.pop_back();
    return res + rhs;
  } else if (!left && !right) {
    return lhs + path::sep + rhs;
  } else {
    return lhs + rhs;
  }
}

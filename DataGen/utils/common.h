#pragma once

#include "types.h"
#include <string>
#include <cctype>
#include <vector>
#include <map>
#include <atomic>
#include <string.h>

class Exception {
public:
  Exception(char const* fmt, ...);
  Exception(Exception const& e)
    : str_(e.str_)
  {}

  virtual char const* what() const throw() {
    return str_.c_str();
  }
private:
  std::string str_;
};

std::string fmtstring(char const* fmt, ...);
std::string varfmtstring(char const* fmt, va_list list);

void _qmemset(uint32* mem, uint32 fill, uint32 count);

#ifdef Z_SOLO
void* gzalloc(void* opaque, unsigned int items, unsigned int size);
void gzfree(void* opaque, void* address);
#else
extern void* (*gzalloc)(void* opaque, unsigned int items, unsigned int size);
extern void (*gzfree)(void* opaque, void* address);
#endif

uint32 gzdeflate(uint8 const* in, uint32 in_size, uint8* out, uint32* out_size);
uint32 gzencode(uint8 const* in, uint32 in_size, uint8* out, uint32* out_size);
uint32 gzinflate(uint8 const* in, uint32 in_size, uint8* out, uint32* out_size);
uint32 gzdecode(uint8 const* in, uint32 in_size, uint8* out, uint32* out_size);

struct ci_char_traits : public std::char_traits < char > {
  static bool eq(char c1, char c2) { return std::toupper(c1) == std::toupper(c2); }
  static bool ne(char c1, char c2) { return std::toupper(c1) != std::toupper(c2); }
  static bool lt(char c1, char c2) { return std::toupper(c1) < std::toupper(c2); }
  static int compare(char const* s1, char const* s2, size_t n) {
    while (n--) {
      char c1 = std::toupper(*s1++);
      char c2 = std::toupper(*s2++);
      if (c1 != c2) return (c1 < c2 ? -1 : 1);
    }
    return 0;
  }
  static char const* find(char const* s, size_t n, char a) {
    a = std::toupper(a);
    while (n--) {
      if (std::toupper(*s) == a) {
        return s;
      }
      ++s;
    }
    return nullptr;
  }
};
class istring : public std::basic_string<char, ci_char_traits> {
public:
  typedef std::basic_string<char, ci_char_traits> _Base;
  istring() {}
  istring(istring const& str) : _Base(str) {}
  istring(std::string const& str) : _Base(str.c_str()) {}
  istring(char const* str) : _Base(str) {}
  istring(istring&& str) : _Base(str) {}
  istring(char const* str, size_t n) : _Base(str) {}
  template<class Iter>
  istring(Iter begin, Iter end) : _Base(begin, end) {}

  istring& operator=(istring const& str) {
    assign(str.c_str());
    return *this;
  }
  istring& operator=(std::string const& str) {
    assign(str.c_str());
    return *this;
  }
  operator std::string() const {
    return std::string(c_str());
  }
};
template<class To>
using Map = std::map<istring, To>;
typedef Map<std::string> Dictionary;

std::string strlower(std::string const& src);

template<class T>
inline int basic_compare(T const& lhs, T const& rhs) {
  if (lhs < rhs) return -1;
  if (lhs > rhs) return 1;
  return 0;
}

std::vector<std::string> split(std::string const& str, char sep = ' ');
std::vector<std::wstring> split(std::wstring const& str, wchar_t sep = ' ');
std::vector<std::string> split_multiple(std::string const& str, char const* sep);
std::string join(std::vector<std::string> const& list, char sep = ' ');
std::string join(std::vector<std::string> const& list, std::string const& sep);
template<class Iter>
inline std::string join(Iter left, Iter right, char const* sep = " ") {
  std::string res;
  while (left != right) {
    if (!res.empty()) res.append(sep);
    res.append(*left++);
  }
  return res;
}
template<class Iter>
inline std::wstring wjoin(Iter left, Iter right, wchar_t const* sep = L" ") {
  std::wstring res;
  while (left != right) {
    if (!res.empty()) res.append(sep);
    res.append(*left++);
  }
  return res;
}

std::wstring utf8_to_utf16(std::string const& str);
std::string utf16_to_utf8(std::wstring const& str);
std::string trim(std::string const& str);

template<int TS>
struct FlipTraits {};

template<> struct FlipTraits<1> {
  typedef unsigned char T;
  static T flip(T x) { return x; }
};

#ifdef _MSC_VER
#include <intrin.h>
template<> struct FlipTraits<2> {
  typedef unsigned short T;
  static T flip(T x) { return _byteswap_ushort(x); }
};
template<> struct FlipTraits<4> {
  typedef unsigned long T;
  static T flip(T x) { return _byteswap_ulong(x); }
};
template<> struct FlipTraits<8> {
  typedef unsigned long long T;
  static T flip(T x) { return _byteswap_uint64(x); }
};
#else
template<> struct FlipTraits<2> {
  typedef unsigned short T;
  static T flip(T x) { return __builtin_bswap16(x); }
};
template<> struct FlipTraits<4> {
  typedef unsigned long T;
  static T flip(T x) { return __builtin_bswap32(x); }
};
template<> struct FlipTraits<8> {
  typedef unsigned long long T;
  static T flip(T x) { return __builtin_bswap64(x); }
};
#endif

template<typename T>
void flip(T& x) {
  typedef FlipTraits<sizeof(T)> Flip;
  x = static_cast<T>(Flip::flip(static_cast<typename Flip::T>(x)));
}
template<typename T>
T flipped(T x) {
  typedef FlipTraits<sizeof(T)> Flip;
  return static_cast<T>(Flip::flip(static_cast<typename Flip::T>(x)));
}

#ifndef NO_SYSTEM

size_t file_size(char const* path);
void delete_file(char const* path);
void create_dir(char const* path);
void rename_file(char const* src, char const* dst);

#ifndef _MSC_VER
uint32 GetTickCount();
#endif

#endif

template<class map>
class key_iterator {
public:
  key_iterator() {}
  key_iterator(key_iterator const&) = default;
  key_iterator(key_iterator&&) = default;
  key_iterator& operator=(key_iterator const&) = default;
  key_iterator& operator=(key_iterator&&) = default;

  key_iterator(typename map::iterator it)
    : it_(it)
  {}
  key_iterator(typename map::const_iterator it)
    : it_(it)
  {}

  key_iterator& operator++() {
    ++it_;
    return *this;
  }
  key_iterator operator++(int) {
    return key_iterator(it_++);
  }

  const typename map::key_type& operator*() const {
    return it_->first;
  }

  bool operator==(key_iterator const& it) const {
    return it_ == it.it_;
  }
  bool operator!=(key_iterator const& it) const {
    return it_ != it.it_;
  }

private:
  typename map::const_iterator it_;
};

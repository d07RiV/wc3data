#include <stdarg.h>
#include <clocale>
#include <algorithm>
#include "common.h"

#ifndef NO_SYSTEM
#include <sys/stat.h>
#ifdef _MSC_VER
#include <windows.h>
#else
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#endif
#endif

std::string fmtstring(char const* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  std::string res = varfmtstring(fmt, ap);
  va_end(ap);
  return res;
}
std::string varfmtstring(char const* fmt, va_list list) {
  va_list va;
  va_copy(va, list);
  uint32 len = vsnprintf(nullptr, 0, fmt, va);
  va_end(va);
  std::string dst;
  dst.resize(len + 1);
  vsnprintf(&dst[0], len + 1, fmt, list);
  dst.resize(len);
  return dst;
}

Exception::Exception(char const* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  str_ = varfmtstring(fmt, ap);
  va_end(ap);
}

void _qmemset(uint32* mem, uint32 fill, uint32 count) {
  while (count--) {
    *mem++ = fill;
  }
}

#include "zlib/zlib.h"

/*
#ifdef _MSC_VER
#include "zlib/zlib.h"
#ifdef _WIN64
  #ifdef _DEBUG
    #pragma comment(lib, "zlib/zlib64d.lib")
  #else
    #pragma comment(lib, "zlib/zlib64r.lib")
  #endif
#else
  #ifdef _DEBUG
    #pragma comment(lib, "zlib/zlib32d.lib")
  #else
    #pragma comment(lib, "zlib/zlib32r.lib")
  #endif
#endif
#else
#include <zlib.h>
#endif
*/

#ifdef Z_SOLO
void* gzalloc(void* opaque, unsigned int items, unsigned int size) {
  return malloc(items * size);
}
void gzfree(void* opaque, void* address) {
  free(address);
}
#else
void* (*gzalloc)(void* opaque, unsigned int items, unsigned int size) = nullptr;
void(*gzfree)(void* opaque, void* address) = nullptr;
#endif

uint32 gzdeflate(uint8 const* in, uint32 in_size, uint8* out, uint32* out_size) {
  z_stream z;
  memset(&z, 0, sizeof z);
  z.next_in = const_cast<Bytef*>(in);
  z.avail_in = in_size;
  z.total_in = in_size;
  z.next_out = out;
  z.avail_out = *out_size;
  z.total_out = 0;
  z.zalloc = gzalloc;
  z.zfree = gzfree;

  memset(out, 0, *out_size);

  int result = deflateInit(&z, Z_DEFAULT_COMPRESSION);
  if (result == Z_OK) {
    result = deflate(&z, Z_FINISH);
    *out_size = z.total_out;
    deflateEnd(&z);
  }
  return (result == Z_STREAM_END ? 0 : -1);
}
uint32 gzencode(uint8 const* in, uint32 in_size, uint8* out, uint32* out_size) {
  z_stream z;
  memset(&z, 0, sizeof z);
  z.next_in = const_cast<Bytef*>(in);
  z.avail_in = in_size;
  z.total_in = in_size;
  z.next_out = out;
  z.avail_out = *out_size;
  z.total_out = 0;
  z.zalloc = gzalloc;
  z.zfree = gzfree;

  int result = deflateInit2(&z, 6, Z_DEFLATED, 16 + MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
  if (result == Z_OK) {
    result = deflate(&z, Z_FINISH);
    *out_size = z.total_out;
    deflateEnd(&z);
  }
  return ((result == Z_OK || result == Z_STREAM_END) ? 0 : 1);
}
uint32 gzinflate(uint8 const* in, uint32 in_size, uint8* out, uint32* out_size) {
  z_stream z;
  memset(&z, 0, sizeof z);
  z.next_in = const_cast<Bytef*>(in);
  z.avail_in = in_size;
  z.total_in = in_size;
  z.next_out = out;
  z.avail_out = *out_size;
  z.total_out = 0;
  z.zalloc = gzalloc;
  z.zfree = gzfree;

  memset(out, 0, *out_size);

  int result = inflateInit(&z);
  if (result == Z_OK) {
    result = inflate(&z, Z_FINISH);
    *out_size = z.total_out;
    inflateEnd(&z);
  }
  return (z.avail_out == 0 ? 0 : -1);
}
uint32 gzdecode(uint8 const* in, uint32 in_size, uint8* out, uint32* out_size) {
  z_stream z;
  memset(&z, 0, sizeof z);
  z.next_in = const_cast<Bytef*>(in);
  z.avail_in = in_size;
  z.total_in = in_size;
  z.next_out = out;
  z.avail_out = *out_size;
  z.total_out = 0;
  z.zalloc = gzalloc;
  z.zfree = gzfree;

  int result = inflateInit2(&z, 16 + MAX_WBITS);
  if (result == Z_OK) {
    result = inflate(&z, Z_FINISH);
    *out_size = z.total_out;
    deflateEnd(&z);
  }
  return (z.avail_out == 0 ? 0 : 1);
}

std::string strlower(std::string const& str) {
  std::string dest(str.size(), ' ');
  std::transform(str.begin(), str.end(), dest.begin(), tolower);
  return dest;
}

std::vector<std::string> split(std::string const& str, char sep) {
  std::vector<std::string> res;
  std::string cur;
  for (char c : str) {
    if (c == sep) {
      res.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  res.push_back(cur);
  return res;
}
std::vector<std::wstring> split(std::wstring const& str, wchar_t sep) {
  std::vector<std::wstring> res;
  std::wstring cur;
  for (wchar_t c : str) {
    if (c == sep) {
      res.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  res.push_back(cur);
  return res;
}
std::vector<std::string> split_multiple(std::string const& str, char const* sep) {
  std::vector<std::string> res;
  std::string cur;
  for (char c : str) {
    if (strchr(sep, c)) {
      res.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  res.push_back(cur);
  return res;
}

std::string join(std::vector<std::string> const& list, char sep) {
  std::string res;
  for (auto& str : list) {
    if (!res.empty()) res.push_back(sep);
    res.append(str);
  }
  return res;
}
std::string join(std::vector<std::string> const& list, std::string const& sep) {
  std::string res;
  for (auto& str : list) {
    if (!res.empty()) res.append(sep);
    res.append(str);
  }
  return res;
}

std::wstring utf8_to_utf16(std::string const& str) {
  std::wstring dst;
  for (size_t i = 0; i < str.size();) {
    uint32 cp = (unsigned char)str[i++];
    size_t next = 0;
    if (cp <= 0x7F) {
      // do nothing
    } else if (cp <= 0xBF) {
      throw Exception("not a valid utf-8 string");
    } else if (cp <= 0xDF) {
      cp &= 0x1F;
      next = 1;
    } else if (cp <= 0xEF) {
      cp &= 0x0F;
      next = 2;
    } else if (cp <= 0xF7) {
      cp &= 0x07;
      next = 3;
    } else {
      throw Exception("not a valid utf-8 string");
    }
    while (next--) {
      if (i >= str.size() || (unsigned char)str[i] < 0x80 || (unsigned char)str[i] > 0xBF) {
        throw Exception("not a valid utf-8 string");
      }
      cp = (cp << 6) | (str[i++] & 0x3F);
    }
    if ((cp >= 0xD800 && cp <= 0xDFFF) || cp > 0x10FFFF) {
      throw Exception("not a valid utf-8 string");
    }

    if (cp <= 0xFFFF) {
      dst.push_back(cp);
    } else {
      cp -= 0x10000;
      dst.push_back((cp >> 10) + 0xD800);
      dst.push_back((cp & 0x3FF) + 0xDC00);
    }
  }
  return dst;
}

std::string utf16_to_utf8(std::wstring const& str) {
  std::string dst;
  for (size_t i = 0; i < str.size();) {
    uint32 cp = str[i++];
    if (cp >= 0xD800 && cp <= 0xDFFF) {
      if (cp >= 0xDC00) throw Exception("not a valid utf-16 string");
      if (i >= str.size() || str[i] < 0xDC00 || str[i] > 0xDFFF) throw Exception("not a valid utf-16 string");
      cp = 0x10000 + ((cp - 0xD800) << 10) + (str[i++] - 0xDC00);
    }
    if (cp >= 0x10FFFF) throw Exception("not a valid utf-16 string");
    if (cp <= 0x7F) {
      dst.push_back(cp);
    } else if (cp <= 0x7FF) {
      dst.push_back((cp >> 6) | 0xC0);
      dst.push_back((cp & 0x3F) | 0x80);
    } else if (cp <= 0xFFFF) {
      dst.push_back((cp >> 12) | 0xE0);
      dst.push_back(((cp >> 6) & 0x3F) | 0x80);
      dst.push_back((cp & 0x3F) | 0x80);
    } else {
      dst.push_back((cp >> 18) | 0xF0);
      dst.push_back(((cp >> 12) & 0x3F) | 0x80);
      dst.push_back(((cp >> 6) & 0x3F) | 0x80);
      dst.push_back((cp & 0x3F) | 0x80);
    }
  }
  return dst;
}

std::string trim(std::string const& str) {
  size_t left = 0, right = str.size();
  while (left < str.length() && isspace((unsigned char)str[left])) ++left;
  while (right > left && isspace((unsigned char)str[right - 1])) --right;
  return str.substr(left, right - left);
}

#ifndef NO_SYSTEM
size_t file_size(char const* path) {
  struct stat st;
  if (stat(path, &st)) {
    return 0;
  }
  return st.st_size;
}

void delete_file(char const* path) {
  remove(path);
}

void create_dir(char const* path) {
#ifdef _MSC_VER
  CreateDirectory(path, NULL);
#else
  struct stat st;
  if (stat(path, &st)) {
    mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  }
#endif
}

#ifndef _MSC_VER
#include <time.h>
static uint64 msec_since_epoch() {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return 1000ULL * ts.tv_sec + ts.tv_nsec / 1000000;
}
uint32 GetTickCount() {
  static uint64 initial = msec_since_epoch();
  return msec_since_epoch() - initial;
}
#endif

void rename_file(char const* src, char const* dst) {
#ifdef _MSC_VER
  MoveFile(src, dst);
#else
  rename(src, dst);
#endif
}

#endif

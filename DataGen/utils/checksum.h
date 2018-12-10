#pragma once

#include "types.h"
#include <string>

uint32 update_crc(uint32 crc, void const* vbuf, uint32 length);
uint32 crc32(void const* buf, uint32 length);
uint32 crc32(std::string const& str);

class MD5 {
  uint8 buffer[64];
  uint32 digest[4];
  uint64 length;
  uint32 bufSize;
  void run();
public:
  enum { DIGEST_SIZE = 16 };

  MD5();
  void process(void const* buf, uint32 size);
  void finish(void* digest);

  static std::string format(void const* digest);

  static void checksum(void const* buf, uint32 size, void* digest) {
    MD5 md5;
    md5.process(buf, size);
    md5.finish(digest);
  }
};

uint64 jenkins(void const* buf, uint32 length);
uint32 hashlittle(void const* buf, uint32 length, uint32 initval);

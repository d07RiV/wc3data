#include "checksum.h"
#include <string.h>

uint32 update_crc(uint32 crc, void const* vbuf, uint32 length) {
  static uint32 crc_table[256];
  static bool table_computed = false;
  uint8 const* buf = (uint8*)vbuf;
  if (!table_computed) {
    for (uint32 i = 0; i < 256; i++) {
      uint32 c = i;
      for (int k = 0; k < 8; k++) {
        if (c & 1) {
          c = 0xEDB88320L ^ (c >> 1);
        } else {
          c = c >> 1;
        }
      }
      crc_table[i] = c;
    }
    table_computed = true;
  }
  for (uint32 i = 0; i < length; i++) {
    crc = crc_table[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
  }
  return crc;
}
uint32 crc32(void const* buf, uint32 length) {
  return ~update_crc(0xFFFFFFFF, buf, length);
}
uint32 crc32(std::string const& str) {
  return ~update_crc(0xFFFFFFFF, str.c_str(), str.length());
}

//////////////////////////////////////////////////////////////////

static const uint32 MD5_R[64] = {
  7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
  5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
  4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
  6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
};
static uint32 MD5_K[64] = {
  0xD76AA478, 0xE8C7B756, 0x242070DB, 0xC1BDCEEE,
  0xF57C0FAF, 0x4787C62A, 0xA8304613, 0xFD469501,
  0x698098D8, 0x8B44F7AF, 0xFFFF5BB1, 0x895CD7BE,
  0x6B901122, 0xFD987193, 0xA679438E, 0x49B40821,
  0xF61E2562, 0xC040B340, 0x265E5A51, 0xE9B6C7AA,
  0xD62F105D, 0x02441453, 0xD8A1E681, 0xE7D3FBC8,
  0x21E1CDE6, 0xC33707D6, 0xF4D50D87, 0x455A14ED,
  0xA9E3E905, 0xFCEFA3F8, 0x676F02D9, 0x8D2A4C8A,
  0xFFFA3942, 0x8771F681, 0x6D9D6122, 0xFDE5380C,
  0xA4BEEA44, 0x4BDECFA9, 0xF6BB4B60, 0xBEBFBC70,
  0x289B7EC6, 0xEAA127FA, 0xD4EF3085, 0x04881D05,
  0xD9D4D039, 0xE6DB99E5, 0x1FA27CF8, 0xC4AC5665,
  0xF4292244, 0x432AFF97, 0xAB9423A7, 0xFC93A039,
  0x655B59C3, 0x8F0CCC92, 0xFFEFF47D, 0x85845DD1,
  0x6FA87E4F, 0xFE2CE6E0, 0xA3014314, 0x4E0811A1,
  0xF7537E82, 0xBD3AF235, 0x2AD7D2BB, 0xEB86D391,
};

MD5::MD5() {
  digest[0] = 0x67452301;
  digest[1] = 0xEFCDAB89;
  digest[2] = 0x98BADCFE;
  digest[3] = 0x10325476;
  length = 0;
  bufSize = 0;
}
void MD5::process(void const* _buf, uint32 size) {
  uint8 const* buf = (uint8*)_buf;
  while (size) {
    uint32 cur = 64 - bufSize;
    if (cur > size) cur = size;
    memcpy(buffer + bufSize, buf, cur);
    bufSize += cur;
    length += cur;
    size -= cur;
    buf += cur;
    if (bufSize == 64) run();
  }
}
void MD5::finish(void* _digest) {
  buffer[bufSize++] = 0x80;
  if (bufSize > 56) {
    if (bufSize < 64) memset(buffer + bufSize, 0, 64 - bufSize);
    run();
  }
  if (bufSize < 64) memset(buffer + bufSize, 0, 64 - bufSize);
  uint64 bits = length * 8;
  memcpy(buffer + 56, &bits, sizeof length);
  run();
  memcpy(_digest, digest, sizeof digest);
}
std::string MD5::format(void const* digest)
{
  std::string result(32, ' ');
  uint8 const* d = (uint8*)digest;
  for (int i = 0; i < DIGEST_SIZE; i++) {
    sprintf(&result[i * 2], "%02x", d[i]);
  }
  return result;
}
static inline uint32 rot(uint32 x, int k) {
  return (x << k) | (x >> (32 - k));
}
void MD5::run() {
  uint32* m = (uint32*)buffer;
  uint32 a = digest[0];
  uint32 b = digest[1];
  uint32 c = digest[2];
  uint32 d = digest[3];
  for (int i = 0; i < 64; i++) {
    uint32 f, g;
    if (i < 16) f = (b & c) | ((~b) & d), g = i;
    else if (i < 32) f = (d & b) | ((~d) & c), g = (i * 5 + 1) & 0x0F;
    else if (i < 48) f = b ^ c ^ d, g = (i * 3 + 5) & 0x0F;
    else f = c ^ (b | (~d)), g = (i * 7) & 0x0F;
    uint32 temp = d;
    d = c;
    c = b;
    b += rot(a + f + MD5_K[i] + m[g], MD5_R[i]);
    a = temp;
  }
  digest[0] += a;
  digest[1] += b;
  digest[2] += c;
  digest[3] += d;
  bufSize = 0;
}

uint64 jenkins(void const* buf, uint32 length) {
  uint32 a, b, c;
  a = b = c = 0xDEADBEEF + length + 2;
  c += 1;

  uint32* k = (uint32*)buf;
  while (length > 12) {
    a += k[0];
    b += k[1];
    c += k[2];

    a -= c; a ^= rot(c, 4); c += b;
    b -= a; b ^= rot(a, 6); a += c;
    c -= b; c ^= rot(b, 8); b += a;
    a -= c; a ^= rot(c, 16); c += b;
    b -= a; b ^= rot(a, 19); a += c;
    c -= b; c ^= rot(b, 4); b += a;

    length -= 12;
    k += 3;
  }

  switch (length) {
  case 12: c += k[2]; b += k[1]; a += k[0]; break;
  case 11: c += k[2] & 0xFFFFFF; b += k[1]; a += k[0]; break;
  case 10: c += k[2] & 0xFFFF; b += k[1]; a += k[0]; break;
  case  9: c += k[2] & 0xFF; b += k[1]; a += k[0]; break;
  case  8: b += k[1]; a += k[0]; break;
  case  7: b += k[1] & 0xFFFFFF; a += k[0]; break;
  case  6: b += k[1] & 0xFFFF; a += k[0]; break;
  case  5: b += k[1] & 0xFF; a += k[0]; break;
  case  4: a += k[0]; break;
  case  3: a += k[0] & 0xFFFFFF; break;
  case  2: a += k[0] & 0xFFFF; break;
  case  1: a += k[0] & 0xFF; break;
  case  0: return (uint64(b) << 32) | uint64(c);
  }

  c ^= b; c -= rot(b, 14);
  a ^= c; a -= rot(c, 11);
  b ^= a; b -= rot(a, 25);
  c ^= b; c -= rot(b, 16);
  a ^= c; a -= rot(c, 4);
  b ^= a; b -= rot(a, 14);
  c ^= b; c -= rot(b, 24);

  return (uint64(b) << 32) | uint64(c);
}
uint32 hashlittle(void const* buf, uint32 length, uint32 initval) {
  uint32 a, b, c;
  a = b = c = 0xDEADBEEF + length + initval;

  uint32* k = (uint32*)buf;
  while (length > 12) {
    a += k[0];
    b += k[1];
    c += k[2];

    a -= c; a ^= rot(c, 4); c += b;
    b -= a; b ^= rot(a, 6); a += c;
    c -= b; c ^= rot(b, 8); b += a;
    a -= c; a ^= rot(c, 16); c += b;
    b -= a; b ^= rot(a, 19); a += c;
    c -= b; c ^= rot(b, 4); b += a;

    length -= 12;
    k += 3;
  }

  switch (length) {
  case 12: c += k[2]; b += k[1]; a += k[0]; break;
  case 11: c += k[2] & 0xFFFFFF; b += k[1]; a += k[0]; break;
  case 10: c += k[2] & 0xFFFF; b += k[1]; a += k[0]; break;
  case  9: c += k[2] & 0xFF; b += k[1]; a += k[0]; break;
  case  8: b += k[1]; a += k[0]; break;
  case  7: b += k[1] & 0xFFFFFF; a += k[0]; break;
  case  6: b += k[1] & 0xFFFF; a += k[0]; break;
  case  5: b += k[1] & 0xFF; a += k[0]; break;
  case  4: a += k[0]; break;
  case  3: a += k[0] & 0xFFFFFF; break;
  case  2: a += k[0] & 0xFFFF; break;
  case  1: a += k[0] & 0xFF; break;
  case  0: return c;
  }

  c ^= b; c -= rot(b, 14);
  a ^= c; a -= rot(c, 11);
  b ^= a; b -= rot(a, 25);
  c ^= b; c -= rot(b, 16);
  a ^= c; a -= rot(c, 4);
  b ^= a; b -= rot(a, 14);
  c ^= b; c -= rot(b, 24);

  return c;
}

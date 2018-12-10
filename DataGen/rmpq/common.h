#pragma once

#include "utils/types.h"
#include "utils/file.h"
#include "utils/checksum.h"

namespace mpq {

inline uint64 offset64(uint32 offset, uint16 offsetHi) {
  return uint64(offset) | (uint64(offsetHi) << 32);
}

inline uint64 hashTo64(uint32 name1, uint32 name2) {
  return uint64(name1) | (uint64(name2) << 32);
}

enum {
  HASH_OFFSET = 0,
  HASH_NAME1 = 1,
  HASH_NAME2 = 2,
  HASH_KEY = 3,
  HASH_ENCRYPT = 4,
  HASH_SIZE = 1280
};

#pragma pack(push, 1)
struct MPQUserData {
  enum {
    signature = 0x1B51504D, // MPQ\x1B
  };
  uint32 id;
  uint32 userDataSize;
  uint32 headerOffs;
  uint32 userDataHeader;
};
struct MPQHeader {
  enum {
    signature = 0x1A51504D, // MPQ\x1A
    size_v1 = 32,
    size_v2 = 44,
    size_v3 = 68,
    size_v4 = 208,
  };

  uint32 id;
  uint32 headerSize;
  uint32 archiveSize;
  uint16 formatVersion;
  uint16 blockSize;

  uint32 hashTablePos;
  uint32 blockTablePos;
  uint32 hashTableSize;
  uint32 blockTableSize;

  // v2
  uint64 hiBlockTablePos64;
  uint16 hashTablePosHi;
  uint16 blockTablePosHi;

  // v3
  uint64 archiveSize64;
  uint64 betTablePos64;
  uint64 hetTablePos64;

  // v4
  uint64 hashTableSize64;
  uint64 blockTableSize64;
  uint64 hiBlockTableSize64;
  uint64 hetTableSize64;
  uint64 betTableSize64;

  uint32 rawChunkSize;
  uint8 md5BlockTable[MD5::DIGEST_SIZE];
  uint8 md5HashTable[MD5::DIGEST_SIZE];
  uint8 md5HiBlockTable[MD5::DIGEST_SIZE];
  uint8 md5BetTable[MD5::DIGEST_SIZE];
  uint8 md5HetTable[MD5::DIGEST_SIZE];
  uint8 md5MPQHeader[MD5::DIGEST_SIZE];

  void upgrade(size_t fileSize);
};

struct MPQHashEntry {
  enum {
    EMPTY = 0xFFFFFFFF,
    DELETED = 0xFFFFFFFE,
  };
  uint32 name1;
  uint32 name2;
  uint16 locale;
  uint16 platform;
  uint32 blockIndex;
};

struct MPQBlockEntry {
  uint32 filePos;
  uint32 cSize;
  uint32 fSize;
  uint32 flags;
};
#pragma pack(pop)

uint32 hashString(char const* str, uint32 hashType);

inline uint64 hashString64(char const* str) {
  return hashTo64(hashString(str, HASH_NAME1), hashString(str, HASH_NAME2));
}

void encryptBlock(void* ptr, uint32 size, uint32 key);
void decryptBlock(void* ptr, uint32 size, uint32 key);

uint32 detectTableSeed(uint32* blocks, uint32 offset, uint32 maxSize);
uint32 detectFileSeed(uint32* data, uint32 size);

bool validateHash(void const* buf, uint32 size, void const* expected);
bool validateHash(File file, void const* expected);

#define ID_WAVE         0x46464952
bool pkzip_compress(void* in, size_t in_size, void* out, size_t* out_size);
bool pkzip_decompress(void* in, size_t in_size, void* out, size_t* out_size);
//bool multi_compress(void* in, size_t in_size, void* out, size_t* out_size, uint8 mask, void* temp);
bool multi_decompress(void* in, size_t in_size, void* out, size_t* out_size, void* temp = nullptr);

}

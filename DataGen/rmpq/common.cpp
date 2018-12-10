#include "common.h"
#include "utils/checksum.h"
#include <vector>

namespace mpq {

namespace {

uint32* cryptTable() {
  static uint32 table[HASH_SIZE];
  static bool initialized = false;
  if (!initialized) {
    uint32 seed = 0x00100001;
    for (int i = 0; i < 256; i++) {
      for (int j = i; j < HASH_SIZE; j += 256) {
        seed = (seed * 125 + 3) % 0x2AAAAB;
        uint32 a = (seed & 0xFFFF) << 16;
        seed = (seed * 125 + 3) % 0x2AAAAB;
        uint32 b = (seed & 0xFFFF);
        table[j] = a | b;
      }
    }
    initialized = true;
  }
  return table;
}

}

uint32 hashString(char const* str, uint32 hashType) {
  uint32 seed1 = 0x7FED7FED;
  uint32 seed2 = 0xEEEEEEEE;
  uint32* table = cryptTable();
  for (int i = 0; str[i]; i++) {
    unsigned char ch = str[i];
    if (ch >= 'a' && ch <= 'z') {
      ch = ch - 'a' + 'A';
    }
    if (ch == '/') {
      ch = '\\';
    }
    seed1 = table[hashType * 256 + ch] ^ (seed1 + seed2);
    seed2 = ch + seed1 + seed2 * 33 + 3;
  }
  return seed1;
}

void encryptBlock(void* ptr, uint32 size, uint32 key) {
  uint32 seed = 0xEEEEEEEE;
  uint32* lptr = (uint32*)ptr;
  size /= sizeof(uint32);
  uint32* table = cryptTable();
  for (uint32 i = 0; i < size; i++) {
    seed += table[HASH_ENCRYPT * 256 + (key & 0xFF)];
    uint32 orig = lptr[i];
    lptr[i] ^= key + seed;
    key = ((~key << 21) + 0x11111111) | (key >> 11);
    seed += orig + seed * 32 + 3;
  }
}

void decryptBlock(void* ptr, uint32 size, uint32 key) {
  uint32 seed = 0xEEEEEEEE;
  uint32* lptr = (uint32*)ptr;
  size /= sizeof(uint32);
  uint32* table = cryptTable();
  for (uint32 i = 0; i < size; i++) {
    seed += table[HASH_ENCRYPT * 256 + (key & 0xFF)];
    lptr[i] ^= key + seed;
    key = ((~key << 21) + 0x11111111) | (key >> 11);
    seed += lptr[i] + seed * 32 + 3;
  }
}

uint32 detectTableSeed(uint32* blocks, uint32 offset, uint32 maxSize) {
  uint32 temp = (blocks[0] ^ offset) - 0xEEEEEEEE;
  uint32* table = cryptTable();
  for (uint32 i = 0; i < 256; i++)
  {
    uint32 key = temp - table[HASH_ENCRYPT * 256 + i];
    if ((key & 0xFF) != i)
      continue;

    uint32 seed = 0xEEEEEEEE + table[HASH_ENCRYPT * 256 + (key & 0xFF)];
    if ((blocks[0] ^ (key + seed)) != offset)
      continue; // sanity check

    uint32 saved = key + 1;
    key = ((~key << 21) + 0x11111111) | (key >> 11);
    seed += offset + seed * 32 + 3;

    seed += table[HASH_ENCRYPT * 256 + (key & 0xFF)];
    if ((blocks[1] ^ (key + seed)) <= offset + maxSize)
      return saved;
  }
  return 0;
}

uint32 detectFileSeed(uint32* data, uint32 size) {
  uint32 fileTable[3][3] = {
    { 0x46464952 /*RIFF*/, size - 8, 0x45564157 /*WAVE*/ },
    { 0x00905A4D, 0x00000003 },
    { 0x34E1F3B9, 0xD5B0DBFA },
  };
  uint32* table = cryptTable();
  uint32 tSize[3] = { 3, 2, 2 };
  for (uint32 set = 0; set < 3; set++) {
    uint32 temp = (data[0] ^ fileTable[set][0]) - 0xEEEEEEEE;
    for (uint32 i = 0; i < 256; i++) {
      uint32 key = temp - table[HASH_ENCRYPT * 256 + i];
      if ((key & 0xFF) != i)
        continue;
      uint32 seed = 0xEEEEEEEE + table[HASH_ENCRYPT * 256 + (key & 0xFF)];
      if ((data[0] ^ (key + seed)) != fileTable[set][0])
        continue;

      uint32 saved = key;
      for (uint32 j = 1; j < tSize[set]; j++) {
        key = ((~key << 21) + 0x11111111) | (key >> 11);
        seed += fileTable[set][j - 1] + seed * 32 + 3;
        seed += table[HASH_ENCRYPT * 256 + (key & 0xFF)];
        if ((data[j] ^ (key + seed)) != fileTable[set][j])
          break;
        if (j == tSize[set] - 1)
          return saved;
      }
    }
  }
  return 0;
}

bool validateHash(void const* buf, uint32 size, void const* expected) {
  uint32 sum = ((uint32*)expected)[0] | ((uint32*)expected)[1] | ((uint32*)expected)[2] | ((uint32*)expected)[3];
  if (sum == 0) {
    return true;
  }
  uint8 digest[MD5::DIGEST_SIZE];
  MD5::checksum(buf, size, digest);
  return memcmp(digest, expected, MD5::DIGEST_SIZE) == 0;
}

bool validateHash(File file, void const* expected) {
  uint32 sum = ((uint32*)expected)[0] | ((uint32*)expected)[1] | ((uint32*)expected)[2] | ((uint32*)expected)[3];
  if (sum == 0) {
    return true;
  }
  uint8 digest[MD5::DIGEST_SIZE];
  file.md5(digest);
  return memcmp(digest, expected, MD5::DIGEST_SIZE) == 0;
}

void MPQHeader::upgrade(size_t fileSize) {
  if (formatVersion <= 1) {
    formatVersion = 1;
    headerSize = size_v1;
    archiveSize = fileSize;
    hiBlockTablePos64 = 0;
    hashTablePosHi = 0;
    blockTablePosHi = 0;
  }
  if (formatVersion <= 3) {
    if (headerSize < size_v3) {
      archiveSize64 = archiveSize;
      if (fileSize > std::numeric_limits<uint32>::max()) {
        uint64 pos = offset64(hashTablePos, hashTablePosHi) + hashTableSize * sizeof(MPQHashEntry);
        if (pos > archiveSize64) {
          archiveSize64 = pos;
        }
        pos = offset64(blockTablePos, blockTablePosHi) + blockTableSize * sizeof(MPQBlockEntry);
        if (pos > archiveSize64) {
          archiveSize64 = pos;
        }
        if (hiBlockTablePos64) {
          pos = hiBlockTablePos64 + blockTableSize * sizeof(uint16);
          if (pos > archiveSize64) {
            archiveSize64 = pos;
          }
        }
      }

      hetTablePos64 = 0;
      betTablePos64 = 0;
    }

    hetTableSize64 = 0;
    betTableSize64 = 0;
    if (hetTablePos64) {
      hetTableSize64 = betTablePos64 - hetTablePos64;
      betTableSize64 = offset64(hashTablePos, hashTablePosHi) - betTablePos64;
    }

    if (formatVersion >= 2) {
      hashTableSize64 = offset64(blockTablePos, blockTablePosHi) - offset64(hashTablePos, hashTablePosHi);
      if (hiBlockTablePos64) {
        blockTableSize64 = hiBlockTablePos64 - offset64(blockTablePos, blockTablePosHi);
        hiBlockTableSize64 = archiveSize64 - hiBlockTablePos64;
      } else {
        blockTableSize64 = archiveSize64 - offset64(blockTablePos, blockTablePosHi);
        hiBlockTableSize64 = 0;
      }
    } else {
      hashTableSize64 = uint64(hashTableSize) * sizeof(MPQHashEntry);
      blockTableSize64 = uint64(blockTableSize) * sizeof(MPQBlockEntry);
      hiBlockTableSize64 = 0;
    }

    rawChunkSize = 0;
    memset(md5BlockTable, 0, sizeof md5BlockTable);
    memset(md5HashTable, 0, sizeof md5HashTable);
    memset(md5HiBlockTable, 0, sizeof md5HiBlockTable);
    memset(md5BetTable, 0, sizeof md5BetTable);
    memset(md5HetTable, 0, sizeof md5HetTable);
    memset(md5MPQHeader, 0, sizeof md5MPQHeader);
  }
}

}

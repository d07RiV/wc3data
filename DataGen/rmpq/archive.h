#pragma once

#include "utils/file.h"
#include "rmpq/locale.h"
#include "rmpq/common.h"

#include <unordered_map>

namespace mpq {

namespace FileFlags {
enum {
  CompressPkWare = 0x00000100,
  CompressMulti = 0x00000200,
  Compressed = 0x0000FF00,
  Encrypted = 0x00010000,
  FixSeed = 0x00020000,
  PatchFile = 0x00100000,
  SingleUnit = 0x01000000,
  DummyFile = 0x02000000,
  SectorCrc = 0x04000000,
  Exists = 0x80000000,
};
}

class ListFile;

class Archive : public FileLoader {
public:
  Archive(File file);

  void listFiles(File list);

  size_t getMaxFiles() const;
  bool fileExists(size_t index) const;
  bool testFile(size_t index);

  bool fileExists(char const* name) const;
  bool fileExists(char const* name, uint16 locale) const;

  MPQHashEntry const& hashEntry(size_t index) {
    return hashTable_[index];
  }

  virtual File load(char const* name) override;
  File load(char const* name, uint16 locale);
  File load(size_t index);

  intptr_t findNextFile(char const* name, intptr_t from = -1) const;
  intptr_t findFile(char const* name) const;
  intptr_t findFile(char const* name, uint16 locale) const;

  char const* getFileName(size_t index) const;

  void loadListFile();

  size_t unknowns() const {
    return unknowns_;
  }

private:
  File file_;
  size_t offset_;
  size_t blockSize_;
  MPQHeader header_;
  std::vector<MPQHashEntry> hashTable_;
  std::unordered_map<uint64, size_t> quickTable_;
  std::vector<MPQBlockEntry> blockTable_;
  std::vector<uint16> hiBlockTable_;
  mutable std::vector<std::string> names_;
  mutable size_t unknowns_;
  std::vector<uint8> buffer_;

  void addName_(char const* name);

  File load_(size_t index, uint32 key, bool keyValid);
};

}

#pragma once

#include "rmpq/common.h"

inline uint32 pathHash1(char const* name) {
  return mpq::hashString(name, mpq::HASH_NAME1);
}
inline uint32 pathHash2(char const* name) {
  return mpq::hashString(name, mpq::HASH_NAME2);
}
inline uint64 pathHash(char const* name) {
  return mpq::hashString64(name);
}

class HashArchive : public Archive, public FileLoader {
public:
  HashArchive() {}
  HashArchive(File file)
    : Archive(file)
  {}

  using Archive::has;
  bool has(char const* name) {
    return Archive::has(pathHash(name));
  }

  using Archive::create;
  File& create(char const* name, bool compression = false);

  using Archive::open;
  MemoryFile open(char const* name) {
    return Archive::open(pathHash(name));
  }

  File load(char const* path) {
    return open(path);
  }

  using Archive::add;
  void add(char const* name, File file, bool compression = false);
};

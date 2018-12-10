#include "hash.h"

File& HashArchive::create(uint64 id, bool compression) {
  if (has(id)) {
    printf("warning: hash collision when writing %u\n", id);
  }
  return Archive::create(id, compression);
}

File& HashArchive::create(char const* name, bool compression) {
  uint64 id = pathHash(name);
  if (has(id)) {
    printf("warning: hash collision when writing %s\n", name);
  }
  return Archive::create(id, compression);
}

void HashArchive::add(uint64 id, File file, bool compression) {
  if (has(id)) {
    printf("warning: hash collision when writing %u\n", id);
  }
  Archive::add(id, file, compression);
}

void HashArchive::add(char const* name, File file, bool compression) {
  uint64 id = pathHash(name);
  if (has(id)) {
    printf("warning: hash collision when writing %s\n", name);
  }
  Archive::add(id, file, compression);
}

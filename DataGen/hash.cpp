#include "hash.h"

File& HashArchive::create(char const* name, bool compression) {
  uint64 id = pathHash(name);
  if (has(id)) {
    printf("warning: hash collision when writing %s\n", name);
  }
  return Archive::create(id, compression);
}

void HashArchive::add(char const* name, File file, bool compression) {
  uint64 id = pathHash(name);
  if (has(id)) {
    printf("warning: hash collision when writing %s\n", name);
  }
  Archive::add(id, file, compression);
}

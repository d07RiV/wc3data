#pragma once

#include "utils/file.h"
#include "utils/strlib.h"
#include "hash.h"

namespace jass
{

#pragma pack(push, 1)
struct Options {
  uint8 indent = 2;
  uint8 restoreInt = 1;
  uint8 restoreId = 1;
  uint8 insertLines = 1;
  uint8 restoreStrings = 1;
  uint8 renameGlobals = 5;
  uint8 renameFunctions = 5;
  uint8 renameLocals = 5;
  uint8 inlineFunctions = 1;
  uint8 getObjectName = 1;
};
#pragma pack(pop)

class JASSDo {
public:
  JASSDo(HashArchive& map, Options const& opt);

  MemoryFile process();

private:
  HashArchive& map_;
  Options const& opt;
  MemoryFile sourceFile;
  std::vector<char> buffer;
  char const* source = nullptr;
  char const* sourceEnd = nullptr;
  char const* getline(size_t& length);
  size_t numlines();
  StringLib wts;
  StringLib names;
};

}

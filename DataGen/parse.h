#pragma once

#include "utils/file.h"
#include "rmpq/archive.h"
#include "datafile/game.h"
#include "hash.h"
#include "utils/json.h"
#include <memory>
#include <functional>

enum : unsigned int {
  PROGRESS_LOAD_OBJECTS = 1,
  PROGRESS_WRITE_OBJECTS = 2,
  PROGRESS_IDENTIFY_FILES = 3,
  PROGRESS_COPY_FILES = 4,
};

class MapParser {
public:
  MapParser(File data, File map);

  bool hasCustomObjects();
  MemoryFile processObjects();
  MemoryFile processAll();

  json::Value info;

  std::function<void(unsigned int)> onProgress;

private:
  std::shared_ptr<HashArchive> dataFiles;
  std::shared_ptr<mpq::Archive> mapArchive;
  GameData data;
  CompositeLoader loader;
};

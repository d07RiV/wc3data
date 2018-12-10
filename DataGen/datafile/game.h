#pragma once

#include "objectdata.h"

class GameData {
public:
  void load(FileLoader& loader, int flags);

  enum Type {
    UNITS,
    ITEMS,
    DESTRUCTABLES,
    DOODADS,
    ABILITIES,
    BUFFS,
    UPGRADES,

    NUM_TYPES
  };

  enum {
    LOAD_UNITS          = 0x0001,
    LOAD_ITEMS          = 0x0002,
    LOAD_DESTRUCTABLES  = 0x0004,
    LOAD_DOODADS        = 0x0008,
    LOAD_ABILITIES      = 0x0010,
    LOAD_BUFFS          = 0x0020,
    LOAD_UPGRADES       = 0x0040,
    LOAD_MERGED         = 0x0080,
    LOAD_ALL            = 0x007F,

    LOAD_NO_WEONLY      = 0x0100,
    LOAD_KEEP_METADATA  = 0x0200,
  };

  std::shared_ptr<ObjectData> data[NUM_TYPES];
  std::shared_ptr<ObjectData> merged;
  std::shared_ptr<MetaData> metaData[NUM_TYPES];
  WTSData wts;
  WEStrings wes;
};

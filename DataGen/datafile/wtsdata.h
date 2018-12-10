#pragma once

#include "utils/file.h"
#include "utils/strlib.h"
#include <unordered_map>

class WTSData : public StringLib {
public:
  WTSData() {};
  WTSData(File file);
  WTSData(WTSData&&) = default;

  WTSData& operator=(WTSData&&) = default;
};

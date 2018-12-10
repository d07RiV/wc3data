#pragma once

#include "utils/types.h"

namespace mpq {

namespace Locale {

enum : uint16 {
  Neutral = 0x0000,
  Chinese = 0x0404,
  Czech = 0x0405,
  German = 0x0407,
  English = 0x0409,
  Spanish = 0x040A,
  French = 0x040C,
  Italian = 0x0410,
  Japanese = 0x0411,
  Korean = 0x0412,
  Polish = 0x0415,
  Portuguese = 0x0416,
  Russian = 0x0419,
  EnglishUK = 0x0809,
};

char const* toString(uint16);

}

}

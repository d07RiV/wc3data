#include "locale.h"

namespace mpq {

char const* Locale::toString(uint16 locale) {
  switch (locale) {
  case Neutral:
    return "Neutral";
  case Chinese:
    return "Chinese";
  case Czech:
    return "Czech";
  case German:
    return "German";
  case English:
    return "English";
  case Spanish:
    return "Spanish";
  case French:
    return "French";
  case Italian:
    return "Italian";
  case Japanese:
    return "Japanese";
  case Korean:
    return "Korean";
  case Polish:
    return "Polish";
  case Portuguese:
    return "Portuguese";
  case Russian:
    return "Russian";
  case EnglishUK:
    return "English (UK)";
  default:
    return "Unknown";
  }
}

}

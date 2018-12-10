#pragma once

#include "types.h"

namespace utf8 {

  enum {
    tf_lower = 1,
    tf_upper = 2,
  };

  uint32 transform(uint8_const_ptr* ptr, uint32 table);
  inline uint32 transform(uint8_const_ptr ptr, uint32 table) {
    return transform(&ptr, table);
  }
  uint8_const_ptr next(uint8_const_ptr ptr);

  uint32 parse(uint32 cp);

}

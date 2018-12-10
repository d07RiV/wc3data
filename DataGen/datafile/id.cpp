#include "id.h"
#include "utils/common.h"

std::string idToString(uint32 id) {
  if ((id & 0xFFFF0000) == 0x000D0000) {
    return fmtstring("0x%08X", id);
  } else {
    std::string res;
    res.push_back(char(id >> 24));
    res.push_back(char(id >> 16));
    res.push_back(char(id >> 8));
    res.push_back(char(id));
    return res;
  }
}

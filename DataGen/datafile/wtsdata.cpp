#include "wtsdata.h"

WTSData::WTSData(File file) {
  if (!file) {
    return;
  }
  file.seek(0, SEEK_SET);
  unsigned char chr[3];
  if (file.read(chr, 3) != 3 || chr[0] != 0xEF || chr[1] != 0xBB || chr[2] != 0xBF) {
    file.seek(0, SEEK_SET);
  }
  std::string line;
  std::string value;
  while (file.getline(line)) {
    line = trim(line);
    int pos = 0;
    if (line.substr(0, 7) == "STRING ") {
      int id = atoi(line.c_str() + 7);
      while (file.getline(line)) {
        line = trim(line);
        if (!line.empty() && line.at(0) == '{') {
          value.clear();
          bool prevr = false;
          char chr;
          while ((chr = (char)file.getc()) && chr != '}') {
            if (chr == '\r') {
              prevr = true;
              value.push_back('\n');
            } else {
              if (chr != '\n' || prevr == false) {
                value.push_back(chr);
              }
              prevr = false;
            }
          }
          if (!value.empty()) {
            value.pop_back();
          }
          value.push_back(0);
          add(id, value);
          break;
        }
      }
    }
  }
}

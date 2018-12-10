#include "westrings.h"

void WEStrings::merge(File file) {
  if (!file) return;

  file.seek(0, SEEK_SET);
  unsigned char chr[3];
  if (file.read(chr, 3) != 3 || chr[0] != 0xEF || chr[1] != 0xBB || chr[2] != 0xBF) {
    file.seek(0, SEEK_SET);
  }

  std::string line;
  while (file.getline(line)) {
    line = trim(line);
    size_t eq = line.find('=');
    if (eq != std::string::npos) {
      std::string left = line.substr(0, eq);
      std::string right = line.substr(eq + 1);
      strings_[left] = buffer_.size();
      buffer_.append(right);
      buffer_.push_back(0);
    }
  }
}

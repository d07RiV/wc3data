#include "search.h"
#include <vector>

FileSearch::FileSearch(mpq::Archive& mpq)
  : mpq_(mpq)
  , states_(mpq.getMaxFiles(), 0)
{}

void FileSearch::search() {
  analyzeObj_("war3map.w3u", false);
  analyzeObj_("war3map.w3t", false);
  analyzeObj_("war3map.w3b", false);
  analyzeObj_("war3map.w3d", true);
  analyzeObj_("war3map.w3a", true);
  analyzeObj_("war3map.w3h", false);
  analyzeObj_("war3map.w3q", true);
  analyzeW3i_("war3map.w3i");
  for (size_t i = 0; i < mpq_.getMaxFiles(); ++i) {
    char const* name = mpq_.getFileName(i);
    if (name && !states_[i]) {
      states_[i] = 1;
      stack_.push_back(i);
    }
  }
  while (stack_.size() && mpq_.unknowns() > 0) {
    size_t item = stack_.back();
    stack_.pop_back();
    analyze_(item);
  }
}

void FileSearch::addString_(char const* name) {
  intptr_t index = mpq_.findFile(name);
  if (index >= 0 && !states_[index]) {
    states_[index] = 1;
    stack_.push_back(index);
  }
}
void FileSearch::addStringEx_(std::string& name) {
  size_t pos = name.size();
  while (pos > 0 && name[pos - 1] == 0) {
    --pos;
  }
  name.resize(pos);
  while (pos > 0 && name[pos - 1] != '.' && name[pos - 1] != '\\' && name[pos - 1] != '/') {
    --pos;
  }
  if (pos > 0 && name[pos - 1] == '.') {
    name.resize(--pos);
  }
  while (pos > 0 && name[pos - 1] != '\\' && name[pos - 1] != '/') {
    --pos;
  }
  if (pos > 0) {
    std::string temp = "ReplaceableTextures\\CommandButtonsDisabled\\DIS";
    temp.append(name, pos);
    size_t size = temp.size();
    temp.resize(size + 5);
    char* base = &temp[0];
    char* ext = base + size;
    memcpy(ext, ".blp", 5);
    addString_(base);
    memcpy(ext, ".tga", 5);
    addString_(base);
  }

  addString_(name.c_str());
  size_t size = name.size();
  name.resize(size + 5);
  char* base = &name[0];
  char* ext = base + size;
  memcpy(ext, ".blp", 5);
  addString_(base);
  memcpy(ext, ".tga", 5);
  addString_(base);
  memcpy(ext, ".mdx", 5);
  addString_(base);
  memcpy(ext, ".mdl", 5);
  addString_(base);
  memcpy(ext, ".mp3", 5);
  addString_(base);
  memcpy(ext, ".wav", 5);
  addString_(base);
}

void FileSearch::analyze_(size_t index) {
  char extbuf[6];
  char const* path = mpq_.getFileName(index);
  char* ext = extbuf + 5;
  *ext = 0;

  if (path) {
    size_t pos = strlen(path), len = pos;
    while (pos > 0 && path[pos - 1] != '.' && path[pos - 1] != '\\' && path[pos - 1] != '/' && len - pos < 4) {
      *--ext = tolower((unsigned char) path[--pos]);
    }
    if (pos > 0 && path[pos - 1] == '.' && len - pos < 4) {
      if (!strcmp(ext, "txt")) {
        analyzeTxt_(index, false);
      } else if (!strcmp(ext, "slk")) {
        analyzeTxt_(index, true);
      } else if (!strcmp(ext, "j")) {
        analyzeJass_(index);
      } else if (!strcmp(ext, "mdx")) {
        analyzeMdx_(index);
      }
    }
    std::string cpath(path);
    addStringEx_(cpath);
  }
}

namespace
{
  std::string& readString(File file, std::string& str) {
    str.clear();
    while (int c = file.getc()) {
      if (c == EOF) {
        break;
      }
      str.push_back(c);
    }
    return str;
  }

}

void FileSearch::analyzeObj_(char const* name, bool ext) {
  auto pos = mpq_.findFile(name);
  if (pos < 0) return;
  states_[pos] = 1;
  File file = mpq_.load(pos);
  if (!file) return;

  if (file.read32() > 3) {
    return;
  }
  size_t end = file.size();
  std::string str;
  for (int tbl = 0; tbl < 2 && end - file.tell() >= 4; tbl++) {
    uint32 count = file.read32();
    if (count > 5000) {
      return;
    }
    for (uint32 i = 0; i < count && end - file.tell() >= 12; i++) {
      file.seek(8, SEEK_CUR);
      uint32 count = file.read32();
      if (count > 500) {
        return;
      }
      for (uint32 j = 0; j < count && end - file.tell() >= 8; j++) {
        uint32 modid = file.read32(true);
        uint32 type = file.read32();
        if (type > 3) {
          return;
        }
        if (ext) {
          file.seek(8, SEEK_CUR);
        }
        if (type == 3) {
          addStringEx_(readString(file, str));
        } else {
          file.seek(4, SEEK_CUR);
        }
        file.seek(4, SEEK_CUR);
      }
    }
  }
}

void FileSearch::analyzeW3i_(char const* name) {
  auto pos = mpq_.findFile(name);
  if (pos < 0) return;
  states_[pos] = 1;
  File file = mpq_.load(pos);
  if (!file) return;

  if (file.read32() > 25) {
    return;
  }
  file.seek(8, SEEK_CUR);
  std::string str;
  readString(file, str);
  readString(file, str);
  readString(file, str);
  readString(file, str);
  file.seek(61, SEEK_CUR);
  int lscr = file.read32();
  if (lscr < 0) {
    addStringEx_(readString(file, str));
  }
}

void FileSearch::analyzeTxt_(size_t pos, bool slk) {
  MemoryFile file = mpq_.load(pos);
  if (!file) return;

  uint8 const* ptr = file.data();
  uint8 const* end = ptr + file.size();
  std::string buffer;
  bool inString = false, inEqual = false;
  while (ptr < end) {
    uint8 chr = *ptr++;
    if (chr == '\r' || chr == '\n') {
      if (inString || inEqual) {
        size_t size = buffer.size();
        while (size > 0 && isspace((unsigned char) buffer[size - 1])) {
          --size;
        }
        buffer.resize(size);
        addStringEx_(buffer);
      }
      inString = false;
      inEqual = false;
    } else if (chr == '"') {
      if (inString && ptr < end && *ptr == '"') {
        buffer.push_back(*ptr++);
      } else if (inString) {
        addStringEx_(buffer);
        inString = false;
      } else {
        buffer.clear();
        inString = true;
      }
    } else if (inString) {
      buffer.push_back(chr);
    } else if (chr == '=' && !slk) {
      inEqual = true;
      buffer.clear();
    } else if (chr == ',' && inEqual) {
      addStringEx_(buffer);
      buffer.clear();
    } else if (inEqual) {
      buffer.push_back(chr);
    }
  }
}

void FileSearch::analyzeJass_(size_t pos) {
  MemoryFile file = mpq_.load(pos);
  if (!file) return;

  uint8 const* ptr = file.data();
  uint8 const* end = ptr + file.size();
  std::string buffer;
  bool inStr = false;
  while (ptr < end) {
    uint8 chr = *ptr++;
    if (inStr) {
      if (chr == '\n' || chr == '\r') {
        inStr = false;
      } else if (chr == '"') {
        addStringEx_(buffer);
        inStr = false;
      } else if (chr == '\\' && ptr < end) {
        buffer.push_back(*ptr++);
      } else {
        buffer.push_back(chr);
      }
    } else if (chr == '"') {
      inStr = true;
      buffer.clear();
    }
  }
}

void FileSearch::analyzeMdx_(size_t pos) {
  std::string buffer;

  File file = mpq_.load(pos);
  if (!file) return;

  if (file.read32(true) != 'MDLX') {
    return;
  }

  size_t eof = file.size();
  while (file.tell() < eof) {
    uint32 id = file.read32(true);
    uint32 size = file.read32();
    auto begin = file.tell();
    auto end = begin + size;
    if (id == 'MODL') {
      file.seek(80, SEEK_CUR);
      buffer.resize(260);
      file.read(&buffer[0], 260);
      addStringEx_(buffer);
    } else if (id == 'TEXS') {
      for (uint32 i = 0; i < size; i += 268) {
        file.seek(begin + i + 4);
        buffer.resize(260);
        file.read(&buffer[0], 260);
        addStringEx_(buffer);
      }
    } else if (id == 'ATCH') {
      for (uint32 i = 0; i < size;) {
        file.seek(begin + i);
        i += file.read32();

        uint32 nodeSize = file.read32();
        file.seek(nodeSize - 4, SEEK_CUR);

        buffer.resize(260);
        file.read(&buffer[0], 260);
        addStringEx_(buffer);
      }
    } else if (id == 'PREM') {
      for (uint32 i = 0; i < size;) {
        file.seek(begin + i);
        i += file.read32();

        uint32 nodeSize = file.read32();
        file.seek(nodeSize + 12, SEEK_CUR);

        buffer.resize(260);
        file.read(&buffer[0], 260);
        addStringEx_(buffer);
      }
    }
    file.seek(end);
  }
}

#include "slk.h"

namespace
{

struct SLKEntry {
  char type;
  std::string val;
};

static char const* SLKReadEntry(char const* line, SLKEntry& e) {
  if (*line++ != ';') {
    return nullptr;
  }
  e.type = *line++;
  e.val.clear();
  if (*line == '"') {
    line++;
    while (*line != '"' && *line) {
      e.val.push_back(*line++);
    }
    while (*line != ';' && *line) {
      line++;
    }
  } else {
    while (*line != ';' && *line) {
      e.val.push_back(*line++);
    }
  }
  return line;
}

static char const* SLKReadType(char const* line, SLKEntry& e) {
  e.type = 0;
  e.val.clear();
  while (*line != ';' && *line) {
    e.val.push_back(*line++);
  }
  return line;
}

}

SLKFile::SLKFile(File file)
  : width_(0)
  , height_(0)
{
  buffer_.push_back(0);

  if (!file) return;

  std::string line;
  SLKEntry e;

  file.seek(0, SEEK_SET);
  while (file.getline(line)) {
    line = trim(line);
    char const* cur = SLKReadType(line.c_str(), e);
    if (e.val == "B") {
      while ((cur = SLKReadEntry(cur, e))) {
        if (e.type == 'X') {
          width_ = stoi(e.val);
        } else if (e.type == 'Y') {
          height_ = stoi(e.val);
        }
      }
    }
  }

  if (!width_ || !height_) {
    return;
  }

  file.seek(0, SEEK_SET);
  table_.resize(width_ * height_, 0);
  int curx = 0;
  int cury = 0;
  while (file.getline(line)) {
    line = trim(line);
    char const* cur = SLKReadType(line.c_str(), e);
    if (e.val == "C") {
      while ((cur = SLKReadEntry(cur, e))) {
        if (e.type == 'X') {
          curx = stoi(e.val) - 1;
        } else if (e.type == 'Y') {
          cury = stoi(e.val) - 1;
        } else if (e.type == 'K') {
          if (curx >= 0 && curx < (int)width_ && cury >= 0 && cury < (int)height_) {
            if (cury == 0) {
              cols_[e.val] = curx;
            }
            table_[curx + cury * width_] = buffer_.size();
            buffer_.append(e.val);
            buffer_.push_back(0);
          }
        }
      }
    } else if (e.val == "F") {
      while ((cur = SLKReadEntry(cur, e))) {
        if (e.type == 'X') {
          curx = stoi(e.val) - 1;
        } else if (e.type == 'Y') {
          cury = stoi(e.val) - 1;
        }
      }
    }
  }
}

void SLKFile::csv(File out) const {
  for (size_t i = 0; i < height_; ++i) {
    for (size_t j = 0; j < width_; ++j) {
      if (j) out.putc(',');
      char const* txt = buffer_.data() + table_[i * width_ + j];
      bool quotes = false;
      for (char const* ptr = txt; *ptr; ++ptr) {
        if (*ptr == '"' || *ptr == ',' || *ptr == '\r' || *ptr == '\n') {
          quotes = true;
          break;
        }
      }
      if (quotes) {
        out.putc('"');
        while (*txt) {
          if (*txt == '"') {
            out.putc('"');
          }
          out.putc(*txt++);
        }
        out.putc('"');
      } else {
        out.printf("%s", txt);
      }
    }
    out.putc('\r');
    out.putc('\n');
  }
}

#include "unitdata.h"
#include "objectdata.h"

namespace
{
  int nextComma(char const* str, int c) {
    if (c >= 0 && str[c] == 0) return -1;
    bool instr = false;
    int i = c + 1;
    for (; str[i]; i++) {
      if (str[i] == '"') {
        instr = !instr;
      }
      if (str[i] == ',' && !instr) {
        return i;
      }
    }
    return i;
  }
}

UnitData::UnitData(ObjectData* owner, uint32 id, std::shared_ptr<UnitData> base)
  : owner_(owner)
  , id_(id)
  , base_(base)
{
  buffer_.push_back(0);
  while (base_ && base_->base_) {
    base_ = base_->base_;
  }
}

void UnitData::set_(size_t col, int offs) {
  if (data_.size() <= col) {
    data_.resize(col + 1, 0);
  }
  data_[col] = offs;
}

void UnitData::setData(size_t col, char const* data, int index) {
  if (index < 0) {
    set_(col, buffer_.size());
    buffer_.append(data);
    buffer_.push_back(0);
  } else {
    int oldpos = (col < data_.size() ? data_[col] : 0);
    std::string* oldbase = &buffer_;
    if (oldpos == 0 && base_ && col < base_->data_.size() && base_->data_[col]) {
      oldpos = base_->data_[col];
      oldbase = &base_->buffer_;
    }
    char const* oldptr = oldbase->data() + oldpos;
    size_t olen = strlen(oldptr);
    int count = 0;
    if (*oldptr) {
      int cur = -1;
      int prev = 0;
      while ((cur = nextComma(oldptr, cur)) >= 0) {
        if (count == index) {
          set_(col, buffer_.size());
          buffer_.append(*oldbase, oldpos, prev);
          buffer_.append(data);
          buffer_.append(*oldbase, oldpos + cur, olen - cur + 1);
          return;
        }
        prev = cur + 1;
        count++;
      }
      set_(col, buffer_.size());
      buffer_.append(*oldbase, oldpos, olen);
      while (count++ <= index) {
        buffer_.push_back(',');
      }
      buffer_.append(data);
      buffer_.push_back(0);
    } else {
      set_(col, buffer_.size());
      while (count++ < index) {
        buffer_.push_back(',');
      }
      buffer_.append(data);
      buffer_.push_back(0);
    }
  }
}

void UnitData::compress() {
  std::string buf;
  buf.push_back(0);
  for (int& offset : data_) {
    if (offset) {
      const char* ptr = buffer_.data() + offset;
      offset = buf.size();
      buf.append(ptr);
      buf.push_back(0);
    }
  }
  buffer_ = buf;
}

std::string UnitData::getStringData(size_t col, int index) const {
  char const* src = getData(col);
  if (index < 0) {
    int cmp = nextComma(src, -1);
    if (cmp >= 2 && src[cmp] == 0 && src[0] == '"' && src[cmp - 1] == '"') {
      return std::string(src + 1, cmp - 2);
    } else {
      return src;
    }
  } else {
    int pos = -1;
    int cur = 0;
    int prev = 0;
    while ((pos = nextComma(src, pos)) >= 0) {
      if (cur == index) {
        if (pos - prev >= 2 && src[prev] == '"' && src[pos - 1] == '"') {
          return std::string(src + prev + 1, pos - prev - 2);
        } else {
          return std::string(src + prev, pos - prev);
        }
      }
      prev = pos + 1;
      cur++;
    }
  }
  return "";
}

char const* UnitData::getData(std::string const& field) const {
  return owner_->getUnitData(this, field);
}
bool UnitData::hasData(std::string const& field) const {
  return owner_->isUnitDataSet(this, field);
}
int UnitData::getIntData(std::string const& field) const {
  return owner_->getUnitInt(this, field);
}
float UnitData::getRealData(std::string const& field) const {
  return owner_->getUnitReal(this, field);
}
std::string UnitData::getStringData(std::string const& field, int index) const {
  return owner_->getUnitString(this, field, index);
}

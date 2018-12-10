#include "strlib.h"

StringLib::StringLib(File file) {
  uint32 count = file.read32();
  uint32 size = file.read32();
  index_.reserve(count);
  for (uint32 i = 0; i < count; ++i) {
    uint32 key = file.read32();
    uint32 value = file.read32();
    index_.emplace(key, value);
  }
  buffer_.resize(size);
  file.read(&buffer_[0], size);
}

void StringLib::write(File file) {
  file.write32(index_.size());
  file.write32(buffer_.size());
  for (auto const& kv : index_) {
    file.write32(kv.first);
    file.write32(kv.second);
  }
  file.write(buffer_.data(), buffer_.size());
}

char const* StringLib::get(uint32 key) {
  auto it = index_.find(key);
  return it == index_.end() ? nullptr : buffer_.data() + it->second;
}
void StringLib::add(uint32 key, char const* value) {
  index_[key] = (uint32)buffer_.size();
  buffer_.append(value);
  buffer_.push_back(0);
}
void StringLib::add(uint32 key, std::string const& value) {
  index_[key] = (uint32)buffer_.size();
  buffer_.append(value);
  buffer_.push_back(0);
}

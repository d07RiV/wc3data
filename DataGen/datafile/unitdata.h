#pragma once

#include <string>
#include <vector>
#include <memory>
#include "utils/types.h"

class ObjectData;

class UnitData {
public:
  UnitData(ObjectData* owner, uint32 id, std::shared_ptr<UnitData> base = nullptr);

  uint32 id() const {
    return id_;
  }

  UnitData* base() {
    return (base_ && base_->id_ != id_) ? base_.get() : nullptr;
  }

  UnitData const* base() const {
    return (base_ && base_->id_ != id_) ? base_.get() : nullptr;
  }

  size_t dataSize() const {
    return buffer_.size() + data_.size() * 4;
  }

  void setData(size_t col, char const* data, int index = -1);

  char const* getData(size_t col) const {
    if (col < data_.size() && data_[col] != 0) {
      return buffer_.data() + data_[col];
    } else if (base_ && col < base_->data_.size()) {
      return base_->buffer_.data() + base_->data_[col];
    } else {
      return buffer_.data();
    }
  }

  std::string getStringData(size_t col, int index = -1) const;

  bool hasData(size_t col) const {
    if (col < data_.size() && data_[col] != 0) {
      return true;
    } else if (base_ && col < base_->data_.size()) {
      return base_->data_[col] != 0;
    } else {
      return false;
    }
  }

  void compress();

  char const* getData(std::string const& field) const;
  bool hasData(std::string const& field) const;
  int getIntData(std::string const& field) const;
  float getRealData(std::string const& field) const;
  std::string getStringData(std::string const& field, int index = -1) const;

private:
  std::string buffer_;
  uint32 id_;
  std::shared_ptr<UnitData> base_;
  std::vector<int> data_;
  ObjectData* owner_;
  void set_(size_t col, int offs);
};

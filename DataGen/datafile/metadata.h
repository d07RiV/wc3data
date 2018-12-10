#pragma once

#include "slk.h"

class MetaData
{
public:

  enum Column {
    ID,
    FIELD,
    INDEX,
    REPEAT,
    APPENDINDEX,
    DATA,
    CATEGORY,
    DISPLAY,
    SORT,
    TYPE,
    USEUNIT,
    USEHERO,
    USEBUILDING,
    USEITEM,
    USESPECIFIC,
    STRINGEXT,

    NUM_COLUMNS
  };

  MetaData(File file);

  bool valid() const {
    return !rows_.empty();
  }

  size_t rows() const {
    return slk_.rows();
  }

  int getRow(uint32 id) const {
    auto it = rows_.find(id);
    return it == rows_.end() ? -1 : it->second;
  }

  uint32 getInt(int row, Column type) const {
    return cols_[type] >= 0 ? atoi(slk_.item(row, cols_[type])) : 0;
  }

  char const* getString(int row, Column type) const {
    return cols_[type] >= 0 ? slk_.item(row, cols_[type]) : nullptr;
  }

  char const* value(int id, int* index = nullptr) const {
    int row = getRow(id);
    if (row < 0) return nullptr;
    if (index) *index = atoi(slk_.item(row, cols_[INDEX]));
    return slk_.item(row, cols_[FIELD]);
  }

  int getId(std::string const& value) const {
    auto it = ids_.find(value);
    return it == ids_.end() ? 0 : it->second;
  }

  void csv(File out) const {
    slk_.csv(out);
  }

private:
  SLKFile slk_;
  std::unordered_map<std::string, uint32> ids_;
  int cols_[NUM_COLUMNS];
  std::unordered_map<uint32, int> rows_;
};

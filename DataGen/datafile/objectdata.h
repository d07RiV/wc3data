#pragma once

#include "id.h"
#include "unitdata.h"
#include "westrings.h"
#include "metadata.h"
#include "wtsdata.h"
#include <unordered_map>
#include <memory>

class ObjectData {
public:
  ObjectData(WEStrings* we = nullptr);

  bool readSLK(File file);
  bool readINI(File file, bool split = false);
  bool readOBJ(File file, MetaData* meta, bool ext, WTSData* wts = NULL);

  size_t numUnits() const {
    return units_.size();
  }

  size_t dataSize() const;

  UnitData* unit(int i) {
    return units_[i].get();
  }
  UnitData const* unit(int i) const
  {
    return units_[i].get();
  }

  UnitData* getUnitById(uint32 id) {
    auto it = rows_.find(id);
    return it == rows_.end() ? nullptr : units_[it->second].get();
  }

  UnitData* getUnitById(char const* id) {
    return getUnitById(idFromString(id));
  }

  UnitData const* getUnitById(uint32 id) const {
    auto it = rows_.find(id);
    return it == rows_.end() ? nullptr : units_[it->second].get();
  }

  UnitData const* getUnitById(char const* id) const {
    return getUnitById(idFromString(id));
  }

  size_t numColumns() const {
    return cols_.size();
  }

  char const* columnName(size_t index) const {
    return colNames_.getData(index);
  }

  int columnIndex(std::string const& field) const {
    auto it = cols_.find(field);
    return it == cols_.end() ? -1 : it->second;
  }

  void setUnitData(UnitData* unit, std::string const& field, char const* data, int index = -1);

  char const* getUnitData(UnitData const* unit, std::string const& field) const {
    int col = columnIndex(field);
    return col < 0 ? nullptr : unit->getData(col);
  }

  bool isUnitDataSet(UnitData const* unit, std::string const& field) const {
    int col = columnIndex(field);
    return col >= 0 && unit->hasData(col);
  }
  int getUnitInt(UnitData const* unit, std::string const& field) const {
    int col = columnIndex(field);
    return col >= 0 ? atoi(unit->getData(col)) : 0;
  }
  float getUnitReal(UnitData const* unit, std::string const& field) const {
    int col = columnIndex(field);
    return col >= 0 ? (float)atof(unit->getData(col)) : 0;
  }
  std::string getUnitString(UnitData const* unit, std::string const& field, int index = -1) const {
    int col = columnIndex(field);
    return col >= 0 ? unit->getStringData(col, index) : "";
  }

  void dump(File f);

private:
  std::unordered_map<uint32, int> rows_;
  Map<int> cols_;
  std::vector<std::shared_ptr<UnitData>> units_;

  UnitData* addUnit_(uint32 id, int base = 0);
  UnitData* addUnit_(char const* id, int base = 0) {
    return addUnit_(idFromString(id), base);
  }

  UnitData colNames_;

  WEStrings* wes_;
  char const* translate_(char const* str) const {
    char const* r = nullptr;
    if (wes_) {
      r = wes_->get(str);
    }
    return r ? r : str;
  }
};

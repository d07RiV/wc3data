#include "objectdata.h"

ObjectData::ObjectData(WEStrings* we)
  : wes_(we)
  , colNames_(nullptr, 0)
{
}

UnitData* ObjectData::addUnit_(uint32 id, int base) {
  auto prev = getUnitById(id);
  if (prev) return prev;

  std::shared_ptr<UnitData> bu;
  if (base) {
    auto it = rows_.find(base);
    if (it != rows_.end()) {
      bu = units_[it->second];
    }
  }
  rows_[id] = units_.size();
  units_.push_back(std::make_shared<UnitData>(this, id, bu));
  return units_.back().get();
}

void ObjectData::setUnitData(UnitData* unit, std::string const& field, char const* data, int index) {
  if (!unit) return;
  int col = columnIndex(field);
  if (col < 0) {
    col = cols_.size();
    cols_[field] = col;
    colNames_.setData(col, field.c_str());
  }
  unit->setData(col, data, index);
}

bool ObjectData::readSLK(File file) {
  SLKFile slk(file);
  if (!slk.valid()) {
    return false;
  }

  std::vector<int> cols(slk.cols());
  for (size_t i = 1; i < slk.cols(); i++) {
    std::string field = slk.columnName(i);
    cols[i] = columnIndex(field);
    if (cols[i] < 0) {
      cols[i] = cols_.size();
      cols_[field] = cols[i];
      colNames_.setData(cols[i], field.c_str());
    }
  }
  for (int i = 0; i < slk.rows(); i++) {
    UnitData* data = addUnit_(slk.item(i, 0));
    for (size_t j = 1; j < slk.cols(); j++) {
      if (slk.has(i, j)) {
        data->setData(cols[j], translate_(slk.item(i, j)));
      }
    }
  }
  return true;
}

bool ObjectData::readINI(File file, bool split) {
  if (!file) {
    return false;
  }

  file.seek(0, SEEK_SET);
  unsigned char chr[3];
  if (file.read(chr, 3) != 3 || chr[0] != 0xEF || chr[1] != 0xBB || chr[2] != 0xBF) {
    file.seek(0, SEEK_SET);
  }

  UnitData* cur = NULL;
  bool ok = true;
  std::string line;
  while (ok && file.getline(line))
  {
    line = trim(line);
    if (line.size() > 0 && line[0] == '[') {
      if (line.size() <= 5 || line[5] != ']') {
        return true; // or false?
      } else {
        cur = getUnitById(line.c_str() + 1);
      }
    } else if (cur && line.size() > 0 && line[0] != '/') {
      size_t eq = line.find('=');
      if (eq != std::string::npos) {
        setUnitData(cur, line.substr(0, eq), translate_(line.c_str() + eq + 1));
      }
    }
  }
  return true;
}

bool ObjectData::readOBJ(File file, MetaData* meta, bool ext, WTSData* wts) {
  if (!file || !meta || !meta->valid()) {
    return false;
  }

  uint64 end = file.size();
  file.seek(4, SEEK_SET);
  for (int tbl = 0; tbl < 2 && end - file.tell() >= 4; tbl++) {
    uint32 count = file.read32();
    if (count > 5000) {
      return false;
    }
    for (uint32 i = 0; i < count && end - file.tell() >= 12; i++) {
      uint32 oldid = file.read32(true);
      uint32 newid = file.read32(true);
      uint32 count = file.read32();
      if (count > 500) {
        return false;
      }
      UnitData* unit = NULL;
      if (tbl) {
        unit = addUnit_(newid, oldid);
      } else {
        auto it = rows_.find(oldid);
        if (it != rows_.end()) {
          units_[it->second] = std::make_shared<UnitData>(this, oldid, units_[it->second]);
          unit = units_[it->second].get();
        }
      }
      for (uint32 j = 0; j < count && end - file.tell() >= 8; j++) {
        uint32 modid = file.read32(true);
        uint32 type = file.read32();
        int index;
        char const* mod_c = meta->value(modid, &index);
        if (!mod_c || type > 3) {
          return false;
        }
        std::string mod = mod_c;
        if (ext) {
          uint32 level = file.read32();
          uint32 data = file.read32();
          if (mod == "Data") {
            mod.push_back(char('A' + data - 1));
          }
          if (level) {
            if (index < 0) {
              mod.append(std::to_string(level));
            } else {
              index += level - 1;
            }
          }
        }
        std::string value;
        if (type == 0) {
          value = std::to_string(file.read32());
        } else if (type == 3) {
          while (int chr = file.getc()) {
            if (chr == EOF) break;
            value.push_back((char)chr);
          }
          if (wts && !strncmp(value.c_str(), "TRIGSTR_", 8)) {
            char const* rep = wts->get(atoi(value.c_str() + 8));
            if (rep) {
              value = rep;
            }
          }
        } else {
          value = fmtstring("%.2f", file.read<float>());
        }
        if (unit) {
          setUnitData(unit, mod, translate_(value.c_str()), index);
        }
        uint32 suf = file.read32(true);
        if (suf != 0 && suf != newid && suf != oldid) {
          //return false;
        }
      }
      if (unit) {
        unit->compress();
      }
    }
  }
  return true;
}

size_t ObjectData::dataSize() const {
  size_t total = 0;
  for (auto const& u : units_) {
    total += u->dataSize();
  }
  return total;
}

void ObjectData::dump(File f) {
  for (auto& unit : units_) {
    if (unit) {
      if (unit->base()) {
        f.printf("[%s:%s]\n", idToString(unit->id()).c_str(), idToString(unit->base()->id()).c_str());
      } else {
        f.printf("[%s]\n", idToString(unit->id()).c_str());
      }
      for (int j = 0; j < cols_.size(); j++) {
        if (unit->hasData(j)) {
          f.printf("%s=%s\n", colNames_.getData(j), unit->getData(j));
        }
      }
    }
  }
}

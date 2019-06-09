#include <memory>
#include <algorithm>
#include <set>
#include <inttypes.h>
#include "datafile/game.h"
//#include "image/image.h"

#include "utils/json.h"
#include "rmpq/archive.h"
#include "rmpq/common.h"
#include "utils/path.h"
#include "hash.h"
#include "search.h"

#include "parse.h"

#ifndef NO_SYSTEM
#include "utils/logger.h"
#endif

std::string transStr(std::string str, WEStrings& wes) {
  if (!wes.get(str)) return str;
  while (true) {
    auto val = wes.get(str);
    if (val) str = val;
    else break;
  }
  if (str.size() && str[0] == '"') {
    size_t next = str.find('"', 1);
    if (next != std::string::npos) {
      str = str.substr(1, next - 1);
    }
  } else {
    str = split(str, ',')[0];
  }
  return str;
}

std::string transStr(std::string str, WTSData& wts) {
  if (!strncmp(str.c_str(), "TRIGSTR_", 8)) {
    char const* rep = wts.get(atoi(str.c_str() + 8));
    if (rep) {
      str = rep;
    }
  }
  return str;
}

std::string fixStr(std::string str, WEStrings& wes) {
  str = transStr(str, wes);
  std::string sv;
  for (auto c : str) {
    if (c != '&') sv.push_back(c);
  }
  return sv;
}

void parseMeta(json::Visitor& out, MetaData* meta, GameData::Type type, WEStrings& wes) {
  out.onOpenArray();
  for (size_t row = 0; row < meta->rows(); ++row) {
    int flags = 0;
    if (type == GameData::UNITS) {
      if (meta->getInt(row, MetaData::USEUNIT)) flags |= 1;
      if (meta->getInt(row, MetaData::USEHERO)) flags |= 2;
      if (meta->getInt(row, MetaData::USEBUILDING)) flags |= 4;
      if (meta->getInt(row, MetaData::USEITEM)) flags |= 8;
    }

    out.onOpenMap();
    out.keyString("field", meta->getString(row, MetaData::FIELD));
    out.keyInteger("index", meta->getInt(row, MetaData::INDEX));
    if (meta->getInt(row, MetaData::REPEAT)) flags |= 16;
    if (meta->getInt(row, MetaData::APPENDINDEX)) flags |= 32;
    out.keyInteger("data", meta->getInt(row, MetaData::DATA));
    out.keyInteger("flags", flags);
    out.keyString("specific", meta->getString(row, MetaData::USESPECIFIC));
    out.keyInteger("stringExt", meta->getInt(row, MetaData::STRINGEXT));

    out.keyString("category", meta->getString(row, MetaData::CATEGORY));
    out.keyString("type", meta->getString(row, MetaData::TYPE));
    auto display = meta->getString(row, MetaData::DISPLAY);
    auto transl = wes.get(display);
    out.keyString("display", transl ? transl : display);
    out.keyString("sort", meta->getString(row, MetaData::SORT));
    out.onCloseMap();
  }
  out.onCloseArray();
}

json::Value parseINI(File file) {
  std::string line;
  json::Value out;
  json::Value* cur = nullptr;
  while (file.getline(line)) {
    line = trim(line);
    if (line.size() > 0 && line[0] == '[') {
      if (line[line.size() - 1] != ']') {
        return json::Value();
      } else {
        cur = &out[line.substr(1, line.size() - 2)];
      }
    } else if (cur && line.size() > 0 && line[0] != '/') {
      size_t pos = line.find('=');
      if (pos != std::string::npos) {
        (*cur)[line.substr(0, pos)] = line.substr(pos + 1);
      }
    }
  }
  return out;
}

json::Value parseTypes(File file, WEStrings& wes) {
  std::string line;
  json::Value out;
  json::Value* cur = nullptr;
  std::map<int, std::string> values;
  while (file.getline(line)) {
    line = trim(line);
    if (line.size() > 0 && line[0] == '[') {
      if (line[line.size() - 1] != ']') {
        return json::Value();
      } else {
        values.clear();
        cur = &out[line.substr(1, line.size() - 2)];
      }
    } else if (cur && line.size() > 0 && line[0] != '/') {
      auto p = split(line, '=');
      if (p.size() == 2 && (p[0].size() == 2 || p[0].size() == 6) && isdigit((unsigned char) p[0][0]) && isdigit((unsigned char) p[0][1])) {
        int key = atoi(p[0].c_str());
        if (p[0].size() == 6) {
          if (p[0].substr(2) == "_Alt") {
            p = split(p[1], ',');
            for (auto& k : p) {
              (*cur)[k] = values[key];
            }
          }
        } else {
          p = split(p[1], ',');
          if (p.size() >= 2) {
            auto sv = fixStr(p[1], wes);
            values[key] = sv;
            (*cur)[p[0]] = sv;
          }
        }
      }
    }
  }
  return out;
}

#pragma pack (push, 1)
struct TGAHeader {
  uint8 idLength;
  uint8 colormapType;
  uint8 imageType;
  uint16 colormapIndex;
  uint16 colormapLength;
  uint8 colormapEntrySize;
  uint16 xOrigin;
  uint16 yOrigin;
  uint16 width;
  uint16 height;
  uint8 pixelSize;
  uint8 imageDesc;
};
#pragma pack (pop)

std::string readString(File file) {
  std::string result;
  while (int c = file.getc()) {
    if (c == EOF) {
      break;
    }
    result.push_back(c);
  }
  return result;
}

MapParser::MapParser(File data, File map) {
  dataFiles = std::make_shared<HashArchive>(data);
  if (map) {
    mapArchive = std::make_shared<mpq::Archive>(map);
    map.seek(8);
    std::string name;
    while (int c = map.read8()) {
      name.push_back(c);
    }
    info["name"] = name;
  }
  if (mapArchive) {
    loader.add(mapArchive);
  }
  loader.add(dataFiles);
}

bool MapParser::hasCustomObjects() {
  static std::string customFiles[] = {
    "war3map.w3u",
    "war3map.w3t",
    "war3map.w3b",
    "war3map.w3d",
    "war3map.w3a",
    "war3map.w3h",
    "war3map.w3q",
    "Doodads\\Doodads.slk",
    "Doodads\\DoodadMetaData.slk",
    "UI\\WorldEditGameStrings.txt",
    "UI\\WorldEditStrings.txt",
    "Units\\AbilityBuffData.slk",
    "Units\\AbilityBuffMetaData.slk",
    "Units\\AbilityData.slk",
    "Units\\AbilityMetaData.slk",
    "Units\\CampaignAbilityFunc.txt",
    "Units\\CampaignAbilityStrings.txt",
    "Units\\CampaignUnitFunc.txt",
    "Units\\CampaignUnitStrings.txt",
    "Units\\CampaignUpgradeFunc.txt",
    "Units\\CampaignUpgradeStrings.txt",
    "Units\\CommonAbilityFunc.txt",
    "Units\\CommonAbilityStrings.txt",
    "Units\\DestructableData.slk",
    "Units\\DestructableMetaData.slk",
    "Units\\HumanAbilityFunc.txt",
    "Units\\HumanAbilityStrings.txt",
    "Units\\HumanUnitFunc.txt",
    "Units\\HumanUnitStrings.txt",
    "Units\\HumanUpgradeFunc.txt",
    "Units\\HumanUpgradeStrings.txt",
    "Units\\ItemAbilityFunc.txt",
    "Units\\ItemAbilityStrings.txt",
    "Units\\ItemData.slk",
    "Units\\ItemFunc.txt",
    "Units\\ItemStrings.txt",
    "Units\\NeutralAbilityFunc.txt",
    "Units\\NeutralAbilityStrings.txt",
    "Units\\NeutralUnitStrings.txt",
    "Units\\NeutralUnitFunc.txt",
    "Units\\NeutralUpgradeFunc.txt",
    "Units\\NeutralUpgradeStrings.txt",
    "Units\\NightElfAbilityFunc.txt",
    "Units\\NightElfAbilityStrings.txt",
    "Units\\NightElfUnitStrings.txt",
    "Units\\NightElfUnitFunc.txt",
    "Units\\NightElfUpgradeFunc.txt",
    "Units\\NightElfUpgradeStrings.txt",
    "Units\\OrcAbilityFunc.txt",
    "Units\\OrcAbilityStrings.txt",
    "Units\\OrcUnitFunc.txt",
    "Units\\OrcUnitStrings.txt",
    "Units\\OrcUpgradeFunc.txt",
    "Units\\OrcUpgradeStrings.txt",
    "Units\\UndeadAbilityFunc.txt",
    "Units\\UndeadAbilityStrings.txt",
    "Units\\UndeadUnitFunc.txt",
    "Units\\UndeadUnitStrings.txt",
    "Units\\UndeadUpgradeFunc.txt",
    "Units\\UndeadUpgradeStrings.txt",
    "Units\\UnitAbilities.slk",
    "Units\\UnitBalance.slk",
    "Units\\UnitData.slk",
    "Units\\UnitMetaData.slk",
    "Units\\unitUI.slk",
    "Units\\UnitWeapons.slk",
    "Units\\UpgradeData.slk",
    "Units\\UpgradeMetaData.slk",
  };
  if (!mapArchive) {
    return false;
  }
  for (const auto& fn : customFiles) {
    if (mapArchive->fileExists(fn.c_str())) {
      return true;
    }
  }
  return false;
}

MemoryFile MapParser::processObjects() {
  std::string typeNames[] = { "unit", "item", "destructible", "doodad", "ability", "buff", "upgrade" };

  data.load(loader, GameData::LOAD_ALL | GameData::LOAD_KEEP_METADATA);
  if (onProgress) onProgress(PROGRESS_LOAD_OBJECTS);

#ifndef NO_SYSTEM
  //for (int type = 0; type < GameData::NUM_TYPES; ++type) {
  //  Logger::log("Data %-10s count=%-8u size=%-8u", typeNames[type].c_str(), data.data[type]->numUnits(), data.data[type]->dataSize());
  //}
#endif

  MemoryFile outFile;
  json::WriterVisitor out(outFile);
  //out.setIndent(2);

  std::set<uint32> icons;
  if (File iconFile = dataFiles->load("images.dat")) {
    size_t count = (size_t)iconFile.size() / 4;
    for (size_t i = 0; i < count; ++i) {
      icons.insert(iconFile.read32());
    }
  }

  auto wed = parseINI(loader.load("UI\\WorldEditData.txt"));

  out.onOpenMap();

  out.onMapKey("objects");
  out.onOpenArray();

  auto fileExists = [&](char const* name) {
    if (mapArchive && mapArchive->fileExists(name)) {
      return true;
    }
    return icons.count(pathHash(name)) != 0;
  };

  for (int type = 0; type < GameData::NUM_TYPES; ++type) {
    int arti = data.data[type]->columnIndex("Art");
    if (arti < 0) {
      arti = data.data[type]->columnIndex("Buffart");
    }

    for (size_t i = 0; i < data.data[type]->numUnits(); ++i) {
      UnitData* unit = data.data[type]->unit(i);

      if (!unit->id()) {
        continue;
      }

      out.onOpenMap();
      out.keyString("id", idToString(unit->id()));
      if (unit->base()) {
        out.keyString("base", idToString(unit->base()->id()));
      }
      out.keyString("type", typeNames[type]);

      std::string art;
      if (arti && unit->hasData(arti)) {
        art = unit->getStringData(arti, 0);
      } else if (type == GameData::DESTRUCTABLES) {
        auto& cats = wed["DestructibleCategories"];
        auto cat = unit->getData("category");
        if (cats.has(cat)) {
          auto p = split(cats[cat].getString(), ',');
          if (p.size() >= 2) {
            art = p[1];
          }
        }
      } else if (type == GameData::DOODADS) {
        auto& cats = wed["DoodadCategories"];
        auto cat = unit->getData("category");
        if (cats.has(cat)) {
          auto p = split(cats[cat].getString(), ',');
          if (p.size() >= 2) {
            art = p[1];
          }
        }
      }

      if (!fileExists(art.c_str())) {
        art = path::path(art) / path::title(art) + ".blp";
        if (!fileExists(art.c_str())) {
          art = "ReplaceableTextures\\WorldEditUI\\DoodadPlaceholder.blp";
        }
      }

      out.onMapKey("icon");
      out.onOpenArray();
      out.onNumber(pathHash1(art.c_str()));
      out.onNumber(pathHash2(art.c_str()));
      out.onCloseArray();

      std::string uname = unit->getStringData("EditorName", 0);
      if (uname.empty() || uname == "_") {
        if (unit->hasData("Name")) uname = unit->getStringData("Name", 0);
        else if (unit->hasData("Bufftip")) uname = unit->getStringData("Bufftip", 0);
        else uname = "Chaos";
      }
      uname = fixStr(uname, data.wes);
      if (unit->hasData("EditorSuffix")) {
        std::string usuf = unit->getStringData("EditorSuffix", 0);
        usuf = fixStr(usuf, data.wes);
        if (usuf != "_") {
          if (!usuf.empty() && !isspace((unsigned char)usuf[0])) {
            uname.push_back(' ');
          }
          uname += usuf;
        }
      }
      out.keyString("name", uname);

      out.onMapKey("data");
      out.onOpenMap();
      for (size_t j = 0; j < data.data[type]->numColumns(); j++) {
        if (unit->hasData(j)) {
          out.keyString(data.data[type]->columnName(j), transStr(unit->getData(j), data.wes));
        }
      }
      out.onCloseMap();

      out.onCloseMap();
    }
  }

  out.onCloseArray();

  out.onMapKey("meta");
  out.onOpenMap();
  for (int type = 0; type < GameData::NUM_TYPES; ++type) {
    if (type == GameData::ITEMS) {
      continue;
    }
    out.onMapKey(typeNames[type]);
    parseMeta(out, data.metaData[type].get(), (GameData::Type)type, data.wes);
  }
  out.onCloseMap();

  out.onMapKey("types");
  parseTypes(loader.load("UI\\UnitEditorData.txt"), data.wes).walk(&out);

  if (wed.has("ObjectEditorCategories")) {
    auto cat = wed["ObjectEditorCategories"];
    out.onMapKey("categories");
    out.onOpenMap();
    for (auto& kv : cat.getMap()) {
      out.keyString(kv.first, fixStr(kv.second.getString(), data.wes));
    }
    out.onCloseMap();
  }

  out.onCloseMap();
  out.onEnd();

  if (onProgress) onProgress(PROGRESS_WRITE_OBJECTS);

  return outFile;
}

MemoryFile MapParser::processAll() {
  HashArchive outArc;

  if (mapArchive) {
    mapArchive->loadListFile();
  }

  if (hasCustomObjects()) {
    MemoryFile outFile = processObjects();
    outArc.add("objects.json", outFile, true);

    StringLib names;
    for (int type = 0; type < GameData::NUM_TYPES; ++type) {
      int col = data.data[type]->columnIndex("name");
      if (col < 0) continue;
      for (size_t i = 0; i < data.data[type]->numUnits(); ++i) {
        UnitData* unit = data.data[type]->unit(i);
        if (unit->hasData(col)) {
          names.add(unit->id(), unit->getStringData(col, 0));
        }
      }
    }
    names.write(outArc.create("names.lib", true));
    data.wts.write(outArc.create("strings.lib", true));
  } else {
    data.wts = WTSData(loader.load("war3map.wts"));
    data.wts.write(outArc.create("strings.lib", true));
  }

  File infoFile = loader.load("war3map.w3i");
  if (infoFile) {
    uint32 ver = infoFile.read32();
    infoFile.seek(ver >= 27 ? 28 : 12, SEEK_SET);
    info["name"] = transStr(readString(infoFile), data.wts);
    info["author"] = transStr(readString(infoFile), data.wts);
    info["description"] = transStr(readString(infoFile), data.wts);
    info["players"] = transStr(readString(infoFile), data.wts);

    json::WriterVisitor writer(outArc.create("info.json"));
    writer.setIndent(0);
    info.walk(&writer);
    writer.onEnd();
  }

  if (mapArchive) {
    if (File list = dataFiles->load("listfile.txt")) {
      mapArchive->listFiles(list);
    }

    FileSearch search(*mapArchive);
    search.search();

    if (onProgress) onProgress(PROGRESS_IDENTIFY_FILES);

    auto listFile = outArc.create("listfile.txt", true);

    size_t count = mapArchive->getMaxFiles();
    uint32 numFound = 0, numMissing = 0, numFailed = 0;
    for (size_t i = 0; i < count; ++i) {
      char const* name = mapArchive->getFileName(i);
      File file = mapArchive->load(i);
      if (file) {
        auto& he = mapArchive->hashEntry(i);
        outArc.add(mpq::hashTo64(he.name1, he.name2), file, true);
        if (name) {
          listFile.printf("%s\n", name);
          ++numFound;
        } else if (mapArchive->fileExists(i)) {
          char const* ext = "";
          if (file.size() >= 4) {
            file.seek(0);
            uint32 id = file.read32(true);
            if (id == 'BLP1' || id == 'BLP2') {
              ext = ".blp";
            } else if (id == 'MDLX') {
              ext = ".mdx";
            }
          }
          listFile.printf("Unknown\\%08X%08X%s\n", he.name2, he.name1, ext);
          ++numMissing;
        }
      } else if (mapArchive->fileExists(i)) {
        ++numFailed;
      }
    }
    if (onProgress) onProgress(PROGRESS_COPY_FILES);
  }

  MemoryFile finalFile;
  outArc.write(finalFile);
  finalFile.seek(0);
  return finalFile;
}

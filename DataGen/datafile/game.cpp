#include "game.h"

void GameData::load(FileLoader& loader, int flags)
{
  if ((flags & LOAD_ALL) == 0) return;

  wts = WTSData(loader.load("war3map.wts"));

  if (flags & (LOAD_DESTRUCTABLES | LOAD_DOODADS)) {
    wes.merge(loader.load("UI\\WorldEditGameStrings.txt"));
    if (!(flags & LOAD_NO_WEONLY)) {
      wes.merge(loader.load("UI\\WorldEditStrings.txt"));
    }
  }

  if (flags & LOAD_MERGED) {
    merged = std::make_shared<ObjectData>(&wes);
  }

  if (flags & LOAD_UNITS) {
    auto dst = merged;
    if (!merged) {
      dst = data[UNITS] = std::make_shared<ObjectData>();
    }

    dst->readSLK(loader.load("Units\\UnitAbilities.slk"));
    dst->readSLK(loader.load("Units\\UnitBalance.slk"));
    dst->readSLK(loader.load("Units\\UnitData.slk"));
    dst->readSLK(loader.load("Units\\unitUI.slk"));
    dst->readSLK(loader.load("Units\\UnitWeapons.slk"));
    dst->readINI(loader.load("Units\\UndeadUnitStrings.txt"));
    dst->readINI(loader.load("Units\\UndeadUnitFunc.txt"));
    dst->readINI(loader.load("Units\\OrcUnitStrings.txt"));
    dst->readINI(loader.load("Units\\OrcUnitFunc.txt"));
    dst->readINI(loader.load("Units\\NightElfUnitStrings.txt"));
    dst->readINI(loader.load("Units\\NightElfUnitFunc.txt"));
    dst->readINI(loader.load("Units\\NeutralUnitStrings.txt"));
    dst->readINI(loader.load("Units\\NeutralUnitFunc.txt"));
    dst->readINI(loader.load("Units\\HumanUnitStrings.txt"));
    dst->readINI(loader.load("Units\\HumanUnitFunc.txt"));
    dst->readINI(loader.load("Units\\CampaignUnitStrings.txt"));
    dst->readINI(loader.load("Units\\CampaignUnitFunc.txt"));

    metaData[UNITS] = std::make_shared<MetaData>(loader.load("Units\\UnitMetaData.slk"));
    dst->readOBJ(loader.load("war3map.w3u"), metaData[UNITS].get(), false, &wts);

    if (flags & LOAD_ITEMS) {
      metaData[ITEMS] = metaData[UNITS];
    }
    if (!(flags & LOAD_KEEP_METADATA)) {
      metaData[UNITS] = nullptr;
    }
  }

  if (flags & LOAD_ITEMS) {
    auto dst = merged;
    if (!merged) {
      dst = data[ITEMS] = std::make_shared<ObjectData>();
    }

    dst->readSLK(loader.load("Units\\ItemData.slk"));
    dst->readINI(loader.load("Units\\ItemFunc.txt"));
    dst->readINI(loader.load("Units\\ItemStrings.txt"));

    if (!metaData[ITEMS]) {
      metaData[ITEMS] = std::make_shared<MetaData>(loader.load("Units\\UnitMetaData.slk"));
    }
    dst->readOBJ(loader.load("war3map.w3t"), metaData[ITEMS].get(), false, &wts);

    if (!(flags & LOAD_KEEP_METADATA)) {
      metaData[ITEMS] = nullptr;
    }
  }

  if (flags & LOAD_DESTRUCTABLES) {
    auto dst = merged;
    if (!merged) {
      dst = data[DESTRUCTABLES] = std::make_shared<ObjectData>();
    }

    dst->readSLK(loader.load("Units\\DestructableData.slk"));

    metaData[DESTRUCTABLES] = std::make_shared<MetaData>(loader.load("Units\\DestructableMetaData.slk"));
    dst->readOBJ(loader.load("war3map.w3b"), metaData[DESTRUCTABLES].get(), false, &wts);

    if (!(flags & LOAD_KEEP_METADATA)) {
      metaData[DESTRUCTABLES] = nullptr;
    }
  }

  if (flags & LOAD_DOODADS) {
    auto dst = merged;
    if (!merged) {
      dst = data[DOODADS] = std::make_shared<ObjectData>();
    }

    dst->readSLK(loader.load("Doodads\\Doodads.slk"));

    metaData[DOODADS] = std::make_shared<MetaData>(loader.load("Doodads\\DoodadMetaData.slk"));
    dst->readOBJ(loader.load("war3map.w3d"), metaData[DOODADS].get(), true, &wts);

    if (!(flags & LOAD_KEEP_METADATA)) {
      metaData[DOODADS] = nullptr;
    }
  }

  if (flags & (LOAD_ABILITIES | LOAD_BUFFS)) {
    std::vector<std::shared_ptr<ObjectData>> dstList;
    std::shared_ptr<ObjectData> abilities, buffs;
    if (flags & LOAD_ABILITIES) {
      abilities = merged;
      if (!merged) {
        abilities = data[ABILITIES] = std::make_shared<ObjectData>();
      }
      dstList.push_back(abilities);
    }
    if (flags & LOAD_BUFFS) {
      buffs = merged;
      if (!merged) {
        buffs = data[BUFFS] = std::make_shared<ObjectData>();
      }
      if (buffs != abilities) {
        dstList.push_back(buffs);
      }
    }

    if (abilities) {
      abilities->readSLK(loader.load("Units\\AbilityData.slk"));
    }
    if (buffs) {
      buffs->readSLK(loader.load("Units\\AbilityBuffData.slk"));
    }

    for (auto& dst : dstList) {
      dst->readINI(loader.load("Units\\UndeadAbilityFunc.txt"), true);
      dst->readINI(loader.load("Units\\UndeadAbilityStrings.txt"), true);
      dst->readINI(loader.load("Units\\CampaignAbilityFunc.txt"), true);
      dst->readINI(loader.load("Units\\CampaignAbilityStrings.txt"), true);
      dst->readINI(loader.load("Units\\CommonAbilityFunc.txt"), true);
      dst->readINI(loader.load("Units\\CommonAbilityStrings.txt"), true);
      dst->readINI(loader.load("Units\\HumanAbilityFunc.txt"), true);
      dst->readINI(loader.load("Units\\HumanAbilityStrings.txt"), true);
      dst->readINI(loader.load("Units\\ItemAbilityFunc.txt"), true);
      dst->readINI(loader.load("Units\\ItemAbilityStrings.txt"), true);
      dst->readINI(loader.load("Units\\NeutralAbilityFunc.txt"), true);
      dst->readINI(loader.load("Units\\NeutralAbilityStrings.txt"), true);
      dst->readINI(loader.load("Units\\NightElfAbilityFunc.txt"), true);
      dst->readINI(loader.load("Units\\NightElfAbilityStrings.txt"), true);
      dst->readINI(loader.load("Units\\OrcAbilityFunc.txt"), true);
      dst->readINI(loader.load("Units\\OrcAbilityStrings.txt"), true);
    }

    if (abilities) {
      metaData[ABILITIES] = std::make_shared<MetaData>(loader.load("Units\\AbilityMetaData.slk"));
      abilities->readOBJ(loader.load("war3map.w3a"), metaData[ABILITIES].get(), true, &wts);
    }
    if (buffs) {
      metaData[BUFFS] = std::make_shared<MetaData>(loader.load("Units\\AbilityBuffMetaData.slk"));
      buffs->readOBJ(loader.load("war3map.w3h"), metaData[BUFFS].get(), false, &wts);
    }

    if (!(flags & LOAD_KEEP_METADATA))
    {
      metaData[ABILITIES] = nullptr;
      metaData[BUFFS] = nullptr;
    }
  }

  if (flags & LOAD_UPGRADES) {
    auto dst = merged;
    if (!merged) {
      dst = data[UPGRADES] = std::make_shared<ObjectData>();
    }

    dst->readSLK(loader.load("Units\\UpgradeData.slk"));
    dst->readINI(loader.load("Units\\NightElfUpgradeFunc.txt"));
    dst->readINI(loader.load("Units\\NightElfUpgradeStrings.txt"));
    dst->readINI(loader.load("Units\\OrcUpgradeFunc.txt"));
    dst->readINI(loader.load("Units\\OrcUpgradeStrings.txt"));
    dst->readINI(loader.load("Units\\UndeadUpgradeFunc.txt"));
    dst->readINI(loader.load("Units\\UndeadUpgradeStrings.txt"));
    dst->readINI(loader.load("Units\\CampaignUpgradeFunc.txt"));
    dst->readINI(loader.load("Units\\NeutralUpgradeFunc.txt"));
    dst->readINI(loader.load("Units\\CampaignUpgradeStrings.txt"));
    dst->readINI(loader.load("Units\\NeutralUpgradeStrings.txt"));
    dst->readINI(loader.load("Units\\HumanUpgradeFunc.txt"));
    dst->readINI(loader.load("Units\\HumanUpgradeStrings.txt"));
    metaData[UPGRADES] = std::make_shared<MetaData>(loader.load("Units\\UpgradeMetaData.slk"));
    dst->readOBJ(loader.load("war3map.w3q"), metaData[UPGRADES].get(), true, &wts);

    if (!(flags & LOAD_KEEP_METADATA)) {
      metaData[UPGRADES] = nullptr;
    }
  }
}

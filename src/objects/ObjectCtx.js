import React from 'react';
import { withAsync } from 'utils';
import AppCache from 'data/cache';

export const RawNames = React.createContext(false);
export const SortNames = React.createContext(true);
export const BuildCtx = React.createContext(0);
export const TypeCtx = React.createContext("");
export const IdCtx = React.createContext("");
export const RawNamesSwitch = React.createContext(undefined); 

export const ObjectIcon = ({object}) => (
  <AppCache.DataContext.Consumer>
    {cache => {
      if (!object || object.icon == null) {
        return null;
      }
      const icon = cache.icon(object.icon);
      return <span className="Icon" style={icon}/>;
    }}
  </AppCache.DataContext.Consumer>
);

export function nextComma(str, c) {
  if (c >= str.length) return -1;
  let instr = false;
  for (++c; c < str.length; ++c) {
    if (str[c] === '"') {
      instr = !instr;
    }
    if (str[c] === ',' && !instr) {
      return c;
    }
  }
  return c;
}

export const UpgradeData = {
  "atdb": "Attack Dice Bonus - Base",
  "atdm": "Attack Dice Bonus - Increment",
  "levb": "Ability Level Bonus - Base",
  "levm": "Ability Level Bonus - Increment",
  "levc": "Ability Affected",
  "hpxb": "Hit Point Bonus - Base",
  "hpxm": "Hit Point Bonus - Increment",
  "mnxb": "Mana Point Bonus - Base",
  "mnxm": "Mana Point Bonus - Increment",
  "mvxb": "Movement Speed Bonus - Base",
  "mvxm": "Movement Speed Bonus - Increment",
  "mnrb": "Mana Regeneration Bonus (%) - Base",
  "mnrm": "Mana Regeneration Bonus (%) - Increment",
  "hpob": "Hit Point Bonus (%) - Base",
  "hpom": "Hit Point Bonus (%) - Increment",
  "manb": "Mana Point Bonus (%) - Base",
  "manm": "Mana Point Bonus (%) - Increment",
  "movb": "Movement Speed Bonus (%) - Base",
  "movm": "Movement Speed Bonus (%) - Increment",
  "atxb": "Attack Damage Bonus - Base",
  "atxm": "Attack Damage Bonus - Increment",
  "lumb": "Lumber Harvest Bonus - Base",
  "lumm": "Lumber Harvest Bonus - Increment",
  "atrb": "Attack Range Bonus - Base",
  "atrm": "Attack Range Bonus - Increment",
  "atsb": "Attack Speed Bonus (%) - Base",
  "atsm": "Attack Speed Bonus (%) - Increment",
  "spib": "Spike Damage - Base",
  "spim": "Spike Damage - Increment",
  "hprb": "Hit Point Regeneration Bonus (%) - Base",
  "hprm": "Hit Point Regeneration Bonus (%) - Increment",
  "sigb": "Sight Range Bonus - Base",
  "sigm": "Sight Range Bonus - Increment",
  "atcb": "Attack Target Count Bonus - Base",
  "atcm": "Attack Target Count Bonus - Increment",
  "adlb": "Attack Damage Loss Bonus - Base",
  "adlm": "Attack Damage Loss Bonus - Increment",
  "minb": "Gold Harvest Bonus - Base",
  "minm": "Gold Harvest Bonus - Increment",
  "raib": "Raise Dead Duration Bonus - Base",
  "raim": "Raise Dead Duration Bonus - Increment",
  "entb": "Gold Harvest Bonus (Entangle) - Base",
  "entm": "Gold Harvest Bonus (Entangle) - Increment",
  "enwb": "Attacks to Enable",
  "audb": "Aura Data Bonus - Base",
  "audm": "Aura Data Bonus - Increment",
  "asdb": "Attack Spill Distance Bonus - Base",
  "asdm": "Attack Spill Distance Bonus - Increment",
  "asrb": "Attack Spill Radius Bonus - Base",
  "asrm": "Attack Spill Radius Bonus - Increment",
  "roob": "Attacks to Enable (Rooted)",
  "urob": "Attacks to Enable (Uprooted)",
  "uart": "New Defense Type",
  "utma": "New Availability",
  "ttma": "Unit Type Affected",
};

export const TileSets = {
  "A": "Ashenvale",
  "B": "Barrens",
  "K": "Black Citadel",
  "Y": "Cityscape",
  "X": "Dalaran",
  "J": "Dalaran Ruins",
  "D": "Dungeon",
  "C": "Felwood",
  "I": "Icecrown Glacier",
  "F": "Lordaeron Fall",
  "L": "Lordaeron Summer",
  "W": "Lordaeron Winter",
  "N": "Northrend",
  "O": "Outland",
  "Z": "Sunken Ruins",
  "G": "Underground",
  "V": "Village",
  "Q": "Village Fall",
  "*": "All",
};
export const DestructableCategory = {
  "D": "Trees/Destructibles",
  "P": "Pathing Blockers",
  "B": "Bridges/Ramps",
};
export const DoodadCategory = {
  "O": "Props",
  "S": "Structures",
  "W": "Water",
  "C": "Cliff/Terrain",
  "E": "Environment",
  "Z": "Cinematic",
};
export const TechList = {
  "HERO": "Any Hero",
  "TALT": "Any Altar",
  "TWN1": "Any Tier 1 Hall",
  "TWN2": "Any Tier 2 Hall",
  "TWN3": "Any Tier 3 Hall",
  "TWN4": "Any Tier 4 Hall",
  "TWN5": "Any Tier 5 Hall",
  "TWN6": "Any Tier 6 Hall",
  "TWN7": "Any Tier 7 Hall",
  "TWN8": "Any Tier 8 Hall",
  "TWN9": "Any Tier 9 Hall",
};

export function getString(object, col, index=-1) {
  const text = object.data[col.toLowerCase()] || "";
  if (index < 0) {
    const cmp = nextComma(text, -1);
    if (cmp >= 2 && cmp >= text.length && text[0] === '"' && text[cmp - 1] === '"') {
      return text.substr(1, cmp - 2);
    } else {
      return text;
    }
  } else {
    let pos = -1;
    let cur = 0;
    let prev = 0;
    while ((pos = nextComma(text, pos)) >= 0) {
      if (cur === index) {
        if (pos - prev >= 2 && text[prev] === '"' && text[pos - 1] === '"') {
          return text.substr(prev + 1, pos - prev - 2);
        } else {
          return text.substr(prev, pos - prev);
        }
      }
      prev = pos + 1;
      cur++;
    }
    return "";
  }
}

export function listObjectData(object, data, rawNames, callback) {
  const type = object.type === "item" ? "unit" : object.type;
  const meta = data.meta[type] || [];

  const getName = meta => {
    if (object.type !== "upgrade") return meta.display;
    const m = meta.field.match(/^(base|mod|code)(\d)$/);
    if (!m) return meta.display;
    const effect = object.data[`effect${m[2]}`] || "";
    const m2 = effect.match(/^r([a-z][a-z][a-z])$/);
    if (!m2) return null;
    const name = UpgradeData[m2[1] + m[1][0]];
    if (!name) return null;
    return meta.display.replace("%s", name);
  };

  const getInt = col => {
    col = col.toLowerCase();
    return object.data[col] && parseInt(object.data[col], 10) || 0;
  }

  let use = 0;
  let levels = 0;
  switch (object.type) {
  case "unit":
    if (getInt("isbldg")) {
      use = 4;
    } else if (object.id.match(/^[A-Z]/)) {
      use = 2;
    } else {
      use = 1;
    }
    break;
  case "item":
    use = 8;
    break;
  case "ability":
    levels = getInt("levels");
    if (object.data.code) {
      callback("code", rawNames ? "code" : "Ability Code", object.data.code, {type: "abilCode"});
    }
    break;
  case "doodad":
    levels = getInt("numVar");
    break;
  case "upgrade":
    levels = getInt("maxlevel");
    break;
  default:
  }

  const metaNames = new Map();
  meta.forEach(row => metaNames.set(row, getName(row)));
  meta.filter(row => metaNames.get(row)).sort((a, b) => {
    if ((a.flags & 16) !== (b.flags & 16)) return (a.flags & 16) - (b.flags & 16);
    if (a.category !== b.category) return data.categories[a.category].localeCompare(data.categories[b.category]);
    return metaNames.get(a).localeCompare(metaNames.get(b));
  }).forEach(row => {
    if ((row.flags & use) !== use) {
      return;
    }
    if (row.specific && row.specific.indexOf(object.type === "ability" ? object.data.code : object.id) < 0) {
      return;
    }

    if ((row.flags & 16) && !levels) {
      return;
    }

    let field = row.field;
    if (row.data) {
      field += String.fromCharCode(65 + row.data - 1);
    }
    if (row.flags & 16) {
      for (let i = 0; i < levels; ++i) {
        let fname = field;
        if (row.index < 0) {
          if (row.flags & 32) {
            if (i > 0) fname += i;
          } else {
            fname += (i + 1);
          }
        }
        const value = (row.index < 0 ? getString(object, fname, -1) : getString(object, field, i));
        callback(`${field}${i}`, rawNames ? fname : `Level ${i + 1} - ${data.categories[row.category]} - ${metaNames.get(row)}`, value, row);
      }
    } else {
      const value = getString(object, field, row.index);
      callback(`${field}${row.index}`, rawNames ? field : `${data.categories[row.category]} - ${metaNames.get(row)}`, value, row);
    }
  });
};

const ObjectFilters = {};

ObjectFilters.unit = [
  {
    name: obj => obj.base ? "Custom Units" : "Standard Units",
    order: ["Standard Units", "Custom Units"],
  },
  {
    name: obj => {
      switch (obj.data.race) {
      case "human": return "Human";
      case "orc": return "Orc";
      case "undead": return "Undead";
      case "nightelf": return "Night Elf";
      case "naga": return "Neutral - Naga";
      default:
        if (parseInt(obj.data.hostilepal, 10)) {
          return "Neutral Hostile";
        } else {
          return "Neutral Passive";
        }
      }
    },
    order: ["Human", "Orc", "Undead", "Night Elf", "Neutral - Naga", "Neutral Hostile", "Neutral Passive"],
  },
  {
    name: obj => parseInt(obj.data.campaign, 10) ? "Campaign" : "Melee",
    order: ["Melee", "Campaign"],
  },
  {
    name: obj => {
      if (parseInt(obj.data.special, 10)) {
        return "Special";
      } else if (parseInt(obj.data.isbldg, 10)) {
        return "Buildings";
      } else if (obj.id.match(/^[A-Z]/)) {
        return "Heroes";
      } else {
        return "Units";
      }
    },
    order: ["Units", "Buildings", "Heroes", "Special"],
  },
];

ObjectFilters.item = [
  {
    name: obj => obj.base ? "Custom Items" : "Standard Items",
    order: ["Standard Items", "Custom Items"],
  },
  {
    name: obj => {
      switch (obj.data.class) {
      case "PowerUp": return "Power Up";
      default: return obj.data.class;
      }
    },
    order: ["Permanent", "Charged", "Power Up", "Artifact", "Purchasable", "Campaign", "Miscellaneous"],
  },
];

ObjectFilters.destructible = [
  {
    name: obj => obj.base ? "Custom Destructibles" : "Standard Destructibles",
    order: ["Standard Destructibles", "Custom Destructibles"],
  },
  {
    name: obj => {
      switch (obj.data.category) {
      case "D": return "Trees/Destructibles";
      case "P": return "Pathing Blockers";
      case "B": return "Bridges/Ramps";
      default: return "Others";
      }
    },
    order: ["Trees/Destructibles", "Pathing Blockers", "Bridges/Ramps", "Others"],
  },
];

ObjectFilters.doodad = [
  {
    name: obj => obj.base ? "Custom Doodads" : "Standard Doodads",
    order: ["Standard Doodads", "Custom Doodads"],
  },
  {
    name: obj => {
      switch (obj.data.category) {
      case "O": return "Props";
      case "S": return "Structures";
      case "W": return "Water";
      case "C": return "Cliff/Terrain";
      case "E": return "Environment";
      case "Z": return "Cinematic";
      default: return "Others";
      }
    },
    order: ["Props", "Structures", "Water", "Cliff/Terrain", "Environment", "Cinematic", "Others"],
  },
];

ObjectFilters.ability = [
  {
    name: obj => obj.base ? "Custom Abilities" : "Standard Abilities",
    order: ["Standard Abilities", "Custom Abilities"],
  },
  {
    name: obj => {
      switch (obj.data.race) {
      case "human": return "Human";
      case "orc": return "Orc";
      case "undead": return "Undead";
      case "nightelf": return "Night Elf";
      case "creeps": return "Neutral Hostile";
      case "naga": return "Neutral Passive";
      case "demon": return "Neutral Passive";
      default: return "Special";
      }
    },
    order: ["Human", "Orc", "Undead", "Night Elf", "Neutral Hostile", "Neutral Passive", "Special"],
  },
  {
    name: obj => {
      if (parseInt(obj.data.hero, 10)) {
        return "Heroes";
      } else if (parseInt(obj.data.item, 10)) {
        return "Items";
      } else {
        return "Units";
      }
    },
    order: ["Units", "Heroes", "Items"],
  },
];

ObjectFilters.buff = [
  {
    name: obj => obj.base ? "Custom Buffs/Effects" : "Standard Buffs/Effects",
    order: ["Standard Buffs/Effects", "Custom Buffs/Effects"],
  },
  {
    name: obj => {
      switch (obj.data.race) {
      case "human": return "Human";
      case "orc": return "Orc";
      case "undead": return "Undead";
      case "nightelf": return "Night Elf";
      default: return "Special";
      }
    },
    order: ["Human", "Orc", "Undead", "Night Elf", "Special"],
  },
  {
    name: obj => {
      if (parseInt(obj.data.isEffect, 10)) {
        return "Effect";
      } else {
        return "Buff";
      }
    },
    order: ["Buff", "Effect"],
  },
];

ObjectFilters.upgrade = [
  {
    name: obj => obj.base ? "Custom Upgrades" : "Standard Upgrades",
    order: ["Standard Upgrades", "Custom Upgrades"],
  },
  {
    name: obj => {
      switch (obj.data.race) {
      case "human": return "Human";
      case "orc": return "Orc";
      case "undead": return "Undead";
      case "nightelf": return "Night Elf";
      default: return "Special";
      }
    },
    order: ["Human", "Orc", "Undead", "Night Elf", "Special"],
  },
];

export { ObjectFilters };

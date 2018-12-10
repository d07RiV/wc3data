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
      if (object.icon == null) {
        return null;
      }
      const icon = cache.icon(object.icon);
      return <span className="Icon" style={icon}/>;
    }}
  </AppCache.DataContext.Consumer>
);

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
        if (parseInt(obj.data.hostilePal, 10)) {
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

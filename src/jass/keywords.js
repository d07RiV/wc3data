export default function parseKeywords(meta) {
  const out = {};

  out["globals"] = "jkey";
  out["endglobals"] = "jkey";
  out["local"] = "jkey";
  out["type"] = "jkey";
  out["constant"] = "jkey";
  out["native"] = "jkey";
  out["function"] = "jkey";
  out["takes"] = "jkey";
  out["returns"] = "jkey";
  out["endfunction"] = "jkey";
  out["return"] = "jkey";
  out["set"] = "jkey";
  out["call"] = "jkey";
  out["if"] = "jkey";
  out["then"] = "jkey";
  out["endif"] = "jkey";
  out["else"] = "jkey";
  out["elseif"] = "jkey";
  out["loop"] = "jkey";
  out["endloop"] = "jkey";
  out["exitwhen"] = "jkey";
  out["or"] = "jkey";
  out["and"] = "jkey";
  out["not"] = "jkey";

  out["array"] = "jtyp";
  out["integer"] = "jtyp";
  out["real"] = "jtyp";
  out["string"] = "jtyp";
  out["boolean"] = "jtyp";
  out["code"] = "jtyp";
  out["handle"] = "jtyp";
  out["nothing"] = "jtyp";

  out["null"] = "jcon";

  const common = (meta.loadFile("Scripts\\common.j") || "").split(/[\r\n]/);
  const commonRe = /^\s*(?:constant\s+)?(\w+)\s+(?:array\s+)?(\w+)\b/;
  common.forEach(line => {
    const m = line.match(commonRe);
    if (m) {
      if (m[1] === "native") {
        out[m[2]] = "jnat";
      } else if (m[1] === "type") {
        out[m[2]] = "jtyp";
      } else {
        out[m[2]] = "jcon";
      }
    }
  });

  const blizzard = (meta.loadFile("Scripts\\Blizzard.j") || "").split(/[\r\n]/);
  let inFunc = false, inGlob = false;
  //debugger;
  blizzard.forEach(line => {
    let m;
    if (inFunc) {
      if (line.match(/^\s*endfunction\b/)) {
        inFunc = false;
      }
    } else if ((m = line.match(/^\s*(globals|endglobals)\b/))) {
      inGlob = (m[1] === "globals");
    } else if (inGlob) {
      if ((m = line.match(/^\s*(?:constant\s+)?\w+\s+(?:array\s+)?(\w+)\b/))) {
        out[m[1]] = "jcon";
      }
    } else {
      if ((m = line.match(/^\s*(?:constant\s+)?function\s+(\w+)\b/))) {
        out[m[1]] = "jbjf";
      }
    }
  });

  return out;
}

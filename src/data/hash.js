let cryptTable = null;

export function pathHashTyped(name, hashType = 1) {
  if (!cryptTable) {
    cryptTable = new Uint32Array(1280);
    let seed = 0x00100001;
    for (let i = 0; i < 256; i++) {
      for (let j = i; j < 1280; j += 256) {
        seed = (seed * 125 + 3) % 0x2AAAAB;
        const a = (seed & 0xFFFF) << 16;
        seed = (seed * 125 + 3) % 0x2AAAAB;
        const b = (seed & 0xFFFF);
        cryptTable[j] = a | b;
      }
    }
  }
  let seed1 = 0x7FED7FED;
  let seed2 = 0xEEEEEEEE;
  for (let i = 0; i < name.length; ++i) {
    let ch = name.charCodeAt(i);
    if (ch >= 0x61 && ch <= 0x7A) {
      ch -= 0x20;
    }
    if (ch === 0x2F) {
      ch = 0x5C;
    }
    seed1 = cryptTable[hashType * 256 + ch] ^ (seed1 + seed2);
    seed2 = (ch + seed1 + seed2 * 33 + 3) | 0;
  }
  return seed1;
}

export default function pathHash(name) {
  let m = name.match(/^(.*)\.(?:blp|dds|gif|jpg|jpeg|png|tga)$/i);
  if (m) {
    name = m[1];
  }
  const u32 = new Uint32Array(2);
  u32[0] = pathHashTyped(name, 1);
  u32[1] = pathHashTyped(name, 2);
  return u32;
}

export function makeUid(id) {
  return id[1].toString(16).padStart(8, '0') + id[0].toString(16).padStart(8, '0');
}

export function parseUid(id) {
  const u32 = new Uint32Array(2);
  u32[0] = parseInt(id.substr(8, 8), 16);
  u32[1] = parseInt(id.substr(0, 8), 16);
  if (isNaN(u32[0]) || isNaN(u32[1])) {
    return null;
  }
  return u32;
}

export function equalUid(a, b) {
  if (!a !== !b) return false;
  if (!a) return true;
  return a[0] === b[0] && a[1] === b[1];
}

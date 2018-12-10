let cryptTable = null;

export default function pathHash(name, hashType = 1) {
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

import LoaderBinary from './ArchiveLoader.wasm';
import LoaderModule from './ArchiveLoader.jscc';
import pathHash, { makeUid } from 'data/hash';
import encoding from 'text-encoding';

class ArchiveLoader {
  images_ = {}

  constructor(wasm) {
    this.wasm = wasm;
  }

  fileId(name) {
    if (typeof name === "string") {
      return pathHash(name);
    } else if (name != null && typeof name[0] === "number") {
      return name;
    } else {
      return null;
    }
  }

  hasFile(name) {
    const id = this.fileId(name);
    if (id == null) return false;
    return this.wasm._hasFile(id[0], id[1]) !== 0;
  }

  loadBinary(name) {
    const id = this.fileId(name);
    if (id == null) return null;
    let result = null;
    window.postResult = buf => result = buf;
    const res = this.wasm._loadFile(id[0], id[1], 1);
    delete window.postResult;
    if (!result) {
      return null;
    }
    return {data: result, flags: res};
  }

  loadFile(name) {
    const id = this.fileId(name);
    if (id == null) return null;
    let result = null;
    window.postResult = buf => result = buf;
    this.wasm._loadFile(id[0], id[1]);
    delete window.postResult;
    if (!result) {
      return null;
    }
    return new encoding.TextDecoder().decode(result);
  }

  loadImage(name) {
    const id = this.fileId(name);
    if (id == null) return null;
    const key = makeUid(id);

    if (this.images_[key]) return this.images_[key];

    let result = null;
    window.postResult = buf => result = buf;
    this.wasm._loadImage(id[0], id[1]);
    delete window.postResult;
    if (!result) {
      return null;
    }

    const blob = new Blob([result], {type: "image/png"});
    return this.images_[key] = URL.createObjectURL(blob);
  }

  loadJASS(options) {
    options = options || {};
    const opt = this.wasm._malloc(10);
    const mem = this.wasm.HEAPU8;
    mem[opt + 0] = options.indent || 0;
    mem[opt + 1] = options.restoreInt || 0;
    mem[opt + 2] = options.restoreId || 0;
    mem[opt + 3] = options.insertLines || 0;
    mem[opt + 4] = options.restoreStrings || 0;
    mem[opt + 5] = options.renameGlobals || 0;
    mem[opt + 6] = options.renameFunctions || 0;
    mem[opt + 7] = options.renameLocals || 0;
    mem[opt + 8] = options.inlineFunctions || 0;
    mem[opt + 9] = options.getObjectName || 0;
    let result = null;
    window.postResult = buf => result = buf;
    this.wasm._loadJASS(opt);
    delete window.postResult;
    this.wasm._free(opt);

    if (!result) {
      return null;
    }

    const lines = [];
    const dec = new encoding.TextDecoder();
    let prev = 0;
    for (let i = 0; i < result.length; ++i) {
      if (result[i] === 0) {
        lines.push(dec.decode(result.subarray(prev, i)));
        prev = i + 1;
      }
    }
    return lines;
  }
}

export default function loadArchive(data) {
  const u32 = new Uint32Array(data.buffer || data, 0, 1);
  if (u32[0] !== 0x31585A47) {
    return Promise.reject("Outdated archive format. Try parsing the map again.");
  }
  return LoaderModule({
    locateFile(name) {
      if (name === "ArchiveLoader.wasm") {
        return LoaderBinary;
      } else {
        return name;
      }
    }
  }).ready.then(wasm => {
    const array = new Uint8Array(data);
    const addr = wasm._malloc(array.length);
    wasm.HEAPU8.set(array, addr);
    wasm._openArchive(addr, array.length);
    wasm._free(addr);

    return new ArchiveLoader(wasm);
  });
}

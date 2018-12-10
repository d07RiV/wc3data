import LoaderBinary from './ArchiveLoader.wasm';
import LoaderModule from './ArchiveLoader.jscc';
import pathHash from 'data/hash';

class ArchiveLoader {
  images_ = {}

  constructor(wasm) {
    this.wasm = wasm;
  }

  fileId(name) {
    if (typeof name === "string") {
      return pathHash(name);
    } else if (typeof name === "number") {
      return name | 0;
    } else {
      return null;
    }
  }

  hasFile(name) {
    const id = this.fileId(name);
    if (id == null) return false;
    return this.wasm._hasFile(id) !== 0;
  }

  loadBinary(name) {
    const id = this.fileId(name);
    if (id == null) return null;
    let result = null;
    window.postResult = buf => result = buf;
    const res = this.wasm._loadFile(id, 1);
    delete window.postResult;
    if (!result) {
      return null;
    }
    return {data: result, text: res};
  }

  loadFile(name) {
    const id = this.fileId(name);
    if (id == null) return null;
    let result = null;
    window.postResult = buf => result = buf;
    this.wasm._loadFile(id);
    delete window.postResult;
    if (!result) {
      return null;
    }
    return new TextDecoder().decode(result);
  }

  loadImage(name) {
    const id = this.fileId(name);
    if (id == null) return null;

    if (this.images_[id]) return this.images_[id];

    let result = null;
    window.postResult = buf => result = buf;
    this.wasm._loadImage(id);
    delete window.postResult;
    if (!result) {
      return null;
    }

    const blob = new Blob([result], {type: "image/png"});
    return this.images_[id] = URL.createObjectURL(blob);
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
    const dec = new TextDecoder();
    let prev = 0;
    for (let i = 0; i < result.length; ++i) {
      if (result[i] == 0) {
        lines.push(dec.decode(result.subarray(prev, i)));
        prev = i + 1;
      }
    }
    return lines;
  }
}

export default function loadArchive(data) {
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

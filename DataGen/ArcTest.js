const ArchiveLoader = require("./ArchiveLoader");
const fs = require('fs');

let wasm = null;

global.postResult = function(msg) {
  fs.writeFileSync('./work/war3mapJs.j', new Buffer(msg));
};

ArchiveLoader().ready.then(wasm_ => {
  wasm = wasm_;

  const src = fs.readFileSync('./work/map2.gzx');
  const dst = wasm._malloc(src.length);
  wasm.HEAPU8.set(src, dst);
  wasm._openArchive(dst, src.length);

  const name = "war3map.j";
  const named = Buffer.from(name, 'utf8');
  const ptr = wasm._malloc(named.length + 1);
  wasm.HEAPU8.set(named, ptr);
  wasm.HEAPU8[ptr + named.length] = 0;

  console.log(wasm._loadFileByName(ptr));
});

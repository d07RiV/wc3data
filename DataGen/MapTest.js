const MapParser = require("./MapParser");
const fs = require('fs');

let wasm = null;

global.postMessage = function(msg) {
  if (msg.result) {
    fs.writeFileSync('./work/mapjs.gzx', new Buffer(msg.result));
  } else {
    console.error(msg.error);
  }
};

MapParser().ready.then(wasm_ => {
  wasm = wasm_;

  const src1 = fs.readFileSync('./work/meta.gzx');
  const src2 = fs.readFileSync('./work/map.w3x');
  const dst1 = wasm._malloc(src1.length);
  const dst2 = wasm._malloc(src2.length);
  wasm.HEAPU8.set(src1, dst1);
  wasm.HEAPU8.set(src2, dst2);

  wasm._process(dst1, src1.length, dst2, src2.length);
});

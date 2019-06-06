const fs = require('fs');
const path = require('path');

const out = [];
const names = {};

const compile_flags = isCpp => `${isCpp ? "--std=c++11 " : ""}-O3 -DNO_SYSTEM -DZ_SOLO -I.`;
const link_flags = (memSize, ex) => `-O3 -s WASM=1 -s MODULARIZE=1 -s EXPORTED_FUNCTIONS="['_malloc', '_free']" --post-js ./module-post.js -s ALLOW_MEMORY_GROWTH=1 -s TOTAL_MEMORY=${memSize}${ex ? " -s DISABLE_EXCEPTION_CATCHING=0" : ""}`;

function mkfile(file) {
  const p = path.parse(file);
  const name = `emcc/${p.name}${names[p.name] || ""}.bc`;
  out.push(`call emcc ${file} -o ${name} ${compile_flags(p.ext.toLowerCase() === ".cpp")}`);
  names[p.name] = (names[p.name] || 0) + 1;
  return [name];
}

function mkdir(dir, rec) {
  const res = [];
  fs.readdirSync(dir).forEach(name => {
    const fn = path.join(dir, name), ext = path.extname(name);
    if (fs.lstatSync(fn).isDirectory()) {
      if (rec) res.push(...mkdir(fn, rec));
    } else if ([".c", ".cpp"].indexOf(ext.toLowerCase()) >= 0) {
      res.push(...mkfile(fn));
    }
  });
  return res;
}

const common = [];
const parser = [];
const image = [];

parser.push(...mkdir('datafile'));
parser.push(...mkdir('rmpq', true));
parser.push(...mkfile('utils/json.cpp'));
parser.push(...mkfile('utils/utf8.cpp'));
parser.push(...mkfile('parse.cpp'));
parser.push(...mkfile('search.cpp'));

image.push(...mkdir('image'));
image.push(...mkdir('jpeg/source'));
image.push(...mkfile('jass.cpp'));
image.push(...mkfile('detect.cpp'));
common.push(...mkdir('zlib/source'));
common.push(...mkfile('utils/checksum.cpp'));
common.push(...mkfile('utils/common.cpp'));
common.push(...mkfile('utils/file.cpp'));
common.push(...mkfile('utils/path.cpp'));
common.push(...mkfile('utils/strlib.cpp'));
common.push(...mkfile('hash.cpp'));

const main_parser = mkfile('webmain.cpp');
const main_archive = mkfile('webarc.cpp');

const res = `${out.join("\n")}

call emcc ${[...common, ...parser, ...main_parser].join(" ")} -o MapParser.js -s EXPORT_NAME="MapParser" ${link_flags(134217728, true)}
call emcc ${[...common, ...main_archive, ...image, 'emcc/common.bc'].join(" ")} -o ArchiveLoader.js -s EXPORT_NAME="ArchiveLoader" ${link_flags(33554432, false)}
`;

fs.writeFileSync('build.bat', res);

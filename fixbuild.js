const fs = require('fs');

const dir = 'build/static/css/';
const delim = 'table{border-collapse:collapse;border-spacing:0}';

const names = fs.readdirSync(dir);

const chunk = names.find(n => n.match(/^\d\..*\.chunk\.css$/i));
const main = names.find(n => n.match(/^main\..*\.chunk\.css$/i));

if (!chunk || !main) {
  console.error('Chunk or main not found!');
  process.exit(0);
}

const chdata = fs.readFileSync(dir + chunk, {encoding: "utf-8"});
let mdata = fs.readFileSync(dir + main, {encoding: "utf-8"});
let pos = mdata.indexOf(delim);
if (pos < 0) {
  console.error('Delimiter not found!');
  process.exit(0);
}

pos += delim.length;
mdata = mdata.substr(0, pos) + '\n' + chdata.replace(/\/\*.*?\*\//g, "") + mdata.substr(pos).replace(/\/\*.*?\*\//g, "");

fs.writeFileSync(dir + chunk, '', {encoding: "utf-8"});
fs.writeFileSync(dir + main, mdata, {encoding: "utf-8"});

names.filter(n => n.match(/\.map$/i)).forEach(n => fs.unlinkSync(dir + n));


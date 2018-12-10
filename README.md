Source code for wc3.rivsoft.net

DataGen project is used to generate static data and build WASM scripts for map parsing in browser.

main.cpp can be configured to generate various data files, using the three define's just before the main function.
USE_CDN=1 and GENERATE_META=1 to generate data for the latest version. Then set GENERATE_META=0 and uncomment various build hashes to generate data for other versions. Then USE_CDN=0 for local old MPQ builds if you have any. TEST_MAP=1 can be used to generate prebuilt data archives for maps.

/public/api folder (for development) should have the following files:

* versions.json - slightly different format than what DataGen creates, correct format is like http://wc3.rivsoft.net/api/versions.json
* <version>.json - JSON data for every listed game version
* icons<number>.png - collections of small (16x16) icons
* images.dat - hashes of all icons in the collections
* images/<id>.png - original size images (can be served from images.gzx archive instead)
* meta.gzx - data used for map parsing
  
In src/maps the jscc/wasm files are built by DataGen project using configure.js/build.bat scripts (requires [emscripten](https://kripken.github.io/emscripten-site/docs/getting_started/downloads.html)). jscc extension is used because we need a different loader for these.

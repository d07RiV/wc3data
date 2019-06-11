#include "ngdp/cdnloader.h"
#include "utils/path.h"
#include <memory>
#include <algorithm>
#include <set>
#include "datafile/game.h"
#include "image/image.h"

#include "utils/json.h"
#include "rmpq/archive.h"
#include "utils/logger.h"
#include "icons.h"
#include "hash.h"
#include "jass.h"
#include "parse.h"

#include <windows.h>

File stringify(json::Value const& js, int indent = 0) {
  MemoryFile mfile;
  json::WriterVisitor writer(mfile);
  writer.setIndent(indent);
  js.walk(&writer);
  writer.onEnd();
  mfile.seek(0);
  return mfile;
}

File gzip(File src) {
  uint32 size = src.size();
  std::vector<uint8> srcbuf(size);
  src.seek(0);
  src.read(srcbuf.data(), size);
  size += 6;
  std::vector<uint8> dstbuf(size);
  if (!gzencode(srcbuf.data(), (uint32)srcbuf.size(), dstbuf.data(), &size)) {
    dstbuf.resize(size);
    return MemoryFile(std::move(dstbuf));
  }
  return File();
}

#define WRITE_ALL_IMAGES 0
#define GENERATE_META 0
#define USE_CDN 1
#define GENERATE_MAPS 0
#define TEST_MAP 0
#define NUM_IMAGE_ARCHIVES 8

MemoryFile write_images(std::set<istring> const& names, CompositeLoader& loader) {
  ImageStorage images(16, 16, 16, 16);
#if WRITE_ALL_IMAGES
  HashArchive imarc[NUM_IMAGE_ARCHIVES];
  HashArchive mdxarc;
  File listFile("rootlist.txt", "wb");
#endif
  for (auto fn : Logger::loop(names)) {
    istring ext = path::ext(fn);

    bool isImage = (ext == ".blp" || ext == ".dds" || ext == ".gif" || ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".tga");
    istring canonPath = fn;
    if (isImage) {
      canonPath = path::path(fn) / path::title(fn);
    }
    uint64 hash = pathHash(canonPath.c_str());

    bool isMod = false;
    if (istring(std::string(fn).substr(1, 7)) == istring(".w3mod:") && isalpha((unsigned char) fn[0])) {
      hash = pathHash(canonPath.c_str() + 8) + (fn[0] - 'A' + 1);
      isMod = true;
    }

#if WRITE_ALL_IMAGES
    if (ext == ".mdx" || ext == ".slk" || ext == ".txt" || ext == ".j") {
      File f = loader.load(fn.c_str());
      if (f) {
        mdxarc.add(hash, f, true);
        listFile.printf("%s\n", fn.c_str());
      }
      continue;
    }
#endif
    if (!isImage) {
      continue;
    }
#if !WRITE_ALL_IMAGES
    if (fn.find("replaceabletextures\\") != 0) {
      continue;
    }
#endif
    File f = loader.load(fn.c_str());
    Image img(f);
    if (img) {
      if (fn.find("replaceabletextures\\") == 0) {
        images.add(pathHash(fn.c_str()), img);
      }
#if WRITE_ALL_IMAGES
      File& imgf = imarc[hash % NUM_IMAGE_ARCHIVES].create(hash);
      img.write(imgf);
      listFile.printf("%s\n", fn.c_str());
#endif
    }
  }
#if WRITE_ALL_IMAGES
  for ( size_t i = 0; i < NUM_IMAGE_ARCHIVES; ++i ) {
    imarc[i].write(File(path::root() / fmtstring("images%d.gzx", (int) i), "wb"));
  }
  mdxarc.write(File(path::root() / "files.gzx", "wb"));
#endif
  MemoryFile hashes;
  images.writeHashes(hashes);
  hashes.seek(0);
  return hashes;
}

MemoryFile write_meta(std::set<istring> const& names, CompositeLoader& loader, File icons) {
  HashArchive metaArc;
  metaArc.add("images.dat", icons, false);

  std::set<istring> toLoad;
  for (auto fn : names) {
    istring dir = path::path(fn);
    istring ext = path::ext(fn);
    if ((dir == "ui" || dir == "units" || dir == "doodads") && (ext == ".txt" || ext == ".slk")) {
      toLoad.insert(fn);
    }
  }
  toLoad.insert("Scripts\\common.j");
  toLoad.insert("Scripts\\Blizzard.j");
  for (auto const& fn : Logger::loop(toLoad)) {
    File f = loader.load(fn.c_str());
    if (f) {
      metaArc.add(fn.c_str(), f, true);
    }
  }

  File listFile(path::root() / "listfile.txt");
  if (listFile) {
    metaArc.add("listfile.txt", listFile, true);
  }

  MemoryFile metaFile;
  metaArc.write(metaFile);
  metaFile.seek(0);
  return metaFile;
}

int main() {
  CompositeLoader loader;


#if !TEST_MAP
#if USE_CDN
  auto build = CdnLoader::ngdp().version().build;
  //build = "38f31eb67143d03da05854bfb559ed42"; // 1.30.1.10211
  //build = "34872da6a3842639ff2d2a86ee9b3755"; // 1.30.2.11024
  //build = "e4473116a14ec84d2e00c46af4c3f42f"; // 1.30.2.11029
  //build = "8741363b75f97365ff584fda9d4b804f"; // 1.30.2.11065
  build = "7c45731c22f6bf4ff30035ab9d905745"; // 1.30.4.11274
  // 1.31.0.12071
  auto cdnloader = std::make_shared<CdnLoader>(build);

  //auto mpqloader = std::make_shared<mpq::Archive>(File(R"(G:\Games\Warcraft III\Maps\Download\DotA v6.79c.w3x)"));
  //mpqloader->loadListFile();

  //loader.add(mpqloader);
  loader.add(std::make_shared<PrefixLoader>("War3.w3mod:_Locales\\enUS.w3mod:_Balance\\Custom_V1.w3mod:", cdnloader));
  loader.add(std::make_shared<PrefixLoader>("War3.w3mod:_Locales\\enUS.w3mod:", cdnloader));
  loader.add(std::make_shared<PrefixLoader>("War3.w3mod:_Balance\\Custom_V1.w3mod:", cdnloader));
  loader.add(std::make_shared<PrefixLoader>("War3.w3mod:_Tilesets\\", cdnloader));
  loader.add(std::make_shared<PrefixLoader>("War3.w3mod:", cdnloader));
  //loader.add(std::make_shared<PrefixLoader>("enUS-", cdnloader));
  //loader.add(std::make_shared<PrefixLoader>("enUS-War3Local.mpq:", cdnloader));
  //loader.add(std::make_shared<PrefixLoader>("War3.mpq:", cdnloader));
  loader.add(cdnloader);

  istring tset = "War3.w3mod:_Tilesets\\";

  std::set<istring> names;
  for (auto fn : cdnloader->files()) {
    if (istring(fn.substr(0, tset.size())) == tset) {
      fn = fn.substr(tset.size());
    } else {
      size_t colon = fn.find_last_of(':');
      if (colon != std::string::npos) {
        fn = fn.substr(colon + 1);
      }
    }
    names.insert(fn);
  }

  auto info = cdnloader->buildInfo();
#else
  std::string root = R"(G:\Games\Warcraft III)";
  for (auto ar : {"war3patch.mpq", "war3xLocal.mpq", "war3x.mpq", "war3.mpq"}) {
    auto arc = std::make_shared<mpq::Archive>(File(root / ar));
    loader.add(arc);
  }

  std::set<istring> names;
  {
    File listf(path::root() / "listfile.txt", "rb");
    std::string line;
    while (listf.getline(line)) {
      names.insert(trim(line));
    }
  }

  CdnLoader::BuildInfo info;
  info.build = 7085;
  info.version = "1.27.1.7085";
#endif

#if GENERATE_META
  MemoryFile icons = write_images(names, loader);
  File(path::root() / "images.dat", "wb").copy(icons);
  icons.seek(0);
#else
  File icons(path::root() / "images.dat", "rb");
#endif

  MemoryFile meta = write_meta(names, loader, icons);

#if GENERATE_META
  File(path::root() / "meta.gzx", "wb").copy(meta);
  meta.seek(0);
#endif

  MapParser parser(meta, File());

  auto result = parser.processObjects();
  result.seek(0);
  File(path::root() / fmtstring("%u.json", info.build), "wb").copy(result);
  File(path::root() / fmtstring("%u.json.gz", info.build), "wb").copy(gzip(result));

  json::Value versions;
  json::parse(File(path::root() / "versions.json"), versions);
  versions["versions"][std::to_string(info.build)] = info.version;
  json::write(File(path::root() / "versions.json", "wb"), versions);

  Logger::log("Wrote %s", info.version.c_str());

#if GENERATE_MAPS
  json::Value& mlist = versions["custom"];
  std::vector<std::string> mapnames;
  for (auto fn : names) {
    istring ext = path::ext(fn);
    if (ext == ".w3x" || ext == ".w3m") {
      mapnames.push_back(fn);
    }
  }
  for (auto fn : Logger::loop(mapnames)) {
    auto hash = pathHash(fn.c_str());
    if (File mf = loader.load(fn.c_str())) {
      fn = std::string(fn).substr(0, fn.length() - 4);
      for (auto& c : fn) {
        if (c == '\\' || c == '/') {
          c = '~';
        }
      }

      MapParser parser(meta, mf);
      auto pf = parser.processAll();
      File(fmtstring("maps/%s.gzx", fn.c_str()).c_str(), "wb").copy(pf);

      std::string name = parser.info["name"].getString();
      std::string desc = parser.info["description"].getString();

      auto& mdata = mlist[fmtstring("%016llx", hash)];
      mdata["name"] = name;
      mdata["data"] = fmtstring("maps/%s.gzx", fn.c_str());
      mdata["desc"] = desc;
    }
  }
  json::write(File(path::root() / "versions.json", "wb"), versions);
#endif

#else
  File meta(path::root() / "meta.gzx", "rb");
  File map(path::root() / "war.w3m", "rb");
  MapParser parser(meta, map);

  uint32 t0 = GetTickCount();
  parser.onProgress = [&](unsigned int stage) {
    uint32 t1 = GetTickCount();
    Logger::log("Stage %u - %.3f ms\n", stage, float(t1 - t0) / 1000.0f);
    t0 = t1;
  };

  auto pf = parser.processAll();
  File("map.gzx", "wb").copy(pf);

  //HashArchive arc(File(path::root() / "map.gzx"));
  //jass::Options opt;
  //jass::JASSDo jd(arc, opt);
  //auto mf = jd.process();
  //File(path::root() / "war3map.j", "wb").copy(mf);
#endif

  return 0;
}

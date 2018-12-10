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

MemoryFile write_images(std::set<std::string> const& names, CompositeLoader& loader) {
  ImageStorage images(16, 16, 16, 16);
  HashArchive imarc;
  for (auto fn : Logger::loop(names)) {
    auto ext = path::ext(fn);
    if (ext != ".blp" && ext != ".dds" && ext != ".gif" && ext != ".jpg" && ext != ".jpeg" && ext != ".png" && ext != ".tga") {
      continue;
    }
    File f = loader.load(fn.c_str());
    Image img(f);
    if (img) {
      uint64 hash = pathHash(fn.c_str());
      if (fn.find("replaceabletextures\\") == 0) {
        images.add(hash, img);
      }
      File& imgf = imarc.create(hash);
      img.write(imgf);
      imgf.seek(0);
      File(path::root() / "png" / path::path(fn) / path::title(fn) + ".png", "wb").copy(imgf);
    }
  }
  imarc.write(File(path::root() / "images.gzx", "wb"));
  MemoryFile hashes;
  images.writeHashes(hashes);
  hashes.seek(0);
  return hashes;
}

MemoryFile write_meta(std::set<std::string> const& names, CompositeLoader& loader, File icons) {
  HashArchive metaArc;
  metaArc.add("images.dat", icons, false);

  std::set<std::string> toLoad;
  for (auto fn : names) {
    auto dir = path::path(fn);
    auto ext = path::ext(fn);
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

#define GENERATE_META 0
#define USE_CDN 1
#define TEST_MAP 0

int main() {
  CompositeLoader loader;
  Image img(path::root() / "image.jpg");
  img.write(path::root() / "image.png");

#if !TEST_MAP
#if USE_CDN
  auto build = CdnLoader::ngdp().version().build;
  //build = "38f31eb67143d03da05854bfb559ed42"; // 1.30.1.10211
  //build = "34872da6a3842639ff2d2a86ee9b3755"; // 1.30.2.11024
  //build = "e4473116a14ec84d2e00c46af4c3f42f"; // 1.30.2.11029
  auto cdnloader = std::make_shared<CdnLoader>(build);

  //auto mpqloader = std::make_shared<mpq::Archive>(File(R"(G:\Games\Warcraft III\Maps\Download\DotA v6.79c.w3x)"));
  //mpqloader->loadListFile();

  //loader.add(mpqloader);
  loader.add(std::make_shared<PrefixLoader>("enUS-", cdnloader));
  loader.add(std::make_shared<PrefixLoader>("enUS-War3Local.mpq:", cdnloader));
  loader.add(std::make_shared<PrefixLoader>("War3.mpq:", cdnloader));
  loader.add(cdnloader);

  std::set<std::string> names;
  for (auto fn : cdnloader->files()) {
    size_t colon = fn.find_last_of(':');
    if (colon != std::string::npos) {
      fn = fn.substr(colon + 1);
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

  std::set<std::string> names;
  {
    File listf(path::root() / "listfile.txt", "rb");
    std::string line;
    while (listf.getline(line)) {
      names.insert(strlower(trim(line)));
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
  versions[std::to_string(info.build)] = info.version;
  json::write(File(path::root() / "versions.json", "wb"), versions);

  Logger::log("Wrote %s", info.version.c_str());
#else
  File meta(path::root() / "meta.gzx", "rb");
  File map(path::root() / "map3.w3x", "rb");
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

#include "cdnloader.h"

CdnLoader::CdnLoader(std::string const& build)
  : archives_(ngdp())
{
  File file = ngdp().load(build);
  if (!file) throw Exception("failed to load build %s", build.c_str());
  buildConfig_ = NGDP::ParseConfig(file);

  auto encodingHashes = split(buildConfig_["encoding"]);
  if (encodingHashes.size() != 2) throw Exception("failed to parse build config");
  File rawFile = ngdp().load(encodingHashes[1], "data", false, "Fetching encoding file");
  if (!rawFile) throw Exception("failed to load encoding file");
  encoding_.reset(new NGDP::Encoding(NGDP::DecodeBLTE(rawFile, stoi(split(buildConfig_["encoding-size"])[0]))));
  rawFile.release();

  NGDP::Hash hash;
  NGDP::from_string(hash, buildConfig_["root"]);
  File root = load_(hash);
  if (!root) {
    throw Exception("invalid root file");
  }

  std::string line;
  while (root.getline(line)) {
    auto parts = split(line, '|');
    if (parts.size() >= 2 && parts[1].size() == 32) {
      fixPath_(parts[0]);
      NGDP::from_string(root_[parts[0]]._, parts[1]);
    }
  }
}

void CdnLoader::fixPath_(std::string& path) {
  for (char& chr : path) {
    if (chr == '/') {
      chr = '\\';
    } else {
      //chr = ::tolower(chr);
    }
  }
}

File CdnLoader::load_(const NGDP::Hash hash) {
  auto* entry = encoding_->getEncoding(hash);
  File raw = archives_.load(entry->keys[0]);
  if (!raw) return raw;
  return NGDP::DecodeBLTE(raw, entry->usize);
}

File CdnLoader::load(char const* cpath) {
  std::string path = cpath;
  fixPath_(path);
  auto it = root_.find(path);
  if (it != root_.end()) {
    return load_(it->second._);
  } else {
    return File();
  }
}

CdnLoader::BuildInfo CdnLoader::buildInfo() const {
  std::string const& name = buildConfig_.at("build-name");
  BuildInfo info;
  for (auto const& p : split_multiple(name, "._- ")) {
    if (!p.empty() && std::isdigit((unsigned char)p[0])) {
      uint32 ver = 0, i;
      for (i = 0; i < p.size() && std::isdigit((unsigned char)p[i]); ++i) {
        ver = ver * 10 + (p[i] - '0');
      }
      if (ver >= 10000) {
        info.build = ver;
      } else {
        if (!info.version.empty()) info.version.push_back('.');
        info.version.append(p.substr(0, i));
      }
    }
  }
  if (!info.version.empty()) info.version.push_back('.');
  info.version.append(fmtstring("%d", info.build));
  return info;
}

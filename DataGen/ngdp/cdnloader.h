#pragma once

#include "ngdp.h"

class CdnLoader : public FileLoader {
public:
  CdnLoader(std::string const& build);

  static NGDP::NGDP& ngdp() {
    static NGDP::NGDP ng("w3", "us");
    return ng;
  }

  virtual File load(char const* path) override;

  std::map<std::string, std::string> buildConfig() const {
    return buildConfig_;
  }

  struct BuildInfo {
    uint32 build;
    std::string version;
  };
  BuildInfo buildInfo() const;

  std::vector<std::string> files() const {
    std::vector<std::string> out;
    for (auto const& kv : root_) {
      out.push_back(kv.first);
    }
    return out;
  }

private:
  NGDP::ArchiveIndex archives_;
  std::unique_ptr<NGDP::Encoding> encoding_;

  static void fixPath_(std::string& path);
  Map<NGDP::Hash_container> root_;

  std::map<std::string, std::string> buildConfig_;

  File load_(const NGDP::Hash hash);
};

#pragma once

#include "rmpq/archive.h"
#include <vector>

class FileSearch {
public:
  FileSearch(mpq::Archive& mpq);

  void search();

private:
  mpq::Archive& mpq_;
  std::vector<uint8> states_;
  std::vector<size_t> stack_;

  void addString_(char const* name);
  void addStringEx_(std::string& name);
  void analyzeObj_(char const* name, bool ext);
  void analyzeW3i_(char const* name);
  void analyzeTxt_(size_t pos, bool slk);
  void analyzeJass_(size_t pos);
  void analyzeMdx_(size_t pos);
  void analyze_(size_t pos);
};

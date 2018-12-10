#include "ngdp.h"
#include "utils/http.h"
#include "utils/path.h"
#include "utils/checksum.h"
#include "utils/logger.h"
#include <algorithm>
  
namespace NGDP {

  const std::string HOST = "http://us.patch.battle.net:1119";
  const std::string PROGRAM = "w3";
  const std::string CACHE = "cdncache";

  void preload(char const* task, File& file) {
    static uint8 buffer[1 << 18];
    size_t size = file.size();
    Logger::begin(size, task);
    while (file.tell() < size) {
      file.read(buffer, sizeof buffer);
      Logger::progress(file.tell(), false);
    }
    Logger::end();
    file.seek(0);
  }

  std::map<std::string, CdnData> GetCdns(std::string const& app) {
    std::map<std::string, CdnData> result;
    File f = HttpRequest::get(HOST + "/" + app + "/cdns");
    if (!f) return result;
    bool first = true;
    for (std::string const& line : f) {
      if (line.substr(0, 2) == "##") continue;
      if (first) { first = false; continue; }
      auto parts = split(line, '|');
      auto& config = result[parts[0]];
      config.path = parts[1];
      config.hosts = split(parts[2], ' ');
    }
    return result;
  }
  std::map<std::string, VersionData> GetVersions(std::string const& app) {
    std::map<std::string, VersionData> result;
    File f = HttpRequest::get(HOST + "/" + app + "/versions");
    if (!f) return result;
    bool first = true;
    for (std::string const& line : f) {
      if (line.substr(0, 2) == "##") continue;
      if (first) { first = false; continue; }
      auto parts = split(line, '|');
      auto& config = result[parts[0]];
      config.build = parts[1];
      config.cdn = parts[2];
      config.id = std::stoi(parts[4]);
      config.version = parts[5];
    }
    return result;
  }

  NGDP::NGDP(std::string const& app, std::string const& region) {
    auto cdns = GetCdns(app);
    auto versions = GetVersions(app);
    if (!cdns.count(region) || !versions.count(region)) {
      throw Exception("invalid region");
    }
    base_ = "http://" + cdns[region].hosts[0] + "/" + cdns[region].path + "/";
    version_ = versions[region];
    if (app == "d3") {
      auto v2 = GetVersions("d3t");
      if (v2.count(region)) {
        version_.build2 = v2[region].build;
      }
    }
  }

  std::string NGDP::geturl(std::string const& hash, std::string const& type, bool index) const {
    std::string url = base_ + type + "/" + hash.substr(0, 2) + "/" + hash.substr(2, 2) + "/" + hash;
    if (index) url += ".index";
    return url;
  }
  File NGDP::load(std::string const& hash, std::string const& type, bool index, char const* preload) const {
    std::string path = path::root() / CACHE / type / hash;
    if (index) path += ".index";
    File file(path);
    if (file) return file;
    file = HttpRequest::get(geturl(hash, type, index));
    if (!file) return file;
    if (preload) {
      ::NGDP::preload(preload, file);
    }
    File(path, "wb").copy(file);
    file.seek(0);
    return file;
  }

  File DecodeBLTE(File blte, uint32 eusize) {
    if (blte.read32(true) != 0x424c5445 /*BLTE*/) return File();
    uint32 headerSize = blte.read32(true);
    if (headerSize) {
      std::vector<uint32> csize;
      std::vector<uint32> usize;
      uint16 flags = blte.read16(true);
      uint16 chunks = blte.read16(true);
      for (uint16 i = 0; i < chunks; ++i) {
        csize.push_back(blte.read32(true));
        usize.push_back(blte.read32(true));
        blte.seek(16, SEEK_CUR);
      }
      MemoryFile dst;
      std::vector<uint8> tmp;
      for (uint16 i = 0; i < chunks; ++i) {
        uint8 type = blte.read8();
        if (type == 'N') {
          if (csize[i] - 1 != usize[i]) return File();
          blte.read(dst.alloc(usize[i]), usize[i]);
        } else if (type == 'Z') {
          tmp.resize(csize[i] - 1);
          blte.read(&tmp[0], tmp.size());
          if (gzinflate(&tmp[0], tmp.size(), dst.alloc(usize[i]), &usize[i])) return File();
        } else {
          // unsupported compression
          return File();
        }
      }
      dst.seek(0);
      return dst;
    } else {
      uint8 type = blte.read8();
      uint64 offset = blte.tell();
      uint64 size = blte.size() - offset;
      if (type == 'N') {
        return blte.subfile(offset, size);
      } else if (type == 'Z' && eusize) {
        std::vector<uint8> tmp(size);
        blte.read(&tmp[0], size);
        MemoryFile dst;
        if (gzinflate(&tmp[0], size, dst.alloc(eusize), &eusize)) return File();
        dst.seek(0);
        return dst;
      } else {
        // unsupported compression
        return File();
      }
    }
  }

  std::map<std::string, std::string> ParseConfig(File file) {
    std::map<std::string, std::string> result;
    if (!file) return result;
    for (std::string const& line : file) {
      if (line[0] == '#') continue;
      size_t pos = line.find(" = ");
      if (pos == std::string::npos) continue;
      result[line.substr(0, pos)] = line.substr(pos + 3);
    }
    return result;
  }

#pragma pack(push, 1)
  struct EncodingFileHeader {
    uint16 signature;
    uint8 unk;
    uint8 sizeA;
    uint8 sizeB;
    uint16 flagsA;
    uint16 flagsB;
    uint32 entriesA;
    uint32 entriesB;
    uint8 unk2;
    uint32 stringSize;
  };
#pragma pack(pop)

  void from_string(Hash hash, std::string const& str) {
    int val;
    for (size_t i = 0; i < sizeof(Hash); ++i) {
      sscanf(&str[i * 2], "%02x", &val);
      hash[i] = val;
    }
  }
  std::string to_string(const Hash hash) {
    return MD5::format(hash);
  }

  Encoding::Encoding(File file) {
    EncodingFileHeader header;
    file.read(&header, sizeof header);
    flip(header.signature);
    flip(header.entriesA);
    flip(header.entriesB);
    flip(header.stringSize);
    if (header.signature != 0x454e /*EN*/ || header.sizeA != 16 || header.sizeB != 16) {
      throw Exception("invalid encoding file");
    }

    uint32 size = file.size();
    uint32 posHeaderA = sizeof(EncodingFileHeader) + header.stringSize;
    uint32 posEntriesA = posHeaderA + header.entriesA * 32;
    uint32 posHeaderB = posEntriesA + header.entriesA * 4096;
    uint32 posEntriesB = posHeaderB + header.entriesB * 32;
    uint32 posLayout = posEntriesB + header.entriesB * 4096;

    data_.resize(header.stringSize + (header.entriesA + header.entriesB) * 4096 + (size - posLayout));
    char* layouts = (char*) &data_[0];
    uint8* entriesA = &data_[header.stringSize];
    uint8* entriesB = entriesA + header.entriesA * 4096;
    layout_ = (char*) (entriesB + header.entriesB * 4096);

    file.read(layouts, header.stringSize);
    file.seek(posEntriesA, SEEK_SET);
    file.read(entriesA, header.entriesA * 4096);
    file.seek(posEntriesB, SEEK_SET);
    file.read(entriesB, header.entriesB * 4096);
    file.read(layout_, size - posLayout);

    for (char* ptr = layouts; ptr < layouts + header.stringSize; ++ptr) {
      layouts_.push_back(ptr);
      while (*ptr) ++ptr;
    }

    file.seek(posHeaderA, SEEK_SET);
    encodingTable_.resize(header.entriesA);
    for (uint32 i = 0; i < header.entriesA; ++i) {
      file.read(encodingTable_[i].hash, sizeof(Hash));
      Hash blockHash, realHash;
      file.read(blockHash, sizeof blockHash);
      MD5::checksum(entriesA, 4096, realHash);
      if (memcmp(realHash, blockHash, sizeof(Hash))) {
        throw Exception("encoding file checksum mismatch");
      }
      for (uint8* ptr = entriesA; ptr + sizeof(EncodingEntry) <= entriesA + 4096;) {
        EncodingEntry* entry = reinterpret_cast<EncodingEntry*>(ptr);
        if (!entry->keyCount) break;
        encodingTable_[i].entries.push_back(entry);
        flip(entry->usize);
        ptr += sizeof(EncodingEntry) + (entry->keyCount - 1) * sizeof(Hash);
      }
      entriesA += 4096;
    }

    Hash nilHash;
    memset(nilHash, 0, sizeof(Hash));

    file.seek(posHeaderB, SEEK_SET);
    layoutTable_.resize(header.entriesB);
    for (uint32 i = 0; i < header.entriesB; ++i) {
      file.read(layoutTable_[i].key, sizeof(Hash));
      Hash blockHash, realHash;
      file.read(blockHash, sizeof blockHash);
      MD5::checksum(entriesB, 4096, realHash);
      if (memcmp(realHash, blockHash, sizeof(Hash))) {
        throw Exception("encoding file checksum mismatch");
      }
      for (uint8* ptr = entriesB; ptr + sizeof(LayoutEntry) <= entriesB + 4096;) {
        LayoutEntry* entry = reinterpret_cast<LayoutEntry*>(ptr);
        if (!memcmp(entry->key, nilHash, sizeof(Hash))) {
          break;
        }
        layoutTable_[i].entries.push_back(entry);
        flip(entry->stringIndex);
        flip(entry->csize);
        ptr += sizeof(LayoutEntry);
      }
      entriesB += 4096;
    }
  }

  template<class Vec, class Comp>
  typename Vec::const_iterator find_hash(Vec const& vec, Comp less) {
    if (vec.empty() || less(vec[0])) return vec.end();
    size_t left = 0, right = vec.size();
    while (right - left > 1) {
      size_t mid = (left + right) / 2;
      if (less(vec[mid])) {
        right = mid;
      } else {
        left = mid;
      }
    }
    return vec.begin() + left;
  }

  Encoding::EncodingEntry const* Encoding::getEncoding(const Hash hash) const {
    auto it = find_hash(encodingTable_, [&hash](EncodingHeader const& rhs) {
      return memcmp(hash, rhs.hash, sizeof(Hash)) < 0;
    });
    if (it == encodingTable_.end()) return nullptr;
    auto sub = find_hash(it->entries, [&hash](EncodingEntry const* rhs) {
      return memcmp(hash, rhs->hash, sizeof(Hash)) < 0;
    });
    if (sub == it->entries.end()) return nullptr;
    if (memcmp((*sub)->hash, hash, sizeof(Hash))) return nullptr;
    return *sub;
  }
  Encoding::LayoutEntry const* Encoding::getLayout(const Hash key) const {
    auto it = find_hash(layoutTable_, [&key](LayoutHeader const& rhs) {
      return memcmp(key, rhs.key, sizeof(Hash)) < 0;
    });
    if (it == layoutTable_.end()) return nullptr;
    auto sub = find_hash(it->entries, [&key](LayoutEntry const* rhs) {
      return memcmp(key, rhs->key, sizeof(Hash)) < 0;
    });
    if (sub == it->entries.end()) return nullptr;
    if (memcmp((*sub)->key, key, sizeof(Hash))) return nullptr;
    return *sub;
  }

  ArchiveIndex::ArchiveIndex(NGDP const& ngdp, uint32 blockSize)
    : ngdp_(ngdp)
    , blockSize_(blockSize)
  {
    Hash nilHash;
    memset(nilHash, 0, sizeof(Hash));

    File cdnFile = ngdp.load(ngdp.version().cdn);
    if (!cdnFile) return;
    std::vector<std::string> archives = split(ParseConfig(cdnFile)["archives"]);

    archives_.resize(archives.size());
    Logger::begin(archives.size(), "Loading indices");
    for (size_t i = 0; i < archives.size(); ++i) {
      archives_[i].name = archives[i];

      Logger::item(nullptr);
      File index = ngdp.load(archives[i], "data", true);
      if (!index) continue;
      size_t size = index.size();
      for (size_t block = 0; block + 4096 <= size; block += 4096) {
        Hash hash;
        index.seek(block, SEEK_SET);
        for (size_t pos = 0; pos + 24 <= 4096; pos += 24) {
          index.read(hash, sizeof(Hash));
          if (!memcmp(hash, nilHash, sizeof(Hash))) {
            block = size;
            break;
          }
          auto& dst = index_[Hash_container::from(hash)];
          dst.index = i;
          dst.size = index.read32(true);
          dst.offset = index.read32(true);
        }
      }

      File map(path::root() / CACHE / "data" / archives[i] + ".map");
      if (map) {
        archives_[i].map.resize(map.size() / sizeof(uint32));
        map.read(&archives_[i].map[0], archives_[i].map.size() * sizeof(uint32));
      }
    }
    Logger::end();
  }

  //void ArchiveIndex::convert() {
  //  auto* task = Logger::begin(archives_.size(), "Converting archives");
  //  for (auto const& arch : archives_) {
  //    Logger::item(arch.name.c_str(), task);
  //    if (!File::exists(path::work() / CACHE / "data" / arch.name)) continue;
  //    File src(path::work() / CACHE / "data" / arch.name, "rb");
  //    File dst(path::work() / CACHE / "data2" / arch.name, "wb");
  //    File dstmap(path::work() / CACHE / "data2" / arch.name + ".map", "wb");
  //    size_t size = src.size();
  //    size_t blocks = (size + blockSize_ - 1) / blockSize_;
  //    size_t offset = 0;
  //    for (size_t i = 0; i < blocks; ++i) {
  //      if (arch.mask[i / 8] & (1 << (i & 7))) {
  //        dstmap.write32(offset);
  //        src.seek(i * blockSize_);
  //        size_t curSize = std::min<size_t>(blockSize_, size - i * blockSize_);
  //        dst.copy(src, curSize);
  //        offset += curSize;
  //      } else {
  //        dstmap.write32(0xFFFFFFFF);
  //      }
  //    }
  //  }
  //  Logger::end(false, task);
  //  for (std::string const& name : path::listdir(path::work() / CACHE / "data")) {
  //    char id[256], type[256];
  //    if (sscanf(name.c_str(), "%s.%s", id, type) == 2) {
  //      if (!strcmp(type, "index")) {
  //        File(path::work() / CACHE / "data2" / name, "wb").copy(File(path::work() / CACHE / "data" / name, "rb"));
  //      }
  //    } else if (!File::exists(path::work() / CACHE / "data" / name + ".index")) {
  //      File(path::work() / CACHE / "data2" / name, "wb").copy(File(path::work() / CACHE / "data" / name, "rb"));
  //    }
  //  }
  //}

  File ArchiveIndex::load(Hash const& hash) {
    auto it = index_.find(Hash_container::from(hash));
    if (it == index_.end()) return ngdp_.load(hash, "data");
    std::string archivePath = path::root() / CACHE / "data" / archives_[it->second.index].name;

    uint32 offset = it->second.offset;
    uint32 size = it->second.size;
    uint32 blockStart = offset / blockSize_;
    uint32 blockEnd = (offset + size + blockSize_ - 1) / blockSize_;
    auto& map = archives_[it->second.index].map;
    uint32 getStart = blockEnd;
    uint32 getEnd = blockStart;
    for (uint32 i = blockStart; i < blockEnd; ++i) {
      if (i >= map.size() || map[i] == ArchiveInfo::INVALID) {
        getStart = std::min(getStart, i);
        getEnd = std::max(getEnd, i + 1);
      }
    }
    if (getStart < getEnd) {
      std::string url = ngdp_.geturl(archives_[it->second.index].name, "data");
      HttpRequest request(url);
      request.addHeader(fmtstring("Range: bytes=%u-%u", getStart * blockSize_, getEnd * blockSize_ - 1));
      request.send();
      uint32 status = request.status();
      if (status != 200 && status != 206) return File();
      auto headers = request.headers();
      File result = request.response();
      File archive = File(archivePath, "rb+");
      if (!archive) archive = File(archivePath, "wb+");
      if (headers.count("Content-Range")) {
        uint32 start, end, total, bstart, bend;
        if (sscanf(headers["Content-Range"].c_str(), "bytes %u-%u/%u", &start, &end, &total) != 3) {
          return File();
        }
        bstart = (start + blockSize_ - 1) / blockSize_;
        if (end >= total - 1) {
          bend = (total + blockSize_ - 1) / blockSize_;
        } else {
          bend = (end + 1) / blockSize_;
        }
        map.resize((total + blockSize_ - 1) / blockSize_, ArchiveInfo::INVALID);
        size_t filepos = archive.size();
        archive.seek(filepos, SEEK_SET);
        for (size_t i = bstart; i < bend; ++i) {
          if (map[i] == ArchiveInfo::INVALID) {
            map[i] = filepos;
            size_t size = std::min<size_t>(blockSize_, total - i * blockSize_);
            result.seek(i * blockSize_ - start, SEEK_SET);
            archive.copy(result, size);
            filepos += size;
          }
        }
        File(path::root() / CACHE / "data" / archives_[it->second.index].name + ".map", "wb").write(&map[0], map.size() * sizeof(uint32));
      } else {
        archive.copy(result);
        size_t size = result.size();
        size_t blocks = (size + blockSize_ - 1) / blockSize_;
        map.resize(blocks);
        for (size_t i = 0; i < blocks; ++i) {
          map[i] = i * blockSize_;
        }
        File(path::root() / CACHE / "data" / archives_[it->second.index].name + ".map", "wb").write(&map[0], map.size() * sizeof(uint32));
      }
    }

    File data(archivePath);
    if (!data) return File();
    MemoryFile result;
    uint8* output = result.alloc(size);
    for (uint32 i = blockStart; i < blockEnd; ++i) {
      if (map[i] == ArchiveInfo::INVALID) continue;
      uint32 curstart = std::max<uint32>(offset, i * blockSize_);
      uint32 curend = std::min<uint32>(offset + size, i * blockSize_ + blockSize_);
      data.seek(map[i] + curstart - i * blockSize_, SEEK_SET);
      data.read(output + curstart - offset, curend - curstart);
    }
    result.seek(0);
    return result;
  }

  CascStorage::CascStorage(std::string const& root)
    : root_(root)
  {
    CreateDirectory((root / "config").c_str(), nullptr);
    CreateDirectory((root / "data").c_str(), nullptr);
    CreateDirectory((root / "indices").c_str(), nullptr);
    CreateDirectory((root / "patch").c_str(), nullptr);

    std::vector<std::string> names;
    WIN32_FIND_DATA fdata;
    HANDLE hFind = FindFirstFile((root / "data" / "*").c_str(), &fdata);
    if (hFind == INVALID_HANDLE_VALUE) return;
    do {
      if (!(fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        names.push_back(fdata.cFileName);
      }
    } while (FindNextFile(hFind, &fdata));
    FindClose(hFind);

    for (std::string const& name : names) {
      DeleteFile((root / "data" / name).c_str());
    }
  }

  File& CascStorage::addConfig(std::string const& hash, File& file) {
    file.seek(0);
    File(root_ / "config" / hash.substr(0, 2) / hash.substr(2, 2) / hash, "wb").copy(file);
    file.seek(0);
    return file;
  }
  File& CascStorage::addIndex(std::string const& hash, File& file) {
    file.seek(0);
    File(root_ / "indices" / hash + ".index", "wb").copy(file);
    file.seek(0);
    return file;
  }
  File& CascStorage::addArchive(std::string const& hash, File& file) {
    file.seek(0);
    File(root_ / "tmp" / hash, "wb").copy(file);
    file.seek(0);
    return file;
  }
  File CascStorage::addArchive(std::string const& hash) {
    File res = File(root_ / "tmp" / hash, "rb+");
    if (!res) res = File(root_ / "tmp" / hash, "wb+");
    return res;
  }

  File CascStorage::addData(std::string const& name) {
    return File(root_ / "data" / name, "wb");
  }

  File CascStorage::getIndex(std::string const& hash) {
    return File(root_ / "indices" / hash + ".index");
  }
  File CascStorage::getArchive(std::string const& hash) {
    return File(root_ / "tmp" / hash);
  }

  DataStorage::DataStorage(CascStorage& storage)
    : storage_(storage)
    , indexCount_(0)
    , dataCount_(0)
  {
  }

  File& DataStorage::addFile(const Hash hash, File& file) {
    if (!file) return file;
    if (index_.size() >= MaxIndexEntries) {
      writeIndex();
    }
    if (!data_ || data_.size() + file.size() + 30 > MaxDataSize) {
      data_ = storage_.addData(fmtstring("data.%03u", dataCount_++));
    }
    index_.emplace_back();
    auto& entry = index_.back();
    memcpy(entry.hash, hash, sizeof(Hash));
    entry.index = dataCount_ - 1;
    entry.offset = data_.tell();
    entry.size = 30 + file.size();

    for (int i = 15; i >= 0; --i) {
      data_.write8(hash[i]);
    }
    data_.write32(30 + file.size());
    data_.write16(0);
    data_.write32(0);
    data_.write32(0);
    file.seek(0);
    data_.copy(file);
    file.seek(0);
    return file;
  }

#pragma pack(push, 1)
  struct IndexHeader {
    uint16 version = 7;
    uint8 keyIndex;
    uint8 extraBytes = 0;
    uint8 sizeBytes = 4;
    uint8 offsBytes = 5;
    uint8 keyBytes = 9;
    uint8 segmentBits = 30;
    uint64 maxOffset;
  };
  struct WriteIndexEntry {
    uint8 hash[9];
    uint8 pos[5];
    uint32 size;
  };
#pragma pack(pop)

  void DataStorage::writeIndex() {
    if (index_.empty()) return;

    IndexHeader header;
    header.keyIndex = indexCount_++;
    header.maxOffset = _byteswap_uint64(MaxDataSize);

    File index = storage_.addData(fmtstring("%02x%08x.idx", indexCount_ - 1, 1));
    index.write32(sizeof(IndexHeader));
    index.write32(hashlittle(&header, sizeof header, 0));
    index.write(&header, sizeof header);

    std::sort(index_.begin(), index_.end(), [](IndexEntry const& lhs, IndexEntry const& rhs) {
      return memcmp(lhs.hash, rhs.hash, sizeof(Hash)) < 0;
    });

    // pad
    index.write32(0);
    index.write32(0);

    uint32 blockPos = index.tell();
    uint32 blockSize = 0;
    uint32 blockHash = 0;
    index.write32(blockSize);
    index.write32(blockHash);
    for (IndexEntry const& entry : index_) {
      WriteIndexEntry write;
      memcpy(write.hash, entry.hash, sizeof(write.hash));
      *(uint32*) (write.pos + 1) = _byteswap_ulong(entry.offset);
      write.pos[0] = entry.index / 4;
      write.pos[1] |= ((entry.index & 3) << 6);
      write.size = entry.size;

      index.write(&write, sizeof write);
      blockSize += sizeof(write);
      blockHash = hashlittle(&write, sizeof write, blockHash);
    }

    while (index.tell() & 3) {
      index.write8(0);
    }
    while (index.tell() < 0xA0000) {
      index.write32(0);
    }

    index.seek(blockPos, SEEK_SET);
    index.write32(blockSize);
    index.write32(blockHash);

    index_.clear();
  }
  /*
  void DownloadGame(NGDP const& ngdp, std::string const& build, std::string const& path, std::vector<std::string> const& tags) {
    CascStorage storage(path / "Data");

    printf("fetching configuration...\n");
    auto cdnConfig = ParseConfig(storage.addConfig(ngdp.version().cdn, ngdp.load(ngdp.version().cdn)));
    auto buildConfig = ParseConfig(storage.addConfig(build, ngdp.load(build)));

    printf("downloading %s\n", buildConfig["build-name"].c_str());

    DataStorage data(storage);

    std::string encodingCdnHash = split(buildConfig["encoding"])[1];
    Hash hash;
    from_string(hash, encodingCdnHash);
    printf("fetching encoding file...\n");
    File rawFile = storage.getArchive(encodingCdnHash);
    if (!rawFile) rawFile = storage.addArchive(encodingCdnHash, ngdp.load(encodingCdnHash, "data"));
    Encoding encoding(DecodeBLTE(data.addFile(hash, rawFile)));
    rawFile.release();

    printf("fetching data indices...\n");
    ArchiveIndex index(ngdp, storage, split(cdnConfig["archives"]));

    printf("downloading files...\n");

    from_string(hash, buildConfig["download"]);
    Encoding::EncodingEntry const* enc = encoding.getEncoding(hash);
    rawFile = storage.getArchive(to_string(enc->keys[0]));
    if (!rawFile) rawFile = storage.addArchive(to_string(enc->keys[0]), ngdp.load(enc->keys[0], "data"));
    File download = DecodeBLTE(data.addFile(enc->keys[0], rawFile), enc->usize);
    rawFile.release();

    if (download.read16(true) != 'DL') {
      throw Exception("invalid download file");
    }
    download.seek(3, SEEK_CUR);
    uint32 dlEntries = download.read32(true);
    uint16 dlTags = download.read16(true);
    uint32 dlStart = download.tell();
    download.seek(dlEntries * 26, SEEK_CUR);
    uint32 dlMaskSize = (dlEntries + 7) / 8;
    std::vector<uint8> mask(dlMaskSize, 0xFF);
    std::vector<uint8> tmpMask(dlMaskSize);
    for (uint16 i = 0; i < dlTags; ++i) {
      std::string name;
      while (char c = download.getc()) {
        name.push_back(c);
      }
      uint16 type = download.read16(true);
      download.read(&tmpMask[0], dlMaskSize);
      if (std::find(tags.begin(), tags.end(), name) != tags.end()) {
        for (uint32 j = 0; j < dlMaskSize; ++j) {
          mask[j] &= tmpMask[j];
        }
      }
    }


    uint32 dlMaskedEntries = 0;
    uint64 dlTotalSize = 0;
    download.seek(dlStart, SEEK_SET);
    for (uint32 i = 0; i < dlEntries; ++i) {
      Hash hash;
      download.read(hash, sizeof hash);
      download.seek(10, SEEK_CUR);
      if (!(mask[i / 8] & (1 << (7 - (i & 7))))) continue;

      ++dlMaskedEntries;
      auto const* layout = encoding.getLayout(hash);
      if (layout) dlTotalSize += layout->csize;
    }
      
    printf("0/%d", dlMaskedEntries);

    uint32 dlGotEntries = 0;
    uint32 dlErrors = 0;
    uint64 dlGotSize = 0;
    download.seek(dlStart, SEEK_SET);
    File failed(path / "failed", "wb");
    for (uint32 i = 0; i < dlEntries; ++i) {
      Hash hash;
      download.read(hash, sizeof hash);
      download.seek(10, SEEK_CUR);
      if (!(mask[i / 8] & (1 << (7 - (i & 7))))) continue;

      if (dlErrors) {
        printf("\r%u/%u %I64u/%I64uM [%d failed]", dlGotEntries++, dlMaskedEntries, dlGotSize >> 20, dlTotalSize >> 20, dlErrors);
      } else {
        printf("\r%u/%u %I64u/%I64uM", dlGotEntries++, dlMaskedEntries, dlGotSize >> 20, dlTotalSize >> 20);
      }

      auto const* layout = encoding.getLayout(hash);
      if (layout) dlGotSize += layout->csize;

      File file = index.load(hash);
      if (file) {
        data.addFile(hash, file);
      } else {
        failed.printf("%s\r\n", to_string(hash).c_str());
        ++dlErrors;
      }
    }
    download.release();
    printf(" done\n");

    printf("installing files...\n");

    from_string(hash, buildConfig["install"]);
    enc = encoding.getEncoding(hash);
    rawFile = storage.getArchive(to_string(enc->keys[0]));
    if (!rawFile) rawFile = storage.addArchive(to_string(enc->keys[0]), ngdp.load(enc->keys[0], "data"));
    File install = DecodeBLTE(data.addFile(enc->keys[0], rawFile), enc->usize);
    rawFile.release();

    if (install.read16(true) != 'IN') {
      throw Exception("invalid install file");
    }
    install.seek(2, SEEK_CUR);
    uint16 inTags = install.read16(true);
    uint32 inEntries = install.read32(true);
    uint32 inStart = install.tell();
    uint32 inMaskSize = (inEntries + 7) / 8;
    mask.assign(inEntries, 0xFF);
    tmpMask.resize(inMaskSize);
    for (uint16 i = 0; i < inTags; ++i) {
      std::string name;
      while (char c = install.getc()) {
        name.push_back(c);
      }
      uint16 type = install.read16(true);
      install.read(&tmpMask[0], inMaskSize);
      if (std::find(tags.begin(), tags.end(), name) != tags.end()) {
        for (uint32 j = 0; j < inMaskSize; ++j) {
          mask[j] &= tmpMask[j];
        }
      }
    }

    for (uint32 i = 0; i < inEntries; ++i) {
      std::string name;
      while (char c = install.getc()) {
        name.push_back(c);
      }
      Hash hash;
      install.read(hash, sizeof hash);
      uint32 usize = install.read32(true);

      if (!(mask[i / 8] & (1 << (7 - (i & 7))))) continue;

      enc = encoding.getEncoding(hash);
      if (!enc) {
        printf("file not found: %s\n", name.c_str());
        continue;
      }
      File file = index.load(enc->keys[0]);
      if (!file) {
        printf("file not found: %s\n", name.c_str());
        continue;
      }
      File(path / name, "wb").copy(DecodeBLTE(file, usize));
    }
    install.release();
    printf("done.\n");
  }*/

}

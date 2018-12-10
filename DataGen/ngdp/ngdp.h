#pragma once
#include "utils/common.h"
#include "utils/json.h"
#include "utils/file.h"
#include <unordered_map>

namespace NGDP {

  extern const std::string HOST;
  extern const std::string PROGRAM;

  typedef uint8 Hash[16];
  void from_string(Hash hash, std::string const& str);
  std::string to_string(const Hash hash);

  void preload(char const* task, File& file);

  struct Hash_container {
    Hash _;
    struct hash {
      size_t operator()(Hash_container const& hash) const {
        return *reinterpret_cast<size_t const*>(hash._);
      }
    };
    struct equal {
      bool operator()(Hash_container const& lhs, Hash_container const& rhs) const {
        return !memcmp(lhs._, rhs._, sizeof(Hash));
      }
    };
    static Hash_container const& from(const Hash hash) {
      return *reinterpret_cast<Hash_container const*>(hash);
    }
  };

  struct CdnData {
    std::string path;
    std::vector<std::string> hosts;
  };
  std::map<std::string, CdnData> GetCdns(std::string const& app = PROGRAM);
  struct VersionData {
    std::string build;
    std::string build2;
    std::string cdn;
    uint32 id;
    std::string version;
  };
  std::map<std::string, VersionData> GetVersions(std::string const& app = PROGRAM);

  class NGDP {
  public:
    NGDP(std::string const& app = PROGRAM, std::string const& region = "us");

    VersionData const& version() const {
      return version_;
    }

    std::string geturl(std::string const& hash, std::string const& type = "config", bool index = false) const;
    File load(std::string const& hash, std::string const& type = "config", bool index = false, char const* preload = nullptr) const;
    File load(const Hash hash, std::string const& type = "config", bool index = false, char const* preload = nullptr) const {
      return load(to_string(hash), type, index, preload);
    }

  private:
    std::string base_;
    VersionData version_;
  };

  File DecodeBLTE(File blte, uint32 usize = 0);
  std::map<std::string, std::string> ParseConfig(File file);

  class Encoding {
  public:
    Encoding(File file);

#pragma pack(push, 1)
    struct EncodingEntry {
      uint16 keyCount;
      uint32 usize;
      Hash hash;
      Hash keys[1];
    };
    struct LayoutEntry {
      Hash key;
      uint32 stringIndex;
      uint8 unk;
      uint32 csize;
    };
#pragma pack(pop)

    EncodingEntry const* getEncoding(const Hash hash) const;
    LayoutEntry const* getLayout(const Hash key) const;

    char const* const& layout(uint32 index) const {
      return layouts_[index];
    }
    char const* const& layout() const {
      return layout_;
    }

  private:
    std::vector<uint8> data_;
    struct EncodingHeader {
      Hash hash;
      std::vector<EncodingEntry*> entries;
    };
    struct LayoutHeader {
      Hash key;
      std::vector<LayoutEntry*> entries;
    };
    std::vector<EncodingHeader> encodingTable_;
    std::vector<LayoutHeader> layoutTable_;
    std::vector<char*> layouts_;
    char* layout_;
  };

  class CascStorage {
  public:
    CascStorage(std::string const& root);

    File& addConfig(std::string const& hash, File& file);
    File& addIndex(std::string const& hash, File& file);
    File& addArchive(std::string const& hash, File& file);
    File addArchive(std::string const& hash);

    File addData(std::string const& name);

    File getIndex(std::string const& hash);
    File getArchive(std::string const& hash);

  private:
    std::string root_;
  };

  class ArchiveIndex {
  public:
    ArchiveIndex(NGDP const& ngdp, uint32 blockSize = (1U<<20));

    File load(Hash const& hash);

  private:
    struct IndexEntry {
      uint16 index;
      uint32 size;
      uint32 offset;
    };
    struct ArchiveInfo {
      std::string name;
      std::vector<uint32> map;
      static const uint32 INVALID = 0xFFFFFFFFUL;
    };
    NGDP const& ngdp_;
    uint32 blockSize_;
    std::vector<ArchiveInfo> archives_;
    std::unordered_map<Hash_container, IndexEntry, Hash_container::hash, Hash_container::equal> index_;
  };

  class DataStorage {
  public:
    DataStorage(CascStorage& storage);
    ~DataStorage() {
      finish();
    }

    File& addFile(const Hash hash, File& file); // <- original (compressed) file

    void finish() {
      writeIndex();
    }

  private:
    enum {
      MaxIndexEntries = (0x8E000 - 0x28) / 18,
      MaxDataSize = 0x40000000,
    };
    CascStorage& storage_;
    struct IndexEntry {
      Hash hash;
      uint32 size;
      uint16 index;
      uint32 offset;
    };
    std::vector<IndexEntry> index_;
    File data_;
    uint32 indexCount_;
    uint32 dataCount_;

    void writeIndex();
  };

  void DownloadGame(NGDP const& ngdp, std::string const& build, std::string const& path, std::vector<std::string> const& tags);

}

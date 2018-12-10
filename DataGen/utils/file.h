#pragma once

#include "common.h"
#include <string>
#include <memory>

class FileBuffer {
public:
  virtual ~FileBuffer() {}

  virtual int getc() {
    uint8 chr;
    if (read(&chr, 1) != 1) {
      return EOF;
    }
    return chr;
  }
  virtual void putc(int chr) {
    write(&chr, 1);
  }

  virtual uint64 tell() const = 0;
  virtual void seek(int64 pos, int mode) = 0;
  virtual uint64 size() {
    uint64 pos = tell();
    seek(0, SEEK_END);
    uint64 res = tell();
    seek(pos, SEEK_SET);
    return res;
  }

  virtual size_t read(void* ptr, size_t size) = 0;
  virtual size_t write(void const* ptr, size_t size) = 0;
};

class File {
protected:
  std::shared_ptr<FileBuffer> file_;
public:
  File()
  {}
  File(std::shared_ptr<FileBuffer> const& file)
    : file_(file)
  {}
  File(File const& file)
    : file_(file.file_)
  {}
  File(File&& file)
    : file_(std::move(file.file_))
  {}

#ifndef NO_SYSTEM
  explicit File(char const* name, char const* mode = "rb");
  explicit File(std::string const& name, char const* mode = "rb")
    : File(name.c_str(), mode)
  {}
#endif

  void release() {
    file_.reset();
  }

  File& operator=(File const& file) {
    if (file_ == file.file_) {
      return *this;
    }
    file_ = file.file_;
    return *this;
  }
  File& operator=(File&& file) {
    if (file_ == file.file_) {
      return *this;
    }
    file_ = std::move(file.file_);
    return *this;
  }

  operator bool() const {
    return file_ != nullptr;
  }

  int getc() {
    return file_->getc();
  }
  void putc(int chr) {
    file_->putc(chr);
  }

  void seek(int64 pos, int mode = SEEK_SET) {
    file_->seek(pos, mode);
  }
  uint64 tell() const {
    return file_->tell();
  }
  uint64 size() {
    return file_->size();
  }

  size_t read(void* dst, size_t size) {
    return file_->read(dst, size);
  }
  template<class T>
  T read() {
    T x;
    file_->read(&x, sizeof(T));
    return x;
  }
  uint8 read8() {
    uint8 x;
    file_->read(&x, 1);
    return x;
  }
  uint16 read16(bool big = false) {
    uint16 x;
    file_->read(&x, 2);
    if (big) flip(x);
    return x;
  }
  uint32 read32(bool big = false) {
    uint32 x;
    file_->read(&x, 4);
    if (big) flip(x);
    return x;
  }
  uint64 read64(bool big = false) {
    uint64 x;
    file_->read(&x, 8);
    if (big) flip(x);
    return x;
  }

  size_t write(void const* ptr, size_t size) {
    return file_->write(ptr, size);
  }
  template<class T>
  bool write(T const& x) {
    return file_->write(&x, sizeof(T)) == sizeof(T);
  }
  bool write8(uint8 x) {
    return file_->write(&x, 1) == 1;
  }
  bool write16(uint16 x, bool big = false) {
    if (big) flip(x);
    return file_->write(&x, 2) == 2;
  }
  bool write32(uint32 x, bool big = false) {
    if (big) flip(x);
    return file_->write(&x, 4) == 4;
  }
  bool write64(uint64 x, bool big = false) {
    if (big) flip(x);
    return file_->write(&x, 8) == 8;
  }

  void printf(char const* fmt, ...);

  bool getline(std::string& line);
  bool getwline(std::wstring& line);
  bool getwline_flip(std::wstring& line);

  template<class string_t>
  class LineIterator;

  LineIterator<std::string> begin();
  LineIterator<std::string> end();

  LineIterator<std::wstring> wbegin();
  LineIterator<std::wstring> wend();

  File subfile(uint64 offset, uint64 size);

  void copy(File src, uint64 size = max_uint64);
  void md5(void* digest);

  std::shared_ptr<FileBuffer> buffer() const {
    return file_;
  }

#ifndef NO_SYSTEM
  static bool exists(char const* path);
  static bool exists(std::string const& path) {
    return exists(path.c_str());
  }
#endif
};

class MemoryFile : public File {
public:
  MemoryFile();
  MemoryFile(std::vector<uint8> const& data);
  MemoryFile(std::vector<uint8>&& data);
  MemoryFile(void const* data, size_t size);
  MemoryFile(File const& file);

  static MemoryFile from(File file);

  uint8 const* data() const;
  uint8* alloc(size_t size);
  void resize(size_t size);
};

template<class string_t>
class File::LineIterator {
  friend class File;
  typedef bool(File::*getter_t)(string_t& line);
  File file_;
  string_t line_;
  getter_t getter_ = nullptr;
  LineIterator(File& file, getter_t getter)
    : getter_(getter)
  {
    if ((file.*getter_)(line_)) {
      file_ = file;
    }
  }
public:
  LineIterator() {}

  string_t const& operator*() {
    return line_;
  }
  string_t const* operator->() {
    return &line_;
  }

  bool operator!=(LineIterator const& it) const {
    return file_ != it.file_;
  }

  bool valid() {
    return file_;
  }

  LineIterator& operator++() {
    if (!(file_.*getter_)(line_)) {
      file_.release();
    }
    return *this;
  }
};

class WideFile {
public:
  WideFile(File const& file)
    : file_(file)
  {}

  File::LineIterator<std::wstring> begin() {
    return file_.wbegin();
  }
  File::LineIterator<std::wstring> end() {
    return file_.wend();
  }
private:
  File file_;
};

class Archive {
  struct ArchiveFile {
    // state 1:
    // memFile is working file
    // compressed is empty
    // compression is 1 or 0 depending on whether file needs to be compressed
    // state 2:
    // memFile is null
    // compressed is raw data read from disc
    // compression is decompressed size
    uint32 compression;
    std::vector<uint8> compressed;
    MemoryFile memFile;

    bool compress();
    MemoryFile decompress();
  };
public:
  Archive() {}
  Archive(File file);

  bool has(uint64 id);
  File& create(uint64 id, bool compression = false);
  MemoryFile open(uint64 id);

  void add(uint64 id, File file, bool compression = false);

  void write(File file);

  using iterator = key_iterator<std::map<uint64, ArchiveFile>>;

  iterator begin() const {
    return iterator(files_.begin());
  }
  iterator end() const {
    return iterator(files_.end());
  }

private:
  std::map<uint64, ArchiveFile> files_;
};

class FileLoader {
public:
  virtual ~FileLoader() {};

  virtual File load(char const* path) = 0;
};

#ifndef NO_SYSTEM
class SystemLoader : public FileLoader {
public:
  SystemLoader(std::string const& root = "")
    : root_(root)
  {
  }

  virtual File load(char const* path) override;

private:
  std::string root_;
};
#endif

class PrefixLoader : public FileLoader {
public:
  PrefixLoader(std::string const& prefix, std::shared_ptr<FileLoader> loader)
    : prefix_(prefix)
    , loader_(loader)
  {
  }

  virtual File load(char const* path) override {
    return loader_->load((prefix_ + path).c_str());
  }

private:
  std::string prefix_;
  std::shared_ptr<FileLoader> loader_;
};

class CompositeLoader : public FileLoader {
public:
  void add(std::shared_ptr<FileLoader> loader);
  virtual File load(char const* path) override;

private:
  std::vector<std::shared_ptr<FileLoader>> loaders_;
};

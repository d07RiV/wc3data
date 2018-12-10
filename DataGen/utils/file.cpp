#include "file.h"
#include "path.h"
#include <set>
#include <algorithm>
#include <stdarg.h>

#ifndef NO_SYSTEM
class StdFileBuffer : public FileBuffer {
  FILE* file_;
public:
  StdFileBuffer(FILE* file)
    : file_(file)
  {}
  ~StdFileBuffer() {
    fclose(file_);
  }

  int getc() {
    return fgetc(file_);
  }
  void putc(int chr) {
    fputc(chr, file_);
  }

  uint64 tell() const {
    return ftell(file_);
  }
  void seek(int64 pos, int mode) {
    fseek(file_, pos, mode);
  }

  size_t read(void* ptr, size_t size) {
    return fread(ptr, 1, size, file_);
  }
  size_t write(void const* ptr, size_t size) {
    return fwrite(ptr, 1, size, file_);
  }
};

File::File(char const* name, char const* mode) {
  FILE* file = fopen(name, mode);
  if (!file && (mode[0] == 'w' || mode[0] == 'a')) {
    std::string buf;
    for (int i = 0; name[i]; ++i) {
      char chr = name[i];
      if (chr == '/' || chr == '\\') chr = path::sep;
      buf.push_back(chr);
      if (chr == path::sep) {
        create_dir(buf.c_str());
      }
    }
    file = fopen(name, mode);
  }
  if (file) {
    file_ = std::make_shared<StdFileBuffer>(file);
  }
}
#endif

void File::printf(char const* fmt, ...) {
  static char buf[1024];

  va_list ap;
  va_start(ap, fmt);
  int len = vsnprintf(nullptr, 0, fmt, ap);
  va_end(ap);
  char* dst;
  if (len < 1024) {
    dst = buf;
  } else {
    dst = new char[len + 1];
  }
  va_start(ap, fmt);
  vsnprintf(dst, len + 1, fmt, ap);
  va_end(ap);
  file_->write(dst, len);
  if (dst != buf) {
    delete[] dst;
  }
}
bool File::getline(std::string& out) {
  out.clear();
  int chr;
  while ((chr = file_->getc()) != EOF) {
    if (chr == '\r') {
      char next = file_->getc();
      if (next != '\n' && next != EOF) {
        file_->seek(-1, SEEK_CUR);
      }
      return true;
    }
    if (chr == '\n') {
      return true;
    }
    out.push_back(chr);
  }
  return !out.empty();
}
bool File::getwline(std::wstring& out) {
  out.clear();
  wchar_t chr;
  while (read(&chr, sizeof chr) == 2) {
    if (chr == L'\r') {
      wchar_t next;
      uint64 pos = file_->tell();
      if (read(&next, sizeof next) != 2 || next != L'\n') {
        file_->seek(pos, SEEK_SET);
      }
      return true;
    }
    if (chr == L'\n') {
      return true;
    }
    out.push_back(chr);
  }
  return !out.empty();
}
bool File::getwline_flip(std::wstring& out) {
  out.clear();
  wchar_t chr;
  while (read(&chr, sizeof chr) == 2) {
    flip(chr);
    if (chr == L'\r') {
      wchar_t next;
      uint64 pos = file_->tell();
      if (read(&next, sizeof next) != 2 || flipped(next) != L'\n') {
        file_->seek(pos, SEEK_SET);
      }
      return true;
    }
    if (chr == L'\n') {
      return true;
    }
    out.push_back(chr);
  }
  return !out.empty();
}

File::LineIterator<std::string> File::begin() {
  return LineIterator<std::string>(*this, &File::getline);
}
File::LineIterator<std::string> File::end() {
  return LineIterator<std::string>();
}
File::LineIterator<std::wstring> File::wbegin() {
  int bom = read16();
  if (bom == 0xFEFF) {
    return LineIterator<std::wstring>(*this, &File::getwline);
  } else if (bom == 0xFFFE) {
    return LineIterator<std::wstring>(*this, &File::getwline_flip);
  } else {
    seek(-2, SEEK_CUR);
    return LineIterator<std::wstring>(*this, &File::getwline);
  }
}
File::LineIterator<std::wstring> File::wend() {
  return LineIterator<std::wstring>();
}

class SubFileBuffer : public FileBuffer {
  File file_;
  uint64 start_;
  uint64 end_;
  uint64 pos_;
public:
  SubFileBuffer(File& file, uint64 offset, uint64 size)
    : file_(file)
    , start_(offset)
    , end_(offset + size)
    , pos_(offset)
  {
    file.seek(offset);
  }

  int getc() {
    if (pos_ >= end_) return EOF;
    ++pos_;
    return file_.getc();
  }
  void putc(int chr) {}

  uint64 tell() const {
    return pos_ - start_;
  }
  void seek(int64 pos, int mode) {
    switch (mode) {
    case SEEK_SET:
      pos += start_;
      break;
    case SEEK_CUR:
      pos += pos_;
      break;
    case SEEK_END:
      pos += end_;
      break;
    }
    if (static_cast<uint64>(pos) < start_) pos = start_;
    if (static_cast<uint64>(pos) > end_) pos = end_;
    pos_ = pos;
    file_.seek(pos_);
  }
  uint64 size() {
    return end_ - start_;
  }

  size_t read(void* ptr, size_t size) {
    if (size + pos_ > end_) {
      size = end_ - pos_;
    }
    if (size) {
      size = file_.read(ptr, size);
      pos_ += size;
    }
    return size;
  }
  size_t write(void const* ptr, size_t size) {
    return 0;
  }
};

File File::subfile(uint64 offset, uint64 size) {
  return File(std::make_shared<SubFileBuffer>(*this, offset, size));
}

class MemoryBuffer : public FileBuffer {
  size_t pos_ = 0;
  std::vector<uint8> data_;
public:
  MemoryBuffer()
  {}
  MemoryBuffer(std::vector<uint8> const& data)
    : data_(data)
  {}
  MemoryBuffer(std::vector<uint8>&& data)
    : data_(std::move(data))
  {}

  int getc() {
    return (pos_ < data_.size() ? data_[pos_++] : EOF);
  }
  void putc(int chr) {
    if (pos_ >= data_.size()) {
      data_.push_back(chr);
    } else {
      data_[pos_] = chr;
    }
    pos_++;
  }

  uint64 tell() const {
    return pos_;
  }
  void seek(int64 pos, int mode) {
    switch (mode) {
    case SEEK_CUR:
      pos += pos_;
      break;
    case SEEK_END:
      pos += data_.size();
      break;
    }
    if (pos < 0) pos = 0;
    if (static_cast<size_t>(pos) > data_.size()) pos = data_.size();
    pos_ = pos;
  }
  uint64 size() {
    return data_.size();
  }

  size_t read(void* ptr, size_t size) {
    if (size + pos_ > data_.size()) {
      size = data_.size() - pos_;
    }
    if (size) {
      memcpy(ptr, data_.data() + pos_, size);
      pos_ += size;
    }
    return size;
  }

  size_t write(void const* ptr, size_t size) {
    memcpy(alloc(size), ptr, size);
    return size;
  }

  uint8 const* data() const {
    return data_.data();
  }

  void resize(size_t size) {
    data_.resize(size);
  }

  uint8* alloc(size_t size) {
    if (size + pos_ > data_.size()) {
      data_.resize(size + pos_);
    }
    uint8* result = data_.data() + pos_;
    pos_ += size;
    return result;
  }
};

class RawMemoryBuffer : public FileBuffer {
  uint8 const* data_;
  size_t size_;
  size_t pos_ = 0;
public:
  RawMemoryBuffer(void const* data, size_t size)
    : data_(reinterpret_cast<uint8 const*>(data))
    , size_(size)
  {}

  int getc() {
    return (pos_ < size_ ? data_[pos_++] : EOF);
  }
  void putc(int chr) {};

  uint64 tell() const {
    return pos_;
  }
  void seek(int64 pos, int mode) {
    switch (mode) {
    case SEEK_CUR:
      pos += pos_;
      break;
    case SEEK_END:
      pos += size_;
      break;
    }
    if (pos < 0) pos = 0;
    if (static_cast<size_t>(pos) > size_) pos = size_;
    pos_ = pos;
  }
  uint64 size() {
    return size_;
  }

  size_t read(void* ptr, size_t size) {
    if (size + pos_ > size_) {
      size = size_ - pos_;
    }
    if (size) {
      memcpy(ptr, data_ + pos_, size);
      pos_ += size;
    }
    return size;
  }

  size_t write(void const* ptr, size_t size) {
    return 0;
  }

  uint8 const* data() const {
    return data_;
  }
};

MemoryFile::MemoryFile()
  : File(std::make_shared<MemoryBuffer>())
{}
MemoryFile::MemoryFile(std::vector<uint8> const& data)
  : File(std::make_shared<MemoryBuffer>(data))
{}
MemoryFile::MemoryFile(std::vector<uint8>&& data)
  : File(std::make_shared<MemoryBuffer>(std::move(data)))
{}
MemoryFile::MemoryFile(void const* data, size_t size)
  : File(std::make_shared<RawMemoryBuffer>(data, size))
{}
MemoryFile::MemoryFile(File const& file)
  : File(std::dynamic_pointer_cast<MemoryBuffer>(file.buffer()))
{}

MemoryFile MemoryFile::from(File file) {
  MemoryFile mem(file);
  if (mem) {
    return mem;
  }
  auto pos = file.tell();
  std::vector<uint8> buffer(file.size());
  file.seek(0);
  file.read(buffer.data(), buffer.size());
  mem = MemoryFile(std::move(buffer));
  file.seek(pos);
  mem.seek(pos);
  return mem;
}

uint8 const* MemoryFile::data() const {
  MemoryBuffer* buffer = dynamic_cast<MemoryBuffer*>(file_.get());
  if (buffer) return buffer->data();
  RawMemoryBuffer* rawBuffer = dynamic_cast<RawMemoryBuffer*>(file_.get());
  return (rawBuffer ? rawBuffer->data() : nullptr);
}
uint8* MemoryFile::alloc(size_t size) {
  MemoryBuffer* buffer = dynamic_cast<MemoryBuffer*>(file_.get());
  return (buffer ? buffer->alloc(size) : nullptr);
}
void MemoryFile::resize(size_t size) {
  MemoryBuffer* buffer = dynamic_cast<MemoryBuffer*>(file_.get());
  if (buffer) buffer->resize(size);
}

#include "zlib/zlib.h"

struct ArchiveFile {
  bool compression;
  std::vector<uint8> compressed;
  MemoryFile memFile;
};

bool Archive::ArchiveFile::compress() {
  if (!compression || !memFile) {
    return true;
  }
  uint32 outSize = memFile.size() * 11 / 10 + 6;
  compressed.resize(outSize);
  if (gzencode(memFile.data(), memFile.size(), compressed.data(), &outSize) || outSize >= memFile.size()) {
    std::vector<uint8>().swap(compressed);
    return false;
  }
  compressed.resize(outSize);
  return true;
}

MemoryFile Archive::ArchiveFile::decompress() {
  if (memFile) {
    return memFile;
  }
  MemoryFile out;
  uint32 outSize = compression;
  if (gzdecode(compressed.data(), compressed.size(), out.alloc(compression), &outSize) || outSize != compression) {
    return File();
  }
  return out;
}

bool Archive::has(uint64 id) {
  return files_.count(id) > 0;
}

File& Archive::create(uint64 id, bool compression) {
  auto& f = files_[id];
  f.memFile = MemoryFile();
  std::vector<uint8>().swap(f.compressed);
  f.compression = compression ? 1 : 0;
  return f.memFile;
}

MemoryFile Archive::open(uint64 id) {
  auto it = files_.find(id);
  if (it == files_.end()) {
    return File();
  }
  MemoryFile mf = it->second.decompress();
  mf.seek(0);
  return mf;
}

enum {ARCHIVE_SIGNATURE = 0x31585A47}; // GZX1
#pragma pack(push, 1)
struct ArchiveEntry {
  uint64 id;
  uint32 offset;
  uint32 size;
  uint32 usize;
};
#pragma pack(pop)

Archive::Archive(File file) {
  if (!file) return;
  file.seek(0);
  if (file.read32() != ARCHIVE_SIGNATURE) return;
  uint32 count = file.read32();
  for (uint32 i = 0; i < count; ++i) {
    file.seek(i * sizeof(ArchiveEntry) + 8);
    auto entry = file.read<ArchiveEntry>();
    file.seek(entry.offset);
    auto& f = files_[entry.id];
    if (entry.size < entry.usize) {
      f.memFile = File();
      f.compression = entry.usize;
      f.compressed.resize(entry.size);
      if (file.read(f.compressed.data(), entry.size) != entry.size) {
        files_.erase(entry.id);
      }
    } else {
      f.compression = 0;
      if (file.read(f.memFile.alloc(entry.usize), entry.usize) != entry.usize) {
        files_.erase(entry.id);
      }
    }
  }
}

void Archive::add(uint64 id, File file, bool compression) {
  auto& f = files_[id];
  std::vector<uint8>().swap(f.compressed);
  f.compression = compression ? 1 : 0;
  f.memFile = MemoryFile(file);
  if (!f.memFile) {
    f.memFile = MemoryFile();
    file.seek(0);
    f.memFile.copy(file);
  }
}

void Archive::write(File file) {
  uint32 offset = files_.size() * 16 + 4;
  file.write32(ARCHIVE_SIGNATURE);
  file.write32((uint32) files_.size());
  for (auto& kv : files_) {
    ArchiveEntry entry;
    entry.id = kv.first;
    entry.offset = offset;
    kv.second.compress();
    if (kv.second.compressed.empty()) {
      uint32 size = (uint32)kv.second.memFile.size();
      entry.size = size;
      entry.usize = size;
      offset += size;
    } else {
      uint32 size = (uint32) kv.second.compressed.size();
      uint32 usize = (uint32) kv.second.memFile.size();
      entry.size = size;
      entry.usize = usize;
      offset += size;
    }
    file.write(entry);
  }
  for (auto& kv : files_) {
    if (kv.second.compressed.empty()) {
      file.write(kv.second.memFile.data(), kv.second.memFile.size());
    } else {
      file.write(kv.second.compressed.data(), kv.second.compressed.size());
      if (kv.second.memFile) {
        std::vector<uint8>().swap(kv.second.compressed);
      }
    }
  }
}

void File::copy(File src, uint64 size) {
  auto mem = dynamic_cast<MemoryBuffer*>(src.file_.get());
  if (mem) {
    uint64 pos = mem->tell();
    size = std::min(size, mem->size() - pos);
    write(mem->data() + pos, size);
    mem->seek(size, SEEK_CUR);
  } else {
    uint8 buf[65536];
    while (size_t count = src.read(buf, std::min<size_t>(sizeof buf, size))) {
      write(buf, count);
      size -= count;
    }
  }
}
#include "checksum.h"
void File::md5(void* digest) {
  auto mem = dynamic_cast<MemoryBuffer*>(file_.get());
  if (mem) {
    MD5::checksum(mem->data(), mem->size(), digest);
  } else {
    uint64 pos = tell();
    seek(0, SEEK_SET);
    uint8 buf[65536];
    MD5 checksum;
    while (size_t count = read(buf, sizeof buf)) {
      checksum.process(buf, count);
    }
    checksum.finish(digest);
    seek(pos, SEEK_SET);
  }
}

#ifndef NO_SYSTEM
#include <sys/stat.h>

bool File::exists(char const* path) {
  struct stat buffer;
  return (stat(path, &buffer) == 0);
}

File SystemLoader::load(char const* path) {
  return File(root_ / path);
}
#endif

void CompositeLoader::add(std::shared_ptr<FileLoader> loader) {
  loaders_.push_back(loader);
}
File CompositeLoader::load(char const* path) {
  for (auto& loader : loaders_) {
    File file = loader->load(path);
    if (file) {
      return file;
    }
  }
  return File();
}

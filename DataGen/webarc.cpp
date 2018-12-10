#ifdef NO_SYSTEM
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#define EM_JS(ret, name, args, body) ret name args
#endif

#include "utils/file.h"
#include "image/image.h"
#include "jass.h"
#include "hash.h"
#include <stdio.h>

std::unique_ptr<Archive> archive;

EM_JS(void, write_output, (void const* ptr, int size), {
  window.postResult(HEAPU8.slice(ptr, ptr + size));
});

inline bool valid_cont(uint8 c) {
  return c >= 0x80 && c <= 0xBF;
}

size_t count_print(uint8 const* ptr, size_t size) {
  size_t print = 0;
  for (size_t i = 0; i < size; ++i) {
    uint8 head = ptr[i];
    if (head <= 0x7F) {
      print += ((head >= 0x20 && head <= 0x7E) || head == 0x09 || head == 0x0A || head == 0x0D);
    } else if (head >= 0xC0 && head <= 0xDF) {
      if (i + 1 < size) {
        uint8 next = ptr[i + 1];
        if (valid_cont(next)) {
          uint32 cp = (uint32(head & 0x1F) << 6) | uint32(next & 0x3F);
          print += (cp >= 0xA0);
          i += 1;
        }
      }
    } else if (head >= 0xE0 && head <= 0xEF) {
      if (i + 2 < size && valid_cont(ptr[i + 1]) && valid_cont(ptr[i + 2])) {
        print += 1;
        i += 2;
      }
    } else if (head >= 0xF0 && head <= 0xF7) {
      if (i + 3 < size && valid_cont(ptr[i + 1]) && valid_cont(ptr[i + 2]) && valid_cont(ptr[i + 3])) {
        print += 1;
        i += 3;
      }
    }
  }
  return print;
}

extern "C" {
  EMSCRIPTEN_KEEPALIVE void openArchive(void const* data_ptr, int data_size) {
    archive.reset(new Archive(MemoryFile(data_ptr, data_size)));
  }
  EMSCRIPTEN_KEEPALIVE int hasFile(unsigned int id) {
    return archive->has(id) ? 1 : 0;
  }
  EMSCRIPTEN_KEEPALIVE int loadFile(unsigned int id, int checkText) {
    MemoryFile file = archive->open(id);
    if (file) {
      uint8 const* ptr = file.data();
      size_t size = file.size();
      write_output(ptr, size);
      if (checkText) {
        size_t print = count_print(ptr, size);
        return (size - print) < size / 20;
      }
      return 1;
    }
    return 0;
  }
  EMSCRIPTEN_KEEPALIVE int loadFileByName(char const* name) {
    return loadFile(pathHash(name), 0);
  }
  EMSCRIPTEN_KEEPALIVE int loadImage(unsigned int id) {
    Image image(archive->open(id));
    if (image) {
      MemoryFile file;
      image.write(file);
      write_output(file.data(), file.size());
      return 1;
    }
    return 0;
  }
  EMSCRIPTEN_KEEPALIVE int loadImageByName(char const* name) {
    return loadImage(pathHash(name));
  }

  EMSCRIPTEN_KEEPALIVE int loadJASS(void const* options) {
    jass::JASSDo jd(*archive, *(jass::Options*)options);
    if (auto mf = jd.process()) {
      write_output(mf.data(), mf.size());
      return 1;
    } else {
      return 0;
    }
  }
}

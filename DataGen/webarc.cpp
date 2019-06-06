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
#include "detect.h"
#include <stdio.h>

std::unique_ptr<HashArchive> archive;

EM_JS(void, write_output, (void const* ptr, int size), {
  window.postResult(HEAPU8.slice(ptr, ptr + size));
});

extern "C" {
  EMSCRIPTEN_KEEPALIVE void openArchive(void const* data_ptr, uint32 data_size) {
    archive.reset(new HashArchive(MemoryFile(data_ptr, data_size)));
  }
  EMSCRIPTEN_KEEPALIVE int hasFile(uint32 id1, uint32 id2) {
    return archive->has(mpq::hashTo64(id1, id2)) ? 1 : 0;
  }
  EMSCRIPTEN_KEEPALIVE int loadFile(uint32 id1, uint32 id2, int checkText) {
    MemoryFile file = archive->open(mpq::hashTo64(id1, id2));
    if (file) {
      uint8 const* ptr = file.data();
      size_t size = file.size();
      write_output(ptr, size);
      if (checkText) {
        int flags = 0;
        if (is_text(ptr, size)) flags |= 1;
        if (is_wave(ptr, size)) flags |= 2;
        if (is_mp3(ptr, size)) flags |= 4;
        if (is_mdx(ptr, size)) flags |= 8;
        return flags;
      }
      return 1;
    }
    return 0;
  }
  EMSCRIPTEN_KEEPALIVE int loadImage(uint32 id1, uint32 id2) {
    Image image(archive->open(mpq::hashTo64(id1, id2)));
    if (image) {
      MemoryFile file;
      image.write(file);
      write_output(file.data(), file.size());
      return 1;
    }
    return 0;
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

#ifdef NO_SYSTEM
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#define EM_JS(ret, name, args, body) ret name args;
#endif
#include "utils/file.h"
#include "parse.h"

EM_JS(void, write_progress, (unsigned int value), {
  self.postMessage({ progress: value });
  });
EM_JS(void, write_output, (void const* ptr, int size), {
  self.postMessage({ result: HEAPU8.slice(ptr, ptr + size) });
  });
EM_JS(void, write_error, (char const* err), {
  //const end = HEAPU8.indexOf(0, err);
  //const text = new TextDecoder().decode(HEAPU8.subarray(err, end));
  //const text = (new Buffer(HEAPU8.subarray(err, end))).toString('utf8');
  self.postMessage({ error: HEAPU8.subarray(err, end) });
});

extern "C" {
  EMSCRIPTEN_KEEPALIVE void process(void const* data_ptr, int data_size, void const* map_ptr, int map_size) {
    try {
      MemoryFile data((uint8*)data_ptr, (size_t)data_size);
      File map;
      if (map_ptr) {
        map = MemoryFile((uint8*)map_ptr, (size_t)map_size);
      }

      MapParser parser(data, map);
      parser.onProgress = write_progress;
      auto mf = parser.processAll();
      write_output(mf.data(), (int)mf.size());
    }
    catch (Exception e) {
      write_error(e.what());
    }
  }
}

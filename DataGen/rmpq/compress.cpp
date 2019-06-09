#include "rmpq/common.h"
#include "zlib/zlib.h"
#include "huffman/huff.h"
#include "pklib/pklib.h"
#include "adpcm/adpcm.h"
#include <algorithm>

namespace mpq {

bool zlib_compress(void* in, size_t in_size, void* out, size_t* out_size) {
  uint32 out32 = *out_size;
  if (gzencode((uint8*)in, in_size, (uint8*)out, &out32)) {
    return false;
  }
  *out_size = out32;
  return true;
}

bool zlib_decompress(void* in, size_t in_size, void* out, size_t* out_size) {
  uint32 out32 = *out_size;
  if (gzinflate((uint8*)in, in_size, (uint8*)out, &out32)) {
    return false;
  }
  *out_size = out32;
  return true;
}

struct BUFFERINFO {
  void* out;
  size_t outPos;
  size_t outLen;
  void* in;
  size_t inPos;
  size_t inLen;

  static unsigned int input(char* buffer, unsigned int* size, void* param) {
    BUFFERINFO* bi = (BUFFERINFO*)param;
    size_t bufSize = std::min<size_t>(*size, bi->inLen - bi->inPos);
    memcpy(buffer, (char*)bi->in + bi->inPos, bufSize);
    bi->inPos += bufSize;
    return bufSize;
  }

  static void output(char* buffer, unsigned int* size, void* param) {
    BUFFERINFO* bi = (BUFFERINFO*)param;
    size_t bufSize = std::min<size_t>(*size, bi->outLen - bi->outPos);
    memcpy((char*)bi->out + bi->outPos, buffer, bufSize);
    bi->outPos += bufSize;
  }
};

bool pkzip_compress(void* in, size_t in_size, void* out, size_t* out_size) {
  std::vector<char> buf(CMP_BUFFER_SIZE);

  BUFFERINFO bi;
  bi.in = in;
  bi.inPos = 0;
  bi.inLen = in_size;
  bi.out = out;
  bi.outPos = 0;
  bi.outLen = *out_size;

  unsigned int compType = CMP_BINARY;
  unsigned int dictSize;
  if (in_size >= 0xC00) {
    dictSize = 0x1000;
  } else if (in_size < 0x600) {
    dictSize = 0x400;
  } else {
    dictSize = 0x800;
  }

  implode(BUFFERINFO::input, BUFFERINFO::output, buf.data(), &bi, &compType, &dictSize);
  *out_size = bi.outPos;

  return true;
}

bool pkzip_decompress(void* in, size_t in_size, void* out, size_t* out_size) {
  std::vector<char> buf(CMP_BUFFER_SIZE);

  BUFFERINFO bi;
  bi.in = in;
  bi.inPos = 0;
  bi.inLen = in_size;
  bi.out = out;
  bi.outPos = 0;
  bi.outLen = *out_size;

  unsigned int compType = CMP_BINARY;
  unsigned int dictSize;
  if (in_size >= 0xC00) {
    dictSize = 0x1000;
  } else if (in_size < 0x600) {
    dictSize = 0x400;
  } else {
    dictSize = 0x800;
  }

  explode(BUFFERINFO::input, BUFFERINFO::output, buf.data(), &bi);
  *out_size = bi.outPos;

  return true;
}

bool wave_compress_mono(void* in, size_t in_size, void* out, size_t* out_size) {
  *out_size = CompressADPCM(out, *out_size, in, in_size, 1, 5);
  return true;
}

bool wave_decompress_mono(void* in, size_t in_size, void* out, size_t* out_size) {
  *out_size = DecompressADPCM(out, *out_size, in, in_size, 1);
  return true;
}

bool wave_compress_stereo(void* in, size_t in_size, void* out, size_t* out_size) {
  *out_size = CompressADPCM(out, *out_size, in, in_size, 2, 5);
  return true;
}

bool wave_decompress_stereo(void* in, size_t in_size, void* out, size_t* out_size) {
  *out_size = DecompressADPCM(out, *out_size, in, in_size, 2);
  return true;
}

bool huff_compress(void* in, size_t in_size, void* out, size_t* out_size) {
  THuffmannTree ht(true);
  TOutputStream os(out, *out_size);
  *out_size = ht.Compress(&os, in, in_size, 7);
  return true;
}

bool huff_decompress(void* in, size_t in_size, void* out, size_t* out_size) {
  THuffmannTree ht(false);
  TInputStream is(in, in_size);
  *out_size = ht.Decompress(out, *out_size, &is);
  return true;
}

struct CompressionType {
  uint8 id;
  bool(*func) (void* in, size_t in_size, void* out, size_t* out_size);
};
static CompressionType decomp_table[] = {
  //  {0x10, bzip2_decompress},
  { 0x08, pkzip_decompress },
  { 0x02, zlib_decompress },
  { 0x01, huff_decompress },
  { 0x80, wave_decompress_stereo },
  { 0x40, wave_decompress_mono }
};

bool multi_decompress(void* in, size_t in_size, void* out, size_t* out_size, void* temp) {
  if (in_size == *out_size) {
    if (in != out) {
      memcpy(out, in, in_size);
    }
    return true;
  }

  uint8 method = *(uint8*)in;
  void* in_ptr = (uint8*)in + 1;
  in_size--;

  uint8 cur_method = method;
  size_t func_count = 0;
  for (auto& met : decomp_table) {
    if ((cur_method & met.id) == met.id) {
      func_count++;
      cur_method &= ~met.id;
    }
  }
  if (cur_method != 0) {
    return false;
  }

  if (func_count == 0) {
    if (in_size > *out_size) {
      return false;
    }
    if (in != out) {
      memcpy(out, in_ptr, in_size);
    }
    *out_size = in_size;
    return true;
  }

  std::vector<uint8> temp2;
  void* tmp = temp;
  if (!tmp) {
    temp2.resize(*out_size);
    tmp = temp2.data();
  }

  size_t cur_size = in_size;
  void* cur_ptr = in_ptr;
  void* cur_out = ((func_count & 1) && (out != in) ? out : tmp);
  cur_method = method;
  for (auto& met : decomp_table) {
    if ((cur_method & met.id) == met.id) {
      size_t size = *out_size;
      if (!met.func(cur_ptr, cur_size, cur_out, &size)) {
        return false;
      }
      cur_size = size;
      cur_ptr = cur_out;
      cur_out = (cur_out == tmp ? out : tmp);
      cur_method &= ~met.id;
    }
  }
  if (cur_ptr != out) {
    memcpy(out, cur_ptr, cur_size);
  }
  *out_size = cur_size;
  return true;
}

}

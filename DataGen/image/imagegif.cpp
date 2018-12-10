#include "image.h"
#include "utils/common.h"
#include <stdlib.h>

#define MAX_CODES 4095

#pragma pack(push, 1)
struct GifHeader {
  char sign[6];
  sint16 screenWidth;
  sint16 screenHeight;
  sint8 misc;
  sint8 back;
  sint8 null;
};
struct GifImageHeader {
  sint8 sign;
  sint16 x, y;
  sint16 width, height;
  sint8 misc;
};

struct Rgb {
  uint8 red;
  uint8 green;
  uint8 blue;
};

struct Rgba {
  uint8 red;
  uint8 green;
  uint8 blue;
  uint8 alpha;
};
#pragma pack(pop)

struct GifDecoder {
  sint32 codeSize;
  sint32 clearCode;
  sint32 endingCode;
  sint32 freeCode;
  sint32 topSlot;
  sint32 slot;
  uint8 b1;
  uint8 buf[257];
  uint8* pbytes;
  sint32 bytesInBlock;
  sint32 bitsLeft;
  sint32 codeMask[13];
  uint8 stack[MAX_CODES + 1];
  uint8 suffix[MAX_CODES + 1];
  sint32 prefix[MAX_CODES + 1];
  GifDecoder() {
    bytesInBlock = 0;
    bitsLeft = 0;
    for (int i = 0; i < 13; i++) {
      codeMask[i] = (1 << i) - 1;
    }
  }

  int loadGifBlock(File& f) {
    pbytes = buf;

    bytesInBlock = f.getc();
    if (bytesInBlock == EOF) {
      return -1;
    }
    if (bytesInBlock > 0 && f.read(buf, bytesInBlock) != bytesInBlock) {
      return -1;
    }
    return bytesInBlock;
  }
  int getGifCode(File& f) {
    if (bitsLeft == 0) {
      if (bytesInBlock <= 0 && loadGifBlock(f) == -1) {
        return -1;
      }
      b1 = *pbytes++;
      bitsLeft = 8;
      bytesInBlock--;
    }
    int ret = b1 >> (8 - bitsLeft);
    while (codeSize > bitsLeft) {
      if (bytesInBlock <= 0 && loadGifBlock(f) == -1) {
        return -1;
      }
      b1 = *pbytes++;
      ret |= b1 << bitsLeft;
      bitsLeft += 8;
      bytesInBlock--;
    }
    bitsLeft -= codeSize;
    ret &= codeMask[codeSize];

    return ret;
  }
};

namespace ImagePrivate {
  bool imReadGIF(Image& image, File& file) {
    file.seek(0, SEEK_SET);
    GifHeader hdr;
    GifDecoder gif;

    if (file.read(&hdr, sizeof hdr) != sizeof hdr) {
      return false;
    }
    if (strncmp(hdr.sign, "GIF87a", 6) && strncmp(hdr.sign, "GIF89a", 6)) {
      return false;
    }
    int bitsPerPixel = 1 + (7 & hdr.misc);
    std::vector<Rgb> pal;
    if (hdr.misc & 0x80) {
      pal.resize(1 << bitsPerPixel);
      if (file.read(pal.data(), sizeof(Rgb) * pal.size()) != sizeof(Rgb) * pal.size()) {
        return false;
      }
    }
    GifImageHeader imageHdr;
    if (file.read(&imageHdr, sizeof imageHdr) != sizeof imageHdr) {
      return false;
    }
    while (imageHdr.sign == '!') {
      int extType = *(1 + (char*) &imageHdr);
      int size = *(2 + (char*) &imageHdr);
      file.seek(size + 4 - sizeof imageHdr, SEEK_CUR);
      if (file.read(&imageHdr, sizeof imageHdr) != sizeof imageHdr) {
        return false;
      }
    }
    if (imageHdr.sign != ',') {
      return false;
    }
    if (imageHdr.misc & 0x80) {
      bitsPerPixel = 1 + (imageHdr.misc & 7);
      pal.resize(1 << bitsPerPixel);
      if (file.read(pal.data(), sizeof(Rgb) * pal.size()) != sizeof(Rgb) * pal.size()) {
        return false;
      }
    }
    if (pal.empty()) {
      return false;
    }

    int size = file.getc();
    if (size < 2 || size > 9) {
      return false;
    }

    std::vector<uint8> data(imageHdr.width, imageHdr.height);
    uint8* ptr = data.data();
    uint8* end = data.data() + data.size();

    gif.codeSize = size + 1;
    gif.topSlot = 1 << gif.codeSize;
    gif.clearCode = 1 << size;
    gif.endingCode = gif.clearCode + 1;
    gif.slot = gif.freeCode = gif.endingCode + 1;
    gif.bytesInBlock = gif.bitsLeft = 0;

    uint8* sp;
    sint32 code;
    sint32 c;
    sint32 oc = 0;
    sint32 fc = 0;
    sp = gif.stack;
    while ((c = gif.getGifCode(file)) != gif.endingCode && ptr < end) {
      if (c < 0) {
        return false;
      }
      if (c == gif.clearCode) {
        gif.codeSize = size + 1;
        gif.slot = gif.freeCode;
        gif.topSlot = 1 << gif.codeSize;
        while ((c = gif.getGifCode(file)) == gif.clearCode) {
          // nothing
        }
        if (c == gif.endingCode) {
          break;
        }
        if (c >= gif.slot) {
          c = 0;
        }
        oc = fc = c;
        *ptr++ = c;
      } else {
        code = c;
        if (code >= gif.slot) {
          if (code > gif.slot) {
            return false;
          }
          code = oc;
          *sp++ = fc;
        }
        while (code >= gif.freeCode) {
          *sp++ = gif.suffix[code];
          code = gif.prefix[code];
        }
        *sp++ = code;
        if (gif.slot < gif.topSlot) {
          gif.suffix[gif.slot] = fc = code;
          gif.prefix[gif.slot++] = oc;
          oc = c;
        }
        if (gif.slot >= gif.topSlot) {
          if (gif.codeSize < 12) {
            gif.topSlot <<= 1;
            gif.codeSize++;
          }
        }
        while (sp > gif.stack && ptr < end) {
          *ptr++ = *--sp;
        }
      }
    }

    image = Image(imageHdr.width, imageHdr.height);
    ptr = data.data();
    for (auto& clr : image) {
      auto& p = pal[*ptr++];
      clr = Image::color_t(p.red, p.green, p.blue);
    }

    return true;
  }
}

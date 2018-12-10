#include <stdlib.h>
#include "image.h"
//#include <intrin.h>
#include <vector>

namespace _blp2 {

  struct BLP2Header {
    uint32 id;
    uint32 type;
    uint8 encoding;
    uint8 alphaDepth;
    uint8 alphaEncoding;
    uint8 hasMips;
    uint32 width;
    uint32 height;
    uint32 offsets[16];
    uint32 lengths[16];
    uint32 pal[256];
  };

  bool load_raw(uint8* src, uint32 length, Image::color_t* data, BLP2Header const& hdr) {
    uint32 alphaBits = 0;
    uint32 dim = hdr.width * hdr.height;
    if (hdr.alphaDepth == 1) {
      alphaBits = (dim + 7) / 8;
    } else if (hdr.alphaDepth == 8) {
      alphaBits = dim;
    }
    if (length != dim + alphaBits) {
      return false;
    }
    Image::color_t* cur = data;
    for (uint32 i = 0; i < dim; i++) {
      *cur++ = hdr.pal[*src++] & 0x00FFFFFF;
    }
    if (hdr.alphaDepth == 0) {
      for (uint32 i = 0; i < dim; i++) {
        *data++ |= 0xFF000000;
      }
    } else if (hdr.alphaDepth == 1) {
      for (uint32 i = 0; i < dim; i += 8) {
        uint8 alpha = *src++;
        for (uint32 b = 0; b < 8 && i + b < dim; b++) {
          if (alpha & (1 << b)) {
            *data |= 0xFF000000;
          } else {
            *data = 0;
          }
          data++;
        }
      }
    } else if (hdr.alphaDepth == 8) {
      for (uint32 i = 0; i < dim; i++) {
        data[i] = (data[i] & 0xFFFFFF) | (uint32(*src++) << 24);
      }
    }
    return true;
  }

  typedef Color::XRGB<0, 5, 6, 5> FormatDXT;

  bool load_dxt1(uint8* src, uint32 length, Image::color_t* bits, BLP2Header const& hdr) {
    if (hdr.width < 4 || hdr.height < 4) return false;
    if (length != hdr.width * hdr.height / 2) return false;
    for (uint32 y = 0; y < hdr.height; y += 4) {
      for (uint32 x = 0; x < hdr.width; x += 4) {
        FormatDXT p = *(uint16*)src;
        FormatDXT q = *(uint16*)(src + 2);
        uint32 lookup = *(uint32*)(src + 4);
        src += 8;
        Image::color_t c[4] = { p, q };
        if (p > q) {
          c[2] = Color::mix(c[0], 2, c[1], 1);
          c[3] = Color::mix(c[0], 1, c[1], 2);
        } else {
          c[2] = Color::mix(c[0], 1, c[1], 1);
          c[3] = 0;
        }
        for (uint32 cy = y; cy < y + 4; ++cy) {
          Image::color_t* dst = bits + cy * hdr.width + x;
          for (uint32 cx = x; cx < x + 4; ++cx) {
            *dst++ = c[lookup & 3];
            lookup >>= 2;
          }
        }
      }
    }
    return true;
  }

  bool load_dxt3(uint8* src, uint32 length, Image::color_t* bits, BLP2Header const& hdr) {
    if (hdr.width < 4 || hdr.height < 4) return false;
    if (length != hdr.width * hdr.height) return false;
    for (uint32 y = 0; y < hdr.height; y += 4) {
      for (uint32 x = 0; x < hdr.width; x += 4) {
        uint64 alpha = *(uint64*)src;
        FormatDXT p = *(uint16*)(src + 8);
        FormatDXT q = *(uint16*)(src + 10);
        uint32 lookup = *(uint32*)(src + 12);
        Image::color_t c[4] = { p, q };
        c[2] = Color::mix(c[0], 2, c[1], 1);
        c[3] = Color::mix(c[0], 1, c[1], 2);
        for (uint32 cy = y; cy < y + 4; ++cy) {
          Image::color_t* dst = bits + cy * hdr.width + x;
          for (uint32 cx = x; cx < x + 4; ++cx) {
            *dst = c[lookup & 3];
            dst->alpha = (alpha & 15) * 255 / 15;
            dst++;
            lookup >>= 2;
            alpha >>= 4;
          }
        }
      }
    }
    return true;
  }

  bool load_dxt5(uint8* src, uint32 length, Image::color_t* bits, BLP2Header const& hdr) {
    if (hdr.width < 4 || hdr.height < 4) return false;
    if (length != hdr.width * hdr.height) return false;
    for (uint32 y = 0; y < hdr.height; y += 4) {
      for (uint32 x = 0; x < hdr.width; x += 4) {
        uint32 a0 = *src++;
        uint32 a1 = *src++;
        uint64 amap = 0;
        for (int i = 0; i < 48; i += 8) {
          amap |= (static_cast<uint64>(*src++) << i);
        }
        FormatDXT p = *(uint16*)src;
        FormatDXT q = *(uint16*)(src + 2);
        uint32 lookup = *(uint32*)(src + 4);
        src += 8;
        Image::color_t c[4] = { p, q };
        c[2] = Color::mix(c[0], 2, c[1], 1);
        c[3] = Color::mix(c[0], 1, c[1], 2);
        uint32 a[8] = { a0, a1 };
        if (a0 > a1) {
          a[2] = (a0 * 6 + a1 * 1) / 7;
          a[3] = (a0 * 5 + a1 * 2) / 7;
          a[4] = (a0 * 4 + a1 * 3) / 7;
          a[5] = (a0 * 3 + a1 * 4) / 7;
          a[6] = (a0 * 2 + a1 * 5) / 7;
          a[7] = (a0 * 1 + a1 * 6) / 7;
        } else {
          a[2] = (a0 * 4 + a1 * 1) / 5;
          a[3] = (a0 * 3 + a1 * 2) / 5;
          a[4] = (a0 * 2 + a1 * 3) / 5;
          a[5] = (a0 * 1 + a1 * 4) / 5;
          a[6] = 0;
          a[7] = 255;
        }
        for (uint32 cy = y; cy < y + 4; ++cy) {
          Image::color_t* dst = bits + cy * hdr.width + x;
          for (uint32 cx = x; cx < x + 4; ++cx) {
            *dst = c[lookup & 3];
            dst->alpha = a[amap & 7];
            dst++;
            lookup >>= 2;
            amap >>= 3;
          }
        }
      }
    }
    return true;
  }

}

using namespace _blp2;

namespace ImagePrivate {
  bool imReadBLP2(Image& image, File& file) {
    BLP2Header hdr;
    file.seek(0, SEEK_SET);
    if (file.read(&hdr, sizeof hdr) != sizeof hdr) {
      return false;
    }

    if (hdr.id != '2PLB') return false;
    if (hdr.type != 1) return false; // do not support JPEG compression
    if (hdr.encoding != 1 && hdr.encoding != 2) return false;
    if (hdr.alphaDepth != 0 && hdr.alphaDepth != 1 && hdr.alphaDepth != 8) return false;
    if (hdr.alphaEncoding != 0 && hdr.alphaEncoding != 1 &&
        hdr.alphaEncoding != 7 && hdr.alphaEncoding != 8) {
      return false;
    }
    if ((hdr.width & (hdr.width - 1)) != 0) return false;
    if ((hdr.height & (hdr.height - 1)) != 0) return false;

    std::vector<uint8> src(hdr.lengths[0]);
    file.seek(hdr.offsets[0], SEEK_SET);
    if (file.read(&src[0], hdr.lengths[0]) != hdr.lengths[0]) {
      return false;
    }

    image = Image(hdr.width, hdr.height);
    Image::color_t* bits = image.mutable_bits();

    bool result = false;
    if (hdr.encoding == 1) {
      result = load_raw(&src[0], hdr.lengths[0], bits, hdr);
    } else if (hdr.alphaDepth <= 1) {
      result = load_dxt1(&src[0], hdr.lengths[0], bits, hdr);
    } else if (hdr.alphaEncoding != 7) {
      result = load_dxt3(&src[0], hdr.lengths[0], bits, hdr);
    } else {
      result = load_dxt5(&src[0], hdr.lengths[0], bits, hdr);
    }

    return result;
  }
}

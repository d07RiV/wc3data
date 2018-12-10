#include <stdlib.h>
#include "image.h"
#include <vector>

namespace _dds {

  enum {
    DDPF_ALPHAPIXELS = 0x1,
    DDPF_ALPHA = 0x2,
    DDPF_FOURCC = 0x4,
    DDPF_RGB = 0x40,
    DDPF_YUV = 0x200,
    DDPF_LUMINANCE = 0x20000,
  };

  enum {
    DDSD_CAPS = 0x1,
    DDSD_HEIGHT = 0x2,
    DDSD_WIDTH = 0x4,
    DDSD_PITCH = 0x8,
    DDSD_PIXELFORMAT = 0x1000,
    DDSD_MIPMAPCOUNT = 0x20000,
    DDSD_LINEARSIZE = 0x80000,
    DDSD_DEPTH = 0x800000,
  };
  const uint32 DDSD_REQUIRED = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;

  struct DDS_PIXELFORMAT {
    uint32 dwSize;
    uint32 dwFlags;
    uint32 dwFourCC;
    uint32 dwRGBBitCount;
    uint32 dwRBitMask;
    uint32 dwGBitMask;
    uint32 dwBBitMask;
    uint32 dwABitMask;
  };

  struct DDS_HEADER {
    uint32 dwSize;
    uint32 dwFlags;
    uint32 dwHeight;
    uint32 dwWidth;
    uint32 dwPitchOrLinearSize;
    uint32 dwDepth;
    uint32 dwMipMapCount;
    uint32 dwReserved1[11];
    DDS_PIXELFORMAT ddspf;
    uint32 dwCaps;
    uint32 dwCaps2;
    uint32 dwCaps3;
    uint32 dwCaps4;
    uint32 dwReserved2;
  };

  struct RGBInfo {
    uint32 redMask;
    uint32 redShift;
    uint32 greenMask;
    uint32 greenShift;
    uint32 blueMask;
    uint32 blueShift;
    uint32 alphaMask;
    uint32 alphaShift;

    RGBInfo(DDS_PIXELFORMAT pf)
      : redMask(pf.dwRBitMask)
      , greenMask(pf.dwGBitMask)
      , blueMask(pf.dwBBitMask)
      , alphaMask(pf.dwFlags & DDPF_ALPHAPIXELS ? pf.dwABitMask : 0)
      , redShift(0)
      , greenShift(0)
      , blueShift(0)
      , alphaShift(0)
    {
      if (redMask) while (!(redMask & 1)) { redMask >>= 1; ++redShift; }
      if (greenMask) while (!(greenMask & 1)) { greenMask >>= 1; ++greenShift; }
      if (blueMask) while (!(blueMask & 1)) { blueMask >>= 1; ++blueShift; }
      if (alphaMask) while (!(alphaMask & 1)) { alphaMask >>= 1; ++alphaShift; }
    }

    Image::color_t decode(uint32 src) const {
      int red = 0, green = 0, blue = 0, alpha = 255;
      if (redMask) red = ((src >> redShift) & redMask) * 255 / redMask;
      if (greenMask) green = ((src >> greenShift) & greenMask) * 255 / greenMask;
      if (blueMask) blue = ((src >> blueShift) & blueMask) * 255 / blueMask;
      if (alphaMask) alpha = ((src >> alphaShift) & alphaMask) * 255 / alphaMask;
      return Image::color_t(red, green, blue, alpha);
    }
  };

  typedef Color::XRGB<0, 5, 6, 5> FormatDXT;
  struct LoadDXT1 {
    static size_t blocks(int width, int height) {
      return ((width + 3) & -4) * ((height + 3) & -4) / 16;
    }
    static size_t size(int width, int height) {
      return blocks(width, height) * 8;
    }
    static Image decode(int width, int height, uint8 const* data) {
      Image image(width, height);
      Image::color_t* bits = image.mutable_bits();
      for (uint32 y = 0; y < height; y += 4) {
        for (uint32 x = 0; x < width; x += 4) {
          FormatDXT p = *(uint16*)data;
          FormatDXT q = *(uint16*)(data + 2);
          uint32 lookup = *(uint32*)(data + 4);
          data += 8;
          Image::color_t c[4] = { p, q };
          if (p > q) {
            c[2] = Color::mix(c[0], 2, c[1], 1);
            c[3] = Color::mix(c[0], 1, c[1], 2);
          } else {
            c[2] = Color::mix(c[0], 1, c[1], 1);
            c[3] = 0;
          }
          for (uint32 cy = y; cy < y + 4; ++cy) {
            Image::color_t* dst = bits + cy * width + x;
            for (uint32 cx = x; cx < x + 4; ++cx) {
              *dst++ = c[lookup & 3];
              lookup >>= 2;
            }
          }
        }
      }
      return image;
    }
  };

  struct LoadDXT3 {
    static size_t blocks(int width, int height) {
      return ((width + 3) & -4) * ((height + 3) & -4) / 16;
    }
    static size_t size(int width, int height) {
      return blocks(width, height) * 16;
    }
    static Image decode(int width, int height, uint8 const* data) {
      Image image(width, height);
      Image::color_t* bits = image.mutable_bits();
      for (uint32 y = 0; y < height; y += 4) {
        for (uint32 x = 0; x < width; x += 4) {
          uint64 alpha = *(uint64*)data;
          FormatDXT p = *(uint16*)(data + 8);
          FormatDXT q = *(uint16*)(data + 10);
          uint32 lookup = *(uint32*)(data + 12);
          Image::color_t c[4] = { p, q };
          c[2] = Color::mix(c[0], 2, c[1], 1);
          c[3] = Color::mix(c[0], 1, c[1], 2);
          data += 16;
          for (uint32 cy = y; cy < y + 4; ++cy) {
            Image::color_t* dst = bits + cy * width + x;
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
      return image;
    }
  };

  struct LoadDXT4 {
    static size_t blocks(int width, int height) {
      return ((width + 3) & -4) * ((height + 3) & -4) / 16;
    }
    static size_t size(int width, int height) {
      return blocks(width, height) * 16;
    }
    static Image decode(int width, int height, uint8 const* data) {
      Image image(width, height);
      Image::color_t* bits = image.mutable_bits();
      for (uint32 y = 0; y < height; y += 4) {
        for (uint32 x = 0; x < width; x += 4) {
          uint32 a0 = *data++;
          uint32 a1 = *data++;
          uint64 amap = 0;
          for (int i = 0; i < 48; i += 8) {
            amap |= (static_cast<uint64>(*data++) << i);
          }
          FormatDXT p = *(uint16*)data;
          FormatDXT q = *(uint16*)(data + 2);
          uint32 lookup = *(uint32*)(data + 4);
          data += 8;
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
            Image::color_t* dst = bits + cy * width + x;
            for (uint32 cx = x; cx < x + 4; ++cx) {
              Image::color_t color = c[lookup & 3];
              int alpha = a[amap & 7];
              if (alpha) {
                color = Image::color_t(
                  color.red * 255 / alpha,
                  color.green * 255 / alpha,
                  color.blue * 255 / alpha
                );
              } else {
                color.alpha = alpha;
              }
              *dst++ = color;
              lookup >>= 2;
              amap >>= 3;
            }
          }
        }
      }
      return image;
    }
  };

  struct LoadDXT5 {
    static size_t blocks(int width, int height) {
      return ((width + 3) & -4) * ((height + 3) & -4) / 16;
    }
    static size_t size(int width, int height) {
      return blocks(width, height) * 16;
    }
    static Image decode(int width, int height, uint8 const* data) {
      Image image(width, height);
      Image::color_t* bits = image.mutable_bits();
      for (uint32 y = 0; y < height; y += 4) {
        for (uint32 x = 0; x < width; x += 4) {
          uint32 a0 = *data++;
          uint32 a1 = *data++;
          uint64 amap = 0;
          for (int i = 0; i < 48; i += 8) {
            amap |= (static_cast<uint64>(*data++) << i);
          }
          FormatDXT p = *(uint16*)data;
          FormatDXT q = *(uint16*)(data + 2);
          uint32 lookup = *(uint32*)(data + 4);
          data += 8;
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
            Image::color_t* dst = bits + cy * width + x;
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
      return image;
    }
  };

  struct LoadBC4 {
    static size_t blocks(int width, int height) {
      return ((width + 3) & -4) * ((height + 3) & -4) / 16;
    }
    static size_t size(int width, int height) {
      return blocks(width, height) * 8;
    }
    static Image decode(int width, int height, uint8 const* data) {
      Image image(width, height);
      Image::color_t* bits = image.mutable_bits();
      for (uint32 y = 0; y < height; y += 4) {
        for (uint32 x = 0; x < width; x += 4) {
          uint32 a0 = *data++;
          uint32 a1 = *data++;
          uint64 amap = 0;
          for (int i = 0; i < 48; i += 8) {
            amap |= (static_cast<uint64>(*data++) << i);
          }
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
            Image::color_t* dst = bits + cy * width + x;
            for (uint32 cx = x; cx < x + 4; ++cx) {
              *dst++ = Image::color_t(a[amap & 7], a[amap & 7], a[amap & 7]);
              amap >>= 3;
            }
          }
        }
      }
      return image;
    }
  };

  struct LoadBC5 {
    static size_t blocks(int width, int height) {
      return ((width + 3) & -4) * ((height + 3) & -4) / 16;
    }
    static size_t size(int width, int height) {
      return blocks(width, height) * 16;
    }
    static Image decode(int width, int height, uint8 const* data) {
      Image image(width, height);
      Image::color_t* bits = image.mutable_bits();
      for (uint32 y = 0; y < height; y += 4) {
        for (uint32 x = 0; x < width; x += 4) {
          uint32 a0 = *data++;
          uint32 a1 = *data++;
          uint64 amap = 0;
          for (int i = 0; i < 48; i += 8) {
            amap |= (static_cast<uint64>(*data++) << i);
          }
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
          uint32 b0 = *data++;
          uint32 b1 = *data++;
          uint64 bmap = 0;
          for (int i = 0; i < 48; i += 8) {
            bmap |= (static_cast<uint64>(*data++) << i);
          }
          uint32 b[8] = { b0, b1 };
          if (b0 > b1) {
            b[2] = (b0 * 6 + b1 * 1) / 7;
            b[3] = (b0 * 5 + b1 * 2) / 7;
            b[4] = (b0 * 4 + b1 * 3) / 7;
            b[5] = (b0 * 3 + b1 * 4) / 7;
            b[6] = (b0 * 2 + b1 * 5) / 7;
            b[7] = (b0 * 1 + b1 * 6) / 7;
          } else {
            b[2] = (b0 * 4 + b1 * 1) / 5;
            b[3] = (b0 * 3 + b1 * 2) / 5;
            b[4] = (b0 * 2 + b1 * 3) / 5;
            b[5] = (b0 * 1 + b1 * 4) / 5;
            b[6] = 0;
            b[7] = 255;
          }
          for (uint32 cy = y; cy < y + 4; ++cy) {
            Image::color_t* dst = bits + cy * width + x;
            for (uint32 cx = x; cx < x + 4; ++cx) {
              *dst++ = Image::color_t(a[amap & 7], b[bmap & 7], 0);
              amap >>= 3;
              bmap >>= 3;
            }
          }
        }
      }
      return image;
    }
  };

  uint8 f16_to_u8(uint16 x) {
    if (x & 0x8000) return 0;
    if ((x >> 10) >= 15) return 255;
    uint8 e = (x >> 10);
    uint32 m = 0x400 + (x & 0x3FF);
    return uint8(e >= 15 ? 255 : m >> (17 - e));
  }

  struct LoadRGBA16 {
    static size_t size(int width, int height) {
      return width * height * 8;
    }
    static Image decode(int width, int height, uint8 const* data) {
      Image image(width, height);
      uint8* dst = reinterpret_cast<uint8*>(image.mutable_bits());
      uint16 const* src = reinterpret_cast<uint16 const*>(data);
      for (uint32 i = 0; i < width * height * 4; ++i) {
        *dst++ = f16_to_u8(*src++);
      }
      return image;
    }
  };

  struct LoadR16 {
    static size_t size(int width, int height) {
      return width * height * 2;
    }
    static Image decode(int width, int height, uint8 const* data) {
      Image image(width, height);
      Image::color_t* dst = image.mutable_bits();
      uint16 const* src = reinterpret_cast<uint16 const*>(data);
      for (uint32 i = 0; i < width * height; ++i) {
        uint8 value = f16_to_u8(*src++);
        *dst++ = Image::color_t(value, 0, 0);
      }
      return image;
    }
  };

  struct LoadRG16 {
    static size_t size(int width, int height) {
      return width * height * 4;
    }
    static Image decode(int width, int height, uint8 const* data) {
      Image image(width, height);
      Image::color_t* dst = image.mutable_bits();
      uint16 const* src = reinterpret_cast<uint16 const*>(data);
      for (uint32 i = 0; i < width * height; ++i) {
        uint8 r = f16_to_u8(*src++);
        uint8 g = f16_to_u8(*src++);
        *dst++ = Image::color_t(r, g, 0);
      }
      return image;
    }
  };

  template<typename source_t>
  struct LoadRaw {
    static size_t size(int width, int height) {
      return width * height * sizeof(source_t);
    }
    static Image decode(int width, int height, uint8 const* data) {
      Image image(width, height);
      Image::color_t* dst = image.mutable_bits();
      source_t const* src = (source_t*)data;
      for (int i = 0; i < width * height; ++i) {
        *dst++ = *src++;
      }
      return image;
    }
  };

  template<class Loader>
  Image LoadProxy(File file, DDS_HEADER const& hdr) {
    size_t size = Loader::size(hdr.dwWidth, hdr.dwHeight);
    std::vector<uint8> data(size);
    if (file.read(data.data(), size) != size) return Image();
    return Loader::decode(hdr.dwWidth, hdr.dwHeight, data.data());
  }

  struct Loader {
    uint32 id;
    Image(*load)(File file, DDS_HEADER const& hdr);
  };
  Loader base_loaders[] = {
    { 'DXT1', LoadProxy<LoadDXT1> },
    { 'DXT4', LoadProxy<LoadDXT4> },
    { 'DXT5', LoadProxy<LoadDXT5> },
  };
}

using namespace _dds;

namespace ImagePrivate {
  bool imReadDDS(Image& image, File& file) {
    file.seek(0, SEEK_SET);
    if (file.read32() != 0x20534444) return false;
    DDS_HEADER hdr;
    if (file.read(&hdr, sizeof hdr) != sizeof hdr) {
      return false;
    }

    if (hdr.dwSize != sizeof hdr) return false;
    if ((hdr.dwFlags & DDSD_REQUIRED) != DDSD_REQUIRED) return false;

    if (hdr.ddspf.dwFlags & DDPF_FOURCC) {
      flip(hdr.ddspf.dwFourCC);
      Loader const* loader = nullptr;
      for (auto& ldr : base_loaders) {
        if (ldr.id == hdr.ddspf.dwFourCC) {
          loader = &ldr;
          break;
        }
      }
      if (!loader) return false;
      image = loader->load(file, hdr);
      return image;
    } else if (hdr.ddspf.dwFlags & DDPF_RGB) {
      image = Image(hdr.dwWidth, hdr.dwHeight);
      Image::color_t* bits = image.mutable_bits();
      RGBInfo rgb(hdr.ddspf);
      uint32 bytes = (hdr.ddspf.dwRGBBitCount + 7) / 8;
      uint32 pitch = hdr.dwWidth * bytes;
      if (hdr.dwFlags & DDSD_PITCH) pitch = hdr.dwPitchOrLinearSize;
      uint64 pos0 = file.tell();
      for (uint32 y = 0; y < hdr.dwHeight; ++y) {
        file.seek(pos0 + pitch * y);
        for (uint32 x = 0; x < hdr.dwWidth; ++x) {
          uint32 color = 0;
          file.read(&color, bytes);
          bits[x] = rgb.decode(color);
        }
        bits += hdr.dwWidth;
      }
      return true;
    } else {
      return false;
    }
  }
}

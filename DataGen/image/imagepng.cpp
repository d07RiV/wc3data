#include "image.h"
#include <vector>
#include "utils/common.h"

namespace ImagePrivate {

  namespace _png {

    unsigned char const pngSignature[8] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };

    struct PNGChunk {
      uint32 length;
      uint32 type;
      uint8* data = nullptr;
      uint32 crc;
      ~PNGChunk() {
        delete[] data;
      }
    };

#pragma pack(push, 1)
    struct PNGHeader {
      uint32 width;
      uint32 height;
      uint8 bitDepth;
      uint8 colorType;
      uint8 compressionMethod;
      uint8 filterMethod;
      uint8 interlaceMethod;
    };
#pragma	pack(pop)

    uint32 update_crc(uint32 crc, void const* vbuf, uint32 length) {
      static uint32 crc_table[256];
      static bool table_computed = false;
      uint8 const* buf = (uint8*)vbuf;
      if (!table_computed) {
        for (uint32 i = 0; i < 256; i++) {
          uint32 c = i;
          for (int k = 0; k < 8; k++) {
            if (c & 1) c = 0xEDB88320L ^ (c >> 1);
            else c = c >> 1;
          }
          crc_table[i] = c;
        }
        table_computed = true;
      }
      for (uint32 i = 0; i < length; i++) {
        crc = crc_table[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
      }
      return crc;
    }
    bool read_chunk(PNGChunk& ch, File& f) {
      delete[] ch.data;
      ch.data = NULL;
      if (f.read(&ch.length, 4) != 4) return false;
      if (f.read(&ch.type, 4) != 4) return false;
      flip(ch.length);
      ch.data = new uint8[ch.length];
      if (f.read(ch.data, ch.length) != ch.length) return false;
      if (f.read(&ch.crc, 4) != 4) return false;
      flip(ch.crc);
      uint32 crc = update_crc(0xFFFFFFFF, &ch.type, 4);
      crc = update_crc(crc, ch.data, ch.length);
      crc = ~crc;
      if (crc != ch.crc) return false;
      flip(ch.type);
      return true;
    }

    void write_chunk(uint32 type, uint32 length, void const* data, File& f) {
      flip(type);
      f.write32(flipped(length));
      f.write32(type);
      f.write(data, length);
      uint32 crc = update_crc(0xFFFFFFFF, &type, 4);
      crc = ~update_crc(crc, data, length);
      f.write32(flipped(crc));
    }

    struct PNGPalette {
      Image::color_t rgba[256];
      uint32 size;
      bool useColorKey;
      uint32 colorKey[3];
    };

    uint32 bitsPerPixel(PNGHeader const& hdr) {
      switch (hdr.colorType) {
      case 3: return hdr.bitDepth;
      case 4: return hdr.bitDepth * 2;
      case 2: return hdr.bitDepth * 3;
      case 6: return hdr.bitDepth * 4;
      default: return hdr.bitDepth;
      }
    }
    uint32 get_image_size(uint32 width, uint32 height, PNGHeader const& hdr) {
      uint32 bpp = bitsPerPixel(hdr);
      uint32 bpl = (bpp * width + 7) / 8;
      return height * (bpl + 1);
    }

    uint8 paethPredictor(uint8 a, uint8 b, uint8 c) {
      int ia = int(a) & 0xFF;
      int ib = int(b) & 0xFF;
      int ic = int(c) & 0xFF;
      int p = ia + ib - ic;
      int pa = abs(p - ia);
      int pb = abs(p - ib);
      int pc = abs(p - ic);
      if (pa <= pb && pa <= pc) return a;
      if (pb <= pc) return b;
      return c;
    }

    class BitStream {
      uint8 const* buf;
      uint8 cur;
    public:
      BitStream(uint8 const* src);
      uint32 read(uint32 count);
    };
    BitStream::BitStream(uint8 const* src) {
      buf = src;
      cur = 8;
    }
    uint32 BitStream::read(uint32 count) {
      if (count == 16) {
        uint32 res = buf[1] + (buf[0] << 8);
        buf += 2;
        return res;
      } else if (count == 8) {
        return *buf++;
      } else if (count == 4) {
        uint32 res = (buf[0] >> (cur - 4)) & 0x0F;
        cur -= 4;
        if (cur == 0) {
          cur = 8;
          buf++;
        }
        return res;
      } else if (count == 2) {
        uint32 res = (buf[0] >> (cur - 2)) & 0x03;
        cur -= 2;
        if (cur == 0) {
          cur = 8;
          buf++;
        }
        return res;
      } else if (count == 1) {
        uint32 res = (buf[0] >> (cur - 1)) & 0x01;
        cur -= 1;
        if (cur == 0) {
          cur = 8;
          buf++;
        }
        return res;
      }
      return 0;
    }
    uint32 fix_color(uint32 src, uint32 count) {
      if (count == 16) {
        return src >> 8;
      } else if (count < 8) {
        return (src * 255) / ((1 << count) - 1);
      } else {
        return src;
      }
    }

    uint8 fix_sample(uint8 cur, uint8 a, uint8 b, uint8 c, uint8 filter) {
      switch (filter) {
      case 0: return cur;
      case 1: return cur + a;
      case 2: return cur + b;
      case 3: return cur + (a + b) / 2;
      case 4: return cur + paethPredictor(a, b, c);
      default: return cur;
      }
    }

    bool read_sub_image(uint32 width, uint32 height, Image::color_t* data,
      uint8 const* src, PNGHeader const& hdr, PNGPalette const& pal) {
      uint32 bpp = bitsPerPixel(hdr);
      uint32 bpl = (bpp * width + 7) / 8;
      bpp = (bpp + 7) / 8;

      std::vector<uint8> prevLine(bpl, 0);
      std::vector<uint8> curLine(bpl);

      for (uint32 y = 0; y < height; y++) {
        uint8 filter = *src++;
        if (filter > 4) {
          return false;
        }
        for (uint32 i = 0; i < bpl; i++) {
          uint8 cur = src[i];
          uint8 a = i < bpp ? 0 : curLine[i - bpp];
          uint8 b = prevLine[i];
          uint8 c = i < bpp ? 0 : prevLine[i - bpp];
          curLine[i] = fix_sample(cur, a, b, c, filter);
        }
        BitStream buf(&curLine[0]);
        for (uint32 x = 0; x < width; x++) {
          Image::color_t cur = 0;
          if (hdr.colorType == 0) {
            uint32 gray = buf.read(hdr.bitDepth);
            if (pal.useColorKey && gray == pal.colorKey[0]) {
              cur = 0;
            } else {
              gray = fix_color(gray, hdr.bitDepth);
              cur = Image::color_t(gray, gray, gray);
            }
          } else if (hdr.colorType == 2) {
            uint32 red = buf.read(hdr.bitDepth);
            uint32 green = buf.read(hdr.bitDepth);
            uint32 blue = buf.read(hdr.bitDepth);
            if (pal.useColorKey && red == pal.colorKey[0] &&
              green == pal.colorKey[1] && blue == pal.colorKey[2]) {
              cur = 0;
            } else {
              cur = Image::color_t(fix_color(red, hdr.bitDepth),
                fix_color(green, hdr.bitDepth), fix_color(blue, hdr.bitDepth));
            }
          } else if (hdr.colorType == 3) {
            uint32 index = buf.read(hdr.bitDepth);
            if (index >= pal.size) {
              return false;
            }
            cur = pal.rgba[index];
          } else if (hdr.colorType == 4) {
            uint32 gray = fix_color(buf.read(hdr.bitDepth), hdr.bitDepth);
            uint32 alpha = fix_color(buf.read(hdr.bitDepth), hdr.bitDepth);
            cur = Image::color_t(gray, gray, gray, alpha);
          } else if (hdr.colorType == 6) {
            uint32 red = fix_color(buf.read(hdr.bitDepth), hdr.bitDepth);
            uint32 green = fix_color(buf.read(hdr.bitDepth), hdr.bitDepth);
            uint32 blue = fix_color(buf.read(hdr.bitDepth), hdr.bitDepth);
            uint32 alpha = fix_color(buf.read(hdr.bitDepth), hdr.bitDepth);
            cur = Image::color_t(red, green, blue, alpha);
          }
          *data++ = cur;
        }
        prevLine = curLine;
        src += bpl;
      }
      return true;
    }

    struct SubImage {
      uint32 width;
      uint32 height;
      uint32 src_size;
      uint8* src;
      Image::color_t* data;
      int ref;
    };

    void write_image(Image::color_t* src, uint32 srcw, uint32 srch,
      Image::color_t* dst, uint32 dstw, uint32 dsth,
      uint32 sx, uint32 sy, uint32 dx, uint32 dy)
    {
      for (uint32 y = 0; y < srch; y++) {
        Image::color_t* srcl = src + y * srcw;
        Image::color_t* dstl = dst + (sy + y * dy) * dstw + sx;
        for (uint32 x = 0; x < srcw; x++) {
          *dstl = *srcl;
          srcl++;
          dstl += dx;
        }
      }
    }

  }

  using namespace _png;

  bool imWritePNG(Image const& image, File& file, int gray) {
    file.write(pngSignature, 8);

    uint32 width = image.width();
    uint32 height = image.height();
    Image::color_t const* bits = image.bits();

    PNGHeader hdr;
    hdr.width = flipped(width);
    hdr.height = flipped(height);
    hdr.bitDepth = 8;
    if (gray == 0) hdr.colorType = 6;
    else if (gray == 1) hdr.colorType = 0;
    else hdr.colorType = 4;
    hdr.compressionMethod = 0;
    hdr.filterMethod = 0;
    hdr.interlaceMethod = 0;
    write_chunk(0x49484452 /*IHDR*/, sizeof hdr, &hdr, file);

    int channels = (gray == 0 ? 4 : (gray == 1 ? 1 : 2));
    uint32 usize = (width * channels + 1) * height;
    std::vector<uint8> udata(usize);
    uint8* dst = &udata[0];
    for (size_t y = 0; y < height; y++) {
      *dst++ = 0;
      for (size_t x = 0; x < width; x++) {
        Image::color_t color = *bits++;
        if (gray) {
          int sum = color.red + color.green + color.blue;
          *dst++ = sum / 3;
          if (gray > 1) *dst++ = color.alpha;
        } else {
          *dst++ = color.red;
          *dst++ = color.green;
          *dst++ = color.blue;
          *dst++ = color.alpha;
        }
      }
    }
    uint32 csize = (usize * 11) / 10 + 32;
    std::vector<uint8> cdata(csize);
    if (gzdeflate(&udata[0], usize, &cdata[0], &csize)) {
      return false;
    }

    write_chunk(0x49444154 /*IDAT*/, csize, &cdata[0], file);
    write_chunk(0x49454e44 /*IEND*/, 0, NULL, file);

    return true;
  }

  bool imReadPNG(Image& image, File& file) {
    uint8 sig[8];
    file.seek(0, SEEK_SET);
    if (file.read(sig, 8) != 8 || memcmp(sig, pngSignature, 8)) {
      return false;
    }

    PNGChunk ch;
    PNGHeader hdr;
    if (!read_chunk(ch, file)) return false;
    if (ch.type != 0x49484452 /*IHDR*/ || ch.length != sizeof hdr) return false;
    memcpy(&hdr, ch.data, sizeof hdr);
    hdr.width = flipped(hdr.width);
    hdr.height = flipped(hdr.height);
    if (hdr.width == 0 || hdr.height == 0) {
      return false; // zero sized images not allowed
    }
    if (hdr.bitDepth < 1 || hdr.bitDepth > 16 || (hdr.bitDepth & (hdr.bitDepth - 1)) != 0) {
      return false; // unknown bit depth (not 1 2 4 8 or 16)
    }
    if (hdr.colorType != 0 && hdr.colorType != 2 && hdr.colorType != 3 &&
      hdr.colorType != 4 && hdr.colorType != 6) {
      return false; // unknown color type (not 0 2 3 4 or 6)
    }
    if (hdr.colorType != 0 && hdr.colorType != 3 && hdr.bitDepth < 8) {
      return false; // bit depth of 1 2 or 4 only allowed for color type 0 or 3
    }
    if (hdr.colorType == 3 && hdr.bitDepth == 16) {
      return false; // bit depth of 16 not allowed for color type 3
    }
    if (hdr.compressionMethod != 0) {
      return false; // unknown compression method
    }
    if (hdr.filterMethod != 0) {
      return false; // unknown filter method
    }
    if (hdr.interlaceMethod != 0 && hdr.interlaceMethod != 1) {
      return false; // unknown interlace method
    }

    SubImage passes[7];
    int numPasses;
    uint32 total_src_size = 0;
    uint32 total_data_size = 0;
    if (hdr.interlaceMethod == 0) {
      numPasses = 1;
      passes[0].width = hdr.width;
      passes[0].height = hdr.height;
      passes[0].src_size = get_image_size(hdr.width, hdr.height, hdr);
      passes[0].ref = 0;
      total_src_size += passes[0].src_size;
      total_data_size += hdr.width * hdr.height;
    } else
    {
      numPasses = 0;
      passes[0].width = (hdr.width + 7) / 8;
      passes[0].height = (hdr.height + 7) / 8;
      passes[1].width = (hdr.width + 3) / 8;
      passes[1].height = (hdr.height + 7) / 8;
      passes[2].width = (hdr.width + 3) / 4;
      passes[2].height = (hdr.height + 3) / 8;
      passes[3].width = (hdr.width + 1) / 4;
      passes[3].height = (hdr.height + 3) / 4;
      passes[4].width = (hdr.width + 1) / 2;
      passes[4].height = (hdr.height + 1) / 4;
      passes[5].width = hdr.width / 2;
      passes[5].height = (hdr.height + 1) / 2;
      passes[6].width = hdr.width;
      passes[6].height = hdr.height / 2;

      for (int i = 0; i < 7; i++) {
        passes[i].ref = -1;
        if (passes[i].width && passes[i].height) {
          if (i > numPasses) {
            passes[numPasses].width = passes[i].width;
            passes[numPasses].height = passes[i].height;
          }
          passes[i].ref = numPasses;
          passes[numPasses].src_size = get_image_size(passes[numPasses].width, passes[numPasses].height, hdr);
          total_src_size += passes[numPasses].src_size;
          total_data_size += passes[numPasses].width * passes[numPasses].height;
          numPasses++;
        }
      }
    }
    std::vector<uint8> src(total_src_size);
    std::vector<Image::color_t> data(total_data_size);

    uint32 cur_src = 0;
    uint32 cur_data = 0;
    for (int i = 0; i < numPasses; i++) {
      passes[i].src = &src[cur_src];
      passes[i].data = &data[cur_data];
      cur_src += passes[i].src_size;
      cur_data += passes[i].width * passes[i].height;
    }

    PNGPalette pal;
    pal.size = 0;
    pal.useColorKey = false;

    std::vector<uint8> buf;
    while (true) {
      if (!read_chunk(ch, file)) {
        return false;
      }
      if (ch.type == 0x49454e44 /*IEND*/) {
        break;
      } else if (ch.type == 0x504c5445 /*PLTE*/) {
        pal.size = ch.length / 3;
        if (ch.length != pal.size * 3) {
          return false;
        }
        for (uint32 i = 0; i < pal.size; i++) {
          uint8 red = ch.data[i * 3 + 0];
          uint8 green = ch.data[i * 3 + 1];
          uint8 blue = ch.data[i * 3 + 2];
          pal.rgba[i] = Image::color_t(red, green, blue);
        }
      } else if (ch.type == 0x74524e53 /*tRNS*/) {
        if (hdr.colorType == 3) {
          if (ch.length > pal.size) {
            return false;
          }
          for (uint32 i = 0; i < pal.size && i < ch.length; i++) {
            pal.rgba[i].alpha = ch.data[i];
          }
        } else if (hdr.colorType == 0) {
          if (ch.length != 2) {
            return false;
          }
          pal.useColorKey = true;
          pal.colorKey[0] = ch.data[1] + (ch.data[0] << 8);
        } else if (hdr.colorType == 2) {
          if (ch.length != 6) {
            return false;
          }
          pal.useColorKey = true;
          pal.colorKey[0] = ch.data[1] + (ch.data[0] << 8);
          pal.colorKey[1] = ch.data[3] + (ch.data[2] << 8);
          pal.colorKey[2] = ch.data[5] + (ch.data[4] << 8);
        } else {
          return false;
        }
      } else if (ch.type == 0x49444154 /*IDAT*/) {
        buf.insert(buf.end(), ch.data, ch.data + ch.length);
      }
    }
    if (gzinflate(&buf[0], static_cast<uint32>(buf.size()), &src[0], &total_src_size)) {
      return false;
    }

    for (int i = 0; i < numPasses; i++) {
      read_sub_image(passes[i].width, passes[i].height, passes[i].data, passes[i].src, hdr, pal);
    }

    image = Image(hdr.width, hdr.height);
    if (hdr.interlaceMethod == 0) {
      memcpy(image.mutable_bits(), passes[0].data, image.size());
    } else {
      uint32 sx[7] = { 0, 4, 0, 2, 0, 1, 0 };
      uint32 dx[7] = { 8, 8, 4, 4, 2, 2, 1 };
      uint32 sy[7] = { 0, 0, 4, 0, 2, 0, 1 };
      uint32 dy[7] = { 8, 8, 8, 4, 4, 2, 2 };
      for (int i = 0; i < 7; i++) {
        if (passes[i].ref >= 0) {
          int j = passes[i].ref;
          write_image(passes[j].data, passes[j].width, passes[j].height,
            image.mutable_bits(), image.width(), image.height(),
            sx[i], sy[i], dx[i], dy[i]);
        }
      }
    }
    return true;
  }

}

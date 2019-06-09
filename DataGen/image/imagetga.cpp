#include "image.h"
#include "utils/common.h"
#include <stdlib.h>

#pragma pack (push, 1)
struct TGAHeader {
  uint8 idLength;
  uint8 colormapType;
  uint8 imageType;
  uint16 colormapIndex;
  uint16 colormapLength;
  uint8 colormapEntrySize;
  uint16 xOrigin;
  uint16 yOrigin;
  uint16 width;
  uint16 height;
  uint8 pixelSize;
  uint8 imageDesc;
};
#pragma pack (pop)

namespace ImagePrivate {
  bool imReadTGA(Image& image, File& file) {
    file.seek(0, SEEK_SET);
    TGAHeader hdr;
    if (file.read(&hdr, sizeof hdr) != sizeof hdr) {
      return false;
    }
    if (hdr.imageType != 0 && hdr.imageType != 1 && hdr.imageType != 2 &&
        hdr.imageType != 3 && hdr.imageType != 9 && hdr.imageType != 10 &&
        hdr.imageType != 11) {
      return false;
    }
    if (hdr.pixelSize != 32 && hdr.pixelSize != 24 && hdr.pixelSize != 16 &&
        hdr.pixelSize != 15 && hdr.pixelSize != 8) {
      return false;
    }
    if (!hdr.width || !hdr.height) {
      return false;
    }
    if (hdr.width > 32768 || hdr.height > 32768 || hdr.width * hdr.height > 8192 * 8192) {
      return false;
    }

    image = Image(hdr.width, hdr.height);
    Image::color_t* bits = image.mutable_bits();

    file.seek((sizeof hdr) + hdr.idLength, SEEK_SET);

    std::vector<Image::color_t> pal;
    if (hdr.imageType == 1 || hdr.imageType == 9) {
      pal.resize(hdr.colormapLength);
      if (hdr.colormapEntrySize == 15 || hdr.colormapEntrySize == 16) {
        for (int i = 0; i < hdr.colormapLength; i++) {
          int a = file.getc();
          int b = file.getc();
          if (b == EOF) {
            return false;
          }
          pal[i] = Image::color_t(
            ((b & 0x7C) >> 2) << 16,
            (((b & 0x03) << 3) | ((a & 0xE0) >> 5)) << 8,
            a & 0x1F
          );
        }
      } else if (hdr.colormapEntrySize == 24) {
        for (int i = 0; i < hdr.colormapLength; i++) {
          int r = file.getc();
          int g = file.getc();
          int b = file.getc();
          if (b == EOF) {
            return false;
          }
          pal[i] = Image::color_t(r, g, b);
        }
      } else if (hdr.colormapEntrySize == 32) {
        for (int i = 0; i < hdr.colormapLength; i++) {
          int r = file.getc();
          int g = file.getc();
          int b = file.getc();
          int a = file.getc();
          if (a == EOF) {
            return false;
          }
          pal[i] = Image::color_t(r, g, b, a);
        }
      }
    }
    if (hdr.imageType >= 1 && hdr.imageType <= 3) {
      for (int y = 0; y < image.height(); y++) {
        Image::color_t* out;
        if (hdr.imageDesc & 0x20) {
          out = bits + y * image.width();
        } else {
          out = bits + (image.height() - y - 1) * image.width();
        }
        for (int x = 0; x < image.width(); x++) {
          if (hdr.pixelSize == 8) {
            int c = file.getc();
            if (c == EOF) {
              return false;
            }
            if (pal.size()) {
              *out++ = pal[c];
            } else {
              *out++ = Image::color_t(c, c, c);
            }
          } else if (hdr.pixelSize == 24) {
            int b = file.getc();
            int g = file.getc();
            int r = file.getc();
            if (r == EOF) {
              return false;
            }
            *out++ = Image::color_t(r, g, b);
          } else {
            int b = file.getc();
            int g = file.getc();
            int r = file.getc();
            int a = file.getc();
            if (a == EOF) {
              return false;
            }
            *out++ = Image::color_t(r, g, b, a);
          }
        }
      }
    } else {
      int x = 0;
      int y = 0;
      Image::color_t* out;
      if (hdr.imageDesc & 0x20) {
        out = bits + y * image.width();
      } else {
        out = bits + (image.height() - y - 1) * image.width();
      }
      while (y < image.height()) {
        int pkHdr = file.getc();
        if (pkHdr == EOF) {
          return false;
        }
        int pkSize = (pkHdr & 0x7F) + 1;
        Image::color_t color;
        for (int i = 0; i < pkSize; i++) {
          if (i == 0 || (pkHdr & 0x80) == 0) {
            if (hdr.pixelSize == 8) {
              int c = file.getc();
              if (c == EOF) {
                return false;
              }
              if (pal.size()) {
                color = pal[c];
              } else {
                color = Image::color_t(c, c, c);
              }
            } else if (hdr.pixelSize == 24) {
              int b = file.getc();
              int g = file.getc();
              int r = file.getc();
              if (r == EOF) {
                return false;
              }
              color = Image::color_t(r, g, b);
            } else {
              int b = file.getc();
              int g = file.getc();
              int r = file.getc();
              int a = file.getc();
              if (a == EOF) {
                return false;
              }
              color = Image::color_t(r, g, b, a);
            }
          }
          *out++ = color;
          if (++x >= image.width()) {
            x = 0;
            if (++y >= image.height()) {
              break;
            }
            if (hdr.imageDesc & 0x20) {
              out = bits + y * image.width();
            } else {
              out = bits + (image.height() - y - 1) * image.width();
            }
          }
        }
      }
    }
    return true;
  }
}

#include "image.h"
#include "utils/path.h"
#include <algorithm>

decltype(ImageFilter::Box) ImageFilter::Box;
decltype(ImageFilter::Triangle) ImageFilter::Triangle;
decltype(ImageFilter::Hermite) ImageFilter::Hermite;
decltype(ImageFilter::Bell) ImageFilter::Bell;
decltype(ImageFilter::CubicBSpline) ImageFilter::CubicBSpline;
decltype(ImageFilter::Lanczos3) ImageFilter::Lanczos3;
decltype(ImageFilter::Mitchell) ImageFilter::Mitchell;
decltype(ImageFilter::Cosine) ImageFilter::Cosine;
decltype(ImageFilter::CatmullRom) ImageFilter::CatmullRom;
decltype(ImageFilter::Quadratic) ImageFilter::Quadratic;
decltype(ImageFilter::QuadraticBSpline) ImageFilter::QuadraticBSpline;
decltype(ImageFilter::CubicConvolution) ImageFilter::CubicConvolution;
decltype(ImageFilter::Lanczos8) ImageFilter::Lanczos8;

namespace ImagePrivate {

  bool imReadPNG(Image& image, File& file);
  bool imReadBLP2(Image& image, File& file);
  bool imReadBLP(Image& image, File& file);
  bool imReadTGA(Image& image, File& file);
  bool imReadJPG(Image& image, File& file);
  bool imReadGIF(Image& image, File& file);
  bool imReadDDS(Image& image, File& file);
  bool imWritePNG(Image const& image, File& file, int gray);
  bool _imWritePNG(Image const& image, File& file) {
    return imWritePNG(image, file, 0);
  }
  bool _imWritePNGGray(Image const& image, File& file) {
    return imWritePNG(image, file, 1);
  }
  bool _imWritePNGGrayAlpha(Image const& image, File& file) {
    return imWritePNG(image, file, 2);
  }

  typedef bool(*imReader)(Image& image, File& file);
  typedef bool(*imWriter)(Image const& image, File& file);

  imReader readers[ImageFormat::NumFormats] = {
    nullptr,
    imReadPNG,
    imReadPNG,
    imReadBLP,
    imReadBLP2,
    imReadPNG,
    imReadDDS,
    imReadGIF,
    imReadTGA,
    imReadJPG
  };
  imWriter writers[ImageFormat::NumFormats] = {
    _imWritePNG,
    _imWritePNG,
    _imWritePNGGray,
    nullptr,
    nullptr,
    _imWritePNGGrayAlpha,
  };
  struct { char const* ext; ImageFormat::Type format; } extensions[] = {
    { ".png", ImageFormat::PNG },
  };

  ImageFormat::Type getFormat(std::string const& name) {
    std::string ext = path::ext(name);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    for (auto& fmt : extensions) {
      if (ext == fmt.ext) {
        return fmt.format;
      }
    }
    return ImageFormat::Unknown;
  }

  bool imWrite(Image const& image, File file, ImageFormat::Type format) {
    if (writers[format]) {
      return writers[format](image, file);
    } else {
      return false;
    }
  }

  Image imRead(File file, ImageFormat::Type format) {
    if (!file) return Image();
    Image image;
    if (readers[format] && readers[format](image, file)) {
      return image;
    }
    for (auto reader : readers) {
      if (reader && reader(image, file)) {
        return image;
      }
    }
    image.release();
    return image;
  }

}

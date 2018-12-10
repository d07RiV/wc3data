#pragma once

#include "utils/common.h"
#include "utils/file.h"
#include <algorithm>
#include <cmath>

namespace Color {

  template<
    typename value_t,
    int redBits, int redShift,
    int greenBits, int greenShift,
    int blueBits, int blueShift,
    int alphaBits, int alphaShift
  >
  struct ColorType {
  public:
    template<int p_bits, int p_shift>
    struct ColorComponent {
      static const int bits = p_bits;
      static const int shift = p_shift;
      static const value_t max = (value_t(1) << p_bits) - 1;
      static const value_t mask = (max << p_shift);

      ColorComponent() {}

      operator int() const {
        return ((value_ >> shift) & max) * 255 / max;
      }
      ColorComponent& operator=(int value) {
        value_ = (value_ & ~mask) | ((static_cast<value_t>(value) * max / 255) << shift);
        return *this;
      }
      ColorComponent& clamp(int value) {
        if (value < 0) value = 0;
        if (value > 255) value = 255;
        return *this = value;
      }
    private:
      value_t value_;
      friend struct ColorType;
      ColorComponent(value_t value)
        : value_(value)
      {}
    };

    template<int p_shift>
    struct ColorComponent<0, p_shift> {
      static const int bits = 0;
      static const int shift = p_shift;
      static const value_t max = 0;
      static const value_t mask = 0;

      ColorComponent() {}

      operator int() const {
        return 255;
      }
      ColorComponent& operator=(int value) {
        return *this;
      }
      ColorComponent& clamp(int value) {
        return *this;
      }
    private:
      value_t value_;
      friend struct ColorType;
      ColorComponent(value_t value)
        : value_(value)
      {}
    };

    typedef ColorComponent<redBits, redShift> red_t;
    typedef ColorComponent<greenBits, greenShift> green_t;
    typedef ColorComponent<blueBits, blueShift> blue_t;
    typedef ColorComponent<alphaBits, alphaShift> alpha_t;

    ColorType()
      : red(0)
    {}
    ColorType(value_t value)
      : red(value)
    {}
    ColorType(int r, int g, int b, int a = alpha_t::max)
      : red(0)
    {
      red = r;
      green = g;
      blue = b;
      alpha = a;
    }

    static ColorType clamp(int r, int g, int b, int a = alpha_t::max) {
      ColorType result;
      result.red.clamp(r);
      result.green.clamp(g);
      result.blue.clamp(b);
      result.alpha.clamp(a);
      return result;
    }

    ColorType(ColorType const& src)
      : red(src.red.value_)
    {}

    template<typename ov, int orb, int ors, int ogb, int ogs, int obb, int obs, int oab, int oas>
    ColorType(ColorType<ov, orb, ors, ogb, ogs, obb, obs, oab, oas> const& src)
      : red(0)
    {
      red = src.red;
      green = src.green;
      blue = src.blue;
      alpha = src.alpha;
    }

    ColorType& operator=(ColorType const& src) {
      red.value_ = src.red.value_;
      return *this;
    }

    template<typename ov, int orb, int ors, int ogb, int ogs, int obb, int obs, int oab, int oas>
    ColorType& operator=(ColorType<ov, orb, ors, ogb, ogs, obb, obs, oab, oas> const& src) {
      red = src.red;
      green = src.green;
      blue = src.blue;
      alpha = src.alpha;
      return *this;
    }

    operator value_t() const {
      return red.value_;
    }
    operator value_t&() {
      return red.value_;
    }

    union {
      red_t red;
      green_t green;
      blue_t blue;
      alpha_t alpha;
    };
  };

  template<typename Type>
  Type mix(Type a, int ka, Type b, int kb) {
    return Type(
      (a.red * ka + b.red * kb) / (ka + kb),
      (a.green * ka + b.green * kb) / (ka + kb),
      (a.blue * ka + b.blue * kb) / (ka + kb),
      (a.alpha * ka + b.alpha * kb) / (ka + kb)
    );
  }
  template<typename Type>
  Type mix(Type a, Type b, double t) {
    return Type::clamp(
      static_cast<int>(a.red * (1 - t) + b.red * t),
      static_cast<int>(a.green * (1 - t) + b.green * t),
      static_cast<int>(a.blue * (1 - t) + b.blue * t),
      static_cast<int>(a.alpha * (1 - t) + b.alpha * t)
    );
  }
  template<typename Type>
  Type blend(Type dst, Type src) {
    return mix(dst, 255 - src.alpha, src, src.alpha);
  }
  template<typename Type>
  Type modulate(Type lhs, Type rhs) {
    return Type(
      lhs.red * rhs.red / 255,
      lhs.green * rhs.green / 255,
      lhs.blue * rhs.blue / 255,
      lhs.alpha * rhs.alpha / 255
    );
  }

  template<int A, int B, int G, int R>
  using ABGR = ColorType<UInt<R + G + B + A>, R, 0, G, R, B, R + G, A, R + G + B>;
  template<int X, int B, int G, int R>
  using XBGR = ColorType<UInt<R + G + B + X>, R, 0, G, R, B, R + G, 0, 0>;
  template<int A, int R, int G, int B>
  using ARGB = ColorType<UInt<R + G + B + A>, R, B + G, G, B, B, 0, A, B + G + R>;
  template<int X, int R, int G, int B>
  using XRGB = ColorType<UInt<R + G + B + X>, R, B + G, G, B, B, 0, 0, 0>;
  template<int R, int G, int B, int A>
  using RGBA = ColorType<UInt<R + G + B + A>, R, G + B + A, G, B + A, B, A, A, 0>;
  template<int B, int G, int R, int A>
  using BGRA = ColorType<UInt<R + G + B + A>, R, A, G, R + A, B, G + R + A, A, 0>;

  typedef ARGB<8, 8, 8, 8> Default;

}

namespace ImageFormat {
  enum Type {
    Unknown = 0,
    PNG = 1,
    PNGGrayscale = 2,
    BLP = 3,
    BLP2 = 4,
    PNGGrayAlpha = 5,
    DDS = 6,
    GIF = 7,
    TGA = 8,
    JPG = 9,

    NumFormats
  };
}

struct ImageFilter {
  //enum Type {
  //  Box,
  //  Triangle,
  //  Hermite,
  //  Bell,
  //  CubicBSpline,
  //  Lanczos3,
  //  Mitchell,
  //  Cosine,
  //  CatmullRom,
  //  Quadratic,
  //  QuadraticBSpline,
  //  CubicConvolution,
  //  Lanczos8,
  //};

  typedef double(*Function)(double);
  static double PI() { return 3.1415926535897932384626433832795; }

  static struct {
    static double radius() { return 0.5; }
    static double value(double x) {
      return (x <= 0.5 ? 1.0 : 0.0);
    }
  } Box;
  static struct {
    static double radius() { return 1.0; }
    static double value(double x) {
      return std::max(1.0 - x, 0.0);
    }
  } Triangle;
  static struct {
    static double radius() { return 1.0; }
    static double value(double x) {
      return (x < 1.0 ? (2 * x - 3) * x * x + 1 : 0.0);
    }
  } Hermite;
  static struct {
    static double radius() { return 1.5; }
    static double value(double x) {
      if (x < 0.5) return 0.75 - x * x;
      if (x < 1.5) return 0.5 * (1.5 - x) * (1.5 - x);
      return 0.0;
    }
  } Bell;
  static struct {
    static double radius() { return 2.0; }
    static double value(double x) {
      if (x < 1.0) return 0.5 * x * x * x - x * x + 2.0 / 3.0;
      if (x < 2.0) return (2.0 - x) * (2.0 - x) * (2.0 - x) / 6.0;
      return 0.0;
    }
  } CubicBSpline;
  static struct {
    static double radius() { return 3.0; }
    static double sinc(double x) {
      if (x == 0.0) return 1.0;
      x *= PI();
      return std::sin(x) / x;
    }
    static double value(double x) {
      return (x < 3.0 ? sinc(x) * sinc(x / 3.0) : 0.0);
    }
  } Lanczos3;
  static struct {
    static double radius() { return 2.0; }
    static double value(double x) {
      if (x < 1.0) return 7.0 / 6.0 * x * x * x - 2.0 * x * x + 8.0 / 9.0;
      if (x < 2.0) return -7.0 / 18.0 * x * x * x + 2.0 * x * x - 10.0 / 3.0 * x + 16.0 / 9.0;
      return 0.0;
    }
  } Mitchell;
  static struct {
    static double radius() { return 1.0; }
    static double value(double x) {
      if (x < 1.0) return (std::cos(x * PI()) + 1.0) / 2.0;
      return 0.0;
    }
  } Cosine;
  static struct {
    static double radius() { return 2.0; }
    static double value(double x) {
      if (x < 1.0) return 1.5 * x * x * x - 2.5 * x * x + 1.0;
      if (x < 2.0) return -0.5 * x * x * x + 2.5 * x * x - 4.0 * x + 2.0;
      return 0.0;
    }
  } CatmullRom;
  static struct {
    static double radius() { return 1.5; }
    static double value(double x) {
      if (x < 0.5) return -2.0 * x * x + 1.0;
      if (x < 1.5) return x * x - 2.5 * x + 1.5;
      return 0.0;
    }
  } Quadratic;
  static struct {
    static double radius() { return 1.5; }
    static double value(double x) {
      if (x < 0.5) return -x * x + 0.75;
      if (x < 1.5) return 0.5 * x * x - 1.5 * x + 1.125;
      return 0.0;
    }
  } QuadraticBSpline;
  static struct {
    static double radius() { return 3.0; }
    static double value(double x) {
      if (x < 1.0) return 4.0 / 3.0 * x * x * x - 7.0 / 3.0 * x * x + 1.0;
      if (x < 2.0) return -7.0 / 12.0 * x * x * x + 3.0 * x * x - 59.0 / 12.0 * x + 2.5;
      if (x < 3.0) return 1.0 / 12.0 * x * x * x - 2.0 / 3.0 * x * x + 1.75 * x - 1.5;
      return 0.0;
    }
  } CubicConvolution;
  static struct {
    static double radius() { return 8.0; }
    static double sinc(double x) {
      if (x == 0.0) return 1.0;
      x *= PI();
      return std::sin(x) / x;
    }
    static double value(double x) {
      return (x < 8.0 ? sinc(x) * sinc(x / 8.0) : 0.0);
    }
  } Lanczos8;
};

template<typename color_type>
class ImageBase {
public:
  typedef color_type color_t;

  ImageBase() {};

  template<class other_t>
  ImageBase(ImageBase<other_t> const& image)
    : data_(std::make_shared<Data>(image.width(), image.height()))
  {
    other_t const* src = image.bits();
    for (int i = 0; i < data_->width_ * data_->height_; ++i) {
      data_->bits_[i] = src[i];
    }
  }
  ImageBase(ImageBase<color_t> const& image)
    : data_(image.data_)
  {}
  ImageBase(ImageBase<color_t>&& image)
    : data_(std::move(image.data_))
  {}

  operator bool() const {
    return data_ != nullptr;
  }

  void release() {
    data_.reset();
  }

  template<class other_t>
  ImageBase& operator=(ImageBase<other_t> const& image) {
    if (!image.data_) {
      data_.reset();
      return *this;
    }
    if (data_->width_ * data_->height_ != image.width() * image.height()) {
      data_ = std::make_shared<Data>(image.width(), image.height());
    } else {
      splice();
    }
    data_->width_ = image.width();
    data_->height_ = image.height();
    other_t const* src = image.bits();
    for (int i = 0; i < data_->width_ * data_->height_; ++i) {
      data_->bits_[i] = src[i];
    }
    return *this;
  }
  ImageBase& operator=(ImageBase<color_t> const& image) {
    if (image.data_ == data_) {
      return *this;
    }
    data_ = image.data_;
    return *this;
  }
  ImageBase& operator=(ImageBase<color_t>&& image) {
    if (image.data_ == data_) {
      return *this;
    }
    data_ = std::move(image.data_);
    return *this;
  }

  ImageBase(int width, int height, color_t color = 0)
    : data_(std::make_shared<Data>(width, height))
  {
    for (int i = 0; i < width * height; ++i) {
      data_->bits_[i] = color;
    }
  }
  ImageBase(int width, int height, color_t const* bits)
    : data_(std::make_shared<Data>(width, height))
  {
    memcpy(data_->bits_, bits, width * height * sizeof(color_t));
  }

  color_t const* bits() const {
    return data_->bits_;
  }
  color_t* mutable_bits() {
    splice();
    return data_->bits_;
  }
  size_t size() const {
    return data_->width_ * data_->height_ * sizeof(color_t);
  }
  int width() const {
    return data_->width_;
  }
  int height() const {
    return data_->height_;
  }

  color_t* begin() {
    splice();
    return data_->bits_;
  }
  color_t* end() {
    splice();
    return data_->bits_ + data_->width_ * data_->height_;
  }
  color_t const* begin() const {
    return data_->bits_;
  }
  color_t const* end() const {
    return data_->bits_ + data_->width_ * data_->height_;
  }

  ImageBase subimage(int left, int top, int right, int bottom) const {
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right > data_->width_) right = data_->width_;
    if (bottom > data_->height_) bottom = data_->height_;
    ImageBase result;
    result.data_ = std::make_shared<Data>(right - left, bottom - top);
    for (int y = top; y < bottom; ++y) {
      color_t const* src = data_->bits_ + y * data_->width_ + left;
      color_t* dst = result.data_->bits_ + (y - top) * result.data_->width_;
      for (int x = left; x < right; ++x) {
        *dst++ = *src++;
      }
    }
    return result;
  }
  ImageBase subimagef(float left, float top, float right, float bottom) const {
    return subimage(
      static_cast<int>(data_->width_ * left + 0.5f),
      static_cast<int>(data_->height_ * top + 0.5f),
      static_cast<int>(data_->width_ * right + 0.5f),
      static_cast<int>(data_->height_ * bottom + 0.5f)
    );
  }

  ImageBase(File file, ImageFormat::Type format = ImageFormat::Unknown);
  bool write(File file, ImageFormat::Type format = ImageFormat::PNG);
  bool read(File file, ImageFormat::Type format = ImageFormat::Unknown);

#ifndef NO_SYSTEM
  ImageBase(std::string const& path, ImageFormat::Type format = ImageFormat::Unknown);
  bool write(std::string const& path, ImageFormat::Type format = ImageFormat::Unknown);
  bool read(std::string const& path, ImageFormat::Type format = ImageFormat::Unknown);
#endif

  template<typename Filter = decltype(ImageFilter::Lanczos3)>
  ImageBase resize(int width, int height, Filter filter = ImageFilter::Lanczos3) const;
  template<typename Filter = decltype(ImageFilter::Lanczos3)>
  ImageBase scale(double horz, double vert, Filter filter = ImageFilter::Lanczos3) const;
  void blt(int x, int y, ImageBase const& src, int sx, int sy, int sw, int sh) {
    int dx = x - sx, dy = y - sy;
    int x0 = std::max(0, std::max(sx, -dx));
    int y0 = std::max(0, std::max(sy, -dy));
    int x1 = std::min(src.width(), std::min(sx + sw, data_->width_ - dx));
    int y1 = std::min(src.height(), std::min(sy + sh, data_->height_ - dy));
    if (y1 <= y0 || x1 <= x0) return;
    splice();
    for (int y = y0; y < y1; ++y) {
      color_t const* psrc = src.bits() + y * src.width() + x0;
      color_t* pdst = data_->bits_ + (y + dy) * data_->width_ + x0 + dx;
      for (int cnt = x1 - x0; cnt--;) {
        *pdst = Color::blend(*pdst, *psrc++);
        ++pdst;
      }
    }
  }
  void blt(int x, int y, ImageBase const& src) {
    blt(x, y, src, 0, 0, src.width(), src.height());
  }

private:
  struct Data {
    Data(int w, int h)
      : width_(w)
      , height_(h)
      , bits_(new color_t[w * h])
    {}
    ~Data() {
      delete[] bits_;
    }

    int width_;
    int height_;
    color_t* bits_;
  };
  std::shared_ptr<Data> data_;
  void splice() {
    if (data_.use_count() > 1) {
      auto tmp = std::make_shared<Data>(data_->width_, data_->height_);
      memcpy(tmp->bits_, data_->bits_, sizeof(color_t) * data_->width_ * data_->height_);
      data_ = tmp;
    }
  }
};

typedef ImageBase<Color::Default> Image;

namespace ImagePrivate {
  ImageFormat::Type getFormat(std::string const& name);
  bool imWrite(Image const& image, File file, ImageFormat::Type format);
  Image imRead(File file, ImageFormat::Type format);

  template<typename color_t>
  class Accum {
  public:
    void add(color_t value, double weight) {
      r += weight * value.red;
      g += weight * value.green;
      b += weight * value.blue;
      a += weight * value.alpha;
      w += weight;
    }
    color_t get() const {
      if (w == 0) return 0;
      return color_t::clamp(
        static_cast<int>(r / w + 0.5),
        static_cast<int>(g / w + 0.5),
        static_cast<int>(b / w + 0.5),
        static_cast<int>(a / w + 0.5)
      );
    }
  private:
    double r = 0, g = 0, b = 0, a = 0, w = 0;
  };

  template<class color_t, class Filter>
  class LinScaler {
  public:
    LinScaler(int from, int to, double scale) {
      if (scale < 1) {
        double radius = Filter::radius() / scale;
        for (int i = 0; i < to; ++i) {
          double center = (i + 0.5) / scale;
          int left = std::max(0, static_cast<int>(floor(center - radius)));
          int right = std::min<int>(from, static_cast<int>(ceil(center + radius)) + 1);
          pixels.emplace_back(left, right - left);
          for (int j = left; j < right; ++j) {
            factors.push_back(Filter::value(std::abs((center - j - 0.5) * scale)));
          }
        }
      } else {
        double radius = Filter::radius();
        for (int i = 0; i < to; ++i) {
          double center = (i + 0.5) / scale;
          int left = std::max(0, static_cast<int>(floor(center - radius)));
          int right = std::min<int>(from, static_cast<int>(ceil(center + radius)) + 1);
          pixels.emplace_back(left, right - left);
          for (int j = left; j < right; ++j) {
            factors.push_back(Filter::value(std::abs(center - j - 0.5)));
          }
        }
      }
    }
    void scale(color_t const* src, uint32 src_pitch,
      color_t* dst, uint32 dst_pitch)
    {
      double const* factor = &factors[0];
      for (auto& p : pixels) {
        color_t const* from = src + p.first * src_pitch;
        Accum<color_t> accum;
        for (uint32 i = p.second; i--;) {
          accum.add(*from, *factor++);
          from += src_pitch;
        }
        *dst = accum.get();
        dst += dst_pitch;
      }
    }
  private:
    std::vector<double> factors;
    std::vector<std::pair<uint32, uint32>> pixels;
  };
}

template<class color_t>
ImageBase<color_t>::ImageBase(File file, ImageFormat::Type format)
  : ImageBase(ImagePrivate::imRead(file, format))
{}
#ifndef NO_SYSTEM
template<class color_t>
ImageBase<color_t>::ImageBase(std::string const& path, ImageFormat::Type format)
  : ImageBase(ImagePrivate::imRead(File(path),
    format == ImageFormat::Unknown ? ImagePrivate::getFormat(path) : format))
{}
#endif
template<class color_t>
bool ImageBase<color_t>::write(File file, ImageFormat::Type format) {
  return ImagePrivate::imWrite(*this, file, format);
}
template<class color_t>
bool ImageBase<color_t>::read(File file, ImageFormat::Type format) {
  *this = ImagePrivate::imRead(file, format);
  return data_ != nullptr;
}
#ifndef NO_SYSTEM
template<class color_t>
bool ImageBase<color_t>::write(std::string const& path, ImageFormat::Type format) {
  return ImagePrivate::imWrite(*this, File(path, "wb"),
    format == ImageFormat::Unknown ? ImagePrivate::getFormat(path) : format);
}
template<class color_t>
bool ImageBase<color_t>::read(std::string const& path, ImageFormat::Type format) {
  *this = ImagePrivate::imRead(File(path),
    format == ImageFormat::Unknown ? ImagePrivate::getFormat(path) : format);
  return data_ != nullptr;
}
#endif
template<class color_t>
template<typename Filter>
ImageBase<color_t> ImageBase<color_t>::resize(int width, int height, Filter filter) const {
  ImageBase<color_t> cur(*this);
  if (cur.width() != width) {
    ImagePrivate::LinScaler<color_t, Filter> scaler(cur.width(), width, static_cast<double>(width) / static_cast<double>(cur.width()));
    ImageBase<color_t> next(width, cur.height());
    for (int y = 0; y < cur.height(); ++y) {
      scaler.scale(cur.bits() + y * cur.width(), 1, next.mutable_bits() + y * next.width(), 1);
    }
    cur = next;
  }
  if (cur.height() != height) {
    ImagePrivate::LinScaler<color_t, Filter> scaler(cur.height(), height, static_cast<double>(height) / static_cast<double>(cur.height()));
    ImageBase<color_t> next(cur.width(), height);
    for (int x = 0; x < cur.width(); ++x) {
      scaler.scale(cur.bits() + x, cur.width(), next.mutable_bits() + x, next.width());
    }
    cur = next;
  }
  return cur;
}
template<class color_t>
template<typename Filter>
ImageBase<color_t> ImageBase<color_t>::scale(double horz, double vert, Filter filter) const {
  ImageBase<color_t> cur(*this);
  if (horz != 1) {
    uint32 width = static_cast<int>(ceil(static_cast<double>(cur.width()) * horz));
    ImagePrivate::LinScaler<color_t, Filter> scaler(cur.width(), width, horz);
    ImageBase<color_t> next(width, cur.height());
    for (int y = 0; y < cur.height(); ++y) {
      scaler.scale(cur.bits() + y * cur.width(), 1, next.mutable_bits() + y * next.width(), 1);
    }
    cur = next;
  }
  if (vert != 1) {
    uint32 height = static_cast<int>(ceil(static_cast<double>(cur.height()) * vert));
    ImagePrivate::LinScaler<color_t, Filter> scaler(cur.height(), height, vert);
    ImageBase<color_t> next(cur.width(), height);
    for (int x = 0; x < cur.width(); ++x) {
      scaler.scale(cur.bits() + x, cur.width(), next.mutable_bits() + x, next.width());
    }
    cur = next;
  }
  return cur;
}

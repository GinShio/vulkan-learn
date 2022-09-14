#include "base_type.hpp"
#include "scope_guard.hpp"

#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

namespace {

auto copy_image_data(void const *src, int width, int height)
    -> unsigned char * {
  size_t size = width * height * 4;
  auto *data = new unsigned char[size];
  ::memcpy(data, src, size);
  return data;
}

} // namespace

Image::Image(::std::vector<unsigned char> const &image) {
  stbi_uc *pixels =
      stbi_load_from_memory(image.data(), image.size(), &this->width_,
                            &this->height_, nullptr, STBI_rgb_alpha);
  assert(pixels && "failed to load texture image!");
  MAKE_SCOPE_GUARD { stbi_image_free(pixels); };
  this->data_ = copy_image_data(pixels, this->width_, this->height_);
}

Image::Image(unsigned char const *image, int width, int height)
    : width_{width}, height_{height} {
  this->data_ = copy_image_data(image, this->width_, this->height_);
}

Image::Image(Image const &other)
    : width_{other.width_}, height_{other.height_} {
  this->data_ = copy_image_data(other.data_, this->width_, this->height_);
}

Image::Image(Image &&other) noexcept
    : data_{other.data_}, width_{other.width_}, height_{other.height_} {
  other.data_ = nullptr;
}

auto Image::operator=(Image const &other) -> Image & {
  if (this == &other) {
    return *this;
  }
  this->width_ = other.width_;
  this->height_ = other.height_;
  this->data_ = copy_image_data(other.data_, this->width_, this->height_);
  return *this;
}

auto Image::operator=(Image &&other) noexcept -> Image & {
  ::memcpy(this, &other, sizeof(Image));
  other.data_ = nullptr;
  return *this;
}

Image::~Image() {
  if (this->data_ != nullptr) {
    delete[] this->data_;
    this->data_ = nullptr;
  }
}

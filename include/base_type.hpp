#ifndef BASE_TYPE_HPP_
#define BASE_TYPE_HPP_

#include <utility>
#include <vector>

#include <glm/glm.hpp>

struct MVP {
  alignas(16)::glm::mat4 model, view, project;
};

struct Image {
public:
  Image() = default;
  Image(::std::vector<unsigned char> const &image);
  Image(unsigned char const *image, int width, int height);
  Image(Image const &);
  Image(Image &&) noexcept;
  auto operator=(Image const &) -> Image &;
  auto operator=(Image &&) noexcept -> Image &;
  ~Image();

  auto get_data() const -> unsigned char * { return this->data_; }
  auto get_width() const -> int { return this->width_; }
  auto get_height() const -> int { return this->height_; }
  auto get_size() const -> size_t { return this->width_ * this->height_ * 4; }

private:
  unsigned char *data_{nullptr};
  int width_{0}, height_{0};
};

#endif // BASE_TYPE_HPP_

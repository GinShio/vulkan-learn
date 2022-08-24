#ifndef BASE_TYPE_HPP_
#define BASE_TYPE_HPP_

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <initializer_list>
#include <type_traits>

#define BASETYPE_COMMON_DEFINITION(CLASS, BASE_CLASS)                          \
  CLASS() = default;                                                           \
  CLASS(::std::initializer_list<value_type> ilist) : BASE_CLASS() {            \
    ::std::copy(ilist.begin(), ilist.end(), v);                                \
  }                                                                            \
  CLASS(CLASS const &other) : BASE_CLASS() {                                   \
    ::memcpy(v, other.v, sizeof(other));                                       \
  }                                                                            \
  CLASS(CLASS &&other) noexcept : BASE_CLASS() {                               \
    ::memcpy(v, other.v, sizeof(other));                                       \
  }                                                                            \
  ~CLASS() = default;                                                          \
  auto operator=(CLASS const &other)->CLASS & {                                \
    if (this == &other) {                                                      \
      return *this;                                                            \
    }                                                                          \
    return this->operator=(::std::move(const_cast<CLASS &>(other)));           \
  }                                                                            \
  auto operator=(CLASS &&other) noexcept->CLASS & {                            \
    return *static_cast<decltype(this)>(::memcpy(v, other.v, sizeof(other)));  \
  }                                                                            \
                                                                               \
  auto operator[](size_type idx)->value_type & { return v[idx]; }              \
                                                                               \
  auto begin()->iterator { return v; }                                         \
  auto begin() const->const_iterator {                                         \
    return const_cast<CLASS *>(this)->begin();                                 \
  }                                                                            \
  auto cbegin() const->const_iterator {                                        \
    return const_cast<CLASS *>(this)->begin();                                 \
  }                                                                            \
  auto end()->iterator { return v + dimensionality; }                          \
  auto end() const->const_iterator {                                           \
    return const_cast<CLASS *>(this)->end();                                   \
  }                                                                            \
  auto cend() const->const_iterator { return const_cast<CLASS *>(this)->end(); }

struct Vec2;
struct Vec3;
struct Vec4;

template <typename T, size_t N> struct Vec {
  using vector_type =
      ::std::conditional_t<N == 2, Vec2,
                           ::std::conditional_t<N == 3, Vec3, Vec4>>;

  using size_type = size_t;
  using value_type = T;
  using iterator = value_type *;
  using const_iterator = value_type const *;

  static constexpr size_type dimensionality{N};

  value_type v[dimensionality]{0.f};
};

struct Vec2 : public Vec<float, 2> {
  BASETYPE_COMMON_DEFINITION(Vec2, Vec);
};

struct Vec3 : public Vec<float, 3> {
  BASETYPE_COMMON_DEFINITION(Vec3, Vec);
};

struct Vec4 : public Vec<float, 4> {
  BASETYPE_COMMON_DEFINITION(Vec4, Vec);
};

struct Mat2;
struct Mat3;
struct Mat4;

template <typename T, size_t N> struct Mat {
  using vector_type = typename Vec<T, N>::vector_type;
  using matrix_type =
      ::std::conditional_t<N == 2, Mat2,
                           ::std::conditional_t<N == 3, Mat3, Mat4>>;

  using size_type = size_t;
  using value_type = vector_type;
  using iterator = value_type *;
  using const_iterator = value_type const *;

  static constexpr size_type dimensionality{N};

  value_type v[dimensionality]{{0.f}};

  static auto get_eye(float value = 1.f) -> matrix_type {
    matrix_type mat;
    for (size_type i = 0; i < N; ++i) {
      mat[i][i] = value;
    }
    return mat;
  }
};

struct Mat2 : public Mat<float, 2> {
  BASETYPE_COMMON_DEFINITION(Mat2, Mat);
};

struct Mat3 : public Mat<float, 3> {
  BASETYPE_COMMON_DEFINITION(Mat3, Mat);
};

struct Mat4 : public Mat<float, 4> {
  BASETYPE_COMMON_DEFINITION(Mat4, Mat);
};

struct Vertex {
  Vec2 position;
  Vec3 color;
};

struct MVP {
  Mat4 model, view, project;
};

#undef BASETYPE_COMMON_DEFINITION

#endif // BASE_TYPE_HPP_

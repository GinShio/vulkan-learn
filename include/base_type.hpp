#ifndef BASE_TYPE_HPP_
#define BASE_TYPE_HPP_

struct Vec2 {
  float x{0.f}, y{0.f};
};

struct Vec3 {
  float x{0.f}, y{0.f}, z{0.f};
};

struct RGB {
  float r{0.f}; // red
  float g{0.f}; // green
  float b{0.f}; // blue
};

struct Vertex {
  Vec2 position;
  RGB color;
};

#endif // BASE_TYPE_HPP_

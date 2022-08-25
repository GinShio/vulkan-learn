#ifndef BASE_TYPE_HPP_
#define BASE_TYPE_HPP_

#include <glm/glm.hpp>

struct Vertex {
  ::glm::vec2 position;
  ::glm::vec3 color;
};

struct MVP {
  ::glm::mat4 model, view, project;
};

#undef BASETYPE_COMMON_DEFINITION

#endif // BASE_TYPE_HPP_

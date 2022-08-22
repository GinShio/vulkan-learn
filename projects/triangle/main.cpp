
#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "renderer.hpp"
#include "triangle.hpp"
#include "window.hpp"

namespace fs = ::std::filesystem;

auto main(int argc, char const *const argv[]) -> int {
  if (argc != 2) {
    ::std::cerr << "usage: " << argv[0] << " "
                << "shader_path" << ::std::endl;
    return EXIT_FAILURE;
  }
  auto shader_path = ::fs::path{argv[1]};

  try {
    TriangleApplication triangle;
    triangle.init(shader_path);
    triangle.run();
    triangle.destroy();
  } catch (::std::exception const &e) {
    ::std::cerr << e.what() << ::std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

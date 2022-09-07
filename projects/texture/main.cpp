#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "renderer.hpp"
#include "texture.hpp"

namespace fs = ::std::filesystem;

::fs::path shader_path;

auto main(int argc, char const *const argv[]) -> int {
  if (argc < 2) {
    ::std::cerr << "usage: " << argv[0] << " "
                << "shader_path" << ::std::endl;
    return EXIT_FAILURE;
  }
  shader_path = ::fs::path{argv[1]};

  try {
    TextureApplication app;
    app.init();
    app.run();
    app.destroy();
  } catch (::std::exception const &e) {
    ::std::cerr << e.what() << ::std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

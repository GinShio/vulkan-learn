#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "renderer.hpp"
#include "texture.hpp"

namespace fs = ::std::filesystem;

::fs::path shader_path;
::fs::path image_path;

auto main(int argc, char const *const argv[]) -> int {
  if (argc < 3) {
    ::std::cerr << "usage: " << argv[0] << " "
                << "shader_path"
                << " "
                << "image_path" << ::std::endl;
    return EXIT_FAILURE;
  }
  shader_path = ::fs::path{argv[1]};
  image_path = ::fs::path{argv[2]};

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

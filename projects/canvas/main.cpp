#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

#include "canvas.hpp"
#include "renderer.hpp"

namespace fs = ::std::filesystem;

::fs::path shader_path;
::std::string shader_name;

auto main(int argc, char const *const argv[]) -> int {
  if (argc < 3) {
    ::std::cerr << "usage: " << argv[0] << " "
                << "shader_path shader_name" << ::std::endl;

    if (argc < 2) {
      return EXIT_FAILURE;
    }
    shader_path = ::fs::path{argv[1]};
    ::std::cerr << ::std::endl << "support shader name:" << ::std::endl;
    for (auto const &dir_entry :
         std::filesystem::directory_iterator{shader_path}) {
      auto filename = dir_entry.path().filename().string();
      auto pos = filename.find(".frag.spv");
      if (pos != decltype(filename)::npos) {
        ::std::cout << "\t" << filename.substr(0, pos) << ::std::endl;
      }
    }
    return EXIT_FAILURE;
  }
  shader_path = ::fs::path{argv[1]};
  shader_name = argv[2];

  try {
    CanvasApplication canvas;
    canvas.init();
    canvas.run();
    canvas.destroy();
  } catch (::std::exception const &e) {
    ::std::cerr << e.what() << ::std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

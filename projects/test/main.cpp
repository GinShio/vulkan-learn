#include <iostream>
#include <vector>

#include <SDL.h>
#include <SDL_events.h>
#include <SDL_vulkan.h>

#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtx/string_cast.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

auto get_window_extensions(SDL_Window *window) -> ::std::vector<char const *> {
  unsigned int count{0};
  SDL_Vulkan_GetInstanceExtensions(window, &count, nullptr);
  ::std::vector<char const *> names{
      static_cast<decltype(names)::size_type>(count)};
  SDL_Vulkan_GetInstanceExtensions(window, &count, names.data());
  return names;
}

template <typename T>
auto print_extensions(::std::vector<T> const &extensions) -> void {
  for (auto const &extension : extensions) {
    ::std::cout << "\t" << extension.extensionName << ::std::endl;
  }
}

template <>
auto print_extensions(::std::vector<char const *> const &extensions) -> void {
  for (auto const &extension : extensions) {
    ::std::cout << "\t" << extension << ::std::endl;
  }
}

int main() {
  SDL_Window *window = SDL_CreateWindow("Vulkan Window", SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED, 800, 600,
                                        SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);

  auto instance_extensions = ::vk::enumerateInstanceExtensionProperties();
  ::std::cout << instance_extensions.size() << " instance extensions supported"
              << ::std::endl;
  print_extensions(instance_extensions);

  ::std::cout << ::std::endl;

  auto window_extensions = get_window_extensions(window);
  ::std::cout << window_extensions.size() << " SDL extensions needed"
              << ::std::endl;
  print_extensions(window_extensions);

  ::glm::mat4 matrix{1.f};
  ::glm::vec4 vec{2.f};
  ::std::cout << ::glm::to_string(matrix) << ::std::endl;
  ::std::cout << ::glm::to_string(vec) << ::std::endl;
  auto ans = matrix * vec;
  ::std::cout << "matrix * vec: " << ::glm::to_string(ans) << ::std::endl;

  SDL_Event event;
  bool is_quited{false};
  while (!is_quited) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        is_quited = true;
      }
    }
  }

  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}

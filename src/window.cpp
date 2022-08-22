#include "window.hpp"

namespace {

::std::string const default_name{"SDL_Vulkan"};

int const default_width{800};

int const default_height{600};

Uint32 const default_flags{SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN};

} // namespace

Window::Window()
    : Window(default_name, default_width, default_height, default_flags) {}

Window::Window(::std::string const &name)
    : Window(name, default_width, default_height, default_flags) {}

Window::Window(Uint32 flags)
    : Window(default_name, default_width, default_height, flags) {}

Window::Window(int width, int height)
    : Window(default_name, width, height, default_flags) {}

Window::Window(::std::string const &name, int width, int height, Uint32 flags)
    : window_{SDL_CreateWindow(name.c_str(), SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED, width, height, flags)} {}

auto Window::get_window() -> SDL_Window * { return this->window_; }
auto Window::get_window() const -> SDL_Window const * { return this->window_; }

auto Window::get_extensions() -> ::std::vector<char const *> {
  unsigned int count{0};
  SDL_Vulkan_GetInstanceExtensions(this->window_, &count, nullptr);
  ::std::vector<char const *> names{
      static_cast<decltype(names)::size_type>(count)};
  SDL_Vulkan_GetInstanceExtensions(this->window_, &count, names.data());
  return names;
}

Window::~Window() {
  SDL_DestroyWindow(this->window_);
  SDL_Quit();
}

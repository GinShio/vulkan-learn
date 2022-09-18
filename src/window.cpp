#include "window.hpp"

#include <utility>

namespace {

::std::string const kDefaultName{"SDL_Vulkan"};

int const kDefaultWidth{800};

int const kDefaultHeight{600};

Uint32 const kDefaultFlags{SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN};

} // namespace

Window::Window()
    : Window(kDefaultName, kDefaultWidth, kDefaultHeight, kDefaultFlags) {}

Window::Window(::std::string const &name)
    : Window(name, kDefaultWidth, kDefaultHeight, kDefaultFlags) {}

Window::Window(Uint32 flags)
    : Window(kDefaultName, kDefaultWidth, kDefaultHeight, flags) {}

Window::Window(int width, int height)
    : Window(kDefaultName, width, height, kDefaultFlags) {}

Window::Window(::std::string const &name, int width, int height, Uint32 flags)
    : window_{SDL_CreateWindow(name.c_str(), SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED, width, height, flags)} {}

Window::Window(Window &&other) noexcept
    : is_quited_{other.is_quited_}, window_{other.window_},
      event_{other.event_}, keycode_{other.keycode_}, mouse_{other.mouse_} {
  other.window_ = nullptr;
}

Window &Window::operator=(Window &&other) noexcept {
  ::std::swap(this->is_quited_, other.is_quited_);
  ::std::swap(this->window_, other.window_);
  ::std::swap(this->event_, other.event_);
  ::std::swap(this->keycode_, other.keycode_);
  ::std::swap(this->mouse_, other.mouse_);
  return *this;
}

auto Window::get_window() -> SDL_Window * { return this->window_; }
auto Window::get_window() const -> SDL_Window const * { return this->window_; }

auto Window::get_size() const -> ::std::pair<int, int> {
  int width{0};
  int height{0};
  SDL_GetWindowSize(this->window_, &width, &height);
  return ::std::make_pair(width, height);
}

auto Window::get_mouse_state() const -> ::std::pair<int, int> {
  return this->mouse_;
}

auto Window::get_keycode() const -> SDL_Keycode { return this->keycode_; }

auto Window::get_extensions() -> ::std::vector<char const *> {
  unsigned int count{0};
  SDL_Vulkan_GetInstanceExtensions(this->window_, &count, nullptr);
  ::std::vector<char const *> names{
      static_cast<decltype(names)::size_type>(count)};
  SDL_Vulkan_GetInstanceExtensions(this->window_, &count, names.data());
  return names;
}

Window::~Window() {
  if (this->window_ != nullptr) {
    SDL_DestroyWindow(this->window_);
  }
}

auto Window::event_handling() -> void {
  if (this->event_.type == SDL_MOUSEMOTION) {
    SDL_GetMouseState(&this->mouse_.first, &this->mouse_.second);
  }
  if (this->event_.type == SDL_QUIT) {
    this->is_quited_ = true;
  }
  switch (this->event_.key.keysym.sym) {
  case SDLK_ESCAPE: {
    this->is_quited_ = true;
    break;
  }
  default: {
    this->keycode_ = this->event_.key.keysym.sym;
    break;
  }
  }
}

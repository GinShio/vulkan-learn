#ifndef WINDOW_HPP_
#define WINDOW_HPP_

#include <string>
#include <vector>

#include <SDL.h>
#include <SDL_events.h>
#include <SDL_vulkan.h>

class Window final {
public:
  Window();
  explicit Window(::std::string const &name);
  explicit Window(Uint32 flags);
  Window(int width, int height);
  Window(::std::string const &name, int width, int height, Uint32 flags);
  Window(Window const &) = default;
  Window(Window &&) noexcept = default;
  Window &operator=(Window const &) = default;
  Window &operator=(Window &&) noexcept = default;
  ~Window();

  template <typename App> auto main_loop(App *app, void (*func)(App *)) -> void;

  auto get_window() -> SDL_Window *;
  auto get_window() const -> SDL_Window const *;

  auto get_extensions() -> ::std::vector<char const *>;

private:
public:
private:
  SDL_Window *window_{nullptr};
  SDL_Event event_;
  bool is_quited_{false};
};

template <typename App>
inline auto Window::main_loop(App *app, void (*func)(App *)) -> void {
  while (!this->is_quited_) {
    while (SDL_PollEvent(&this->event_)) {
      if (this->event_.type == SDL_QUIT) {
        this->is_quited_ = true;
      }
      func(app);
    }
  }
}

#endif // WINDOW_HPP_

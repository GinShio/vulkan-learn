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
  Window(Window const &) = delete;
  Window(Window &&other) noexcept;
  Window &operator=(Window const &) = delete;
  Window &operator=(Window &&other) noexcept;
  ~Window();

  template <typename App> auto main_loop(App *app, void (*func)(App *)) -> void;

  auto get_window() -> SDL_Window *;
  auto get_window() const -> SDL_Window const *;

  auto get_size() const -> ::std::pair<int, int>;

  auto get_mouse_state() const -> ::std::pair<int, int>;
  auto get_keycode() const -> SDL_Keycode;

  auto get_extensions() -> ::std::vector<char const *>;

private:
  auto event_handling() -> void;

public:
private:
  bool is_quited_{false};
  SDL_Window *window_{nullptr};
  SDL_Event event_;
  SDL_Keycode keycode_{0};
  ::std::pair<int, int> mouse_{0, 0};
};

template <typename App>
inline auto Window::main_loop(App *app, void (*func)(App *)) -> void {
  while (!this->is_quited_) {
    while (SDL_PollEvent(&this->event_)) {
      this->event_handling();
    }
    func(app);
  }
}

#endif // WINDOW_HPP_

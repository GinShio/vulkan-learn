#ifndef CANVAS_HPP_
#define CANVAS_HPP_

#include "renderer.hpp"

class CanvasApplication : public Renderer<CanvasApplication> {
  friend class Renderer<CanvasApplication>;

private:
  auto app_init(QueueFamilyIndices &queue_indices) -> void;

  auto app_destroy() -> void;

  auto get_vertex_input_description() -> decltype(auto);

  auto record_command(::vk::CommandBuffer &cbuf, ::vk::Framebuffer &fbuf)
      -> void;

private:
  ::vk::DeviceMemory device_memory_{nullptr};
  ::std::vector<::vk::Buffer> device_buffers_;
};

#endif // CANVAS_HPP_

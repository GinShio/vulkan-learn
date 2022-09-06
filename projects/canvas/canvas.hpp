#ifndef CANVAS_HPP_
#define CANVAS_HPP_

#include "renderer.hpp"

class CanvasApplication : public Renderer<CanvasApplication> {
  using this_class = CanvasApplication;
  using base_class = Renderer<this_class>;

  friend base_class;

public:
  CanvasApplication();

private:
  auto app_init(QueueFamilyIndices &queue_indices) -> void;

  auto app_destroy() -> void;

  auto get_vertex_input_description() -> decltype(auto);

  auto record_command(::vk::CommandBuffer &cbuf, ::vk::Framebuffer &fbuf)
      -> void;

  ::vk::DeviceMemory device_memory_{nullptr};
  ::std::vector<::vk::Buffer> device_buffers_;
  ::std::vector<::vk::ShaderModule> shader_modules_;
};

#endif // CANVAS_HPP_

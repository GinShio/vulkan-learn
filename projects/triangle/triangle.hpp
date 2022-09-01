#ifndef TRIANGLE_HPP_
#define TRIANGLE_HPP_

#include "renderer.hpp"

class TriangleApplication : public Renderer<TriangleApplication> {
  using this_class = TriangleApplication;
  using base_class = Renderer<this_class>;

  friend base_class;

private:
  auto app_init(QueueFamilyIndices &queue_indices) -> void;

  auto app_destroy() -> void;

  auto get_vertex_input_description() -> decltype(auto);

  auto record_command(::vk::CommandBuffer &cbuf, ::vk::Framebuffer &fbuf)
      -> void;

  ::vk::DeviceMemory device_memory_{nullptr};
  ::vk::DescriptorSetLayout set_layout_{nullptr};
  ::vk::DescriptorPool desc_pool_{nullptr};
  ::vk::DescriptorSet desc_set_{nullptr};

  ::std::vector<::vk::Buffer> device_buffers_;
};

#endif // TRIANGLE_HPP_

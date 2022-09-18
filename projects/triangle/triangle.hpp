#ifndef TRIANGLE_HPP_
#define TRIANGLE_HPP_

#include "renderer.hpp"

#include <chrono>

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

  ::std::chrono::time_point<::std::chrono::system_clock> start_time_{
      ::std::chrono::system_clock::now()};
  ::vk::DeviceMemory device_memory_{nullptr};
  ::vk::DescriptorSetLayout set_layout_{nullptr};
  ::vk::DescriptorPool desc_pool_{nullptr};

  ::std::vector<::vk::Buffer> device_buffers_;
  ::std::vector<::vk::ShaderModule> shader_modules_;
  ::std::vector<::vk::DescriptorSet> desc_sets_;
};

#endif // TRIANGLE_HPP_

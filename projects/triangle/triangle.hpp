#ifndef TRIANGLE_HPP_
#define TRIANGLE_HPP_

#include "renderer.hpp"

class TriangleApplication : public Renderer<TriangleApplication> {
  friend class Renderer<TriangleApplication>;

private:
  auto app_init(QueueFamilyIndices &queue_indices,
                ::std::filesystem::path const &shader_path) -> void;

  auto app_destroy() -> void;

  auto get_vertex_input_description() -> decltype(auto);

  auto create_pipeline_layout() -> ::vk::PipelineLayout;

  auto record_command(::vk::CommandBuffer &cbuf, ::vk::Framebuffer &fbuf)
      -> void;

  auto render() -> void;

private:
  size_t current_frame_{0};
  ::vk::RenderPass render_pass_{nullptr};
  ::vk::CommandPool cmd_pool_{nullptr};
  ::vk::DeviceMemory device_memory_{nullptr};

  ::vk::DescriptorSetLayout set_layout_{nullptr};
  ::vk::DescriptorPool desc_pool_{nullptr};
  ::vk::DescriptorSet desc_set_{nullptr};
  ::vk::Pipeline pipeline_{nullptr};

  ::std::vector<::vk::Framebuffer> framebuffers_;
  ::std::vector<::vk::CommandBuffer> cmd_buffers_;
  ::std::vector<::vk::Buffer> device_buffers_;
  ::std::vector<::vk::Semaphore> image_avaliables_;
  ::std::vector<::vk::Semaphore> present_finishes_;
  ::std::vector<::vk::Fence> fences_;
};

#endif // TRIANGLE_HPP_

#ifndef TEXTURE_HPP_
#define TEXTURE_HPP_

#include "renderer.hpp"

class TextureApplication : public Renderer<TextureApplication> {
  using this_class = TextureApplication;
  using base_class = Renderer<this_class>;

  friend base_class;

private:
  auto app_init(QueueFamilyIndices &queue_indices) -> void;

  auto app_destroy() -> void;

  auto get_vertex_input_description() -> decltype(auto);

  auto record_command(::vk::CommandBuffer &cbuf, ::vk::Framebuffer &fbuf)
      -> void;

  ::vk::DeviceMemory texture_memory_{nullptr};
  ::vk::DeviceMemory device_memory_{nullptr};
  ::vk::Sampler sampler_{nullptr};

  ::std::vector<::vk::Buffer> device_buffers_;
  ::std::vector<::vk::Image> texture_images_;
  ::std::vector<::vk::ImageView> texture_imageviews_;
  ::std::vector<::vk::DescriptorPool> desc_pools_;
  ::std::vector<::vk::DescriptorSet> desc_sets_;
  ::std::vector<::vk::DescriptorSetLayout> set_layouts_;
  ::std::vector<::vk::ShaderModule> shader_modules_;
};

#endif // TEXTURE_HPP_

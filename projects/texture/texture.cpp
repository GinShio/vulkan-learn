#include "texture.hpp"

#include "base_type.hpp"
#include "create.hpp"
#include "scope_guard.hpp"

#include <stddef.h>
#include <string.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <limits>
#include <tuple>
#include <utility>

#include <vulkan/vulkan.hpp>

#include <iostream>

extern ::std::filesystem::path shader_path;

namespace {

struct Vertex {
  ::glm::vec2 position;
  ::glm::vec2 texture;
};

// triangle
::std::array vertices{
    Vertex{{-.5f, -.5f}, {1.f, 0.f}},
    Vertex{{.5f, -.5f}, {0.f, 0.f}},
    Vertex{{.5f, .5f}, {0.f, 1.f}},
    Vertex{{-.5f, .5f}, {1.f, 1.f}},
};

::std::array<uint16_t, 6> indices{0, 1, 2, 2, 3, 0};

auto create_render_pass(::vk::Device &device,
                        SwapchainRequiredInfo &required_info)
    -> ::vk::RenderPass {
  ::vk::AttachmentDescription att_desc;
  att_desc.setSamples(::vk::SampleCountFlagBits::e1)
      .setLoadOp(::vk::AttachmentLoadOp::eClear)
      .setStoreOp(::vk::AttachmentStoreOp::eStore)
      .setStencilLoadOp(::vk::AttachmentLoadOp::eDontCare)
      .setStencilStoreOp(::vk::AttachmentStoreOp::eDontCare)
      .setFormat(required_info.format.format)
      .setInitialLayout(::vk::ImageLayout::eUndefined)
      .setFinalLayout(::vk::ImageLayout::ePresentSrcKHR);

  ::vk::AttachmentReference att_ref;
  att_ref.setLayout(::vk::ImageLayout::eColorAttachmentOptimal)
      .setAttachment(0);
  ::vk::SubpassDescription sub_desc;
  sub_desc.setPipelineBindPoint(::vk::PipelineBindPoint::eGraphics)
      .setColorAttachments(att_ref);

  ::vk::RenderPassCreateInfo info;
  info.setAttachments(att_desc).setSubpasses(sub_desc);
  ::vk::RenderPass render_pass = device.createRenderPass(info);
  assert(render_pass && "render pass create failed!");
  return render_pass;
}

auto create_pipeline_layout(
    ::vk::Device &device, ::std::vector<::vk::DescriptorSetLayout> &set_layouts)
    -> ::vk::PipelineLayout {
  ::vk::PipelineLayoutCreateInfo info;
  info.setSetLayouts(set_layouts);
  ::vk::PipelineLayout layout = device.createPipelineLayout(info);
  assert(layout && "pipeline layout create failed!");
  return layout;
}

auto create_texture_sampler(::vk::PhysicalDevice &physical,
                            ::vk::Device &device) -> ::vk::Sampler {
  auto properties = physical.getProperties();
  ::vk::SamplerCreateInfo info;
  info.setMagFilter(::vk::Filter::eLinear)
      .setMinFilter(::vk::Filter::eLinear)
      .setAddressModeU(::vk::SamplerAddressMode::eRepeat)
      .setAddressModeV(::vk::SamplerAddressMode::eRepeat)
      .setAddressModeW(::vk::SamplerAddressMode::eRepeat)
      .setAnisotropyEnable(VK_TRUE)
      .setMaxAnisotropy(properties.limits.maxSamplerAnisotropy)
      .setBorderColor(::vk::BorderColor::eIntOpaqueBlack)
      .setUnnormalizedCoordinates(VK_FALSE)
      .setCompareEnable(VK_FALSE)
      .setCompareOp(::vk::CompareOp::eAlways)
      .setMipmapMode(::vk::SamplerMipmapMode::eLinear)
      .setMipLodBias(.0f)
      .setMinLod(.0f)
      .setMaxLod(.0f);

  ::vk::Sampler sampler = device.createSampler(info);
  assert(sampler && "sampler create failed!");
  return sampler;
}

auto create_descriptor_pool(::vk::Device &device, size_t max_size)
    -> ::vk::DescriptorPool {
  static ::vk::DescriptorPoolSize size{
      ::vk::DescriptorType::eCombinedImageSampler,
      static_cast<uint32_t>(max_size)};
  ::vk::DescriptorPoolCreateInfo info;
  info.setFlags(::vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
      .setPoolSizes(size)
      .setMaxSets(max_size);
  auto pool = device.createDescriptorPool(info);
  assert(pool && "descriptor pool create failed!");
  return pool;
}

auto allocate_descriptor_set(::vk::Device &device, ::vk::DescriptorPool &pool,
                             ::std::vector<::vk::ImageView> &views,
                             ::vk::Sampler &sampler)
    -> ::std::pair<::std::vector<::vk::DescriptorSetLayout>,
                   ::std::vector<::vk::DescriptorSet>> {
  ::std::vector<::vk::DescriptorSetLayout> layouts;
  layouts.reserve(views.size());
  ::vk::DescriptorSetLayoutBinding binding;
  for (auto end = views.size(), i = static_cast<decltype(end)>(0); i < end;
       ++i) {
    binding = ::vk::DescriptorSetLayoutBinding{
        0, ::vk::DescriptorType::eCombinedImageSampler, 1,
        ::vk::ShaderStageFlagBits::eFragment};
    ::vk::DescriptorSetLayout layout =
        device.createDescriptorSetLayout(::vk::DescriptorSetLayoutCreateInfo{
            ::vk::DescriptorSetLayoutCreateFlags{}, 1, &binding});
    assert(layout && "descriptor set layout create failed!");
    layouts.emplace_back(layout);
  }

  ::vk::DescriptorSetAllocateInfo info;
  info.setSetLayouts(layouts)
      .setDescriptorSetCount(views.size())
      .setDescriptorPool(pool);
  ::std::vector<::vk::DescriptorSet> sets = device.allocateDescriptorSets(info);
  assert(!sets.empty() && "descriptor set allocate failed!");

  ::vk::DescriptorImageInfo image_info;
  image_info.setImageLayout(::vk::ImageLayout::eShaderReadOnlyOptimal)
      .setSampler(sampler);
  for (auto end = views.size(), i = static_cast<decltype(end)>(0); i < end;
       ++i) {
    image_info.setImageView(views[i]);
    ::vk::WriteDescriptorSet write_set;
    write_set.setDescriptorType(::vk::DescriptorType::eCombinedImageSampler)
        .setDstSet(sets[i])
        .setDstArrayElement(0)
        .setDstBinding(0)
        .setImageInfo(image_info);
    device.updateDescriptorSets(write_set, {});
  }
  return ::std::make_pair(layouts, sets);
}

} // namespace

auto TextureApplication::app_init(QueueFamilyIndices &queue_indices) -> void {
  this->render_pass_ = create_render_pass(this->device_, this->required_info_);
  this->framebuffers_ =
      create_frame_buffers(this->device_, this->swapchain_imageviews_,
                           this->render_pass_, this->required_info_);

  ::std::vector buffers = {
      wrap_buffer(this->physical_, this->device_, queue_indices, vertices,
                  ::vk::BufferUsageFlagBits::eVertexBuffer),
      wrap_buffer(this->physical_, this->device_, queue_indices, indices,
                  ::vk::BufferUsageFlagBits::eIndexBuffer),
  };
  ::std::tie(this->device_buffers_, this->device_memory_) =
      allocate_memory<::vk::Buffer>(this->physical_, this->device_,
                                    this->cmd_pool_, this->graphics_queue_,
                                    buffers,
                                    ::vk::MemoryPropertyFlagBits::eDeviceLocal);

  auto rin_image = create_image_data("images/KagamineRin.png");
  auto len_image = create_image_data("images/KagamineLen.png");
  MAKE_SCOPE_GUARD {
    delete[] ::std::get<0>(rin_image);
    delete[] ::std::get<0>(len_image);
  };
  ::std::vector images = {
      wrap_image(this->physical_, this->device_, queue_indices,
                 ::std::get<0>(rin_image), ::std::get<1>(rin_image),
                 ::std::get<2>(rin_image), ::vk::ImageUsageFlagBits::eSampled),
      wrap_image(this->physical_, this->device_, queue_indices,
                 ::std::get<0>(len_image), ::std::get<1>(len_image),
                 ::std::get<2>(len_image), ::vk::ImageUsageFlagBits::eSampled),
  };
  ::std::tie(this->texture_images_, this->texture_memory_) =
      allocate_memory<::vk::Image>(this->physical_, this->device_,
                                   this->cmd_pool_, this->graphics_queue_,
                                   images,
                                   ::vk::MemoryPropertyFlagBits::eDeviceLocal);
  this->texture_imageviews_ = create_image_views(
      this->device_, this->texture_images_, ::vk::Format::eR8G8B8A8Srgb);
  this->sampler_ = create_texture_sampler(this->physical_, this->device_);
  this->desc_pool_ =
      create_descriptor_pool(this->device_, this->required_info_.image_count);
  ::std::tie(this->set_layouts_, this->desc_sets_) =
      allocate_descriptor_set(this->device_, this->desc_pool_,
                              this->texture_imageviews_, this->sampler_);

  this->shader_modules_ = {
      create_shader_module(this->device_, shader_path / "main.vert.spv"),
      create_shader_module(this->device_, shader_path / "main.frag.spv"),
  };
  this->layout_ = create_pipeline_layout(this->device_, this->set_layouts_);
  ::vk::PipelineShaderStageCreateInfo vert_stage;
  vert_stage.setStage(::vk::ShaderStageFlagBits::eVertex)
      .setModule(this->shader_modules_[0])
      .setPName("main");
  ::vk::PipelineShaderStageCreateInfo frag_stage;
  frag_stage.setStage(::vk::ShaderStageFlagBits::eFragment)
      .setModule(this->shader_modules_[1])
      .setPName("main");
  this->pipeline_ = this->create_pipeline({vert_stage, frag_stage});
}

auto TextureApplication::app_destroy() -> void {
  this->device_.destroyPipeline(this->pipeline_);
  this->device_.destroyPipelineLayout(this->layout_);
  for (auto &shader : this->shader_modules_) {
    this->device_.destroyShaderModule(shader);
  }
  this->device_.freeDescriptorSets(this->desc_pool_, this->desc_sets_);
  for (auto &setlayout : this->set_layouts_) {
    this->device_.destroyDescriptorSetLayout(setlayout);
  }
  this->device_.destroyDescriptorPool(this->desc_pool_);
  this->device_.destroySampler(this->sampler_);
  this->device_.freeMemory(this->texture_memory_);
  for (auto end = this->texture_images_.size(),
            i = static_cast<decltype(end)>(0);
       i < end; ++i) {
    this->device_.destroyImageView(this->texture_imageviews_[i]);
    this->device_.destroyImage(this->texture_images_[i]);
  }
  this->device_.freeMemory(this->device_memory_);
  for (auto &buffer : this->device_buffers_) {
    this->device_.destroyBuffer(buffer);
  }
  for (auto &buffer : this->framebuffers_) {
    this->device_.destroyFramebuffer(buffer);
  }
  this->device_.destroyRenderPass(this->render_pass_);
}

auto TextureApplication::get_vertex_input_description() -> decltype(auto) {
  ::std::array<::vk::VertexInputAttributeDescription, 2> attr_descs;
  attr_descs[0]
      .setBinding(0)
      .setLocation(0)
      .setFormat(::vk::Format::eR32G32Sfloat)
      .setOffset(offsetof(Vertex, position));
  attr_descs[1]
      .setBinding(0)
      .setLocation(1)
      .setFormat(::vk::Format::eR32G32Sfloat)
      .setOffset(offsetof(Vertex, texture));

  ::vk::VertexInputBindingDescription bind_desc;
  bind_desc.setBinding(0)
      .setInputRate(::vk::VertexInputRate::eVertex)
      .setStride(sizeof(Vertex));

  return ::std::make_pair(attr_descs, bind_desc);
}

auto TextureApplication::record_command(::vk::CommandBuffer &cbuf,
                                        ::vk::Framebuffer &fbuf) -> void {
  ::vk::CommandBufferBeginInfo begin_info;
  begin_info.setFlags(::vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
  [[maybe_unused]] auto result = cbuf.begin(&begin_info);
  assert(result == ::vk::Result::eSuccess && "command buffer record failed!");

  ::vk::ClearValue value{::std::array<float, 4>{1.f, 1.f, 1.f, 1.f}};
  ::vk::RenderPassBeginInfo render_pass_begin;
  render_pass_begin.setRenderPass(this->render_pass_)
      .setRenderArea(
          ::vk::Rect2D{::vk::Offset2D{0, 0}, this->required_info_.extent})
      .setClearValues(value)
      .setFramebuffer(fbuf);
  cbuf.beginRenderPass(render_pass_begin, ::vk::SubpassContents::eInline);
  cbuf.bindPipeline(::vk::PipelineBindPoint::eGraphics, this->pipeline_);

  cbuf.bindVertexBuffers(0, this->device_buffers_[0], {0});
  cbuf.bindIndexBuffer(this->device_buffers_[1], 0, ::vk::IndexType::eUint16);
  cbuf.bindDescriptorSets(::vk::PipelineBindPoint::eGraphics, this->layout_, 0,
                          this->desc_sets_, {});

  cbuf.drawIndexed(indices.size(), 1, 0, 0, 0);

  cbuf.endRenderPass();
  cbuf.end();
}

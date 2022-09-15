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

#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.hpp>

extern ::std::filesystem::path shader_path;

namespace {

struct Vertex {
  ::glm::vec2 position;
  ::glm::vec2 texture;
};

// triangle
::std::array vertices{
    // rin
    Vertex{{.125f, -1.f}, {1.f, 0.f}},
    Vertex{{.875f, -1.f}, {0.f, 0.f}},
    Vertex{{.875f, 0.f}, {0.f, 1.f}},
    Vertex{{.125f, 0.f}, {1.f, 1.f}},
    // len
    Vertex{{-.875f, -1.f}, {1.f, 0.f}},
    Vertex{{-.125f, -1.f}, {0.f, 0.f}},
    Vertex{{-.125f, 0.f}, {0.f, 1.f}},
    Vertex{{-.875f, 0.f}, {1.f, 1.f}},
};

::std::array<uint16_t, 12> indices{0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4};

::std::array ebos{
    // rin
    MVP{
        ::glm::rotate(::glm::mat4(1.f), ::glm::radians(-20.f),
                      ::glm::vec3(0.f, 1.f, 0.f)),
        ::glm::translate(::glm::mat4(1.f), glm::vec3(0.f, 0.5f, -3.6f)),
        ::glm::perspective(::glm::radians(30.f), 4.f / 3.f, .1f, 5.f),
    },
    // len
    MVP{
        ::glm::rotate(::glm::mat4(1.f), ::glm::radians(20.f),
                      ::glm::vec3(0.f, 1.f, 0.f)),
        ::glm::translate(::glm::mat4(1.f), glm::vec3(0.f, 0.5f, -3.6f)),
        ::glm::perspective(::glm::radians(30.f), 4.f / 3.f, .1f, 5.f),
    },
};

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

} // namespace

auto TextureApplication::app_init(QueueFamilyIndices &queue_indices) -> void {
  this->render_pass_ = create_render_pass(this->device_, this->required_info_);
  this->framebuffers_ =
      create_frame_buffers(this->device_, this->swapchain_imageviews_,
                           this->render_pass_, this->required_info_);

  ::std::vector buffers{
      wrap_buffer(this->physical_, this->device_, queue_indices, vertices,
                  ::vk::BufferUsageFlagBits::eVertexBuffer),
      wrap_buffer(this->physical_, this->device_, queue_indices, indices,
                  ::vk::BufferUsageFlagBits::eIndexBuffer),
      wrap_buffer(this->physical_, this->device_, queue_indices, ebos.data(), 1,
                  ::vk::BufferUsageFlagBits::eUniformBuffer),
      wrap_buffer(this->physical_, this->device_, queue_indices, &ebos[1], 1,
                  ::vk::BufferUsageFlagBits::eUniformBuffer),
  };
  ::std::tie(this->device_buffers_, this->device_memory_) =
      allocate_memory<::vk::Buffer>(this->physical_, this->device_,
                                    this->cmdpool_, this->graphics_, buffers,
                                    ::vk::MemoryPropertyFlagBits::eDeviceLocal);
  auto [pool0, layout0, sets0] = allocate_descriptor_set<::vk::Buffer>(
      this->device_, this->device_buffers_.begin() + 2,
      this->device_buffers_.end(), ::vk::DescriptorType::eUniformBuffer,
      ::vk::ShaderStageFlagBits::eVertex, sizeof(MVP));

  ::std::vector images{
      wrap_image(this->physical_, this->device_, queue_indices,
                 create_image_data("images/KagamineRin.png"),
                 ::vk::ImageUsageFlagBits::eSampled),
      wrap_image(this->physical_, this->device_, queue_indices,
                 create_image_data("images/KagamineLen.png"),
                 ::vk::ImageUsageFlagBits::eSampled),
  };
  ::std::tie(this->texture_images_, this->texture_memory_) =
      allocate_memory<::vk::Image>(this->physical_, this->device_,
                                   this->cmdpool_, this->graphics_, images,
                                   ::vk::MemoryPropertyFlagBits::eDeviceLocal);
  this->texture_imageviews_ = create_image_views(
      this->device_, this->texture_images_, ::vk::Format::eR8G8B8A8Srgb);
  this->sampler_ = create_texture_sampler(this->physical_, this->device_);
  auto [pool1, layout1, sets1] = allocate_descriptor_set<::vk::ImageView>(
      this->device_, this->texture_imageviews_.begin(),
      this->texture_imageviews_.end(),
      ::vk::DescriptorType::eCombinedImageSampler,
      ::vk::ShaderStageFlagBits::eFragment, this->sampler_);

  this->desc_pools_ = {pool0, pool1};
  this->set_layouts_ = {layout0, layout1};
  this->desc_sets_.insert(this->desc_sets_.end(), sets0.begin(), sets0.end());
  this->desc_sets_.insert(this->desc_sets_.end(), sets1.begin(), sets1.end());
  this->layout_ = create_pipeline_layout(this->device_, this->set_layouts_);
  this->shader_modules_ = {
      create_shader_module(this->device_, shader_path / "main.vert.spv"),
      create_shader_module(this->device_, shader_path / "main.frag.spv"),
  };
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
  for (auto &setlayout : this->set_layouts_) {
    this->device_.destroyDescriptorSetLayout(setlayout);
  }
  for (auto end = this->desc_pools_.size(), i = static_cast<decltype(end)>(0);
       i < end; ++i) {
    this->device_.freeDescriptorSets(this->desc_pools_[i], this->desc_sets_[i]);
    this->device_.destroyDescriptorPool(this->desc_pools_[i]);
  }
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

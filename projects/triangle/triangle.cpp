#include "triangle.hpp"

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

extern ::std::filesystem::path shader_path;

namespace {

struct Vertex {
  ::glm::vec2 position;
  ::glm::vec3 color;
};

// triangle
::std::array vertices{
    Vertex{{0.f, -0.5f}, {1.f, 0.f, 0.f}},
    Vertex{{0.5f, 0.5f}, {0.f, 1.f, 0.f}},
    Vertex{{-0.5f, 0.5f}, {0.f, 0.f, 1.f}},
};

::std::array<uint16_t, 3> indices{0, 1, 2};

MVP ebo{
    // model
    ::glm::mat4{1.f},
    // view
    ::glm::mat4{1.f},
    // project
    ::glm::mat4{1.f},
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

auto create_pipeline_layout(::vk::Device &device,
                            ::vk::DescriptorSetLayout &set_layout)
    -> ::vk::PipelineLayout {
  ::vk::PushConstantRange range;
  range.setOffset(0)
      .setStageFlags(::vk::ShaderStageFlagBits::eVertex)
      .setSize(sizeof(float));
  ::vk::PipelineLayoutCreateInfo info;
  info.setSetLayouts(set_layout).setPushConstantRanges(range);
  ::vk::PipelineLayout layout = device.createPipelineLayout(info);
  assert(layout && "pipeline layout create failed!");
  return layout;
}

} // namespace

auto TriangleApplication::app_init(QueueFamilyIndices &queue_indices) -> void {
  this->render_pass_ = create_render_pass(this->device_, this->required_info_);
  this->framebuffers_ =
      create_frame_buffers(this->device_, this->swapchain_imageviews_,
                           this->render_pass_, this->required_info_);
  ::std::vector buffers = {
      wrap_buffer(this->physical_, this->device_, queue_indices, vertices,
                  ::vk::BufferUsageFlagBits::eVertexBuffer),
      wrap_buffer(this->physical_, this->device_, queue_indices, indices,
                  ::vk::BufferUsageFlagBits::eIndexBuffer),
      wrap_buffer(this->physical_, this->device_, queue_indices, &ebo, 1,
                  ::vk::BufferUsageFlagBits::eUniformBuffer),
  };
  ::std::tie(this->device_buffers_, this->device_memory_) =
      allocate_memory<::vk::Buffer>(this->physical_, this->device_,
                                    this->cmdpool_, this->graphics_, buffers,
                                    ::vk::MemoryPropertyFlagBits::eDeviceLocal);

  ::std::tie(this->desc_pool_, this->set_layout_, this->desc_sets_) =
      allocate_descriptor_set<::vk::Buffer>(
          this->device_, --this->device_buffers_.end(),
          this->device_buffers_.end(), ::vk::DescriptorType::eUniformBuffer,
          ::vk::ShaderStageFlagBits::eVertex, sizeof(MVP));

  this->shader_modules_ = {
      create_shader_module(this->device_, shader_path / "main.vert.spv"),
      create_shader_module(this->device_, shader_path / "main.frag.spv"),
  };
  this->layout_ = create_pipeline_layout(this->device_, this->set_layout_);
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

auto TriangleApplication::app_destroy() -> void {
  this->device_.destroyPipeline(this->pipeline_);
  this->device_.destroyPipelineLayout(this->layout_);
  for (auto &shader : this->shader_modules_) {
    this->device_.destroyShaderModule(shader);
  }
  this->device_.freeDescriptorSets(this->desc_pool_, this->desc_sets_);
  this->device_.destroyDescriptorPool(this->desc_pool_);
  this->device_.destroyDescriptorSetLayout(set_layout_);
  this->device_.freeMemory(this->device_memory_);
  for (auto &buffer : this->device_buffers_) {
    this->device_.destroyBuffer(buffer);
  }
  for (auto &buffer : this->framebuffers_) {
    this->device_.destroyFramebuffer(buffer);
  }
  this->device_.destroyRenderPass(this->render_pass_);
}

auto TriangleApplication::get_vertex_input_description() -> decltype(auto) {
  ::std::array<::vk::VertexInputAttributeDescription, 2> attr_descs;
  attr_descs[0]
      .setBinding(0)
      .setLocation(0)
      .setFormat(::vk::Format::eR32G32Sfloat)
      .setOffset(offsetof(Vertex, position));
  attr_descs[1]
      .setBinding(0)
      .setLocation(1)
      .setFormat(::vk::Format::eR32G32B32Sfloat)
      .setOffset(offsetof(Vertex, color));

  ::vk::VertexInputBindingDescription bind_desc;
  bind_desc.setBinding(0)
      .setInputRate(::vk::VertexInputRate::eVertex)
      .setStride(sizeof(Vertex));

  return ::std::make_pair(attr_descs, bind_desc);
}

auto TriangleApplication::record_command(::vk::CommandBuffer &cbuf,
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

  auto time = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
                  this->start_time_ - ::std::chrono::system_clock::now())
                  .count() /
              1000.f;
  float color = ::glm::abs(::glm::mix(.0, 1., ::glm::sin(time)));
  cbuf.pushConstants(this->layout_, ::vk::ShaderStageFlagBits::eVertex, 0,
                     sizeof(float), &color);

  // cbuf.draw(vertices.size(), 1, 0, 0);
  cbuf.drawIndexed(indices.size(), 1, 0, 0, 0);

  cbuf.endRenderPass();
  cbuf.end();
}

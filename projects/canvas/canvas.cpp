#include "canvas.hpp"

#include "base_type.hpp"
#include "create.hpp"
#include "scope_guard.hpp"

#include <math.h>
#include <stddef.h>
#include <string.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <limits>
#include <tuple>
#include <utility>

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

extern ::std::filesystem::path shader_path;
extern ::std::string shader_name;

namespace {

::std::array vertices{
    ::glm::vec2{-1.f, -1.f},
    ::glm::vec2{-1.f, 1.f},
    ::glm::vec2{1.f, -1.f},
    ::glm::vec2{1.f, 1.f},
};

::std::array<uint16_t, 6> indices{0, 1, 2, 1, 2, 3};

struct PushConstantObject {
  alignas(4)::glm::float32_t time;
  alignas(8)::glm::vec2 extent;
  alignas(8)::glm::vec2 mouse;
} pco;

struct SpecializationConstantData {
  float PI{::acos(-1.f)};
} scd;

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

auto create_pipeline_layout(::vk::Device &device) -> ::vk::PipelineLayout {
  ::vk::PushConstantRange range;
  range.setOffset(0)
      .setStageFlags(::vk::ShaderStageFlagBits::eFragment)
      .setSize(sizeof(PushConstantObject));
  ::vk::PipelineLayoutCreateInfo info;
  info.setPushConstantRanges(range);
  ::vk::PipelineLayout layout = device.createPipelineLayout(info);
  assert(layout && "pipeline layout create failed!");
  return layout;
}

auto get_special_map_entries()
    -> ::std::array<::vk::SpecializationMapEntry, 1> {
  ::std::array<::vk::SpecializationMapEntry, 1> entries;
  entries[0]
      .setConstantID(0)
      .setSize(sizeof(SpecializationConstantData::PI))
      .setOffset(offsetof(SpecializationConstantData, PI));
  return entries;
}

auto update_pushconstant(
    Window &window, ::vk::Extent2D &extent,
    ::std::chrono::time_point<::std::chrono::system_clock> &start) {
  auto now = ::std::chrono::system_clock::now();
  pco.time =
      ::std::chrono::duration_cast<::std::chrono::milliseconds>(now - start)
          .count() /
      1000.f;
  pco.extent = {extent.width, extent.height};
  auto [mouse_x, mouse_y] = window.get_mouse_state();
  pco.mouse = {mouse_x, mouse_y};
}

} // namespace

CanvasApplication::CanvasApplication() : base_class() {
  this->window_ = Window{::std::string{"Canvas - "} + shader_name};
  this->start_time_ = decltype(this->start_time_)::clock::now();
}

auto CanvasApplication::app_init(QueueFamilyIndices &queue_indices) -> void {
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
                                    this->cmdpool_, this->graphics_, buffers,
                                    ::vk::MemoryPropertyFlagBits::eDeviceLocal);

  this->layout_ = create_pipeline_layout(this->device_);
  auto entries = get_special_map_entries();
  ::vk::SpecializationInfo special_info;
  special_info.setMapEntries(entries)
      .setDataSize(sizeof(SpecializationConstantData))
      .setPData(&scd);
  this->shader_modules_ = {
      create_shader_module(this->device_, shader_path / "main.vert.spv"),
      create_shader_module(this->device_,
                           shader_path / (shader_name + ".frag.spv")),
  };
  ::vk::PipelineShaderStageCreateInfo vert_stage;
  vert_stage.setStage(::vk::ShaderStageFlagBits::eVertex)
      .setModule(this->shader_modules_[0])
      .setPName("main");
  ::vk::PipelineShaderStageCreateInfo frag_stage;
  frag_stage.setStage(::vk::ShaderStageFlagBits::eFragment)
      .setModule(this->shader_modules_[1])
      .setPName("main")
      .setPSpecializationInfo(&special_info);
  this->pipeline_ = this->create_pipeline({vert_stage, frag_stage});
}

auto CanvasApplication::app_destroy() -> void {
  this->device_.destroyPipeline(this->pipeline_);
  this->device_.destroyPipelineLayout(this->layout_);
  for (auto &shader : this->shader_modules_) {
    this->device_.destroyShaderModule(shader);
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

auto CanvasApplication::get_vertex_input_description() -> decltype(auto) {
  ::vk::VertexInputAttributeDescription attr_desc;
  attr_desc.setFormat(::vk::Format::eR32G32Sfloat)
      .setBinding(0)
      .setLocation(0)
      .setOffset(0);
  ::vk::VertexInputBindingDescription bind_desc;
  bind_desc.setBinding(0)
      .setInputRate(::vk::VertexInputRate::eVertex)
      .setStride(sizeof(::glm::vec2));
  return ::std::make_pair(attr_desc, bind_desc);
}

auto CanvasApplication::record_command(::vk::CommandBuffer &cbuf,
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

  update_pushconstant(this->window_, this->required_info_.extent,
                      this->start_time_);
  cbuf.bindVertexBuffers(0, this->device_buffers_[0], {0});
  cbuf.bindIndexBuffer(this->device_buffers_[1], 0, ::vk::IndexType::eUint16);
  cbuf.pushConstants(this->layout_, ::vk::ShaderStageFlagBits::eFragment, 0,
                     sizeof(PushConstantObject), &pco);
  cbuf.drawIndexed(indices.size(), 1, 0, 0, 0);

  cbuf.endRenderPass();
  cbuf.end();
}

#include "canvas.hpp"

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
  int millisec;
  uint32_t : 1; // out of range if delete it
  ::glm::vec2 extent;
} pco;

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

template <typename T, size_t N>
auto wrap_buffer(::vk::PhysicalDevice &physical, ::vk::Device &device,
                 QueueFamilyIndices &queue_indices,
                 ::std::array<T, N> const &data, ::vk::BufferUsageFlags flag)
    -> ::std::tuple<::vk::Buffer, ::vk::DeviceMemory, ::vk::Buffer,
                    ::vk::DeviceSize> {
  ::vk::DeviceSize size = sizeof(data);
  ::vk::Buffer host_buffer = create_buffer(
      device, queue_indices, size, ::vk::BufferUsageFlagBits::eTransferSrc);
  ::vk::DeviceMemory host_memory =
      allocate_memory(physical, device, host_buffer,
                      ::vk::MemoryPropertyFlagBits::eHostVisible |
                          ::vk::MemoryPropertyFlagBits::eHostCoherent);
  copy_data(device, host_memory, 0, size, data.data());
  ::vk::Buffer device_buffer =
      create_buffer(device, queue_indices, size,
                    ::vk::BufferUsageFlagBits::eTransferDst | flag);
  return ::std::make_tuple(host_buffer, host_memory, device_buffer, size);
}

} // namespace

auto CanvasApplication::app_init(QueueFamilyIndices &queue_indices) -> void {
  this->render_pass_ = create_render_pass(this->device_, this->required_info_);
  this->framebuffers_ =
      create_frame_buffers(this->device_, this->image_views_,
                           this->render_pass_, this->required_info_);
  ::std::vector buffers = {
      wrap_buffer(this->physical_, this->device_, queue_indices, vertices,
                  ::vk::BufferUsageFlagBits::eVertexBuffer),
      wrap_buffer(this->physical_, this->device_, queue_indices, indices,
                  ::vk::BufferUsageFlagBits::eIndexBuffer),
  };
  ::std::tie(this->device_buffers_, this->device_memory_) = allocate_memory(
      this->physical_, this->device_, this->cmd_pool_, this->graphics_queue_,
      buffers, ::vk::MemoryPropertyFlagBits::eDeviceLocal);

  auto vert = this->create_shader_module(shader_path / "main.vert.spv");
  auto frag =
      this->create_shader_module(shader_path / (shader_name + ".frag.spv"));
  this->layout_ = create_pipeline_layout(this->device_);
  this->pipeline_ = this->create_vf_pipeline(vert, frag);

  pco.extent = {this->required_info_.extent.width,
                this->required_info_.extent.height};
}

auto CanvasApplication::app_destroy() -> void {
  this->device_.destroyPipeline(this->pipeline_);
  this->device_.destroyPipelineLayout(this->layout_);
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

  cbuf.bindVertexBuffers(0, this->device_buffers_[0], {0});
  cbuf.bindIndexBuffer(this->device_buffers_[1], 0, ::vk::IndexType::eUint16);

  pco.millisec = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
                     ::std::chrono::system_clock::now().time_since_epoch())
                     .count();
  cbuf.pushConstants(this->layout_, ::vk::ShaderStageFlagBits::eFragment, 0,
                     sizeof(PushConstantObject), &pco);

  cbuf.drawIndexed(indices.size(), 1, 0, 0, 0);

  cbuf.endRenderPass();
  cbuf.end();
}

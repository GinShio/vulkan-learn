#include "triangle.hpp"

#include "base_type.hpp"
#include "create.hpp"
#include "scope_guard.hpp"

#include <algorithm>
#include <ctime>
#include <stddef.h>
#include <string.h>

#include <limits>
#include <utility>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>

namespace {

// triangle
static ::std::array vertices{
    Vertex{{0.f, -0.5f}, {1.f, 0.f, 0.f}},
    Vertex{{0.5f, 0.5f}, {0.f, 1.f, 0.f}},
    Vertex{{-0.5f, 0.5f}, {0.f, 0.f, 1.f}},
};

static ::std::array<uint16_t, 3> indices{0, 1, 2};

// rectangle
// static ::std::array vertices{
//     Vertex{{-0.5f, -0.5f}, {1.f, 0.f, 0.f}},
//     Vertex{{0.5f, -0.5f}, {0.f, 1.f, 0.f}},
//     Vertex{{0.5f, 0.5f}, {0.f, 0.f, 1.f}},
//     Vertex{{-0.5f, 0.5f}, {0.f, 1.f, 0.f}},
// };

// static ::std::array<uint16_t, 6> indices{0, 1, 2, 0, 2, 3};

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

auto create_command_pool(::vk::Device &device,
                         QueueFamilyIndices &queue_indices)
    -> ::vk::CommandPool {
  ::vk::CommandPoolCreateInfo info;
  info.setFlags(::vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
      .setQueueFamilyIndex(queue_indices.graphics_indices.value());

  auto cmd_pool = device.createCommandPool(info);
  assert(cmd_pool && "command pool create failed!");
  return cmd_pool;
}

//! @brief create vertex and index buffer
//! @return wrap the host and device buffer in pair
auto create_vertex_buffer(::vk::PhysicalDevice &physical, ::vk::Device &device,
                          QueueFamilyIndices &queue_indices)
    -> ::std::vector<
        ::std::tuple<::vk::Buffer, ::vk::DeviceMemory, ::vk::Buffer>> {
  // vertex buffer
  ::vk::Buffer host_vertex_buffer =
      create_buffer(device, queue_indices, sizeof(vertices),
                    ::vk::BufferUsageFlagBits::eTransferSrc);
  ::vk::DeviceMemory host_vertex_memory =
      allocate_memory(physical, device, host_vertex_buffer,
                      ::vk::MemoryPropertyFlagBits::eHostVisible |
                          ::vk::MemoryPropertyFlagBits::eHostCoherent);
  copy_data(device, host_vertex_memory, 0, sizeof(vertices), vertices.data());
  ::vk::Buffer device_vertex_buffer =
      create_buffer(device, queue_indices, sizeof(vertices),
                    ::vk::BufferUsageFlagBits::eTransferDst |
                        ::vk::BufferUsageFlagBits::eVertexBuffer);

  // index buffer
  ::vk::Buffer host_index_buffer =
      create_buffer(device, queue_indices, sizeof(indices),
                    ::vk::BufferUsageFlagBits::eTransferSrc);
  ::vk::DeviceMemory host_index_memory =
      allocate_memory(physical, device, host_index_buffer,
                      ::vk::MemoryPropertyFlagBits::eHostVisible |
                          ::vk::MemoryPropertyFlagBits::eHostCoherent);
  copy_data(device, host_index_memory, 0, sizeof(indices), indices.data());
  ::vk::Buffer device_index_buffer =
      create_buffer(device, queue_indices, sizeof(indices),
                    ::vk::BufferUsageFlagBits::eTransferDst |
                        ::vk::BufferUsageFlagBits::eIndexBuffer);

  return ::std::vector{
      ::std::make_tuple(host_vertex_buffer, host_vertex_memory,
                        device_vertex_buffer),
      ::std::make_tuple(host_index_buffer, host_index_memory,
                        device_index_buffer),
  };
}

} // namespace

auto TriangleApplication::app_init(QueueFamilyIndices &queue_indices,
                                   ::std::filesystem::path const &shader_path)
    -> void {
  this->render_pass_ = create_render_pass(this->device_, this->required_info_);
  auto vert = this->create_shader_module(shader_path / "main.vert.spv");
  auto frag = this->create_shader_module(shader_path / "main.frag.spv");
  this->pipeline_ = this->create_vf_pipeline(vert, frag);
  this->framebuffers_ =
      create_frame_buffers(this->device_, this->image_views_,
                           this->render_pass_, this->required_info_);
  this->cmd_pool_ = create_command_pool(this->device_, queue_indices);
  this->cmd_buffers_ = allocate_command_buffers(this->device_, this->cmd_pool_,
                                                this->framebuffers_.size());
  auto buffers =
      create_vertex_buffer(this->physical_, this->device_, queue_indices);
  ::std::tie(this->device_buffers_, this->device_memory_) = allocate_memory(
      this->physical_, this->device_, this->cmd_pool_, this->graphics_queue_,
      buffers, ::vk::MemoryPropertyFlagBits::eDeviceLocal);

  this->image_avaliables_ =
      create_semaphores(this->device_, this->framebuffers_.size());
  this->present_finishes_ =
      create_semaphores(this->device_, this->framebuffers_.size());
  this->fences_ = create_fences(this->device_, this->framebuffers_.size());
}

auto TriangleApplication::app_destroy() -> void {
  for (decltype(::std::declval<decltype(this->framebuffers_)>().size())
           i = 0,
           e = this->framebuffers_.size();
       i < e; ++i) {
    this->device_.destroyFence(this->fences_[i]);
    this->device_.destroySemaphore(this->present_finishes_[i]);
    this->device_.destroySemaphore(this->image_avaliables_[i]);
  }
  this->device_.freeMemory(this->device_memory_);
  for (auto &buffer : this->device_buffers_) {
    this->device_.destroyBuffer(buffer);
  }
  this->device_.freeCommandBuffers(this->cmd_pool_, this->cmd_buffers_);
  this->device_.destroyCommandPool(this->cmd_pool_);
  for (auto &buffer : this->framebuffers_) {
    this->device_.destroyFramebuffer(buffer);
  }
  this->device_.destroyPipeline(this->pipeline_);
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

auto TriangleApplication::create_pipeline_layout() -> ::vk::PipelineLayout {
  ::vk::PipelineLayoutCreateInfo info;
  ::vk::PipelineLayout layout = this->device_.createPipelineLayout(info);
  assert(layout && "pipeline layout create failed!");
  return layout;
}

auto TriangleApplication::record_command(::vk::CommandBuffer &cbuf,
                                         ::vk::Framebuffer &fbuf) -> void {
  ::vk::CommandBufferBeginInfo begin_info;
  begin_info.setFlags(::vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
  auto result = cbuf.begin(&begin_info);
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

  // cbuf.draw(vertices.size(), 1, 0, 0);
  cbuf.drawIndexed(indices.size(), 1, 0, 0, 0);

  cbuf.endRenderPass();
  cbuf.end();
}

auto TriangleApplication::render() -> void {
  auto result = this->device_.acquireNextImageKHR(
      this->swapchain_, ::std::numeric_limits<uint64_t>::max(),
      this->image_avaliables_[this->current_frame_], nullptr);
  assert(result.result == ::vk::Result::eSuccess && "acquire image failed!");
  uint32_t image_index = result.value;

  this->device_.resetFences(this->fences_[this->current_frame_]);
  this->cmd_buffers_[this->current_frame_].reset();
  this->record_command(this->cmd_buffers_[this->current_frame_],
                       this->framebuffers_[image_index]);

  ::vk::PipelineStageFlags flags{
      ::vk::PipelineStageFlagBits::eColorAttachmentOutput};
  ::vk::SubmitInfo submit_info;
  submit_info.setCommandBuffers(this->cmd_buffers_[this->current_frame_])
      .setWaitSemaphores(this->image_avaliables_[this->current_frame_])
      .setSignalSemaphores(this->present_finishes_[this->current_frame_])
      .setWaitDstStageMask(flags);
  this->graphics_queue_.submit(submit_info,
                               this->fences_[this->current_frame_]);

  ::vk::PresentInfoKHR present_info;
  present_info.setImageIndices(image_index)
      .setSwapchains(this->swapchain_)
      .setWaitSemaphores(this->present_finishes_[this->current_frame_]);
  assert(this->present_queue_.presentKHR(present_info) ==
             ::vk::Result::eSuccess &&
         "present failed!");

  assert(this->device_.waitForFences(this->fences_[this->current_frame_], true,
                                     ::std::numeric_limits<uint64_t>::max()) ==
             ::vk::Result::eSuccess &&
         "wait fences failed!");
}

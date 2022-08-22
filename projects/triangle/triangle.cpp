#include "triangle.hpp"

#include "base_type.hpp"
#include "create.hpp"
#include "scope_guard.hpp"

#include <stddef.h>
#include <string.h>

#include <limits>
#include <utility>

#include <vulkan/vulkan.hpp>

namespace {

// triangle
// static ::std::array vertices{
//     Vertex{{0.f, -0.5f}, {1.f, 0.f, 0.f}},
//     Vertex{{0.5f, 0.5f}, {0.f, 1.f, 0.f}},
//     Vertex{{-0.5f, 0.5f}, {0.f, 0.f, 1.f}},
// };

// static ::std::array<uint16_t, 3> indices{0, 1, 2};

// rectangle
static ::std::array vertices{
    Vertex{{-0.5f, -0.5f}, {1.f, 0.f, 0.f}},
    Vertex{{0.5f, -0.5f}, {0.f, 1.f, 0.f}},
    Vertex{{0.5f, 0.5f}, {0.f, 0.f, 1.f}},
    Vertex{{-0.5f, 0.5f}, {0.f, 1.f, 0.f}},
};

static ::std::array<uint16_t, 6> indices{0, 1, 2, 0, 2, 3};

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

auto allocate_command_buffers(::vk::Device &device, ::vk::CommandPool &pool,
                              size_t sz) -> ::std::vector<::vk::CommandBuffer> {
  ::vk::CommandBufferAllocateInfo alloc_info;
  alloc_info.setLevel(::vk::CommandBufferLevel::ePrimary)
      .setCommandPool(pool)
      .setCommandBufferCount(sz);
  return device.allocateCommandBuffers(alloc_info);
}

auto create_device_buffer(::vk::PhysicalDevice &physical, ::vk::Device &device,
                          ::vk::Queue &queue, ::vk::CommandPool &pool,
                          QueueFamilyIndices &queue_indices)
    -> ::std::tuple<::vk::Buffer, ::vk::Buffer, ::vk::DeviceMemory> {
  // vertex buffer
  ::vk::Buffer host_vertex_buffer =
      create_buffer(device, queue_indices, sizeof(vertices),
                    ::vk::BufferUsageFlagBits::eTransferSrc);
  ::vk::DeviceMemory host_vertex_memory =
      allocate_memory(physical, device, {host_vertex_buffer},
                      ::vk::MemoryPropertyFlagBits::eHostVisible |
                          ::vk::MemoryPropertyFlagBits::eHostCoherent);
  void *data = device.mapMemory(host_vertex_memory, 0, sizeof(vertices));
  MAKE_SCOPE_GUARD {
    device.unmapMemory(host_vertex_memory);
    device.freeMemory(host_vertex_memory);
    device.destroyBuffer(host_vertex_buffer);
  };
  ::memcpy(data, vertices.data(), sizeof(vertices));

  // index buffer
  ::vk::Buffer host_index_buffer =
      create_buffer(device, queue_indices, sizeof(indices),
                    ::vk::BufferUsageFlagBits::eTransferSrc);
  ::vk::DeviceMemory host_index_memory =
      allocate_memory(physical, device, {host_index_buffer},
                      ::vk::MemoryPropertyFlagBits::eHostVisible |
                          ::vk::MemoryPropertyFlagBits::eHostCoherent);
  data = device.mapMemory(host_index_memory, 0, sizeof(indices));
  MAKE_SCOPE_GUARD {
    device.unmapMemory(host_index_memory);
    device.freeMemory(host_index_memory);
    device.destroyBuffer(host_index_buffer);
  };
  ::memcpy(data, indices.data(), sizeof(indices));

  // device buffer
  ::vk::Buffer device_vertex_buffer =
      create_buffer(device, queue_indices, sizeof(vertices),
                    ::vk::BufferUsageFlagBits::eTransferDst |
                        ::vk::BufferUsageFlagBits::eVertexBuffer);
  ::vk::Buffer device_index_buffer =
      create_buffer(device, queue_indices, sizeof(indices),
                    ::vk::BufferUsageFlagBits::eTransferDst |
                        ::vk::BufferUsageFlagBits::eIndexBuffer);
  ::vk::DeviceMemory device_memory = allocate_memory(
      physical, device, {device_vertex_buffer, device_index_buffer},
      ::vk::MemoryPropertyFlagBits::eDeviceLocal);

  auto transfer_cmd_buffers = allocate_command_buffers(device, pool, 1);
  MAKE_SCOPE_GUARD { device.freeCommandBuffers(pool, transfer_cmd_buffers); };
  ::vk::CommandBufferBeginInfo begin_info;
  begin_info.setFlags(::vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
  transfer_cmd_buffers.front().begin(begin_info);
  transfer_cmd_buffers.front().copyBuffer(
      host_vertex_buffer, device_vertex_buffer,
      ::vk::BufferCopy{0, 0, sizeof(vertices)});
  transfer_cmd_buffers.front().copyBuffer(
      host_index_buffer, device_index_buffer,
      ::vk::BufferCopy{0, 0, sizeof(indices)});
  transfer_cmd_buffers.front().end();
  ::vk::SubmitInfo submit_info;
  submit_info.setCommandBuffers(transfer_cmd_buffers);
  queue.submit(submit_info);
  device.waitIdle();
  return ::std::make_tuple(::std::move(device_vertex_buffer),
                           ::std::move(device_index_buffer),
                           ::std::move(device_memory));
}

} // namespace

auto TriangleApplication::app_init(QueueFamilyIndices &queue_indices,
                                   ::std::filesystem::path const &shader_path)
    -> void {
  this->layout_ = create_pipeline_layout(this->device_);
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
  ::std::tie(this->vertex_buffer_, this->index_buffer_, this->device_memory_) =
      create_device_buffer(this->physical_, this->device_,
                           this->graphics_queue_, this->cmd_pool_,
                           queue_indices);

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
  this->device_.destroyBuffer(this->index_buffer_);
  this->device_.destroyBuffer(this->vertex_buffer_);
  this->device_.freeCommandBuffers(this->cmd_pool_, this->cmd_buffers_);
  this->device_.destroyCommandPool(this->cmd_pool_);
  for (auto &buffer : this->framebuffers_) {
    this->device_.destroyFramebuffer(buffer);
  }
  this->device_.destroyPipeline(this->pipeline_);
  this->device_.destroyRenderPass(this->render_pass_);
  this->device_.destroyPipelineLayout(this->layout_);
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

  cbuf.bindVertexBuffers(0, this->vertex_buffer_, {0});
  cbuf.bindIndexBuffer(this->index_buffer_, 0, ::vk::IndexType::eUint16);

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

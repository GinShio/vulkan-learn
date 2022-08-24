#include "triangle.hpp"

#include "base_type.hpp"
#include "create.hpp"
#include "scope_guard.hpp"

#include <stddef.h>
#include <string.h>

#include <algorithm>
#include <array>
#include <limits>
#include <tuple>
#include <utility>

#include <vulkan/vulkan.hpp>

namespace {

// triangle
static ::std::array vertices{
    Vertex{{0.f, -0.5f}, {1.f, 0.f, 0.f}},
    Vertex{{0.5f, 0.5f}, {0.f, 1.f, 0.f}},
    Vertex{{-0.5f, 0.5f}, {0.f, 0.f, 1.f}},
};

static ::std::array<uint16_t, 3> indices{0, 1, 2};

static MVP ebo{
    // model
    Mat4::get_eye(),
    // view
    Mat4::get_eye(),
    // project
    Mat4::get_eye(),
};

static constexpr float DELTA{0.8e-3f};

static constexpr float ESP{1e-6f};

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

auto create_descriptor_set_layout(::vk::Device &device)
    -> ::vk::DescriptorSetLayout {
  static auto binding =
      ::vk::DescriptorSetLayoutBinding{0, ::vk::DescriptorType::eUniformBuffer,
                                       1, ::vk::ShaderStageFlagBits::eVertex};
  static ::vk::DescriptorSetLayoutCreateInfo info{
      ::vk::DescriptorSetLayoutCreateFlags{}, 1, &binding};
  ::vk::DescriptorSetLayout layout = device.createDescriptorSetLayout(info);
  assert(layout && "descriptor set layout create failed!");
  return layout;
}

auto create_descriptor_pool(::vk::Device &device, size_t max_size)
    -> ::vk::DescriptorPool {
  static ::vk::DescriptorPoolSize sz{::vk::DescriptorType::eUniformBuffer, 1};
  ::vk::DescriptorPoolCreateInfo info;
  info.setPoolSizes(sz).setMaxSets(max_size).setFlags(
      ::vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
  auto pool = device.createDescriptorPool(info);
  assert(pool && "descriptor pool create failed!");
  return pool;
}

auto allocate_descriptor_set(::vk::Device &device, ::vk::DescriptorPool &pool,
                             ::vk::DescriptorSetLayout &layout,
                             ::vk::Buffer &buffer) -> ::vk::DescriptorSet {
  ::vk::DescriptorSetAllocateInfo info;
  info.setSetLayouts(layout).setDescriptorPool(pool).setDescriptorSetCount(1);
  ::std::vector<::vk::DescriptorSet> sets = device.allocateDescriptorSets(info);
  assert(sets[0] && "descriptor set allocate failed!");

  vk::DescriptorBufferInfo buffer_info;
  buffer_info.setOffset(0).setRange(sizeof(ebo)).setBuffer(buffer);
  vk::WriteDescriptorSet write_set;
  write_set.setDescriptorType(::vk::DescriptorType::eUniformBuffer)
      .setDstSet(sets[0])
      .setDstArrayElement(0)
      .setDstBinding(0)
      .setBufferInfo(buffer_info);

  device.updateDescriptorSets(write_set, {});
  return sets[0];
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

auto TriangleApplication::app_init(QueueFamilyIndices &queue_indices,
                                   ::std::filesystem::path const &shader_path)
    -> void {
  this->render_pass_ = create_render_pass(this->device_, this->required_info_);
  this->framebuffers_ =
      create_frame_buffers(this->device_, this->image_views_,
                           this->render_pass_, this->required_info_);
  this->cmd_pool_ = create_command_pool(this->device_, queue_indices);
  this->cmd_buffers_ = allocate_command_buffers(
      this->device_, this->cmd_pool_, this->required_info_.image_count);
  ::std::vector buffers = {
      wrap_buffer(this->physical_, this->device_, queue_indices, vertices,
                  ::vk::BufferUsageFlagBits::eVertexBuffer),
      wrap_buffer(this->physical_, this->device_, queue_indices, indices,
                  ::vk::BufferUsageFlagBits::eIndexBuffer),
      wrap_buffer(this->physical_, this->device_, queue_indices,
                  ::std::array{ebo}, ::vk::BufferUsageFlagBits::eUniformBuffer),
  };
  ::std::tie(this->device_buffers_, this->device_memory_) = allocate_memory(
      this->physical_, this->device_, this->cmd_pool_, this->graphics_queue_,
      buffers, ::vk::MemoryPropertyFlagBits::eDeviceLocal);

  this->set_layout_ = create_descriptor_set_layout(this->device_);
  this->desc_pool_ =
      create_descriptor_pool(this->device_, this->required_info_.image_count);
  this->desc_set_ =
      allocate_descriptor_set(this->device_, this->desc_pool_,
                              this->set_layout_, this->device_buffers_.back());

  auto vert = this->create_shader_module(shader_path / "main.vert.spv");
  auto frag = this->create_shader_module(shader_path / "main.frag.spv");
  this->pipeline_ = this->create_vf_pipeline(vert, frag);

  this->image_avaliables_ =
      create_semaphores(this->device_, this->required_info_.image_count);
  this->present_finishes_ =
      create_semaphores(this->device_, this->required_info_.image_count);
  this->fences_ =
      create_fences(this->device_, this->required_info_.image_count);
}

auto TriangleApplication::app_destroy() -> void {
  for (decltype(required_info_.image_count) i = 0;
       i < required_info_.image_count; ++i) {
    this->device_.destroyFence(this->fences_[i]);
    this->device_.destroySemaphore(this->present_finishes_[i]);
    this->device_.destroySemaphore(this->image_avaliables_[i]);
  }
  this->device_.destroyPipeline(this->pipeline_);
  this->device_.freeDescriptorSets(this->desc_pool_, this->desc_set_);
  this->device_.destroyDescriptorPool(this->desc_pool_);
  this->device_.destroyDescriptorSetLayout(set_layout_);
  this->device_.freeMemory(this->device_memory_);
  for (auto &buffer : this->device_buffers_) {
    this->device_.destroyBuffer(buffer);
  }
  this->device_.freeCommandBuffers(this->cmd_pool_, this->cmd_buffers_);
  this->device_.destroyCommandPool(this->cmd_pool_);
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

auto TriangleApplication::create_pipeline_layout() -> ::vk::PipelineLayout {
  ::vk::PushConstantRange range;
  range.setOffset(0)
      .setStageFlags(::vk::ShaderStageFlagBits::eVertex)
      .setSize(sizeof(float));
  ::vk::PipelineLayoutCreateInfo info;
  info.setSetLayouts(this->set_layout_).setPushConstantRanges(range);
  ::vk::PipelineLayout layout = this->device_.createPipelineLayout(info);
  assert(layout && "pipeline layout create failed!");
  return layout;
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
                          this->desc_set_, {});
  static float c{0.00f};
  static bool f{true};
  if (f) {
    c = ::std::clamp(c + DELTA, 0.f, 1.f);
    f = 1.f - c > ESP;
  } else {
    c = ::std::clamp(c - DELTA, 0.f, 1.f);
    f = c - 0.f < ESP;
  }
  cbuf.pushConstants(this->layout_, ::vk::ShaderStageFlagBits::eVertex, 0,
                     sizeof(float), &c);

  // cbuf.draw(vertices.size(), 1, 0, 0);
  cbuf.drawIndexed(indices.size(), 1, 0, 0, 0);

  cbuf.endRenderPass();
  cbuf.end();
}

auto TriangleApplication::render() -> void {
  this->device_.resetFences(this->fences_[this->current_frame_]);

  auto option_index = this->device_.acquireNextImageKHR(
      this->swapchain_, ::std::numeric_limits<uint64_t>::max(),
      this->image_avaliables_[this->current_frame_], nullptr);
  assert(option_index.result == ::vk::Result::eSuccess &&
         "acquire image failed!");
  uint32_t image_index = option_index.value;

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
  [[maybe_unused]] auto result = this->present_queue_.presentKHR(present_info);
  assert(result == ::vk::Result::eSuccess && "present failed!");
  result =
      this->device_.waitForFences(this->fences_[this->current_frame_], true,
                                  ::std::numeric_limits<uint64_t>::max());
  assert(result == ::vk::Result::eSuccess && "wait fences failed!");

  this->current_frame_ =
      (this->current_frame_ + 1) % this->required_info_.image_count;
}

#ifndef RENDERER_HPP_
#define RENDERER_HPP_

#include "create.hpp"
#include "window.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>

#include <vulkan/vulkan.hpp>

template <typename App> class Renderer {
public:
  auto init() -> void;

  auto run() -> void;
  auto destroy() -> void;

  static auto render(Renderer<App> *app) -> void;

protected:
  auto create_shader_module(::std::filesystem::path const &filename)
      -> ::vk::ShaderModule;
  auto create_vf_pipeline(::vk::ShaderModule const &vert,
                          ::vk::ShaderModule const &frag) -> ::vk::Pipeline;

private:
  auto underlying() -> App * { return reinterpret_cast<App *>(this); }

public:
protected:
  Window window_;
  size_t current_frame_{0};

  ::vk::Instance instance_{nullptr};
  ::vk::SurfaceKHR surface_{nullptr};
  ::vk::PhysicalDevice physical_{nullptr};
  ::vk::Device device_{nullptr};
  ::vk::Queue graphics_queue_{nullptr};
  ::vk::Queue present_queue_{nullptr};
  ::vk::SwapchainKHR swapchain_{nullptr};
  ::vk::CommandPool cmd_pool_{nullptr};

  SwapchainRequiredInfo required_info_;
  ::vk::RenderPass render_pass_{nullptr};
  ::vk::PipelineLayout layout_{nullptr};
  ::vk::Pipeline pipeline_{nullptr};

  ::std::vector<::vk::Image> images_;
  ::std::vector<::vk::ImageView> image_views_;
  ::std::vector<::vk::Framebuffer> framebuffers_;
  ::std::vector<::vk::CommandBuffer> cmd_buffers_;
  ::std::vector<::vk::Semaphore> image_avaliables_;
  ::std::vector<::vk::Semaphore> present_finishes_;
  ::std::vector<::vk::Fence> fences_;
  ::std::vector<::vk::ShaderModule> shader_modules_;
};

template <typename App> auto Renderer<App>::init() -> void {
  this->instance_ = create_instance(this->window_);
  this->surface_ = create_surface(this->window_, this->instance_);
  this->physical_ = pickup_physical_device(this->instance_, this->surface_);
  QueueFamilyIndices queue_indices =
      pickup_queue_family(this->physical_, this->surface_);
  this->device_ = create_logic_device(this->physical_, queue_indices);
  this->graphics_queue_ =
      this->device_.getQueue(queue_indices.graphics_indices.value(), 0);
  this->present_queue_ =
      this->device_.getQueue(queue_indices.present_indices.value(), 0);
  this->required_info_ = query_swapchain_required_info(
      this->window_.get_window(), this->physical_, this->surface_, 5);
  this->swapchain_ = create_swapchain(this->device_, this->surface_,
                                      queue_indices, this->required_info_);
  this->images_ = this->device_.getSwapchainImagesKHR(this->swapchain_);
  this->image_views_ =
      create_image_views(this->device_, this->images_, this->required_info_);
  this->cmd_pool_ = create_command_pool(this->device_, queue_indices);
  this->cmd_buffers_ = allocate_command_buffers(
      this->device_, this->cmd_pool_, this->required_info_.image_count);

  this->image_avaliables_ =
      create_semaphores(this->device_, this->required_info_.image_count);
  this->present_finishes_ =
      create_semaphores(this->device_, this->required_info_.image_count);
  this->fences_ =
      create_fences(this->device_, this->required_info_.image_count);

  this->underlying()->app_init(queue_indices);
}

template <typename App>
auto Renderer<App>::create_shader_module(
    ::std::filesystem::path const &filename) -> ::vk::ShaderModule {
  ::std::ifstream ifs{filename, ::std::ios::binary | ::std::ios::in};
  ::std::vector<char> content{(::std::istreambuf_iterator<char>(ifs)),
                              ::std::istreambuf_iterator<char>()};
  ifs.close();

  ::vk::ShaderModuleCreateInfo info;
  info.setCodeSize(content.size())
      .setPCode(reinterpret_cast<uint32_t const *>(content.data()));
  auto module_ = this->device_.createShaderModule(info);
  assert(module_ && "shader module create failed!");
  this->shader_modules_.emplace_back(module_);
  return module_;
}

template <typename App>
auto Renderer<App>::create_vf_pipeline(::vk::ShaderModule const &vert,
                                       ::vk::ShaderModule const &frag)
    -> ::vk::Pipeline {
  // shader config
  ::std::array<::vk::PipelineShaderStageCreateInfo, 2> stage_infos{
      ::vk::PipelineShaderStageCreateInfo{
          ::vk::PipelineShaderStageCreateFlags{},
          ::vk::ShaderStageFlagBits::eVertex, vert, "main"},
      ::vk::PipelineShaderStageCreateInfo{
          ::vk::PipelineShaderStageCreateFlags{},
          ::vk::ShaderStageFlagBits::eFragment, frag, "main"},
  };

  // vertex input
  ::vk::PipelineVertexInputStateCreateInfo vertex_input;
  auto [attr_descs, bind_desc] =
      this->underlying()->get_vertex_input_description();
  vertex_input.setVertexAttributeDescriptions(attr_descs)
      .setVertexBindingDescriptions(bind_desc);

  // input assembly
  ::vk::PipelineInputAssemblyStateCreateInfo input_asm;
  input_asm.setTopology(::vk::PrimitiveTopology::eTriangleList)
      .setPrimitiveRestartEnable(false);

  // viewport and scissor
  ::vk::Viewport viewport{
      0.f,
      0.f,
      static_cast<float>(this->required_info_.extent.width),
      static_cast<float>(this->required_info_.extent.height),
      0.f,
      1.f,
  };
  ::vk::Rect2D scissor{::vk::Offset2D{0, 0}, this->required_info_.extent};
  ::vk::PipelineViewportStateCreateInfo viewport_state;
  viewport_state.setViewports(viewport).setScissors(scissor);

  // rasterization
  ::vk::PipelineRasterizationStateCreateInfo rast_info;
  rast_info.setRasterizerDiscardEnable(false)
      .setDepthClampEnable(false)
      .setDepthBiasEnable(false)
      .setLineWidth(1.f)
      .setCullMode(::vk::CullModeFlagBits::eNone)
      .setPolygonMode(::vk::PolygonMode::eFill);

  // multisample
  ::vk::PipelineMultisampleStateCreateInfo multi_info;
  multi_info.setRasterizationSamples(::vk::SampleCountFlagBits::e1)
      .setSampleShadingEnable(false);

  // depth stencil

  // color blend
  ::vk::PipelineColorBlendAttachmentState color_att;
  color_att.setColorWriteMask(
      ::vk::ColorComponentFlagBits::eR | ::vk::ColorComponentFlagBits::eG |
      ::vk::ColorComponentFlagBits::eB | ::vk::ColorComponentFlagBits::eA);
  ::vk::PipelineColorBlendStateCreateInfo color_state;
  color_state.setLogicOpEnable(false).setAttachments(color_att);

  // render pass

  // graphics pipeline
  ::vk::GraphicsPipelineCreateInfo info;
  info.setStages(stage_infos)
      .setPVertexInputState(&vertex_input)
      .setPInputAssemblyState(&input_asm)
      .setLayout(this->layout_)
      .setPViewportState(&viewport_state)
      .setPRasterizationState(&rast_info)
      .setPMultisampleState(&multi_info)
      .setPDepthStencilState(nullptr)
      .setPColorBlendState(&color_state)
      .setRenderPass(this->underlying()->render_pass_);
  auto result = this->device_.createGraphicsPipeline(nullptr, info);
  assert(result.result == ::vk::Result::eSuccess &&
         "vf graphics pipeline create failed!");
  return result.value;
}

template <typename App> auto Renderer<App>::run() -> void {
  this->window_.main_loop(this, &Renderer<App>::render);
  this->device_.waitIdle();
}

template <typename App> auto Renderer<App>::destroy() -> void {
  this->underlying()->app_destroy();

  for (decltype(required_info_.image_count) i = 0;
       i < required_info_.image_count; ++i) {
    this->device_.destroyFence(this->fences_[i]);
    this->device_.destroySemaphore(this->present_finishes_[i]);
    this->device_.destroySemaphore(this->image_avaliables_[i]);
  }

  this->device_.freeCommandBuffers(this->cmd_pool_, this->cmd_buffers_);
  this->device_.destroyCommandPool(this->cmd_pool_);
  for (auto &shader : this->shader_modules_) {
    this->device_.destroyShaderModule(shader);
  }
  for (auto &view : this->image_views_) {
    this->device_.destroyImageView(view);
  }
  this->device_.destroySwapchainKHR(this->swapchain_);
  this->device_.destroy();
  this->instance_.destroySurfaceKHR(this->surface_);
  this->instance_.destroy();

  SDL_Quit();
}

template <typename App> auto Renderer<App>::render(Renderer<App> *app) -> void {
  app->device_.resetFences(app->fences_[app->current_frame_]);

  auto option_index = app->device_.acquireNextImageKHR(
      app->swapchain_, ::std::numeric_limits<uint64_t>::max(),
      app->image_avaliables_[app->current_frame_], nullptr);
  assert(option_index.result == ::vk::Result::eSuccess &&
         "acquire image failed!");
  uint32_t image_index = option_index.value;

  app->cmd_buffers_[app->current_frame_].reset();
  app->underlying()->record_command(app->cmd_buffers_[app->current_frame_],
                                    app->framebuffers_[image_index]);

  ::vk::PipelineStageFlags flags{
      ::vk::PipelineStageFlagBits::eColorAttachmentOutput};
  ::vk::SubmitInfo submit_info;
  submit_info.setCommandBuffers(app->cmd_buffers_[app->current_frame_])
      .setWaitSemaphores(app->image_avaliables_[app->current_frame_])
      .setSignalSemaphores(app->present_finishes_[app->current_frame_])
      .setWaitDstStageMask(flags);
  app->graphics_queue_.submit(submit_info, app->fences_[app->current_frame_]);

  ::vk::PresentInfoKHR present_info;
  present_info.setImageIndices(image_index)
      .setSwapchains(app->swapchain_)
      .setWaitSemaphores(app->present_finishes_[app->current_frame_]);
  [[maybe_unused]] auto result = app->present_queue_.presentKHR(present_info);
  assert(result == ::vk::Result::eSuccess && "present failed!");
  result = app->device_.waitForFences(app->fences_[app->current_frame_], true,
                                      ::std::numeric_limits<uint64_t>::max());
  assert(result == ::vk::Result::eSuccess && "wait fences failed!");

  app->current_frame_ =
      (app->current_frame_ + 1) % app->required_info_.image_count;
  // app->underlying()->render();
}

#endif // RENDERER_HPP_

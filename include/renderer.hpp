#ifndef RENDERER_HPP_
#define RENDERER_HPP_

#include "create.hpp"
#include "window.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

template <typename App> class Renderer {
public:
  auto init(::std::filesystem::path const &shader_path) -> void;

  auto create_shader_module(::std::filesystem::path const &filename)
      -> ::vk::ShaderModule;
  auto create_vf_pipeline(::vk::ShaderModule const &vert,
                          ::vk::ShaderModule const &frag) -> ::vk::Pipeline;

  auto run() -> void;
  auto destroy() -> void;

  static auto render(Renderer<App> *app) -> void;

private:
  auto underlying() -> App * { return reinterpret_cast<App *>(this); }

public:
protected:
  Window window_;

  ::vk::Instance instance_{nullptr};
  ::vk::SurfaceKHR surface_{nullptr};
  ::vk::PhysicalDevice physical_{nullptr};
  ::vk::Device device_{nullptr};
  ::vk::Queue graphics_queue_{nullptr};
  ::vk::Queue present_queue_{nullptr};
  ::vk::SwapchainKHR swapchain_{nullptr};
  SwapchainRequiredInfo required_info_;

  ::std::vector<::vk::Image> images_;
  ::std::vector<::vk::ImageView> image_views_;
  ::std::vector<::vk::ShaderModule> shader_modules_;
};

template <typename App>
auto Renderer<App>::init(::std::filesystem::path const &shader_path) -> void {
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

  this->underlying()->app_init(queue_indices, shader_path);
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

  // layout

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
      .setLayout(this->underlying()->layout_)
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
}

template <typename App> auto Renderer<App>::render(Renderer<App> *app) -> void {
  app->underlying()->render();
}

#endif // RENDERER_HPP_

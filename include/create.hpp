#ifndef CREATE_HPP_
#define CREATE_HPP_

#include "window.hpp"

#include <optional>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_structs.hpp>

struct QueueFamilyIndices {
  ::std::optional<uint32_t> graphics_indices;
  ::std::optional<uint32_t> present_indices;
};

struct SwapchainRequiredInfo {
  ::vk::SurfaceCapabilitiesKHR capabilities;
  ::vk::SurfaceFormatKHR format;
  ::vk::PresentModeKHR present_mode;
  ::vk::Extent2D extent;
  uint32_t image_count;
};

auto create_instance(Window &window,
                     ::std::vector<char const *> const &app_extensions = {})
    -> ::vk::Instance;

auto create_surface(Window &window, ::vk::Instance &instance)
    -> ::vk::SurfaceKHR;

auto pickup_physical_device(::vk::Instance &instance, ::vk::SurfaceKHR &surface)
    -> ::vk::PhysicalDevice;

auto pickup_queue_family(::vk::PhysicalDevice &device,
                         ::vk::SurfaceKHR &surface) -> QueueFamilyIndices;

auto create_logic_device(::vk::PhysicalDevice &physical,
                         QueueFamilyIndices &queue_indices,
                         ::std::vector<char const *> const &app_extensions = {
                             VK_KHR_SWAPCHAIN_EXTENSION_NAME}) -> ::vk::Device;

auto create_swapchain(::vk::Device &device, ::vk::SurfaceKHR &surface,
                      QueueFamilyIndices &indices,
                      SwapchainRequiredInfo &required_info)
    -> ::vk::SwapchainKHR;

auto query_swapchain_required_info(SDL_Window *window,
                                   ::vk::PhysicalDevice &physical,
                                   ::vk::SurfaceKHR &surface,
                                   uint32_t image_count)
    -> SwapchainRequiredInfo;

auto create_image_views(::vk::Device &device,
                        ::std::vector<::vk::Image> &images,
                        SwapchainRequiredInfo &required_info)
    -> ::std::vector<::vk::ImageView>;

auto create_pipeline_layout(::vk::Device &device) -> ::vk::PipelineLayout;

auto create_frame_buffers(::vk::Device &device,
                          ::std::vector<::vk::ImageView> &views,
                          ::vk::RenderPass &render_pass,
                          SwapchainRequiredInfo &required_info)
    -> ::std::vector<::vk::Framebuffer>;

auto create_semaphores(::vk::Device &device, size_t size)
    -> ::std::vector<::vk::Semaphore>;

auto create_fences(::vk::Device &device, size_t size)
    -> ::std::vector<::vk::Fence>;

#endif // CREATE_HPP_

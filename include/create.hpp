#ifndef CREATE_HPP_
#define CREATE_HPP_

#include "base_type.hpp"
#include "window.hpp"

#include <filesystem>
#include <optional>

#include <vulkan/vulkan.hpp>

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

auto create_image_view(::vk::Device &device, ::vk::Image &image,
                       ::vk::Format const &format) -> ::vk::ImageView;

auto create_image_views(::vk::Device &device,
                        ::std::vector<::vk::Image> &images,
                        ::vk::Format const &format)
    -> ::std::vector<::vk::ImageView>;

auto create_frame_buffers(::vk::Device &device,
                          ::std::vector<::vk::ImageView> &views,
                          ::vk::RenderPass &render_pass,
                          SwapchainRequiredInfo &required_info)
    -> ::std::vector<::vk::Framebuffer>;

auto create_command_pool(::vk::Device &device,
                         QueueFamilyIndices &queue_indices)
    -> ::vk::CommandPool;

auto allocate_command_buffers(::vk::Device &device, ::vk::CommandPool &pool,
                              size_t size)
    -> ::std::vector<::vk::CommandBuffer>;

auto create_buffer(::vk::Device &device, QueueFamilyIndices &indices,
                   ::vk::DeviceSize size, ::vk::BufferUsageFlags flag)
    -> ::vk::Buffer;

auto create_image(::vk::Device &device, uint32_t width, uint32_t height,
                  ::vk::ImageUsageFlags flag) -> ::vk::Image;

auto create_shader_module(::vk::Device &device,
                          ::std::filesystem::path const &filename)
    -> ::vk::ShaderModule;

auto create_image_data(::std::filesystem::path const &filename) -> Image;

auto create_descriptor_pool(::vk::Device &device, size_t max_size,
                            ::vk::DescriptorType type) -> ::vk::DescriptorPool;

auto create_texture_sampler(::vk::PhysicalDevice &physical,
                            ::vk::Device &device) -> ::vk::Sampler;

auto create_semaphores(::vk::Device &device, size_t size)
    -> ::std::vector<::vk::Semaphore>;

auto create_fences(::vk::Device &device, size_t size)
    -> ::std::vector<::vk::Fence>;

template <typename T, bool IsBuffer = ::std::is_same_v<T, ::vk::Buffer>,
          bool IsImage = ::std::is_same_v<T, ::vk::Image>,
          typename = ::std::enable_if_t<IsBuffer || IsImage>>
auto allocate_memory(::vk::PhysicalDevice &physical, ::vk::Device &device,
                     T const &buffer, ::vk::MemoryPropertyFlags flag)
    -> ::vk::DeviceMemory;

template <typename T, bool IsBuffer = ::std::is_same_v<T, ::vk::Buffer>,
          bool IsImage = ::std::is_same_v<T, ::vk::Image>,
          typename = ::std::enable_if_t<IsBuffer || IsImage>>
auto allocate_memory(::vk::PhysicalDevice &physical, ::vk::Device &device,
                     ::std::vector<T> const &buffers,
                     ::vk::MemoryPropertyFlags flag) -> ::vk::DeviceMemory;

template <typename T, bool IsBuffer = ::std::is_same_v<T, ::vk::Buffer>,
          bool IsImage = ::std::is_same_v<T, ::vk::Image>,
          typename Container = ::std::conditional_t<
              IsBuffer,
              ::std::vector<::std::tuple<::vk::Buffer, ::vk::DeviceMemory, T,
                                         ::vk::DeviceSize>>,
              ::std::vector<::std::tuple<::vk::Buffer, ::vk::DeviceMemory, T,
                                         uint32_t, uint32_t>>>,
          typename = ::std::enable_if_t<IsBuffer || IsImage>>
auto allocate_memory(::vk::PhysicalDevice &physical, ::vk::Device &device,
                     ::vk::CommandPool &pool, ::vk::Queue &queue,
                     Container const &buffers, ::vk::MemoryPropertyFlags flag)
    -> ::std::pair<::std::vector<T>, ::vk::DeviceMemory>;

auto copy_data(::vk::Device &device, ::vk::DeviceMemory &memory, size_t offset,
               size_t size, void const *data) -> void;

auto copy_buffer(::vk::Device &device, ::vk::CommandPool &pool,
                 ::vk::Queue &queue, ::vk::Buffer const &src,
                 ::vk::Buffer const &dest, ::vk::DeviceSize size) -> void;

auto copy_image(::vk::Device &device, ::vk::CommandPool &pool,
                ::vk::Queue &queue, ::vk::Buffer const &src,
                ::vk::Image const &dest, uint32_t width, uint32_t height)
    -> void;

template <typename T>
auto wrap_buffer(::vk::PhysicalDevice &physical, ::vk::Device &device,
                 QueueFamilyIndices &indices, T const *data, size_t len,
                 ::vk::BufferUsageFlags flag)
    -> ::std::tuple<::vk::Buffer, ::vk::DeviceMemory, ::vk::Buffer,
                    ::vk::DeviceSize>;

auto wrap_image(::vk::PhysicalDevice &physical, ::vk::Device &device,
                QueueFamilyIndices &indices, Image const &image,
                ::vk::ImageUsageFlags flag)
    -> ::std::tuple<::vk::Buffer, ::vk::DeviceMemory, ::vk::Image, uint32_t,
                    uint32_t>;

template <typename T, size_t N>
auto wrap_buffer(::vk::PhysicalDevice &physical, ::vk::Device &device,
                 QueueFamilyIndices &indices, ::std::array<T, N> const &data,
                 ::vk::BufferUsageFlags flag)
    -> ::std::tuple<::vk::Buffer, ::vk::DeviceMemory, ::vk::Buffer,
                    ::vk::DeviceSize>;

template <typename Buffer,
          typename Iter = typename ::std::vector<Buffer>::iterator,
          bool IsBuffer = ::std::is_same_v<Buffer, ::vk::Buffer>,
          bool IsView = ::std::is_same_v<Buffer, ::vk::ImageView>,
          typename Attr = ::std::enable_if_t<
              IsBuffer || IsView,
              ::std::conditional_t<IsBuffer, size_t, ::vk::Sampler>>>
auto allocate_descriptor_set(::vk::Device &device, Iter begin, Iter end,
                             ::vk::DescriptorType type,
                             ::vk::ShaderStageFlags stage, Attr const &attr)
    -> ::std::tuple<::vk::DescriptorPool, ::vk::DescriptorSetLayout,
                    ::std::vector<::vk::DescriptorSet>>;

#include "create.tcc"

#endif // CREATE_HPP_

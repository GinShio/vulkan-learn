#include "create.hpp"
#include "scope_guard.hpp"

#include <assert.h>

#include <algorithm>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <utility>
#include <vector>

#include <SDL.h>
#include <SDL_vulkan.h>

#include <vulkan/vulkan.hpp>

#ifdef DEBUG
#include <iostream>
#endif

auto create_instance(Window &window,
                     ::std::vector<char const *> const &app_extensions)
    -> ::vk::Instance {
  ::std::vector<char const *> extensions = window.get_extensions();
  extensions.insert(extensions.end(), app_extensions.begin(),
                    app_extensions.end());
#ifdef DEBUG
  ::std::clog << "Enabled Instance Extensions:" << ::std::endl;
  for (auto const &ext : extensions) {
    ::std::clog << "\t" << ext << ::std::endl;
  }
  ::std::clog << ::std::endl;
#endif

  ::std::vector<char const *> layers = ::std::initializer_list<char const *>{
#ifdef DEBUG
      "VK_LAYER_KHRONOS_validation",
#endif
  };

  ::vk::ApplicationInfo app_info;
  app_info.setApplicationVersion(
#if defined(VK_API_VERSION_1_3)
      VK_API_VERSION_1_3
#elif defined(VK_API_VERSION_1_2)
      VK_API_VERSION_1_2
#elif defined(VK_API_VERSION_1_1)
      VK_API_VERSION_1_1
#else
      VK_API_VERSION_1_0
#endif
  );

  ::vk::InstanceCreateInfo info;
  info.setPApplicationInfo(&app_info)
      .setPEnabledExtensionNames(extensions)
      .setPEnabledLayerNames(layers);

  ::vk::Instance instance = vk::createInstance(info);
  assert(instance && "instance create failed!");
  return instance;
}

auto create_surface(Window &window, ::vk::Instance &instance)
    -> ::vk::SurfaceKHR {
  VkSurfaceKHR surface{nullptr};
  [[maybe_unused]] auto result =
      SDL_Vulkan_CreateSurface(window.get_window(), instance, &surface);
  assert(result && "surface create failed!");
  return surface;
}

auto pickup_physical_device(::vk::Instance &instance,
                            ::vk::SurfaceKHR & /* surface */)
    -> ::vk::PhysicalDevice {
  auto devices = instance.enumeratePhysicalDevices();
  ::vk::PhysicalDevice ret = devices.front();

#ifdef DEBUG
  ::std::clog << "pickup physical device: " << ret.getProperties().deviceName
              << ::std::endl;
#endif

  return ret;
}

auto pickup_queue_family(::vk::PhysicalDevice &device,
                         ::vk::SurfaceKHR &surface) -> QueueFamilyIndices {
  QueueFamilyIndices indices;
  uint32_t idx{0};
  auto families = device.getQueueFamilyProperties();
  for (auto const &family : families) {
    if (family.queueFlags | ::vk::QueueFlagBits::eGraphics) {
      indices.graphics_indices = idx;
    }
    if (device.getSurfaceSupportKHR(idx, surface)) {
      indices.present_indices = idx;
    }
    if (indices.graphics_indices.has_value() &&
        indices.present_indices.has_value()) {
      break;
    }
    ++idx;
  }
  assert(indices.graphics_indices.has_value() &&
         "gpu not support graphics queue family!");
  assert(indices.present_indices.has_value() &&
         "gpu not support present queue family!");
  return indices;
}

auto create_logic_device(::vk::PhysicalDevice &physical,
                         QueueFamilyIndices &queue_indices,
                         ::std::vector<char const *> const &app_extensions)
    -> ::vk::Device {
  ::std::vector<::vk::DeviceQueueCreateInfo> queue_infos;
  float queue_prioirty{1.0f};
  queue_infos.emplace_back(::vk::DeviceQueueCreateFlags{},
                           queue_indices.graphics_indices.value(), 1,
                           &queue_prioirty);
  [[unlikely]] if (queue_indices.graphics_indices.value() !=
                   queue_indices.present_indices.value()) {
    queue_infos.emplace_back(::vk::DeviceQueueCreateFlags{},
                             queue_indices.present_indices.value(), 1,
                             &queue_prioirty);
  }

  ::std::vector<char const *> extensions;
  extensions.insert(extensions.end(), app_extensions.begin(),
                    app_extensions.end());
#ifdef DEBUG
  ::std::clog << "Enabled Device Extensions:" << ::std::endl;
  for (auto const &ext : extensions) {
    ::std::clog << "\t" << ext << ::std::endl;
  }
  ::std::clog << ::std::endl;
#endif

  ::vk::DeviceCreateInfo info;

  info.setQueueCreateInfos(queue_infos).setPEnabledExtensionNames(extensions);

  ::vk::Device device = physical.createDevice(info);
  assert(device && "logic device create failed!");
  return device;
}

auto create_swapchain(::vk::Device &device, ::vk::SurfaceKHR &surface,
                      QueueFamilyIndices &indices,
                      SwapchainRequiredInfo &required_info)
    -> ::vk::SwapchainKHR {
  ::vk::SwapchainCreateInfoKHR info;
  info.setSurface(surface)
      .setMinImageCount(required_info.image_count)
      .setImageFormat(required_info.format.format)
      .setImageColorSpace(required_info.format.colorSpace)
      .setImageExtent(required_info.extent)
      .setImageArrayLayers(1)
      .setImageUsage(::vk::ImageUsageFlagBits::eColorAttachment)
      .setImageSharingMode(::vk::SharingMode::eExclusive)
      .setQueueFamilyIndices(indices.graphics_indices.value())
      .setPreTransform(::vk::SurfaceTransformFlagBitsKHR::eIdentity)
      .setCompositeAlpha(::vk::CompositeAlphaFlagBitsKHR::eOpaque)
      .setPresentMode(required_info.present_mode);

  [[unlikely]] if (indices.graphics_indices.value() !=
                   indices.present_indices.value()) {
    ::std::array<uint32_t, 2> indices_arr{indices.graphics_indices.value(),
                                          indices.present_indices.value()};
    info.setImageSharingMode(::vk::SharingMode::eConcurrent)
        .setQueueFamilyIndices(indices_arr);
  }

  auto swapchain = device.createSwapchainKHR(info);
  assert(swapchain && "swapchain create failed!");
  return swapchain;
}

auto query_swapchain_required_info(SDL_Window *window,
                                   ::vk::PhysicalDevice &physical,
                                   ::vk::SurfaceKHR &surface,
                                   uint32_t image_count)
    -> SwapchainRequiredInfo {
  SwapchainRequiredInfo info;
  info.capabilities = physical.getSurfaceCapabilitiesKHR(surface);

  auto formats = physical.getSurfaceFormatsKHR(surface);
  info.format = formats[0];
  for (auto const &format : formats) {
    if ((format.format == ::vk::Format::eR8G8B8A8Srgb ||
         format.format == ::vk::Format::eB8G8R8A8Srgb) &&
        format.colorSpace == ::vk::ColorSpaceKHR::eSrgbNonlinear) {
      info.format = format;
      break;
    }
  }

  auto present_modes = physical.getSurfacePresentModesKHR(surface);
  info.present_mode = ::vk::PresentModeKHR::eFifo;
  for (auto const &mode : present_modes) {
    if (mode == ::vk::PresentModeKHR::eMailbox) {
      info.present_mode = mode;
      break;
    }
  }

  int width{0};
  int height{0};
  SDL_GetWindowSize(window, &width, &height);
  info.extent =
      ::vk::Extent2D{::std::clamp(static_cast<uint32_t>(width),
                                  info.capabilities.minImageExtent.width,
                                  info.capabilities.maxImageExtent.width),
                     ::std::clamp(static_cast<uint32_t>(height),
                                  info.capabilities.minImageExtent.height,
                                  info.capabilities.maxImageExtent.height)};
  info.image_count = ::std::clamp(image_count, info.capabilities.minImageCount,
                                  info.capabilities.maxImageCount == 0
                                      ? ::std::numeric_limits<uint32_t>::max()
                                      : info.capabilities.maxImageCount);

  return info;
}

auto create_image_view(::vk::Device &device, ::vk::Image &image,
                       ::vk::Format const &format) -> ::vk::ImageView {
  ::vk::ImageViewCreateInfo info;
  info.setViewType(::vk::ImageViewType::e2D)
      .setFormat(format)
      .setComponents(::vk::ComponentMapping{
          ::vk::ComponentSwizzle::eIdentity, ::vk::ComponentSwizzle::eIdentity,
          ::vk::ComponentSwizzle::eIdentity, ::vk::ComponentSwizzle::eIdentity})
      .setImage(image)
      .setSubresourceRange(::vk::ImageSubresourceRange{
          ::vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});

  ::vk::ImageView view = device.createImageView(info);
  assert(view && "image view create failed!");
  return view;
}

auto create_image_views(::vk::Device &device,
                        ::std::vector<::vk::Image> &images,
                        ::vk::Format const &format)
    -> ::std::vector<::vk::ImageView> {
  ::std::vector<::vk::ImageView> views;
  views.reserve(images.size());
  for (auto &image : images) {
    views.emplace_back(create_image_view(device, image, format));
  }
  return views;
}

auto create_frame_buffers(::vk::Device &device,
                          ::std::vector<::vk::ImageView> &views,
                          ::vk::RenderPass &render_pass,
                          SwapchainRequiredInfo &required_info)
    -> ::std::vector<::vk::Framebuffer> {
  ::std::vector<::vk::Framebuffer> buffers;
  ::vk::FramebufferCreateInfo info;
  info.setRenderPass(render_pass)
      .setLayers(1)
      .setWidth(required_info.extent.width)
      .setHeight(required_info.extent.height);
  for (auto &view : views) {
    info.setAttachments(view);
    buffers.emplace_back(device.createFramebuffer(info));
    assert(buffers.back() && "frame buffers create failed!");
  }
  return buffers;
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
                              size_t size)
    -> ::std::vector<::vk::CommandBuffer> {
  ::vk::CommandBufferAllocateInfo alloc_info;
  alloc_info.setLevel(::vk::CommandBufferLevel::ePrimary)
      .setCommandPool(pool)
      .setCommandBufferCount(size);
  return device.allocateCommandBuffers(alloc_info);
}

auto create_buffer(::vk::Device &device, QueueFamilyIndices &indices,
                   ::vk::DeviceSize size, ::vk::BufferUsageFlags flag)
    -> ::vk::Buffer {
  ::vk::BufferCreateInfo info;
  info.setSharingMode(::vk::SharingMode::eExclusive)
      .setQueueFamilyIndices(indices.graphics_indices.value())
      .setSize(size)
      .setUsage(flag);

  ::vk::Buffer buffer = device.createBuffer(info);
  assert(buffer && "vertex buffer create failed!");
  return buffer;
}

auto create_image(::vk::Device &device, uint32_t width, uint32_t height,
                  ::vk::ImageUsageFlags flag) -> ::vk::Image {
  ::vk::ImageCreateInfo info;
  info.setImageType(::vk::ImageType::e2D)
      .setExtent(::vk::Extent3D{width, height, 1})
      .setMipLevels(1)
      .setArrayLayers(1)
      .setFormat(::vk::Format::eR8G8B8A8Srgb)
      .setTiling(::vk::ImageTiling::eOptimal)
      .setInitialLayout(::vk::ImageLayout::eUndefined)
      .setUsage(flag)
      .setSharingMode(::vk::SharingMode::eExclusive)
      .setSamples(::vk::SampleCountFlagBits::e1);

  auto image = device.createImage(info);
  assert(image && "image create failed!");
  return image;
}

auto create_shader_module(::vk::Device &device,
                          ::std::filesystem::path const &filename)
    -> ::vk::ShaderModule {
  ::std::ifstream ifs{filename, ::std::ios::binary | ::std::ios::in};
  ::std::vector<char> content{(::std::istreambuf_iterator<char>(ifs)),
                              ::std::istreambuf_iterator<char>()};
  ifs.close();

  ::vk::ShaderModuleCreateInfo info;
  info.setCodeSize(content.size())
      .setPCode(reinterpret_cast<uint32_t const *>(content.data()));
  auto shader_module = device.createShaderModule(info);
  assert(shader_module && "shader module create failed!");
  return shader_module;
}

auto create_image_data(::std::filesystem::path const &filename) -> Image {
  ::std::ifstream ifs{filename, ::std::ios::binary | ::std::ios::in};
  ::std::vector<unsigned char> content{(::std::istreambuf_iterator<char>(ifs)),
                                       ::std::istreambuf_iterator<char>()};
  ifs.close();
  return Image{content};
}

auto create_descriptor_pool(::vk::Device &device, size_t max_size,
                            ::vk::DescriptorType type) -> ::vk::DescriptorPool {
  ::vk::DescriptorPoolSize size{type, static_cast<uint32_t>(max_size)};
  ::vk::DescriptorPoolCreateInfo info;
  info.setPoolSizes(size)
      .setMaxSets(::std::numeric_limits<uint16_t>::max())
      .setFlags(::vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
  auto pool = device.createDescriptorPool(info);
  assert(pool && "descriptor pool create failed!");
  return pool;
}

auto create_texture_sampler(::vk::PhysicalDevice &physical,
                            ::vk::Device &device) -> ::vk::Sampler {
  auto properties = physical.getProperties();
  ::vk::SamplerCreateInfo info;
  info.setMagFilter(::vk::Filter::eLinear)
      .setMinFilter(::vk::Filter::eLinear)
      .setAddressModeU(::vk::SamplerAddressMode::eRepeat)
      .setAddressModeV(::vk::SamplerAddressMode::eRepeat)
      .setAddressModeW(::vk::SamplerAddressMode::eRepeat)
      .setAnisotropyEnable(VK_TRUE)
      .setMaxAnisotropy(properties.limits.maxSamplerAnisotropy)
      .setBorderColor(::vk::BorderColor::eIntOpaqueBlack)
      .setUnnormalizedCoordinates(VK_FALSE)
      .setCompareEnable(VK_FALSE)
      .setCompareOp(::vk::CompareOp::eAlways)
      .setMipmapMode(::vk::SamplerMipmapMode::eLinear)
      .setMipLodBias(.0f)
      .setMinLod(.0f)
      .setMaxLod(.0f);

  ::vk::Sampler sampler = device.createSampler(info);
  assert(sampler && "sampler create failed!");
  return sampler;
}

auto create_semaphores(::vk::Device &device, size_t size)
    -> ::std::vector<::vk::Semaphore> {
  ::vk::SemaphoreCreateInfo info;
  ::std::vector<::vk::Semaphore> ret{size};
  for (decltype(size) i = 0; i < size; ++i) {
    ret[i] = device.createSemaphore(info);
    assert(ret[i] && "semophare create failed!");
  }
  return ret;
}

auto create_fences(::vk::Device &device, size_t size)
    -> ::std::vector<::vk::Fence> {
  ::vk::FenceCreateInfo info;
  ::std::vector<::vk::Fence> ret{size};
  for (decltype(size) i = 0; i < size; ++i) {
    ret[i] = device.createFence(info);
    assert(ret[i] && "fence create failed!");
  }
  return ret;
}

auto copy_data(::vk::Device &device, ::vk::DeviceMemory &memory, size_t offset,
               size_t size, void const *data) -> void {
  void *buffer = device.mapMemory(memory, offset, size);
  MAKE_SCOPE_GUARD {
    device.unmapMemory(memory);
    device.waitIdle();
  };
  ::memcpy(buffer, data, size);
}

auto copy_buffer(::vk::Device &device, ::vk::CommandPool &pool,
                 ::vk::Queue &queue, ::vk::Buffer const &src,
                 ::vk::Buffer const &dest, ::vk::DeviceSize size) -> void {
  auto transfer_cmd_buffers = allocate_command_buffers(device, pool, 1);
  ::vk::CommandBufferBeginInfo begin_info;
  begin_info.setFlags(::vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
  MAKE_SCOPE_GUARD { device.freeCommandBuffers(pool, transfer_cmd_buffers); };
  transfer_cmd_buffers.front().begin(begin_info);
  transfer_cmd_buffers.front().copyBuffer(src, dest,
                                          ::vk::BufferCopy{0, 0, size});
  transfer_cmd_buffers.front().end();
  ::vk::SubmitInfo submit_info;
  submit_info.setCommandBuffers(transfer_cmd_buffers);
  queue.submit(submit_info);
  device.waitIdle();
}

auto copy_image(::vk::Device &device, ::vk::CommandPool &pool,
                ::vk::Queue &queue, ::vk::Buffer const &src,
                ::vk::Image const &dest, uint32_t width, uint32_t height)
    -> void {
  ::vk::ImageSubresourceRange range;
  range.setAspectMask(::vk::ImageAspectFlagBits::eColor)
      .setBaseMipLevel(0)
      .setLevelCount(1)
      .setBaseArrayLayer(0)
      .setLayerCount(1);
  ::vk::ImageMemoryBarrier barrier;
  barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
      .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
      .setImage(dest)
      .setSubresourceRange(range);

  auto transfer_cmd_buffers = allocate_command_buffers(device, pool, 1);
  ::vk::CommandBufferBeginInfo begin_info;
  begin_info.setFlags(::vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
  MAKE_SCOPE_GUARD { device.freeCommandBuffers(pool, transfer_cmd_buffers); };
  transfer_cmd_buffers.front().begin(begin_info);

  barrier.setOldLayout(::vk::ImageLayout::eUndefined)
      .setNewLayout(::vk::ImageLayout::eTransferDstOptimal)
      .setSrcAccessMask(::vk::AccessFlagBits::eNone)
      .setDstAccessMask(::vk::AccessFlagBits::eTransferWrite);
  transfer_cmd_buffers.front().pipelineBarrier(
      ::vk::PipelineStageFlagBits::eTopOfPipe,
      ::vk::PipelineStageFlagBits::eTransfer,
      ::vk::DependencyFlagBits::eByRegion, 0, nullptr, 0, nullptr, 1, &barrier);
  ::vk::ImageSubresourceLayers layer;
  layer.setAspectMask(::vk::ImageAspectFlagBits::eColor)
      .setMipLevel(0)
      .setBaseArrayLayer(0)
      .setLayerCount(1);
  ::vk::BufferImageCopy region;
  region.setBufferOffset(0)
      .setBufferRowLength(0)
      .setBufferImageHeight(0)
      .setImageSubresource(layer)
      .setImageOffset(::vk::Offset3D{0, 0, 0})
      .setImageExtent(::vk::Extent3D{width, height, 1});
  transfer_cmd_buffers.front().copyBufferToImage(
      src, dest, ::vk::ImageLayout::eTransferDstOptimal, region);
  barrier.setOldLayout(::vk::ImageLayout::eTransferDstOptimal)
      .setNewLayout(::vk::ImageLayout::eReadOnlyOptimal)
      .setSrcAccessMask(::vk::AccessFlagBits::eTransferWrite)
      .setDstAccessMask(::vk::AccessFlagBits::eShaderRead);
  transfer_cmd_buffers.front().pipelineBarrier(
      ::vk::PipelineStageFlagBits::eTransfer,
      ::vk::PipelineStageFlagBits::eFragmentShader,
      ::vk::DependencyFlagBits::eByRegion, 0, nullptr, 0, nullptr, 1, &barrier);

  transfer_cmd_buffers.front().end();
  ::vk::SubmitInfo submit_info;
  submit_info.setCommandBuffers(transfer_cmd_buffers);
  queue.submit(submit_info);
  device.waitIdle();
}

auto wrap_image(::vk::PhysicalDevice &physical, ::vk::Device &device,
                QueueFamilyIndices &indices, Image const &image,
                ::vk::ImageUsageFlags flag)
    -> ::std::tuple<::vk::Buffer, ::vk::DeviceMemory, ::vk::Image, uint32_t,
                    uint32_t> {
  ::vk::DeviceSize size = image.get_size();
  ::vk::Buffer host_buffer = create_buffer(
      device, indices, size, ::vk::BufferUsageFlagBits::eTransferSrc);
  ::vk::DeviceMemory host_memory =
      allocate_memory(physical, device, host_buffer,
                      ::vk::MemoryPropertyFlagBits::eHostVisible |
                          ::vk::MemoryPropertyFlagBits::eHostCoherent);
  copy_data(device, host_memory, 0, size, image.get_data());
  ::vk::Image device_buffer =
      create_image(device, image.get_width(), image.get_height(),
                   ::vk::ImageUsageFlagBits::eTransferDst | flag);
  return ::std::make_tuple(host_buffer, host_memory, device_buffer,
                           image.get_width(), image.get_height());
}

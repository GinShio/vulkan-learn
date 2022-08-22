#include "create.hpp"

#include <algorithm>
#include <assert.h>

#include <initializer_list>

#include <SDL.h>
#include <SDL_vulkan.h>

#include <limits>
#include <utility>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

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
  VkSurfaceKHR surface;
  assert(SDL_Vulkan_CreateSurface(window.get_window(), instance, &surface) &&
         "surface create failed!");
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

  int width{0}, height{0};
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

auto create_image_views(::vk::Device &device,
                        ::std::vector<::vk::Image> &images,
                        SwapchainRequiredInfo &required_info)
    -> ::std::vector<::vk::ImageView> {
  auto sz = images.size();
  ::std::vector<::vk::ImageView> views{sz, nullptr};
  ::vk::ImageViewCreateInfo info;
  info.setViewType(::vk::ImageViewType::e2D)
      .setFormat(required_info.format.format)
      .setComponents(::vk::ComponentMapping{
          ::vk::ComponentSwizzle::eIdentity, ::vk::ComponentSwizzle::eIdentity,
          ::vk::ComponentSwizzle::eIdentity, ::vk::ComponentSwizzle::eIdentity})
      .setSubresourceRange(::vk::ImageSubresourceRange{
          ::vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});

  for (decltype(sz) i = 0; i < sz; ++i) {
    info.setImage(images[i]);
    views[i] = device.createImageView(info);
    assert(views[i] && "image view create failed!");
  }
  return views;
}

auto create_pipeline_layout(::vk::Device &device) -> ::vk::PipelineLayout {
  ::vk::PipelineLayoutCreateInfo info;
  ::vk::PipelineLayout layout = device.createPipelineLayout(info);
  assert(layout && "pipeline layout create failed!");
  return layout;
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
  for (decltype(::std::declval<decltype(views)>().size()) i = 0,
                                                          e = views.size();
       i < e; ++i) {
    info.setAttachments(views[i]);
    buffers.emplace_back(device.createFramebuffer(info));
    assert(buffers.back() && "frame buffers create failed!");
  }
  return buffers;
}

auto create_buffer(::vk::Device &device, QueueFamilyIndices &indices,
                   size_t size, ::vk::BufferUsageFlags flag) -> ::vk::Buffer {
  ::vk::BufferCreateInfo info;
  info.setSharingMode(::vk::SharingMode::eExclusive)
      .setQueueFamilyIndices(indices.graphics_indices.value())
      .setSize(size)
      .setUsage(flag);

  ::vk::Buffer buffer = device.createBuffer(info);
  assert(buffer && "vertex buffer create failed!");
  return buffer;
}

auto allocate_memory(::vk::PhysicalDevice &physical, ::vk::Device &device,
                     ::std::vector<::vk::Buffer> const &buffers,
                     ::vk::MemoryPropertyFlags flag) -> ::vk::DeviceMemory {
  auto property = physical.getMemoryProperties();
  decltype(property.memoryHeapCount) index{property.memoryTypeCount};
  ::vk::DeviceSize size{0};
  for (auto &buffer : buffers) {
    auto requirement = device.getBufferMemoryRequirements(buffer);
    size +=
        (requirement.size / requirement.alignment +
         static_cast<size_t>(requirement.size % requirement.alignment != 0)) *
        requirement.alignment;

    if (index != property.memoryTypeCount) {
      continue;
    }
    for (decltype(property.memoryHeapCount) i = 0; i < property.memoryTypeCount;
         ++i) {
      if (requirement.memoryTypeBits & (1 << i) &&
          property.memoryTypes[i].propertyFlags & flag) {
        index = i;
        break;
      }
    }
  }

  ::vk::MemoryAllocateInfo info;
  info.setAllocationSize(size).setMemoryTypeIndex(index);

  ::vk::DeviceMemory memory = device.allocateMemory(info);
  assert(memory && "device memory allocate failed!");

  ::vk::DeviceSize offset{0};
  for (auto &buffer : buffers) {
    auto requirement = device.getBufferMemoryRequirements(buffer);
    device.bindBufferMemory(buffer, memory, offset);
    offset +=
        (requirement.size / requirement.alignment +
         static_cast<size_t>(requirement.size % requirement.alignment != 0)) *
        requirement.alignment;
  }
  return memory;
}

auto create_semaphores(::vk::Device &device, size_t sz)
    -> ::std::vector<::vk::Semaphore> {
  ::vk::SemaphoreCreateInfo info;
  ::std::vector<::vk::Semaphore> ret{sz};
  for (decltype(sz) i = 0; i < sz; ++i) {
    ret[i] = device.createSemaphore(info);
    assert(ret[i] && "semophare create failed!");
  }
  return ret;
}

auto create_fences(::vk::Device &device, size_t sz)
    -> ::std::vector<::vk::Fence> {
  ::vk::FenceCreateInfo info;
  ::std::vector<::vk::Fence> ret{sz};
  for (decltype(sz) i = 0; i < sz; ++i) {
    ret[i] = device.createFence(info);
    assert(ret[i] && "fence create failed!");
  }
  return ret;
}

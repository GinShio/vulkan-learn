#ifndef CREATE_TCC
#define CREATE_TCC

#include "create.hpp"
#include "scope_guard.hpp"

#include <iostream>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

namespace details {
template <typename Int> auto gcd(Int num1, Int num2) -> Int {
  return !num2 ? num1 : gcd(num2, num1 % num2);
}
} // namespace details

template <typename T, bool IsBuffer, bool IsImage, typename>
auto allocate_memory(::vk::PhysicalDevice &physical, ::vk::Device &device,
                     T const &buffer, ::vk::MemoryPropertyFlags flag)
    -> ::vk::DeviceMemory {
  ::vk::MemoryRequirements requirement;
  if constexpr (IsBuffer) {
    requirement = device.getBufferMemoryRequirements(buffer);
  } else {
    requirement = device.getImageMemoryRequirements(buffer);
  }
  auto property = physical.getMemoryProperties();
  decltype(property.memoryTypeCount) index{property.memoryTypeCount};
  for (decltype(property.memoryTypeCount) i = 0; i < property.memoryTypeCount;
       ++i) {
    if (((requirement.memoryTypeBits & (1 << i)) != 0u) &&
        (property.memoryTypes[i].propertyFlags & flag)) {
      index = i;
      break;
    }
  }
  ::vk::MemoryAllocateInfo info;
  info.setAllocationSize(requirement.size).setMemoryTypeIndex(index);
  ::vk::DeviceMemory memory = device.allocateMemory(info);
  assert(memory && "device memory allocate failed!");
  if constexpr (IsBuffer) {
    device.bindBufferMemory(buffer, memory, 0);
  } else {
    device.bindImageMemory(buffer, memory, 0);
  }
  return memory;
}

template <typename T, bool IsBuffer, bool IsImage, typename>
auto allocate_memory(::vk::PhysicalDevice &physical, ::vk::Device &device,
                     ::std::vector<T> const &buffers,
                     ::vk::MemoryPropertyFlags flag) -> ::vk::DeviceMemory {
  // get memory properties: allocation size and type index
  auto property = physical.getMemoryProperties();
  decltype(property.memoryHeapCount) index{property.memoryTypeCount};
  ::std::vector<::vk::MemoryRequirements> requirements;
  requirements.reserve(buffers.size());

  uint32_t memory_type = ::std::numeric_limits<uint32_t>::max();
  ::vk::DeviceSize size{0};
  for (auto const &buffer : buffers) {
    if constexpr (IsBuffer) {
      requirements.emplace_back(device.getBufferMemoryRequirements(buffer));
    } else {
      requirements.emplace_back(device.getImageMemoryRequirements(buffer));
    }
    auto &alignment = requirements.back().alignment;
    size += (requirements.back().size + alignment - 1) / alignment * alignment;
    if (::details::gcd(size, alignment) != alignment) {
      size = ((size / alignment) + 1) * alignment;
    }
    memory_type &= requirements.back().memoryTypeBits;
  }
  for (decltype(property.memoryTypeCount) i = 0; i < property.memoryTypeCount;
       ++i) {
    if (((memory_type & (1 << i)) != 0u) &&
        (property.memoryTypes[i].propertyFlags & flag)) {
      index = i;
      break;
    }
  }

  // allocate memory
  ::vk::MemoryAllocateInfo info;
  info.setAllocationSize(size).setMemoryTypeIndex(index);
  ::vk::DeviceMemory memory = device.allocateMemory(info);
  assert(memory && "device memory allocate failed!");

  ::vk::DeviceSize offset{0};
  for (auto end = buffers.size(), i = static_cast<decltype(end)>(0); i < end;
       ++i) {
    if constexpr (IsBuffer) {
      device.bindBufferMemory(buffers[i], memory, offset);
    } else {
      device.bindImageMemory(buffers[i], memory, offset);
    }
    if (i + 1 == end) {
      continue;
    }
    auto next = requirements[i + 1].alignment;
    offset += (requirements[i].size + next - 1) / next * next;
    if (::details::gcd(offset, next) != next) {
      offset = ((offset / next) + 1) * next;
    }
  }
  return memory;
}

template <typename T, bool IsBuffer, bool IsImage, typename Container, typename>
auto allocate_memory(::vk::PhysicalDevice &physical, ::vk::Device &device,
                     ::vk::CommandPool &pool, ::vk::Queue &queue,
                     Container const &buffers, ::vk::MemoryPropertyFlags flag)
    -> ::std::pair<::std::vector<T>, ::vk::DeviceMemory> {
  ::std::vector<T> device_buffers;
  ::std::transform(buffers.begin(), buffers.end(),
                   ::std::back_inserter(device_buffers),
                   [](auto &val) { return ::std::get<2>(val); });
  ::vk::DeviceMemory memory =
      allocate_memory(physical, device, device_buffers, flag);

  // NOTE: in windows, buffer requirement size not equal needed size, copy
  // buffer will warning
  for (auto const &buffer : buffers) {
    MAKE_SCOPE_GUARD {
      device.freeMemory(::std::get<1>(buffer));
      device.destroyBuffer(::std::get<0>(buffer));
    };
    if constexpr (IsBuffer) {
      copy_buffer(device, pool, queue, ::std::get<0>(buffer),
                  ::std::get<2>(buffer), ::std::get<3>(buffer));
    } else {
      copy_image(device, pool, queue, ::std::get<0>(buffer),
                 ::std::get<2>(buffer), ::std::get<3>(buffer),
                 ::std::get<4>(buffer));
    }
  }
  return ::std::make_pair(::std::move(device_buffers), memory);
}

template <typename T>
auto wrap_buffer(::vk::PhysicalDevice &physical, ::vk::Device &device,
                 QueueFamilyIndices &indices, T const *data, size_t len,
                 ::vk::BufferUsageFlags flag)
    -> ::std::tuple<::vk::Buffer, ::vk::DeviceMemory, ::vk::Buffer,
                    ::vk::DeviceSize> {
  ::vk::DeviceSize size = sizeof(T) * len;
  ::vk::Buffer host_buffer = create_buffer(
      device, indices, size, ::vk::BufferUsageFlagBits::eTransferSrc);
  ::vk::DeviceMemory host_memory =
      allocate_memory(physical, device, host_buffer,
                      ::vk::MemoryPropertyFlagBits::eHostVisible |
                          ::vk::MemoryPropertyFlagBits::eHostCoherent);
  copy_data(device, host_memory, 0, size, data);
  ::vk::Buffer device_buffer = create_buffer(
      device, indices, size, ::vk::BufferUsageFlagBits::eTransferDst | flag);
  return ::std::make_tuple(host_buffer, host_memory, device_buffer, size);
}

template <typename T>
auto wrap_image(::vk::PhysicalDevice &physical, ::vk::Device &device,
                QueueFamilyIndices &indices, T const *data, uint32_t width,
                uint32_t height, ::vk::ImageUsageFlags flag)
    -> ::std::tuple<::vk::Buffer, ::vk::DeviceMemory, ::vk::Image, uint32_t,
                    uint32_t> {
  ::vk::DeviceSize size = width * height * 4;
  ::vk::Buffer host_buffer = create_buffer(
      device, indices, size, ::vk::BufferUsageFlagBits::eTransferSrc);
  ::vk::DeviceMemory host_memory =
      allocate_memory(physical, device, host_buffer,
                      ::vk::MemoryPropertyFlagBits::eHostVisible |
                          ::vk::MemoryPropertyFlagBits::eHostCoherent);
  copy_data(device, host_memory, 0, size, data);
  ::vk::Image device_buffer = create_image(
      device, width, height, ::vk::ImageUsageFlagBits::eTransferDst | flag);
  return ::std::make_tuple(host_buffer, host_memory, device_buffer, width,
                           height);
}

template <typename T, size_t N>
auto wrap_buffer(::vk::PhysicalDevice &physical, ::vk::Device &device,
                 QueueFamilyIndices &indices, ::std::array<T, N> const &data,
                 ::vk::BufferUsageFlags flag)
    -> ::std::tuple<::vk::Buffer, ::vk::DeviceMemory, ::vk::Buffer,
                    ::vk::DeviceSize> {
  return wrap_buffer(physical, device, indices, data.data(), N, flag);
}

#endif // CREATE_TCC

/*
 * Provided under the BSD 3-Clause License, see LICENSE.
 *
 * Copyright (c) 2019, Gareth Francis
 * All rights reserved.
 */

#ifndef DEVICEINSTANCE_H
#define DEVICEINSTANCE_H

#include <vulkan/vulkan.hpp>

#include <vector>

/**
 * Base instance/device information for the application
 * - The vulkan instance
 * - Selection of a physical device
 * - creation of logical device
 */
class DeviceInstance
{
public:
  struct QueueRef {
    vk::Queue queue;
    uint32_t famIndex;
    vk::QueueFlags flags;
  };

  DeviceInstance() = delete;
  DeviceInstance(const DeviceInstance&) = delete;
  DeviceInstance(DeviceInstance&&) = delete;
  ~DeviceInstance();

  /**
   * The everything constructor
   *
   * TODO: To use this you need to know what you want in the first place including what physical devices are on the system
   * There should be callbacks and such to let the application interact with the startup process and make decisions based
   * on information that's unknown when this class starts construction
   */
  DeviceInstance(
      const std::vector<const char*>& requiredInstanceExtensions,
      const std::vector<const char*>& requiredDeviceExtensions,
      const std::string& appName,
      uint32_t appVer,
      uint32_t vulkanApiVer,
      std::vector<vk::QueueFlags> qFlags,
      const std::vector<const char*>& enabledLayers = {});


  vk::Instance& instance() { return mInstance.get(); }
  vk::Device& device() { return mDevice.get(); }
  std::vector<vk::PhysicalDevice>& physicalDevices() { return mPhysicalDevices; }
  vk::PhysicalDevice& physicalDevice() { return mPhysicalDevices.front(); }

  /**
   * Get the nth queue for the request flags
   *
   * flags should be one of the flag sets passed to the constructor
   */
  DeviceInstance::QueueRef* getQueue( vk::QueueFlags flags );

  /// Wait until all physical devices are idle
  void waitAllDevicesIdle();


  // Buffer/etc creation functions
  vk::UniqueCommandPool createCommandPool( vk::CommandPoolCreateFlags flags, DeviceInstance::QueueRef& queue );
  vk::UniqueBuffer createBuffer( vk::DeviceSize size, vk::BufferUsageFlags usageFlags );
  /// Select a device memory heap based on flags (vk::MemoryRequirements::memoryTypeBits)
  uint32_t selectDeviceMemoryHeap( vk::MemoryRequirements memoryRequirements, vk::MemoryPropertyFlags requiredFlags );
  /// Allocate device memory suitable for the specified buffer
  vk::UniqueDeviceMemory allocateDeviceMemoryForBuffer( vk::Buffer& buffer, vk::MemoryPropertyFlags userReqs );
  /// Bind memory to a buffer
  void bindMemoryToBuffer(vk::Buffer& buffer, vk::DeviceMemory& memory, vk::DeviceSize offset);
  /// Map a region of device memory to host memory
  void* mapMemory( vk::DeviceMemory& deviceMem, vk::DeviceSize offset, vk::DeviceSize size );
  /// Unmap a region of device memory
  void unmapMemory( vk::DeviceMemory& deviceMem );
  /// Flush memory/caches
  void flushMemoryRanges( vk::ArrayProxy<const vk::MappedMemoryRange> mem );


private:
  void createVulkanInstance(const std::vector<const char*>& requiredExtensions, std::string appName, uint32_t appVer, uint32_t apiVer, const std::vector<const char*>& enabledLayers);
  void createLogicalDevice(std::vector<vk::QueueFlags> qFlags);


  std::vector<vk::PhysicalDevice> mPhysicalDevices;

  vk::UniqueInstance mInstance;
  vk::UniqueDevice mDevice;

  std::vector<QueueRef> mQueues;
};

#endif // DEVICEINSTANCE_H

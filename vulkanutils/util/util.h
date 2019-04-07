/*
 * Provided under the BSD 3-Clause License, see LICENSE.
 *
 * Copyright (c) 2019, Gareth Francis
 * All rights reserved.
 */

#ifndef UTIL_H
#define UTIL_H

#include <vulkan/vulkan.hpp>

class DeviceInstance;

class Util
{
public:
  static void reset();

  static std::string physicalDeviceTypeToString( vk::PhysicalDeviceType type  );
  static std::string vulkanAPIVersionToString( uint32_t version );
  static void printPhysicalDeviceProperties( DeviceInstance& device );
  static void printDetailedPhysicalDeviceInfo( DeviceInstance& device );
  static void printQueueFamilyProperties( std::vector<vk::QueueFamilyProperties>& props );
  static void ensureExtension(const std::vector<vk::ExtensionProperties>& extensions, std::string extensionName);
  static uint32_t findQueue(DeviceInstance& device, vk::QueueFlags requiredFlags);


  static vk::CommandPool& createCommandPool( vk::CommandPoolCreateFlags flags );
  static vk::UniqueBuffer createBuffer( vk::DeviceSize size, vk::BufferUsageFlags usageFlags );
  /// Select a device memory heap based on flags (vk::MemoryRequirements::memoryTypeBits)
  static uint32_t selectDeviceMemoryHeap( vk::MemoryRequirements memoryRequirements, vk::MemoryPropertyFlags requiredFlags );
  /// Allocate device memory suitable for the specified buffer
  static vk::UniqueDeviceMemory allocateDeviceMemoryForBuffer( vk::Buffer& buffer, vk::MemoryPropertyFlags userReqs = {} );
  /// Bind memory to a buffer
  static void bindMemoryToBuffer(vk::Buffer& buffer, vk::DeviceMemory& memory, vk::DeviceSize offset = 0);
  /// Map a region of device memory to host memory
  static void* mapMemory( vk::DeviceMemory& deviceMem, vk::DeviceSize offset, vk::DeviceSize size );
  /// Unmap a region of device memory
  static void unmapMemory( vk::DeviceMemory& deviceMem );
  /// Flush memory/caches
  static void flushMemoryRanges( vk::ArrayProxy<const vk::MappedMemoryRange> mem );

  static std::vector<char> readFile(const std::string& fileName);

#ifdef DEBUG
  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
      VkDebugUtilsMessageTypeFlagsEXT messageType,
      const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
      void* pUserData);

  static void initDidl(vk::Instance& instance);
  static void initDebugMessenger( vk::Instance& instance );

  static vk::DispatchLoaderDynamic mDidl;
  static vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> mDebugUtilsMessenger;
  static bool mDebugCallbackValid;
#endif
};

#endif // UTIL_H

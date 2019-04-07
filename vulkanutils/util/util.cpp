/*
 * Provided under the BSD 3-Clause License, see LICENSE.
 *
 * Copyright (c) 2019, Gareth Francis
 * All rights reserved.
 */

#include "util.h"
#include "deviceinstance.h"

#include <iostream>
#include <fstream>

#ifdef DEBUG
  vk::DispatchLoaderDynamic Util::mDidl;
  vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> Util::mDebugUtilsMessenger;
  bool Util::mDebugCallbackValid;
#endif
void Util::reset() {
#ifdef DEBUG
  mDebugUtilsMessenger.reset();
  mDebugCallbackValid = false;
#endif
}

std::string Util::physicalDeviceTypeToString( vk::PhysicalDeviceType type ) {
  switch(type)
  {
  case vk::PhysicalDeviceType::eCpu: return "CPU";
  case vk::PhysicalDeviceType::eDiscreteGpu: return "Discrete GPU";
  case vk::PhysicalDeviceType::eIntegratedGpu: return "Integrated GPU";
  case vk::PhysicalDeviceType::eOther: return "Other";
  case vk::PhysicalDeviceType::eVirtualGpu: return "Virtual GPU";
  }
  return "Unknown";
};

std::string Util::vulkanAPIVersionToString( uint32_t version ) {
  auto major = VK_VERSION_MAJOR(version);
  auto minor = VK_VERSION_MINOR(version);
  auto patch = VK_VERSION_PATCH(version);
  return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
}

void Util::printPhysicalDeviceProperties( DeviceInstance& device ) {
  vk::PhysicalDeviceProperties props;
  device.physicalDevice().getProperties(&props);

  std::cout << "==== Physical Device Properties ====" << "\n"
            << "API Version    :" << vulkanAPIVersionToString(props.apiVersion) << "\n"
            << "Driver Version :" << props.driverVersion << "\n"
            << "Vendor ID      :" << props.vendorID << "\n"
            << "Device Type    :" << physicalDeviceTypeToString(props.deviceType) << "\n"
            << "Device Name    :" << props.deviceName << "\n" // Just incase it's not nul-terminated
               // pipeline cache uuid
               // physical device limits
               // physical device sparse properties
            << std::endl;
}

void Util::printDetailedPhysicalDeviceInfo( DeviceInstance& device ) {
  vk::PhysicalDeviceMemoryProperties props;
  device.physicalDevice().getMemoryProperties(&props);

  // Information on device memory
  std::cout << "== Device Memory ==" << "\n"
            << "Types : " << props.memoryTypeCount << "\n"
            << "Heaps : " << props.memoryHeapCount << "\n"
            << std::endl;

  for(auto i = 0u; i < props.memoryTypeCount; ++i )
  {
    auto memoryType = props.memoryTypes[i];
    if( (memoryType.propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible) ) {
      std::cout << "Type: " << i << ", Host visible: TRUE" << std::endl;
    } else {
      std::cout << "Type: " << i << ", Host visible: FALSE" << std::endl;
    }
  }
  std::cout << std::endl;
}

void Util::printQueueFamilyProperties( std::vector<vk::QueueFamilyProperties>& props ) {
  std::cout << "== Queue Family Properties ==" << std::endl;
  auto i = 0u;
  for( auto& queueFamily : props ) {
    std::cout << "Queue Family  : " << i++ << "\n"
              << "Queue Count   : " << queueFamily.queueCount << "\n"
              << "Graphics      : " << static_cast<bool>(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) << "\n"
              << "Compute       : " << static_cast<bool>(queueFamily.queueFlags & vk::QueueFlagBits::eCompute) << "\n"
              << "Transfer      : " << static_cast<bool>(queueFamily.queueFlags & vk::QueueFlagBits::eTransfer) << "\n"
              << "Sparse Binding: " << static_cast<bool>(queueFamily.queueFlags & vk::QueueFlagBits::eSparseBinding) << "\n"
              << std::endl;

  }
}

#ifdef DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL Util::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

  // TODO: Proper debug callback
  /*
  if( messageSeverity & (int)vk::DebugUtilsMessageSeverityFlagBitsEXT::eError ) {
    std::cerr << "Vulkan ERROR: " << pCallbackData->pMessage << std::endl;
    std::exit(1);
  }*/
  std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

  return VK_FALSE;
}
#endif

void Util::ensureExtension(const std::vector<vk::ExtensionProperties>& extensions, std::string extensionName) {
  if( std::find_if(extensions.begin(), extensions.end(), [&](auto& e) {return e.extensionName == extensionName;}) == extensions.end())
    throw std::runtime_error("Extension not supported: " + extensionName);
}

uint32_t Util::findQueue(DeviceInstance& device, vk::QueueFlags requiredFlags) {
  auto qFamProps = device.physicalDevice().getQueueFamilyProperties();
  auto it = std::find_if(qFamProps.begin(), qFamProps.end(), [&](auto& p) {
    if( p.queueFlags & requiredFlags ) return true;
    return false;
  });
  if( it == qFamProps.end() ) return std::numeric_limits<uint32_t>::max();
  return static_cast<uint32_t>(it - qFamProps.begin());
}


#ifdef DEBUG
void Util::initDidl( vk::Instance& instance ) {
  mDidl.init(instance, ::vkGetInstanceProcAddr);
}


void Util::initDebugMessenger( vk::Instance& instance ) {
  auto info = vk::DebugUtilsMessengerCreateInfoEXT()
      .setFlags({})
      .setMessageSeverity({vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                           vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                           vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                           vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning})
      .setMessageType({vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                       vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                       vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation})
      .setPfnUserCallback(debugCallback)
      .setPUserData(nullptr)
      ;

  mDebugUtilsMessenger = instance.createDebugUtilsMessengerEXTUnique(info, nullptr, mDidl);
  mDebugCallbackValid = true;
}
#endif

std::vector<char> Util::readFile(const std::string& fileName) {
  std::ifstream file(fileName, std::ios::ate | std::ios::binary);
  if( !file.is_open() ) throw std::runtime_error("Failed to open file: " + fileName);
  auto fileSize = file.tellg(); // We started at the end
  std::vector<char> buf(fileSize);
  file.seekg(0);
  file.read(buf.data(), fileSize);
  file.close();
  return buf;
}

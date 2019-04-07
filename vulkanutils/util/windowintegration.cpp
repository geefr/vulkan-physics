/*
 * Provided under the BSD 3-Clause License, see LICENSE.
 *
 * Copyright (c) 2019, Gareth Francis
 * All rights reserved.
 */

#include "windowintegration.h"

#include <iostream>

WindowIntegration::WindowIntegration(DeviceInstance& deviceInstance, DeviceInstance::QueueRef& queue, vk::PresentModeKHR presentMode)
  : mDeviceInstance(deviceInstance)
  , mPresentMode(presentMode) {
}

WindowIntegration::WindowIntegration(GLFWwindow* window, DeviceInstance& deviceInstance, DeviceInstance::QueueRef& queue, vk::PresentModeKHR presentMode)
  : WindowIntegration(deviceInstance, queue, presentMode) {
  createSurfaceGLFW(window);
  createSwapChain(queue);
  createSwapChainImageViews();
}

WindowIntegration::~WindowIntegration() {
  for(auto& p : mSwapChainImageViews) p.reset();
  mSwapChain.reset();
  vkDestroySurfaceKHR(mDeviceInstance.instance(), mSurface, nullptr);
}

#ifdef USE_GLFW
void WindowIntegration::createSurfaceGLFW(GLFWwindow* window) {
  VkSurfaceKHR surface;
  auto& instance = mDeviceInstance.instance();
  if(glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
    throw std::runtime_error("createSurfaceGLFW: Failed to create window surface");
  }
  mSurface = surface;
}
#endif


void WindowIntegration::createSwapChain(DeviceInstance::QueueRef& queue) {
  if( !mDeviceInstance.physicalDevice().getSurfaceSupportKHR(queue.famIndex, mSurface) ) throw std::runtime_error("createSwapChainXlib: Physical device doesn't support surfaces");

  // Parameters used in swapchain must comply with limits of the surface
  auto caps = mDeviceInstance.physicalDevice().getSurfaceCapabilitiesKHR(mSurface);

  // First the number of images (none, double, triple buffered)
  auto numImages = 3u;
  while( numImages > caps.maxImageCount ) numImages--;
  if( numImages != 3u ) std::cerr << "Creating swapchain with " << numImages << " images" << std::endl;
  if( numImages < caps.minImageCount ) throw std::runtime_error("Unable to create swapchain, invalid num images");

  // Ideally we want full alpha support
  auto alphaMode = vk::CompositeAlphaFlagBitsKHR::ePreMultiplied; // TODO: Pre or post? can't remember the difference
  if( !(caps.supportedCompositeAlpha & alphaMode) ) {
    std::cerr << "Surface doesn't support full alpha, falling back" << std::endl;
    alphaMode = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  }

  auto imageUsage = vk::ImageUsageFlags(vk::ImageUsageFlagBits::eColorAttachment);
  if( !(caps.supportedUsageFlags & imageUsage) ) throw std::runtime_error("Surface doesn't support color attachment");

  // caps.maxImageArrayLayers;
  // caps.minImageExtent;
  // caps.currentTransform;
  // caps.supportedTransforms;

  // Choose a pixel format
  auto surfaceFormats = mDeviceInstance.physicalDevice().getSurfaceFormatsKHR(mSurface);
  // TODO: Just choosing the first one here..
  mSwapChainFormat = surfaceFormats.front().format;
  auto chosenColourSpace = surfaceFormats.front().colorSpace;
  mSwapChainExtent = caps.currentExtent;
  auto info = vk::SwapchainCreateInfoKHR()
      .setFlags({})
      .setSurface(mSurface)
      .setMinImageCount(numImages)
      .setImageFormat(mSwapChainFormat)
      .setImageColorSpace(chosenColourSpace)
      .setImageExtent(mSwapChainExtent)
      .setImageArrayLayers(1)
      .setImageUsage(imageUsage)
      .setImageSharingMode(vk::SharingMode::eExclusive)
      .setPreTransform(caps.currentTransform)
      .setCompositeAlpha(alphaMode)
      .setPresentMode(mPresentMode)
      .setClipped(true)
      .setOldSwapchain({})
      ;

  mSwapChain = mDeviceInstance.device().createSwapchainKHRUnique(info);
  mSwapChainImages = mDeviceInstance.device().getSwapchainImagesKHR(mSwapChain.get());
}

void WindowIntegration::createSwapChainImageViews() {
  for( auto i = 0u; i < mSwapChainImages.size(); ++i ) {
    auto info = vk::ImageViewCreateInfo()
        .setFlags({})
        .setImage(mSwapChainImages[i])
        .setViewType(vk::ImageViewType::e2D)
        .setFormat(mSwapChainFormat)
        .setComponents({})
        .setSubresourceRange({{vk::ImageAspectFlagBits::eColor},0, 1, 0, 1})
        ;
    mSwapChainImageViews.emplace_back( mDeviceInstance.device().createImageViewUnique(info) );
  }
}


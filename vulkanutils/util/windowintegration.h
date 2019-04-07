/*
 * Provided under the BSD 3-Clause License, see LICENSE.
 *
 * Copyright (c) 2019, Gareth Francis
 * All rights reserved.
 */

#ifndef WINDOWINTEGRATION_H
#define WINDOWINTEGRATION_H

#ifdef USE_GLFW
# define GLFW_INCLUDE_VULKAN
# include <GLFW/glfw3.h>
#else
# error "WSI type not supported by WindowIntegration"
#endif
#include <vulkan/vulkan.hpp>

#include "deviceinstance.h"

/**
 * Functionality for window system integration, swapchains and such
 */
class WindowIntegration
{
public:
  WindowIntegration() = delete;
  WindowIntegration(DeviceInstance& deviceInstance, DeviceInstance::QueueRef& queue, vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo);
  WindowIntegration(const WindowIntegration&) = delete;
  WindowIntegration(WindowIntegration&&) = default;
  ~WindowIntegration();

#ifdef USE_GLFW
  WindowIntegration(GLFWwindow* window, DeviceInstance& deviceInstance, DeviceInstance::QueueRef& queue, vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo);
#endif

  // TODO: Giving direct access like this is dangerous, clean this up
  const vk::Extent2D& extent() const { return mSwapChainExtent; }
  const vk::Format& format() const { return mSwapChainFormat; }
  const vk::SwapchainKHR& swapChain() const { return mSwapChain.get(); }
  const std::vector<vk::Image>& swapChainImages() const { return mSwapChainImages; }
  const std::vector<vk::UniqueImageView>& swapChainImageViews() const { return mSwapChainImageViews; }

private:
#ifdef USE_GLFW
  void createSurfaceGLFW(GLFWwindow* window);
#endif

  void createSwapChain(DeviceInstance::QueueRef& queue);
  void createSwapChainImageViews();

  DeviceInstance& mDeviceInstance;

  // These members 'owned' by the unique pointers below
  vk::Format mSwapChainFormat = vk::Format::eUndefined;
  vk::Extent2D mSwapChainExtent;
  std::vector<vk::Image> mSwapChainImages;

  // TODO: >.< For some reason trying to use a UniqueSurfaceKHR here
  // messes up, and we end up with the wrong value for mSurface.m_owner
  // This causes a segfault when destroying the surface as it's calling
  // a function on an invalid surface
  // Maybe it's best to just use the raw types here, the smart pointers
  // are proving to be tricky and as we're still having to manually reset
  // them there's not much point using them in the first place
  vk::SurfaceKHR mSurface;

  // Order of members matters for Unique pointers
  // will be destructed in reverse order
  vk::UniqueSwapchainKHR mSwapChain;
  std::vector<vk::UniqueImageView> mSwapChainImageViews;
  vk::PresentModeKHR mPresentMode;
};

#endif // WINDOWINTEGRATION_H

/*
 * Provided under the BSD 3-Clause License, see LICENSE.
 *
 * Copyright (c) 2019, Gareth Francis
 * All rights reserved.
 */

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <vulkan/vulkan.hpp>

class WindowIntegration;

class FrameBuffer
{
public:
  FrameBuffer() = delete;
  FrameBuffer(const FrameBuffer&) = delete;
  FrameBuffer(FrameBuffer&&) = default;
  FrameBuffer(vk::Device& device, const WindowIntegration& windowIntegration, vk::RenderPass& renderPass);
  ~FrameBuffer() = default;

  const std::vector<vk::UniqueFramebuffer>& frameBuffers() const { return mFrameBuffers; }

private:
  void createFrameBuffers( vk::Device& device, const WindowIntegration& windowIntegration, vk::RenderPass& renderPass );
  std::vector<vk::UniqueFramebuffer> mFrameBuffers;
};

#endif // FRAMEBUFFER_H

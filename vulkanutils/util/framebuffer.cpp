/*
 * Provided under the BSD 3-Clause License, see LICENSE.
 *
 * Copyright (c) 2019, Gareth Francis
 * All rights reserved.
 */

#include "framebuffer.h"

#include "windowintegration.h"

FrameBuffer::FrameBuffer(vk::Device& device, const WindowIntegration& windowIntegration, vk::RenderPass& renderPass)
{
  createFrameBuffers(device, windowIntegration, renderPass);
}

void FrameBuffer::createFrameBuffers( vk::Device& device, const WindowIntegration& windowIntegration, vk::RenderPass& renderPass ) {
  if( !mFrameBuffers.empty() ) throw std::runtime_error("Framenbuffer::createFramebuffers: Already initialised");
  for( auto i = 0u; i < windowIntegration.swapChainImages().size(); ++i ) {
    auto attachment = windowIntegration.swapChainImageViews()[i].get();
    auto info = vk::FramebufferCreateInfo()
        .setFlags({})
        .setRenderPass(renderPass) // Compatible with this render pass (don't use it with any other)
        .setAttachmentCount(1) // 1 attachment - The image from the swap chain
        .setPAttachments(&attachment) // Mapped to the first attachment of the render pass
        .setWidth(windowIntegration.extent().width)
        .setHeight(windowIntegration.extent().height)
        .setLayers(1)
        ;

    mFrameBuffers.emplace_back( device.createFramebufferUnique(info));
  }
}

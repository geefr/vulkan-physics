/*
 * Provided under the BSD 3-Clause License, see LICENSE.
 *
 * Copyright (c) 2019, Gareth Francis
 * All rights reserved.
 */

#include "graphicspipeline.h"
#include "windowintegration.h"
#include "deviceinstance.h"
#include "util.h"

#include <vulkan/vulkan.hpp>

#include <map>

GraphicsPipeline::GraphicsPipeline(WindowIntegration& windowIntegration, DeviceInstance& deviceInstance)
  : Pipeline(deviceInstance)
  , mWindowIntegration(windowIntegration) {

}

void GraphicsPipeline::createRenderPass() {
  // The render pass contains stuff like what the framebuffer attachments are and things
  auto colourAttachment = vk::AttachmentDescription()
      .setFlags({})
      .setFormat(mWindowIntegration.format())
      .setSamples(vk::SampleCountFlagBits::e1)
      .setLoadOp(vk::AttachmentLoadOp::eClear) // What to do before rendering
      .setStoreOp(vk::AttachmentStoreOp::eStore) // What to do after rendering
      .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
      .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
      .setInitialLayout(vk::ImageLayout::eUndefined) // Image layout before render pass begins, we don't care, gonna clear anyway
      .setFinalLayout(vk::ImageLayout::ePresentSrcKHR) // Image layout to transfer to when render pass finishes, ready for presentation
      ;

  // For starters we only need one sub-pass to draw
  // Multiple sub passes are used for multi-pass rendering
  // putting them in one overall render pass means vulkan can reorder
  // them as needed
  auto colourAttachmentRef = vk::AttachmentReference()
      .setAttachment(0) // Index in attachment description array (We only have one so far)
      .setLayout(vk::ImageLayout::eColorAttachmentOptimal) // Layout for the attachment during this subpass. This attachment is our colour buffer so marked as such here
      ;

  auto subpass = vk::SubpassDescription()
      .setFlags({})
      .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics) // Marked as a graphics subpass, looks like we can mix multiple subpass types in one render pass?
      .setColorAttachmentCount(1)
      .setPColorAttachments(&colourAttachmentRef)
      ;
  // Index of colour attachment here matches layout = 0 in fragment shader
  // So the frag shader can reference multiple attachments as its output!


  /*
   * Setup subpass dependencies
   * This is needed to ensure that the render pass
   * doesn't begin until tthe image is available
   *
   * Apparently an alternative is to set the waitStages of the command buffer
   * to top of pipe to ensure rendering doesn't begin until the pipeline is started?
   * https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation
   *
   * TODO: Yeah I don't understand this and the tutorials gloss over it, probably more info in the
   * synchronisation chapter of the book
   */
  auto dep = vk::SubpassDependency()
      .setSrcSubpass(VK_SUBPASS_EXTERNAL) // implicit subpass before this pass
      .setDstSubpass(0) // The index of our subpass
      .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput) // Wait on this operation
      .setSrcAccessMask(vk::AccessFlags()) // Which needs to happen in this stage...here we're waiting for the swap chain to finish reading from the image
      .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput) // The operations which should wait are in the colour attachment stage
      .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite) // and involve the reading and writing of the colour attachment
                                                            // These prevent the transition from happening until it's necessary/allowed, when we want to start writing colours to it..
      ;

  // Now let's actually make the render pass
  auto renderPassInfo = vk::RenderPassCreateInfo()
      .setFlags({})
      .setAttachmentCount(1)
      .setPAttachments(&colourAttachment)
      .setSubpassCount(1)
      .setPSubpasses(&subpass)
      .setDependencyCount(1)
      .setPDependencies(&dep)
      ;

  mRenderPass = mDeviceInstance.device().createRenderPassUnique(renderPassInfo);
}

void GraphicsPipeline::createPipeline() {
  /*
   * The programmable parts of the pipeline are similar to gl
   * And in this case the shaders were even compiled from glsl -> SPIR-V
   */

  // Later we just need to hand a pile of shaders to the pipeline
  auto shaderStages = createShaderStageInfo();
  createRenderPass();

  /*
   * Then there's the fixed function sections of the pipeline
   */
  // Vertex input
  // For now nothing here as vertices are hardcoded in the shader
  uint32_t numVertexBindings = static_cast<uint32_t>(mVertexInputBindings.size());
  uint32_t numVertexAttributes = static_cast<uint32_t>(mVertexInputAttributes.size());
  auto vertInputInfo = vk::PipelineVertexInputStateCreateInfo()
      .setFlags({})
      .setVertexBindingDescriptionCount(numVertexBindings)
      .setPVertexBindingDescriptions(numVertexBindings ? mVertexInputBindings.data() : nullptr)
      .setVertexAttributeDescriptionCount(numVertexAttributes)
      .setPVertexAttributeDescriptions(numVertexAttributes ? mVertexInputAttributes.data() : nullptr)
      ;

  // Input assembly
  // Typical triangles setup
  auto inputAssInfo = vk::PipelineInputAssemblyStateCreateInfo()
      .setFlags({})
      .setTopology(mInputAssemblyPrimitiveTopology)
      .setPrimitiveRestartEnable(false)
      ;

  // Viewport
  vk::Viewport viewport(0.f,0.f, mWindowIntegration.extent().width, mWindowIntegration.extent().height, 0.f, 1.f);
  vk::Rect2D scissor({0,0},mWindowIntegration.extent());
  auto viewportInfo = vk::PipelineViewportStateCreateInfo()
      .setFlags({})
      .setViewportCount(1)
      .setPViewports(&viewport)
      .setScissorCount(1)
      .setPScissors(&scissor)
      ;

  // Rasteriser
  auto rasterisationInfo = vk::PipelineRasterizationStateCreateInfo()
      .setFlags({})
      .setDepthClampEnable(false)
      .setRasterizerDiscardEnable(false)
      .setPolygonMode(vk::PolygonMode::eFill)
      .setCullMode(vk::CullModeFlagBits::eBack)
      .setFrontFace(vk::FrontFace::eCounterClockwise)
      .setDepthBiasEnable(false)
      .setDepthBiasConstantFactor(0.f)
      .setDepthClampEnable(false)
      .setDepthBiasClamp(0.f)
      .setLineWidth(1.f)
      ;

  // Multisampling
  auto multisampleInfo = vk::PipelineMultisampleStateCreateInfo()
      .setSampleShadingEnable(false)
      .setRasterizationSamples(vk::SampleCountFlagBits::e1);

  // Depth/Stencil test
  // Not yet, we're gonna pass null for this one ;)

  // Colour blending
  // attachment state is per-framebuffer
  // creat info is global to the pipeline
  auto colourBlendAttach = vk::PipelineColorBlendAttachmentState()
      .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
      .setBlendEnable(false)
      .setBlendEnable(true)
      .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
      .setDstColorBlendFactor(vk::BlendFactor::eOneMinusDstAlpha)
      .setColorBlendOp(vk::BlendOp::eAdd)
      .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
      .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
      .setAlphaBlendOp(vk::BlendOp::eAdd)
      ;

  auto colourBlendInfo = vk::PipelineColorBlendStateCreateInfo()
      .setFlags({})
      .setLogicOpEnable(false)
      .setLogicOp(vk::LogicOp::eCopy)
      .setAttachmentCount(1)
      .setPAttachments(&colourBlendAttach)
      .setBlendConstants({0.f,0.f,0.f,0.f})
      ;

  // Dynamic state
  // There's a dynamic state section that allows setting things like viewport/etc without rebuilding
  // the entire pipeline
  // If this is provided then the state has to be provided at draw time, so for now it's just another nullptr

  // Pipeline layout
  // Pipeline layout is where uniforms and such go
  // and they have to be known when the pipeline is built
  // So no randomly chucking uniforms around like we do in gl right?
  auto numPushConstantRanges = static_cast<uint32_t>(mPushConstants.size());
  auto layoutInfo = vk::PipelineLayoutCreateInfo()
      .setFlags({})
      .setSetLayoutCount(0)
      .setPSetLayouts(nullptr)
      .setPushConstantRangeCount(numPushConstantRanges)
      .setPPushConstantRanges(numPushConstantRanges ? mPushConstants.data() : nullptr)
      ;

  mPipelineLayout = mDeviceInstance.device().createPipelineLayoutUnique(layoutInfo);


  // Finally let's make the pipeline itself
  auto pipelineInfo = vk::GraphicsPipelineCreateInfo()
      .setFlags({})
      .setStageCount(static_cast<uint32_t>(shaderStages.size()))
      .setPStages(shaderStages.data())
      .setPVertexInputState(&vertInputInfo)
      .setPInputAssemblyState(&inputAssInfo)
      .setPViewportState(&viewportInfo)
      .setPRasterizationState(&rasterisationInfo)
      .setPMultisampleState(&multisampleInfo)
      .setPDepthStencilState(nullptr)
      .setPColorBlendState(&colourBlendInfo)
      .setPDynamicState(nullptr)
      .setLayout(mPipelineLayout.get())
      .setRenderPass(mRenderPass.get()) // The render pass the pipeline will be used in
      .setSubpass(0) // The sub pass the pipeline will be used in
      .setBasePipelineHandle({}) // Derive from an existing pipeline
      .setBasePipelineIndex(-1) // Or the index of a pipeline, which may not yet exist. DERIVATIVE_BIT must be specified in flags to do this
      ;


  mPipeline = mDeviceInstance.device().createGraphicsPipelineUnique({}, pipelineInfo);

  // Shader modules deleted here, only needed for pipeline init
}



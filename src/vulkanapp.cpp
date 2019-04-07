/*
 * Provided under the BSD 3-Clause License, see LICENSE.
 *
 * Copyright (c) 2019, Gareth Francis
 * All rights reserved.
 */

#include "vulkanapp.h"
#include <mutex>
#include <chrono>
#include <random>

#include "glm/gtc/matrix_transform.hpp"

VulkanApp::VulkanApp() {
  // TODO: Must be a multiple of 4, we don't validate buffer size before throwing at vulkan
  auto numParticles = 5000000u;

  std::random_device rd;
   std::mt19937 rdGen(rd());
   std::uniform_real_distribution<> dis(-10.0, 10.0);
   std::uniform_real_distribution<> disM(0.1, 100.0);
   std::uniform_real_distribution<> disC(0.0,1.0);
   for( auto i = 0u; i < numParticles; ++i ) {
     auto p = Particle();
     p.position = {dis(rdGen), dis(rdGen), dis(rdGen), 1};
     p.mass = static_cast<float>(disM(rdGen));
     p.colour = {disC(rdGen), disC(rdGen), disC(rdGen), 1};

     p.velocity = glm::vec4(dis(rdGen), dis(rdGen), dis(rdGen), 1);

     mParticles.emplace_back(p);
   }

}

VulkanApp::~VulkanApp() {

}

void VulkanApp::initWindow() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  mWindow = glfwCreateWindow(mWindowWidth, mWindowHeight, "Vulkan Experiment", nullptr, nullptr);
}

void VulkanApp::initVK() {
  // Initialise the vulkan instance
  // GLFW can give us what extensions it requires, nice
  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  std::vector<const char*> requiredExtensions;
  for(uint32_t i = 0; i < glfwExtensionCount; ++i ) requiredExtensions.push_back(glfwExtensions[i]);
  std::vector<const char*> enabledLayers = {};

  // One for rendering and one for computing
  std::vector<vk::QueueFlags> requiredQueues = { vk::QueueFlagBits::eGraphics, vk::QueueFlagBits::eCompute };
  mDeviceInstance.reset(new DeviceInstance(requiredExtensions, {}, "Vulkan Test Application", 1, VK_API_VERSION_1_0, requiredQueues, enabledLayers));

  mGraphicsQueue = mDeviceInstance->getQueue(requiredQueues[0]);
  mComputeQueue = mDeviceInstance->getQueue(requiredQueues[1]);
  if( !mGraphicsQueue || !mComputeQueue ) throw std::runtime_error("Failed to get graphics and compute queues");

  // Find out what queues are available
  //auto queueFamilyProps = dev.getQueueFamilyProperties();
  //printQueueFamilyProperties(queueFamilyProps);

  // Create a logical device to interact with
  // To do this we also need to specify how many queues from which families we want to create
  // In this case just 1 queue from the first family which supports graphics

  mWindowIntegration.reset(new WindowIntegration(mWindow, *mDeviceInstance.get(), *mGraphicsQueue, vk::PresentModeKHR::eImmediate));

  mGraphicsPipeline.reset(new GraphicsPipeline(*mWindowIntegration.get(), *mDeviceInstance.get()));
  mComputePipeline.reset(new ComputePipeline(*mDeviceInstance.get()));

  // Build the graphics pipeline
  // In this case we can throw away the shader modules after building as they're only used by the one pipeline
  {
    mGraphicsPipeline->shaders()[vk::ShaderStageFlagBits::eVertex] = mGraphicsPipeline->createShaderModule("vert.spv");
    mGraphicsPipeline->shaders()[vk::ShaderStageFlagBits::eFragment] = mGraphicsPipeline->createShaderModule("frag.spv");
    mGraphicsPipeline->inputAssembly_primitiveTopology(vk::PrimitiveTopology::ePointList);

    // The layout of our vertex buffers
    auto vertBufferBinding = vk::VertexInputBindingDescription()
        .setBinding(0)
        .setStride(sizeof(Particle))
        .setInputRate(vk::VertexInputRate::eVertex);
    mGraphicsPipeline->vertexInputBindings().emplace_back(vertBufferBinding);

    // Location, Binding, Format, Offset
    // Only binding the attributes needed for rendering, when infact the buffer contains the rest of the particles info aswell
    // Could alternatively have 2 buffers, one for rendering and one for physics
    mGraphicsPipeline->vertexInputAttributes().emplace_back(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Particle, position));
    mGraphicsPipeline->vertexInputAttributes().emplace_back(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Particle, colour));
    mGraphicsPipeline->vertexInputAttributes().emplace_back(2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Particle, radius));

    // Register our push constant block
    mPushContantsRange = vk::PushConstantRange()
        .setStageFlags(vk::ShaderStageFlagBits::eVertex)
        .setOffset(0)
        .setSize(sizeof(PushConstants));
    mGraphicsPipeline->pushConstants().emplace_back(mPushContantsRange);
    mGraphicsPipeline->build();
  }

  // Build the compute pipeline
  {
    mComputePipeline->shaders()[vk::ShaderStageFlagBits::eCompute] = mComputePipeline->createShaderModule("comp.spv");
    // Input and output buffers to compute shader
    mComputePipeline->addDescriptorSetLayoutBinding(0, 0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute);
    mComputePipeline->addDescriptorSetLayoutBinding(0, 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute);

    mComputeSpecConstants.mComputeBufferWidth = static_cast<uint32_t>(mParticles.size());
    vk::SpecializationMapEntry specs[] = {
      {0, offsetof(ComputeSpecConstants, mComputeBufferWidth), sizeof(uint32_t)},
      {1, offsetof(ComputeSpecConstants, mComputeBufferHeight), sizeof(uint32_t)},
      {2, offsetof(ComputeSpecConstants, mComputeBufferDepth), sizeof(uint32_t)},
      {3, offsetof(ComputeSpecConstants, mComputeGroupSizeX), sizeof(uint32_t)},
      {4, offsetof(ComputeSpecConstants, mComputeGroupSizeY), sizeof(uint32_t)},
      {5, offsetof(ComputeSpecConstants, mComputeGroupSizeZ), sizeof(uint32_t)},
    };
    mComputePipeline->specialisationConstants()[vk::ShaderStageFlagBits::eCompute] = vk::SpecializationInfo(6, specs, sizeof(ComputeSpecConstants), &mComputeSpecConstants);

    mComputePipeline->build();
  }

  mFrameBuffer.reset(new FrameBuffer(mDeviceInstance->device(), *mWindowIntegration.get(), mGraphicsPipeline->renderPass()));

  // Setup our sync primitives
  // imageAvailable - gpu: Used to stall the pipeline until the presentation has finished reading from the image
  // renderFinished - gpu: Used to stall presentation until the pipeline is finished
  // frameInFlightFence - cpu: Used to ensure we don't schedule a second frame for each image until the last is complete

  // Create the semaphores we're gonna use
  mMaxFramesInFlight = static_cast<uint32_t>(mWindowIntegration->swapChainImages().size());
  for( auto i = 0u; i < mMaxFramesInFlight; ++i ) {
    mImageAvailableSemaphores.emplace_back( mDeviceInstance->device().createSemaphoreUnique({}));
    mRenderFinishedSemaphores.emplace_back( mDeviceInstance->device().createSemaphoreUnique({}));
    // Create fence in signalled state so first wait immediately returns and resets fence
    mFrameInFlightFences.emplace_back( mDeviceInstance->device().createFenceUnique({vk::FenceCreateFlagBits::eSignaled}));
  }

  // Create buffers
  createComputeBuffers();
  createComputeDescriptorSet();

  // Command pool/buffers for compute
  // TODO: If both queue pointers are the same should maybe use a single pool?
  {
    mComputeCommandPool = mDeviceInstance->createCommandPool( { vk::CommandPoolCreateFlagBits::eResetCommandBuffer
                                                         /* vk::CommandPoolCreateFlagBits::eTransient | // Buffers will be short-lived and returned to pool shortly after use
                                                            vk::CommandPoolCreateFlagBits::eResetCommandBuffer // Buffers can be reset individually, instead of needing to reset the entire pool
                                                                                                                                                */
                                                       }, *mComputeQueue);

    // Now make a command buffer for each framebuffer
    auto commandBufferAllocateInfo = vk::CommandBufferAllocateInfo()
        .setCommandPool(mComputeCommandPool.get())
        .setCommandBufferCount(static_cast<uint32_t>(mMaxFramesInFlight))
        .setLevel(vk::CommandBufferLevel::ePrimary);
    mComputeCommandBuffers = mDeviceInstance->device().allocateCommandBuffersUnique(commandBufferAllocateInfo);
  }

  // TODO: Could be utilitised
  // Command pool/buffers for rendering
  {
    mCommandPool = mDeviceInstance->createCommandPool( { vk::CommandPoolCreateFlagBits::eResetCommandBuffer
                                                         /* vk::CommandPoolCreateFlagBits::eTransient | // Buffers will be short-lived and returned to pool shortly after use
                                                            vk::CommandPoolCreateFlagBits::eResetCommandBuffer // Buffers can be reset individually, instead of needing to reset the entire pool
                                                                                                                                                */
                                                       }, *mGraphicsQueue);

    // Now make a command buffer for each framebuffer
    auto commandBufferAllocateInfo = vk::CommandBufferAllocateInfo()
        .setCommandPool(mCommandPool.get())
        .setCommandBufferCount(static_cast<uint32_t>(mWindowIntegration->swapChainImages().size()))
        .setLevel(vk::CommandBufferLevel::ePrimary)
        ;

    mCommandBuffers = mDeviceInstance->device().allocateCommandBuffersUnique(commandBufferAllocateInfo);
  }
}

void VulkanApp::buildCommandBuffer(vk::CommandBuffer& commandBuffer, const vk::Framebuffer& frameBuffer, vk::Buffer& particleVertexBuffer) {

  auto beginInfo = vk::CommandBufferBeginInfo()
      .setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse) // Buffer can be resubmitted while already pending execution
      .setPInheritanceInfo(nullptr)
      ;
  commandBuffer.begin(beginInfo);

  // Start the render pass
  // Info for attachment load op clear
  std::array<float,4> col = {0.f,.0f,0.f,1.f};
  vk::ClearValue clearColour(col);

  auto renderPassInfo = vk::RenderPassBeginInfo()
      .setRenderPass(mGraphicsPipeline->renderPass())
      .setFramebuffer(frameBuffer)
      .setClearValueCount(1)
      .setPClearValues(&clearColour);
  renderPassInfo.renderArea.offset = vk::Offset2D(0,0);
  renderPassInfo.renderArea.extent = mWindowIntegration->extent();

  // Barrier to prevent the start of vertex shader until writing has finished to particle buffer
  // Remember:
  // - Access flags should be as minimal as possible here
  // - Barriers must be outside a render pass (there's an exception to this, but keep it simple for now)
  auto particleBufferBarrier = vk::BufferMemoryBarrier()
      .setSrcAccessMask(vk::AccessFlagBits::eShaderWrite)
      .setDstAccessMask(vk::AccessFlagBits::eVertexAttributeRead)
      .setSrcQueueFamilyIndex(mComputeQueue->famIndex)
      .setDstQueueFamilyIndex(mGraphicsQueue->famIndex)
      .setBuffer(particleVertexBuffer)
      .setOffset(0)
      .setSize(VK_WHOLE_SIZE);

  commandBuffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eComputeShader,
        vk::PipelineStageFlagBits::eVertexInput,
        vk::DependencyFlagBits::eByRegion,
        0, nullptr,
        1, &particleBufferBarrier,
        0, nullptr
        );

  // render commands will be embedded in primary buffer and no secondary command buffers
  // will be executed
  commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
  commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, mGraphicsPipeline->pipeline());

  // Set our push constants
  commandBuffer.pushConstants(
        mGraphicsPipeline->pipelineLayout(),
        mPushContantsRange.stageFlags,
        mPushContantsRange.offset,
        sizeof(PushConstants),
        &mPushConstants);

  vk::Buffer buffers[] = { particleVertexBuffer };
  vk::DeviceSize offsets[] = { 0 };
  commandBuffer.bindVertexBuffers(0, 1, buffers, offsets);

  commandBuffer.draw(static_cast<uint32_t>(mParticles.size()), // Draw n vertices
                     1, // Used for instanced rendering, 1 otherwise
                     0, // First vertex
                     0  // First instance
                     );

  // End the render pass
  commandBuffer.endRenderPass();

  // End the command buffer
  commandBuffer.end();
}

void VulkanApp::buildComputeCommandBuffer(vk::CommandBuffer& commandBuffer, vk::DescriptorSet& descriptorSet, vk::Buffer& particleVertexBuffer) {
  auto beginInfo = vk::CommandBufferBeginInfo()
      .setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse) // Buffer can be resubmitted while already pending execution
      .setPInheritanceInfo(nullptr);
  commandBuffer.begin(beginInfo);

  // Bind the compute pipeline
  commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, mComputePipeline->pipeline());

  // Bind the descriptor sets - Bind the descriptor set (which points to the buffers) to the pipeline
  commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                                   mComputePipeline->pipelineLayout(),
                                   0, 1,
                                   &descriptorSet,
                                   0, nullptr);

  // Barrier to prevent the start of compute shader until reading has finished from particle buffer
  // Remember:
  // - Access flags should be as minimal as possible here
  // - Barriers must be outside a render pass (there's an exception to this, but keep it simple for now)
  auto particleBufferBarrier = vk::BufferMemoryBarrier()
      .setSrcAccessMask(vk::AccessFlagBits::eVertexAttributeRead)
      .setDstAccessMask(vk::AccessFlagBits::eShaderWrite)
      .setSrcQueueFamilyIndex(mComputeQueue->famIndex)
      .setDstQueueFamilyIndex(mGraphicsQueue->famIndex)
      .setBuffer(particleVertexBuffer)
      .setOffset(0)
      .setSize(VK_WHOLE_SIZE);

  commandBuffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eVertexInput,
        vk::PipelineStageFlagBits::eComputeShader,
        vk::DependencyFlagBits::eByRegion,
        0, nullptr,
        1, &particleBufferBarrier,
        0, nullptr
        );

  // Dispatch the pipeline - equivalent of a 'draw'
  // Number of groups is specified here, size of a group is set in the shader
  commandBuffer.dispatch(mComputeSpecConstants.mComputeBufferWidth / mComputeSpecConstants.mComputeGroupSizeX,
                         mComputeSpecConstants.mComputeBufferHeight / mComputeSpecConstants.mComputeGroupSizeY,
                         mComputeSpecConstants.mComputeBufferDepth / mComputeSpecConstants.mComputeGroupSizeZ );

  // End the command buffer
  commandBuffer.end();
}

void VulkanApp::buildComputeCommandBufferDataUpload(vk::CommandBuffer& commandBuffer, SimpleBuffer& targetBuffer) {
  auto beginInfo = vk::CommandBufferBeginInfo()
      .setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse) // Buffer can be resubmitted while already pending execution
      .setPInheritanceInfo(nullptr);
  commandBuffer.begin(beginInfo);

  // Upload initial buffer state into target buffer
  auto chunkSize = 65536 / sizeof(Particle);
  if( chunkSize == 0 ) chunkSize = static_cast<uint32_t>(mParticles.size());
  for( auto i = 0u; i < mParticles.size(); i+=chunkSize ) {
    auto numToUpload = chunkSize;
    if( i + numToUpload >= mParticles.size() ) {
      numToUpload = static_cast<uint32_t>(mParticles.size() - i);
    }

    commandBuffer.updateBuffer(targetBuffer.buffer(), i * sizeof(Particle), numToUpload * sizeof(Particle), mParticles.data() + i);
  }

  // End the command buffer
  commandBuffer.end();
}

void VulkanApp::createComputeBuffers() {
  // 0 - input buffer
  // 1 - output buffer
  auto bufSize = sizeof(Particle) * mComputeSpecConstants.mComputeBufferWidth * mComputeSpecConstants.mComputeBufferHeight * mComputeSpecConstants.mComputeBufferDepth;
  for( auto i = 0u; i < mWindowIntegration->swapChainImages().size(); ++i ) {
    mComputeDataBuffers.emplace_back( new SimpleBuffer(
                                         *mDeviceInstance.get(),
                                         bufSize,
                                         vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                                         vk::MemoryPropertyFlagBits::eDeviceLocal /*vk::MemoryPropertyFlagBits::eHostVisible*/ ) );
  }
}

void VulkanApp::createComputeDescriptorSet() {
  // Create a descriptor pool, to allocate descriptor sets from
  auto poolSize = vk::DescriptorPoolSize()
      .setType(vk::DescriptorType::eStorageBuffer)
      .setDescriptorCount(mMaxFramesInFlight * 2);

  auto poolInfo = vk::DescriptorPoolCreateInfo()
      .setFlags({})
      .setMaxSets(mMaxFramesInFlight)
      .setPoolSizeCount(1)
      .setPPoolSizes(&poolSize);
  mComputeDescriptorPool = mDeviceInstance->device().createDescriptorPoolUnique(poolInfo);

  // Create the descriptor sets, one for each particle buffer
  for( auto i = 0u; i < mMaxFramesInFlight; ++i ) {
    const vk::DescriptorSetLayout dsLayouts[] = {mComputePipeline->descriptorSetLayouts()[0].get()};

    auto dsInfo = vk::DescriptorSetAllocateInfo()
        .setDescriptorPool(mComputeDescriptorPool.get())
        .setDescriptorSetCount(1)
        .setPSetLayouts(dsLayouts);
    auto sets = mDeviceInstance->device().allocateDescriptorSets(dsInfo);
    mComputeDescriptorSets.emplace_back(std::move(sets.front())); sets.clear();
  }

  for( auto i = 0u; i < mMaxFramesInFlight; ++i ) {
    auto& sourceBuffer = mComputeDataBuffers[i];
    auto& destBuffer = mComputeDataBuffers[i == mMaxFramesInFlight - 1 ? 0 : i + 1];

    // Update the descriptor set to map to the buffers
    std::vector<vk::DescriptorBufferInfo> uInfos;
    auto uInfo1 = vk::DescriptorBufferInfo()
        .setBuffer(sourceBuffer->buffer())
        .setOffset(0)
        .setRange(VK_WHOLE_SIZE);
    uInfos.emplace_back(uInfo1);

    auto uInfo2 = vk::DescriptorBufferInfo()
        .setBuffer(destBuffer->buffer())
        .setOffset(0)
        .setRange(VK_WHOLE_SIZE);
    uInfos.emplace_back(uInfo2);

    auto wInfo = vk::WriteDescriptorSet()
        .setDstSet(mComputeDescriptorSets[i])
        .setDstBinding(0)
        .setDstArrayElement(0)
        .setDescriptorCount(static_cast<uint32_t>(uInfos.size()))
        .setDescriptorType(vk::DescriptorType::eStorageBuffer)
        .setPImageInfo(nullptr)
        .setPBufferInfo(uInfos.data())
        .setPTexelBufferView(nullptr);

    mDeviceInstance->device().updateDescriptorSets(1, &wInfo, 0, nullptr);
  }
}

void VulkanApp::loop() {
  auto frameIndex = 0u;

  glm::vec3 eyePos = { 0,50,110 };
  float modelRot = 0.f;

  // Seed the particle buffer with data
  {
    buildComputeCommandBufferDataUpload(mComputeCommandBuffers[0].get(), *mComputeDataBuffers[0].get());
    auto subInfo = vk::SubmitInfo()
        .setCommandBufferCount(1)
        .setPCommandBuffers(&mComputeCommandBuffers[0].get());
    auto fence = mDeviceInstance->device().createFenceUnique({});
    mComputeQueue->queue.submit(1, &subInfo, fence.get());
    mDeviceInstance->device().waitForFences(1, &fence.get(), true, std::numeric_limits<uint64_t>::max());
  }

  // Build the compute command buffers for running the pipeline
  for( auto i = 0u; i < mMaxFramesInFlight; ++i ) {
    // Vertex buffer used to pass data from compute -> graphics, one ahead of current frame
    // both pipelines have barriers to synchronise access to this
    auto vertBufIndex = frameIndex + 1;
    if( vertBufIndex == mMaxFramesInFlight ) vertBufIndex = 0;
    buildComputeCommandBuffer(mComputeCommandBuffers[i].get(), mComputeDescriptorSets[i], mComputeDataBuffers[i]->buffer());
  }

  glfwShowWindow(mWindow);

  mLastTime = now();
  mCurTime = mLastTime;

  while(!glfwWindowShouldClose(mWindow) ) {

    glfwPollEvents();

    // Wait for the last frame to finish rendering
    mDeviceInstance->device().waitForFences(1, &mFrameInFlightFences[frameIndex].get(), true, std::numeric_limits<uint64_t>::max());

    // Physics hacks
    mLastTime = mCurTime;
    mCurTime = now();

    // Run the compute pipeline
    {
      auto subInfo = vk::SubmitInfo()
          .setCommandBufferCount(1)
          .setPCommandBuffers(&mComputeCommandBuffers[frameIndex].get());
      mComputeQueue->queue.submit(1, &subInfo, {});
    }

    // Setup matrices
    // Remember now vulkan is z[0,1] +y=down, gl is z[-1,1] +y=up
    // glm should work with the defines we've set
    // So now we've got world space as right handed (y up) but everything after that is left handed (y down)
    //if( eyePos.y < 20 ) eyePos.y += 0.05;

    modelRot += .1f;
    mPushConstants.modelMatrix = glm::mat4(1);//glm::rotate(glm::mat4(1), glm::radians(modelRot), glm::vec3(0.f,-1.f,0.f));
    mPushConstants.viewMatrix = glm::lookAt( eyePos, glm::vec3(0,-100,0), glm::vec3(0,-1,0));
    mPushConstants.projMatrix = glm::perspective(glm::radians(90.f),static_cast<float>(mWindowWidth / mWindowHeight), 0.001f,1000.f);

    // Reset the fence - fences must be reset before being submitted
    auto frameFence = mFrameInFlightFences[frameIndex].get();
    mDeviceInstance->device().resetFences(1, &frameFence);

    // Acquire and image from the swap chain
    auto imageIndex = mDeviceInstance->device().acquireNextImageKHR(
          mWindowIntegration->swapChain(), // Get an image from this
          std::numeric_limits<uint64_t>::max(), // Don't timeout
          mImageAvailableSemaphores[frameIndex].get(), // semaphore to signal once presentation is finished with the image
          vk::Fence()).value; // Dummy fence, we don't care here

    // Submit the command buffer
    vk::SubmitInfo submitInfo = {};
    auto commandBuffer = mCommandBuffers[frameIndex].get();
    auto frameBuffer = mFrameBuffer->frameBuffers()[imageIndex].get();

    // Rebuild the command buffer every frame
    // This isn't the most efficient but we're at least re-using the command buffer
    // In a most complex application we would have multiple command buffers and only rebuild
    // the section that needs changing..I think
    //
    // Data buffer here is the output buffer of the compute pass, so 1 ahead of frameIndex
    auto vertBufIndex = frameIndex + 1;
    if( vertBufIndex == mMaxFramesInFlight ) vertBufIndex = 0;
    buildCommandBuffer(commandBuffer, frameBuffer, mComputeDataBuffers[vertBufIndex]->buffer());

    // Don't execute until this is ready
    vk::Semaphore waitSemaphores[] = {mImageAvailableSemaphores[frameIndex].get()};
    // place the wait before writing to the colour attachment

    vk::PipelineStageFlags waitStages[] {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    // Signal this semaphore when rendering is done
    vk::Semaphore signalSemaphores[] = {mRenderFinishedSemaphores[frameIndex].get()};
    submitInfo.setWaitSemaphoreCount(1)
        .setPWaitSemaphores(waitSemaphores)
        .setPWaitDstStageMask(waitStages)
        .setCommandBufferCount(1)
        .setPCommandBuffers(&commandBuffer)
        .setSignalSemaphoreCount(1)
        .setPSignalSemaphores(signalSemaphores)
        ;

    vk::ArrayProxy<vk::SubmitInfo> submits(submitInfo);
    // submit, signal the frame fence at the end
    mGraphicsQueue->queue.submit(submits.size(), submits.data(), frameFence);

    // Present the results of a frame to the swap chain
    vk::SwapchainKHR swapChains[] = {mWindowIntegration->swapChain()};
    vk::PresentInfoKHR presentInfo = {};
    presentInfo.setWaitSemaphoreCount(1)
        .setPWaitSemaphores(signalSemaphores) // Wait before presentation can start
        .setSwapchainCount(1)
        .setPSwapchains(swapChains)
        .setPImageIndices(&imageIndex)
        .setPResults(nullptr);
    mGraphicsQueue->queue.presentKHR(presentInfo);

    // Advance to next frame index, loop at max
    frameIndex++;
    if( frameIndex == mMaxFramesInFlight ) frameIndex = 0;
  }
}

void VulkanApp::cleanup() {
  // Explicitly cleanup the vulkan objects here
  // Ensure they are shut down before terminating glfw
  // TODO: Destruction order matters, and somehow it's wrong despite having the smart pointers here
  mDeviceInstance->waitAllDevicesIdle();

  mFrameInFlightFences.clear();
  mRenderFinishedSemaphores.clear();
  mImageAvailableSemaphores.clear();
  mCommandBuffers.clear();
  mCommandPool.reset();
  mComputeCommandBuffers.clear();
  mComputeCommandPool.reset();
  mGraphicsPipeline.reset();
  mComputePipeline.reset();
  mFrameBuffer.reset();
  mWindowIntegration.reset();

  mComputeDescriptorPool.reset();

  //mParticleVertexBuffers.clear();
  mComputeDataBuffers.clear();

  mDeviceInstance.reset();

  // TODO: Could wrap the glfw stuff in a smart pointer and
  // remove the need for this method
  glfwDestroyWindow(mWindow);
  glfwTerminate();
}

double VulkanApp::now() {
  auto duration = std::chrono::system_clock::now().time_since_epoch();
  auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
  return millis / 1000.0;
}

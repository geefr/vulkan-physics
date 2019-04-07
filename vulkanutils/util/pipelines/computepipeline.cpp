/*
 * Provided under the BSD 3-Clause License, see LICENSE.
 *
 * Copyright (c) 2019, Gareth Francis
 * All rights reserved.
 */

#include "computepipeline.h"
#include "deviceinstance.h"
#include "util.h"

#include <vulkan/vulkan.hpp>

#include <map>

ComputePipeline::ComputePipeline(DeviceInstance& deviceInstance)
  : Pipeline(deviceInstance) {

}

void ComputePipeline::createPipeline() {
  // Later we just need to hand a pile of shaders to the pipeline
  auto shaderStage = createShaderStageInfo();

  // Pipeline layout
  // Pipeline layout is where uniforms and such go
  // and they have to be known when the pipeline is built
  // So no randomly chucking uniforms around like we do in gl right?
  auto numPushConstantRanges = static_cast<uint32_t>(mPushConstants.size());
  auto numDSLayouts = static_cast<uint32_t>(mDescriptorSetLayouts.size());
  // :/
  std::vector<vk::DescriptorSetLayout> tmpLayouts;
  for( auto& p : mDescriptorSetLayouts ) tmpLayouts.emplace_back(p.get());

  auto layoutInfo = vk::PipelineLayoutCreateInfo()
      .setFlags({})
      .setSetLayoutCount(numDSLayouts)
      .setPSetLayouts(numDSLayouts ? tmpLayouts.data() : nullptr)
      .setPushConstantRangeCount(numPushConstantRanges)
      .setPPushConstantRanges(numPushConstantRanges ? mPushConstants.data() : nullptr)
      ;
  mPipelineLayout = mDeviceInstance.device().createPipelineLayoutUnique(layoutInfo);


  // Finally let's make the pipeline itself
  auto pipelineInfo = vk::ComputePipelineCreateInfo()
      .setFlags({})
      .setStage(shaderStage.front())
      .setLayout(mPipelineLayout.get())
      .setBasePipelineHandle({})
      .setBasePipelineIndex(-1)
      ;

  mPipeline = mDeviceInstance.device().createComputePipelineUnique({}, pipelineInfo);
}

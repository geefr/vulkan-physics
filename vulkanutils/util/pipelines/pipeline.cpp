/*
 * Provided under the BSD 3-Clause License, see LICENSE.
 *
 * Copyright (c) 2019, Gareth Francis
 * All rights reserved.
 */

#include "pipeline.h"
#include "deviceinstance.h"
#include "util.h"

#include <vulkan/vulkan.hpp>

#include <map>

Pipeline::Pipeline(DeviceInstance& deviceInstance)
  : mDeviceInstance(deviceInstance) {

}

vk::Pipeline& Pipeline::build() {
  createDescriptorSetLayouts();
  createPipeline();
  return mPipeline.get();
}

vk::UniqueShaderModule Pipeline::createShaderModule(const std::string& fileName) {
  auto shaderCode = Util::readFile(fileName);
  // Note that data passed to info is as uint32_t*, so must be 4-byte aligned
  // According to tutorial std::vector already satisfies worst case alignment needs
  // TODO: But we should probably double check the alignment here just incase
  auto info = vk::ShaderModuleCreateInfo()
      .setFlags({})
      .setCodeSize(shaderCode.size())
      .setPCode(reinterpret_cast<const uint32_t*>(shaderCode.data()))
      ;
  return mDeviceInstance.device().createShaderModuleUnique(info);
}

void Pipeline::addDescriptorSetLayoutBinding( uint32_t layoutIndex, uint32_t binding, vk::DescriptorType type, uint32_t count, vk::ShaderStageFlags stageFlags) {
  auto dslBinding = vk::DescriptorSetLayoutBinding()
      .setBinding(binding)
      .setDescriptorType(type)
      .setDescriptorCount(count)
      .setStageFlags(stageFlags);
  if( mDescriptorSetLayoutBindings.size() <= layoutIndex ) mDescriptorSetLayoutBindings.resize(layoutIndex + 1);
  mDescriptorSetLayoutBindings[layoutIndex].emplace_back(dslBinding);
}

void Pipeline::createDescriptorSetLayouts() {
  for( auto& dslB : mDescriptorSetLayoutBindings ) {
    auto createInfo = vk::DescriptorSetLayoutCreateInfo()
        .setBindingCount(dslB.size())
        .setPBindings(dslB.data());
    mDescriptorSetLayouts.emplace_back(mDeviceInstance.device().createDescriptorSetLayoutUnique(createInfo));
  }
}

std::vector<vk::PipelineShaderStageCreateInfo> Pipeline::createShaderStageInfo() {
  std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
  for( auto& s : mShaders ) {
    auto specConstants = mSpecialisationConstants.find(s.first);
    auto info = vk::PipelineShaderStageCreateInfo()
        .setFlags({})
        .setStage(s.first)
        .setModule(s.second.get())
        .setPName("main")
        .setPSpecializationInfo(specConstants == mSpecialisationConstants.end() ? nullptr : &(specConstants->second));
    shaderStages.emplace_back(info);
  }
  return shaderStages;
}

/*
 * Provided under the BSD 3-Clause License, see LICENSE.
 *
 * Copyright (c) 2019, Gareth Francis
 * All rights reserved.
 */

#ifndef PIPELINE_H
#define PIPELINE_H

#include <vulkan/vulkan.hpp>

#include <vector>
#include <map>

class DeviceInstance;

/**
 * Base class for pipelines, holds common data between various types
 */
class Pipeline
{
public:
  Pipeline(DeviceInstance& deviceInstance);
  virtual ~Pipeline(){}

  /**
   * Initialisation
   *
   * Pipeline will be built using the current state of the class paramters, ensure
   * all paremeters are correct before calling this method
   *
   * TODO: At first this method can only be called once
   */
  vk::Pipeline& build();

  /**
   * Load a shader from file and build a shader module
   *
   * The caller is responsible for management of the returned module
   * It must not be deleted until all pipelines which reference the shader
   * have been initialised.
   */
  vk::UniqueShaderModule createShaderModule(const std::string& fileName);

  std::map<vk::ShaderStageFlagBits, vk::UniqueShaderModule>& shaders() { return mShaders; }

  vk::Pipeline& pipeline() { return mPipeline.get(); }
  vk::PipelineLayout& pipelineLayout() { return mPipelineLayout.get(); }

  /// Specialisation constants
  std::map<vk::ShaderStageFlagBits, vk::SpecializationInfo>& specialisationConstants() { return mSpecialisationConstants; }

  /**
   * Descriptor set layout bindings
   *
   * @param layoutIndex The index of the descriptor set layout
   * @param binding Binding index of the descriptor set (recommend contiguous, not required)
   * @param type Type of descriptor (sampler, uniform buffer, etc)
   * @param count Number of descriptors
   * @param stageFlags Shader stage flags (likely vk::ShaderStageFlagBits::eCompute)
   */
  void addDescriptorSetLayoutBinding( uint32_t layoutIndex, uint32_t binding, vk::DescriptorType type, uint32_t count, vk::ShaderStageFlags stageFlags);
  const std::vector<vk::UniqueDescriptorSetLayout>& descriptorSetLayouts() const { return mDescriptorSetLayouts; }

  /// Push Constants
  std::vector<vk::PushConstantRange>& pushConstants() { return mPushConstants; }

protected:
  virtual void createPipeline() = 0;
  void createDescriptorSetLayouts();
  std::vector<vk::PipelineShaderStageCreateInfo> createShaderStageInfo();

  DeviceInstance& mDeviceInstance;
  std::map<vk::ShaderStageFlagBits, vk::UniqueShaderModule> mShaders;
  std::map<vk::ShaderStageFlagBits, vk::SpecializationInfo> mSpecialisationConstants;
  /// Layout index, binding info
  std::vector<std::vector<vk::DescriptorSetLayoutBinding>> mDescriptorSetLayoutBindings;
  std::vector<vk::UniqueDescriptorSetLayout> mDescriptorSetLayouts;

  std::vector<vk::PushConstantRange> mPushConstants;

  vk::UniquePipelineLayout mPipelineLayout;
  vk::UniquePipeline mPipeline;
};

#endif

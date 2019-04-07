/*
 * Provided under the BSD 3-Clause License, see LICENSE.
 *
 * Copyright (c) 2019, Gareth Francis
 * All rights reserved.
 */

#ifndef COMPUTEPIPELINE_H
#define COMPUTEPIPELINE_H

#include <vulkan/vulkan.hpp>

#include "pipeline.h"

#include <vector>

class DeviceInstance;

/**
 * Class to setup/manage a compute pipeline
 */
class ComputePipeline : public Pipeline
{
public:
  ComputePipeline(DeviceInstance& deviceInstance);
  virtual ~ComputePipeline() final override {}

private:
  void createPipeline() final override;
  void createDescriptorSetLayouts();
};

#endif

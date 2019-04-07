/*
 * Provided under the BSD 3-Clause License, see LICENSE.
 *
 * Copyright (c) 2019, Gareth Francis
 * All rights reserved.
 */

#ifndef SIMPLEBUFFER_H
#define SIMPLEBUFFER_H

#include <vulkan/vulkan.hpp>

class DeviceInstance;

/**
 * A very simple class for managing buffers
 * - One buffer for each piece of data
 * - Absolutely nothing else
 *
 * This really won't scale as we'll hit the allocation limit
 * at some point
 * But for now let's just roll with it as a starting point
 */
class SimpleBuffer
{
public:
  /**
   * Allocate a buffer
   * Memory will be immediately allocated and bound to the buffer
   * Each buffer will have a separate memory allocation
   */
  SimpleBuffer(
      DeviceInstance& deviceInstance,
      vk::DeviceSize size,
      vk::BufferUsageFlags usageFlags,
      vk::MemoryPropertyFlags memFlags = {vk::MemoryPropertyFlagBits::eHostVisible});
  ~SimpleBuffer();

  void* map();
  void unmap();
  void flush();

  vk::Buffer& buffer();
  vk::DeviceSize size() const { return mSize; }

private:
  SimpleBuffer() = delete;

  DeviceInstance& mDeviceInstance;
  vk::UniqueBuffer mBuffer;
  vk::UniqueDeviceMemory mDeviceMemory;
  vk::DeviceSize mSize;
  vk::BufferUsageFlags mBufferUsageFlags;
  vk::MemoryPropertyFlags mMemoryPropertyFlags;

  bool mMapped = false;
};

inline vk::Buffer& SimpleBuffer::buffer() { return mBuffer.get(); }

#endif

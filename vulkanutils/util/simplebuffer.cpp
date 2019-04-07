/*
 * Provided under the BSD 3-Clause License, see LICENSE.
 *
 * Copyright (c) 2019, Gareth Francis
 * All rights reserved.
 */

#include "simplebuffer.h"

#include "deviceinstance.h"

SimpleBuffer::SimpleBuffer(
    DeviceInstance& deviceInstance,
    vk::DeviceSize size,
    vk::BufferUsageFlags usageFlags,
    vk::MemoryPropertyFlags memFlags)
  : mDeviceInstance(deviceInstance)
  , mSize(size)
  , mBufferUsageFlags(usageFlags)
  , mMemoryPropertyFlags(memFlags)
{
  mBuffer =  mDeviceInstance.createBuffer(mSize, mBufferUsageFlags);
  mDeviceMemory =  mDeviceInstance.allocateDeviceMemoryForBuffer(mBuffer.get(), mMemoryPropertyFlags);
  mDeviceInstance.bindMemoryToBuffer(mBuffer.get(), mDeviceMemory.get(), 0);
}

SimpleBuffer::~SimpleBuffer() {
  if( mMapped) {
    flush();
    unmap();
  }
}

void* SimpleBuffer::map() {
  if( mMapped ) return nullptr;
  auto res = mDeviceInstance.mapMemory(mDeviceMemory.get(), 0, VK_WHOLE_SIZE);
  mMapped = true;
  return res;
}

void SimpleBuffer::unmap() {
  if( !mMapped ) return;
   mDeviceInstance.unmapMemory(mDeviceMemory.get());
   mMapped = false;
}

void SimpleBuffer::flush() {
   mDeviceInstance.flushMemoryRanges({vk::MappedMemoryRange(mDeviceMemory.get(), 0, VK_WHOLE_SIZE)});
}

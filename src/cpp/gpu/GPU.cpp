/*
 * Copyright (c) 2024 pongasoft
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 *
 * @author Yan Pujante
 */

#include "GPU.h"
#include "../Errors.h"
#include <utility>

namespace pongasoft::gpu {

//------------------------------------------------------------------------
// wgpuErrorCallback
//------------------------------------------------------------------------
static void wgpuErrorCallback(const wgpu::Device &, const wgpu::ErrorType iErrorType, const wgpu::StringView iMessage,
                              GPU *iGPU)
{
  iGPU->onGPUError(iErrorType, iMessage);
}

//------------------------------------------------------------------------
// GPU::create
//------------------------------------------------------------------------
std::unique_ptr<GPU> GPU::create()
{
  // 1. Create the instance => enable async
  wgpu::InstanceDescriptor instanceDescriptor;
  static constexpr auto kTimedWaitAny = wgpu::InstanceFeatureName::TimedWaitAny;
  instanceDescriptor.requiredFeatureCount = 1;
  instanceDescriptor.requiredFeatures = &kTimedWaitAny;
  // 2. Initialize the device
  auto gpu = std::make_unique<GPU>(wgpu::CreateInstance(&instanceDescriptor));
  gpu->initDevice();
  return gpu;
}

//------------------------------------------------------------------------
// GPU::GPU
//------------------------------------------------------------------------
GPU::GPU(wgpu::Instance iInstance) : fInstance{std::move(iInstance)}
{
  // Should have been checked before but making sure
  WST_INTERNAL_ASSERT(fInstance != nullptr);
}

//------------------------------------------------------------------------
// GPU::initDevice
//------------------------------------------------------------------------
void GPU::initDevice()
{
  wgpu::Limits limits;
  wgpu::DeviceDescriptor deviceDescriptor;
  deviceDescriptor.requiredLimits = &limits;
  deviceDescriptor.SetUncapturedErrorCallback(wgpuErrorCallback, this);
  deviceDescriptor.SetDeviceLostCallback(wgpu::CallbackMode::AllowSpontaneous,
                                         [](const wgpu::Device &, wgpu::DeviceLostReason reason,
                                            wgpu::StringView message) {
                                           printf("DeviceLost (reason=%d): %.*s\n", reason, (int) message.length,
                                                  message.data);
                                         });

  wgpu::RequestAdapterWebXROptions xrOptions = {};
  wgpu::RequestAdapterOptions options = {};
  options.nextInChain = &xrOptions;

  wgpu::Adapter adapter;
  wgpu::Future f1 = fInstance.RequestAdapter(&options, wgpu::CallbackMode::WaitAnyOnly,
                                             [&adapter](wgpu::RequestAdapterStatus status, wgpu::Adapter ad,
                                                        wgpu::StringView message) {
                                               if(message.length)
                                               {
                                                 printf("RequestAdapter: %.*s\n", (int) message.length, message.data);
                                               }
                                               WST_INTERNAL_ASSERT(status == wgpu::RequestAdapterStatus::Success);
                                               adapter = std::move(ad);
                                             });
  wait(f1);
  WST_INTERNAL_ASSERT(adapter != nullptr, "Cannot create Adapter");
  fAdapter = std::move(adapter);

  wgpu::Device device;
  auto f2 = fAdapter.RequestDevice(&deviceDescriptor, wgpu::CallbackMode::WaitAnyOnly,
                                   [&device](wgpu::RequestDeviceStatus status, wgpu::Device dev,
                                             wgpu::StringView message) {
                                     if(message.length)
                                     {
                                       printf("RequestDevice: %.*s\n", (int) message.length, message.data);
                                     }
                                     WST_INTERNAL_ASSERT(status == wgpu::RequestDeviceStatus::Success);
                                     device = std::move(dev);
                                   });
  wait(f2);
  WST_INTERNAL_ASSERT(device != nullptr, "Cannot create device");
  fDevice = std::move(device);
}

//------------------------------------------------------------------------
// GPU::beginFrame
//------------------------------------------------------------------------
void GPU::beginFrame()
{
  fCommandEncoder = fDevice.CreateCommandEncoder();
}

//------------------------------------------------------------------------
// GPU::endFrame
//------------------------------------------------------------------------
void GPU::endFrame()
{
  WST_INTERNAL_ASSERT(fCommandEncoder != nullptr, "GPU::beginFrame has not been called");
  wgpu::CommandBuffer commands = fCommandEncoder.Finish();
  fDevice.GetQueue().Submit(1, &commands);
  fCommandEncoder = nullptr;
}

//------------------------------------------------------------------------
// GPU::renderPass
//------------------------------------------------------------------------
void GPU::renderPass(wgpu::Color const &iColor,
                     render_pass_fn_t const &iRenderPassFn,
                     wgpu::TextureView const &iTextureView)
{
  WST_INTERNAL_ASSERT(fCommandEncoder != nullptr, "GPU::beginFrame has not been called");

  if(iTextureView == nullptr)
    return;

  wgpu::RenderPassColorAttachment attachment{
    .view = iTextureView,
    .loadOp = wgpu::LoadOp::Clear,
    .storeOp = wgpu::StoreOp::Store,
    .clearValue = iColor
  };

  wgpu::RenderPassDescriptor renderpass{
    .colorAttachmentCount = 1,
    .colorAttachments = &attachment,
  };

  wgpu::RenderPassEncoder pass = fCommandEncoder.BeginRenderPass(&renderpass);
  iRenderPassFn(pass);
  pass.End();
}

//------------------------------------------------------------------------
// GPU::onGPUError
//------------------------------------------------------------------------
void GPU::onGPUError(wgpu::ErrorType iErrorType, wgpu::StringView iMessage)
{
  fError = {iErrorType, std::string(iMessage)};
  // can dynamically be changed (for example, when parsing new shader code)
  printf("[WebGPU] %s error | %.*s\n", errorTypeAsString(iErrorType), static_cast<int>(iMessage.length), iMessage.data);
}

}
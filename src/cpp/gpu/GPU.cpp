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
// GPU::asyncCreate
//------------------------------------------------------------------------
void GPU::asyncCreate(std::function<void(std::shared_ptr<GPU> iGPU)> onCreated,
                      std::function<void(wgpu::StringView)> onError)
{
  auto gpu = std::make_shared<GPU>(wgpu::CreateInstance());
  gpu->asyncInitDevice([gpu, onCreated = std::move(onCreated)] {
    onCreated(gpu);
  }, std::move(onError));
}

//------------------------------------------------------------------------
// GPU::GPU
//------------------------------------------------------------------------
GPU::GPU(wgpu::Instance iInstance) : fInstance{std::move(iInstance)}
{
  WST_INTERNAL_ASSERT(fInstance != nullptr);
}

//------------------------------------------------------------------------
// GPU::asyncInitDevice
//------------------------------------------------------------------------
void GPU::asyncInitDevice(std::function<void()> const &onDeviceInitialized,
                          std::function<void(wgpu::StringView)> const &onError)
{
  wgpu::RequestAdapterWebXROptions xrOptions = {};
  wgpu::RequestAdapterOptions options = {};
  options.nextInChain = &xrOptions;

  fInstance.RequestAdapter(&options, wgpu::CallbackMode::AllowSpontaneous,
                           [this, onDeviceInitialized, onError](wgpu::RequestAdapterStatus status,
                                                                wgpu::Adapter ad,
                                                                wgpu::StringView message) {
                             if(status != wgpu::RequestAdapterStatus::Success)
                             {
                               onError(message);
                               return;
                             }
                             fAdapter = std::move(ad);

                             wgpu::Limits limits;
                             wgpu::DeviceDescriptor deviceDescriptor;
                             deviceDescriptor.requiredLimits = &limits;
                             deviceDescriptor.SetUncapturedErrorCallback([](const wgpu::Device &, // unused
                                                                            const wgpu::ErrorType iErrorType,
                                                                            const wgpu::StringView iMessage,
                                                                            GPU *iGPU) {
                                                                           iGPU->onGPUError(iErrorType, iMessage);
                                                                         },
                                                                         this);
                             deviceDescriptor.SetDeviceLostCallback(wgpu::CallbackMode::AllowProcessEvents,
                                                                    [](const wgpu::Device &,
                                                                       wgpu::DeviceLostReason reason,
                                                                       wgpu::StringView message) {
                                                                      printf("DeviceLost (reason=%d): %.*s\n", reason, (int) message.length,
                                                                             message.data);
                                                                    });

                             wgpu::Device device;
                             fAdapter.RequestDevice(&deviceDescriptor, wgpu::CallbackMode::AllowSpontaneous,
                                                    [this, onDeviceInitialized, onError](wgpu::RequestDeviceStatus status,
                                                                                         wgpu::Device dev,
                                                                                         wgpu::StringView message) {
                                                      if(status != wgpu::RequestDeviceStatus::Success)
                                                      {
                                                        onError(message);
                                                        return;
                                                      }
                                                      fDevice = std::move(dev);
                                                      onDeviceInitialized();
                                                    });
                           });
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
// GPU::pollEvents
//------------------------------------------------------------------------
void GPU::pollEvents()
{
  fInstance.ProcessEvents();
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
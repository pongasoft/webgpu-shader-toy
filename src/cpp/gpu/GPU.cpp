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

#include <emscripten/html5_webgpu.h>
#include "GPU.h"
#include "../Errors.h"
#include <utility>

namespace pongasoft::gpu {

//------------------------------------------------------------------------
// wgpuErrorCallback
//------------------------------------------------------------------------
static void wgpuErrorCallback(WGPUErrorType iErrorType, const char *iMessage, void *iUserData)
{
  auto gpu = reinterpret_cast<GPU *>(iUserData);
  gpu->onGPUError(static_cast<wgpu::ErrorType>(iErrorType), iMessage);
}

//------------------------------------------------------------------------
// GPU::create
//------------------------------------------------------------------------
std::unique_ptr<GPU> GPU::create()
{
  wgpu::Device device = emscripten_webgpu_get_device();
  ASSERT(device != nullptr, "Device not initialized");
  return std::unique_ptr<GPU>(new GPU(std::move(device)));
}

//------------------------------------------------------------------------
// GPU::GPU
//------------------------------------------------------------------------
GPU::GPU(wgpu::Device iDevice) :
  fInstance{wgpu::CreateInstance()},
  fDevice{std::move(iDevice)}
{
  fDevice.SetUncapturedErrorCallback(wgpuErrorCallback, this);
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
  ASSERT(fCommandEncoder != nullptr, "GPU::beginFrame has not been called");
  wgpu::CommandBuffer commands = fCommandEncoder.Finish();
  fDevice.GetQueue().Submit(1, &commands);
  fCommandEncoder = nullptr;
}

//------------------------------------------------------------------------
// GPU::renderPass
//------------------------------------------------------------------------
void GPU::renderPass(wgpu::Color const &iColor,
                     GPU::render_pass_fn_t const &iRenderPassFn,
                     wgpu::TextureView const &iTextureView)
{
  ASSERT(fCommandEncoder != nullptr, "GPU::beginFrame has not been called");
  ASSERT(iTextureView != nullptr);

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
void GPU::onGPUError(wgpu::ErrorType iErrorType, char const *iMessage)
{
  fError = {iErrorType, iMessage};
  // can dynamically be changed (for example when parsing new shader code)
  printf("[WebGPU] %s error | %s\n", errorTypeAsString(iErrorType), iMessage);
}

}
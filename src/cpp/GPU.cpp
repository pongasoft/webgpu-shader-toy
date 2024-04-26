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
#include <emscripten/emscripten.h>
#include "GPU.h"
#include "Errors.h"
#include <cstdio>
#include <utility>

namespace shader_toy {

//------------------------------------------------------------------------
// wgpuErrorCallback
//------------------------------------------------------------------------
static void wgpuErrorCallback(WGPUErrorType iErrorType, const char *iMessage, void *)
{
  const char *error_type_lbl;
  switch(iErrorType)
  {
    case WGPUErrorType_Validation:
      error_type_lbl = "Validation";
      break;
    case WGPUErrorType_OutOfMemory:
      error_type_lbl = "Out of memory";
      break;
    case WGPUErrorType_DeviceLost:
      error_type_lbl = "Device lost";
      break;
    case WGPUErrorType_Unknown:
    default:
      error_type_lbl = "Unknown";
      break;
  }
  printf("[WebGPU] %s error | %s\n", error_type_lbl, iMessage);
}

constexpr float computeGamma(wgpu::TextureFormat iTextureFormat)
{
  float gamma;
  switch (iTextureFormat)
  {
    case wgpu::TextureFormat::ASTC10x10UnormSrgb:
    case wgpu::TextureFormat::ASTC10x5UnormSrgb:
    case wgpu::TextureFormat::ASTC10x6UnormSrgb:
    case wgpu::TextureFormat::ASTC10x8UnormSrgb:
    case wgpu::TextureFormat::ASTC12x10UnormSrgb:
    case wgpu::TextureFormat::ASTC12x12UnormSrgb:
    case wgpu::TextureFormat::ASTC4x4UnormSrgb:
    case wgpu::TextureFormat::ASTC5x5UnormSrgb:
    case wgpu::TextureFormat::ASTC6x5UnormSrgb:
    case wgpu::TextureFormat::ASTC6x6UnormSrgb:
    case wgpu::TextureFormat::ASTC8x5UnormSrgb:
    case wgpu::TextureFormat::ASTC8x6UnormSrgb:
    case wgpu::TextureFormat::ASTC8x8UnormSrgb:
    case wgpu::TextureFormat::BC1RGBAUnormSrgb:
    case wgpu::TextureFormat::BC2RGBAUnormSrgb:
    case wgpu::TextureFormat::BC3RGBAUnormSrgb:
    case wgpu::TextureFormat::BC7RGBAUnormSrgb:
    case wgpu::TextureFormat::BGRA8UnormSrgb:
    case wgpu::TextureFormat::ETC2RGB8A1UnormSrgb:
    case wgpu::TextureFormat::ETC2RGB8UnormSrgb:
    case wgpu::TextureFormat::ETC2RGBA8UnormSrgb:
    case wgpu::TextureFormat::RGBA8UnormSrgb:
      gamma = 2.2f;
      break;
    default:
      gamma = 1.0f;
  }
  return gamma;
}

//------------------------------------------------------------------------
// GPU::create
//------------------------------------------------------------------------
std::unique_ptr<GPU> GPU::create()
{
  wgpu::Device device = emscripten_webgpu_get_device();
  SHADER_TOY_ASSERT(device != nullptr, "Device not initialized");
  return std::unique_ptr<GPU>(new GPU(std::move(device)));
}

//------------------------------------------------------------------------
// GPU::GPU
//------------------------------------------------------------------------
GPU::GPU(wgpu::Device iDevice) :
  fInstance{wgpu::CreateInstance()},
  fDevice{std::move(iDevice)}
{
  fDevice.SetUncapturedErrorCallback(wgpuErrorCallback, nullptr);
}

//------------------------------------------------------------------------
// GPU::createSurface
//------------------------------------------------------------------------
void GPU::createSurface([[maybe_unused]] GLFWwindow *iWindow, std::string_view iImGuiCanvasSelector)
{
  // Implementation note: there is no glfw call to do that (maybe in the future)
  // so it is highly dependent on the platform... Implementing emscripten only for now

  // for Dawn it would be: wgpu::glfw::CreateSurfaceForWindow(fInstance, iWindow);

  wgpu::SurfaceDescriptorFromCanvasHTMLSelector html_surface_desc = {};
  html_surface_desc.selector = iImGuiCanvasSelector.data();

  wgpu::SurfaceDescriptor surface_desc = { .nextInChain = &html_surface_desc };

  fSurface = fInstance.CreateSurface(&surface_desc);
  SHADER_TOY_ASSERT(fSurface != nullptr, "Cannot create surface");
  fPreferredFormat = fSurface.GetPreferredFormat(nullptr);
  fGamma = computeGamma(fPreferredFormat);
}

//------------------------------------------------------------------------
// GPU::createSwapChain
//------------------------------------------------------------------------
void GPU::createSwapChain(int iWidth, int iHeight)
{
  wgpu::SwapChainDescriptor scDesc{
    .usage = wgpu::TextureUsage::RenderAttachment,
    .format = fPreferredFormat,
    .width = static_cast<uint32_t>(iWidth),
    .height = static_cast<uint32_t>(iHeight),
    .presentMode = wgpu::PresentMode::Fifo};
  fSwapChain = fDevice.CreateSwapChain(fSurface, &scDesc);
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
  SHADER_TOY_ASSERT(fCommandEncoder != nullptr, "GPU::beginFrame has not been called");
  wgpu::CommandBuffer commands = fCommandEncoder.Finish();
  fDevice.GetQueue().Submit(1, &commands);
  fCommandEncoder = nullptr;

#ifndef __EMSCRIPTEN__
  fSwapChain.Present();
  fInstance.ProcessEvents();
#endif

}

//------------------------------------------------------------------------
// GPU::renderPass
//------------------------------------------------------------------------
void GPU::renderPass(wgpu::Color const &iColor,
                     GPU::render_pass_fn_t const &iRenderPassFn,
                     wgpu::TextureView const &iTextureView)
{
  SHADER_TOY_ASSERT(fCommandEncoder != nullptr, "GPU::beginFrame has not been called");
  SHADER_TOY_ASSERT(iTextureView != nullptr || fSwapChain != nullptr);

  wgpu::RenderPassColorAttachment attachment{
    .view = iTextureView != nullptr ? iTextureView : fSwapChain.GetCurrentTextureView(),
    .loadOp = wgpu::LoadOp::Clear,
    .storeOp = wgpu::StoreOp::Store,
    .clearValue = wgpu::Color{gammaCorrect(iColor.r), gammaCorrect(iColor.g), gammaCorrect(iColor.b), iColor.a }
  };

  wgpu::RenderPassDescriptor renderpass{
    .colorAttachmentCount = 1,
    .colorAttachments = &attachment,
  };

  wgpu::RenderPassEncoder pass = fCommandEncoder.BeginRenderPass(&renderpass);
  iRenderPassFn(pass);
  pass.End();
}

}
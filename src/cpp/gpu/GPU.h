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

#ifndef WGPU_SHADER_TOY_GPU_H
#define WGPU_SHADER_TOY_GPU_H

#include <webgpu/webgpu_cpp.h>
#include <memory>
#include <functional>

struct GLFWwindow;

namespace pongasoft::gpu {

class GPU
{
public:
  using render_pass_fn_t = std::function<void(wgpu::RenderPassEncoder &)>;

public:
  static std::unique_ptr<GPU> create();

  void createSurface(GLFWwindow *iWindow, std::string_view iImGuiCanvasSelector);
  void createSwapChain(int iWidth, int iHeight);

  wgpu::Device getDevice() const { return fDevice; }
  wgpu::Instance getInstance() const { return fInstance; }

  void beginFrame();
  void renderPass(wgpu::Color const &iColor, render_pass_fn_t const &iRenderPassFn, wgpu::TextureView const &iTextureView = nullptr);
  void endFrame();

  constexpr static float computeGamma(wgpu::TextureFormat iTextureFormat)
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

public: // should be private but called from C
  void onGPUError(wgpu::ErrorType iErrorType, const char *iMessage);

private:
  explicit GPU(wgpu::Device iDevice);

private:
  wgpu::Instance fInstance;
  wgpu::Device fDevice;
  wgpu::CommandEncoder fCommandEncoder{};
};

}

#endif //WGPU_SHADER_TOY_GPU_H
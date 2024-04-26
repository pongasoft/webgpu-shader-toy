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

namespace shader_toy {

class GPU
{
public:
  using render_pass_fn_t = std::function<void(wgpu::RenderPassEncoder &)>;

public:
  static std::unique_ptr<GPU> create();

  void createSurface(GLFWwindow *iWindow, std::string_view iImGuiCanvasSelector);
  void createSwapChain(int iWidth, int iHeight);

  wgpu::TextureFormat getPreferredFormat() const { return fPreferredFormat; }
  wgpu::Device getDevice() const { return fDevice; }

  void beginFrame();
  void renderPass(wgpu::Color const &iColor, render_pass_fn_t const &iRenderPassFn, wgpu::TextureView const &iTextureView = nullptr);
  void endFrame();

private:
  explicit GPU(wgpu::Device iDevice);

  inline double gammaCorrect(double f) const {
    if(fGamma == 1.0)
      return f;
    else
      return std::pow(f, fGamma);
  }


private:
  wgpu::Instance fInstance;
  wgpu::Device fDevice;

  wgpu::Surface fSurface{};
  wgpu::SwapChain fSwapChain{};

  wgpu::CommandEncoder fCommandEncoder{};

  wgpu::TextureFormat fPreferredFormat{wgpu::TextureFormat::Undefined};
  float fGamma{1.0};
};

}

#endif //WGPU_SHADER_TOY_GPU_H
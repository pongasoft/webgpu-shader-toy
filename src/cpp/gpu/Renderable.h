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

#ifndef WGPU_SHADER_TOY_GPU_RENDERABLE_H
#define WGPU_SHADER_TOY_GPU_RENDERABLE_H

#include "GPU.h"

namespace pongasoft::gpu {

class Renderable
{
public:
  using render_fn_t = std::function<void()>;

public:
  struct Size
  {
    int width{};
    int height{};
  };
public:
  explicit Renderable(std::shared_ptr<GPU> iGPU) : fGPU{std::move(iGPU)} {}
  virtual ~Renderable() = default;

  virtual void beforeFrame() {}
  virtual void afterFrame() {}
  virtual bool running() const { return true; }

  virtual void render() {
    fGPU->renderPass(fClearColor, [this](wgpu::RenderPassEncoder &renderPass) {
      doRender(renderPass);
    }, getTextureView());
  }

  wgpu::Color const &getClearColor() const { return fClearColor;}
  void setClearColor(wgpu::Color const &iClearColor) { fClearColor = gammaCorrect(iClearColor); }

protected:
  virtual void doRender(wgpu::RenderPassEncoder &iRenderPass) = 0;

  void initPreferredFormat(wgpu::TextureFormat iPreferredFormat)
  {
    fPreferredFormat = iPreferredFormat;
    fGamma = GPU::computeGamma(fPreferredFormat);
  }

  virtual wgpu::TextureView getTextureView() const = 0;

  inline double gammaCorrect(double f) const {
    if(fGamma == 1.0)
      return f;
    else
      return std::pow(f, fGamma);
  }

  inline wgpu::Color gammaCorrect(wgpu::Color const &iColor) const {
    return wgpu::Color{gammaCorrect(iColor.r), gammaCorrect(iColor.g), gammaCorrect(iColor.b), iColor.a };
  }

protected:
  std::shared_ptr<GPU> fGPU;
  wgpu::Color fClearColor{};
  wgpu::TextureFormat fPreferredFormat{wgpu::TextureFormat::Undefined};
  float fGamma{1.0};
};

template<typename T>
concept IsRenderable = std::is_base_of_v<Renderable, T> && !std::is_same_v<Renderable, T>;

}

#endif //WGPU_SHADER_TOY_GPU_RENDERABLE_H

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

#ifndef WGPU_SHADER_TOY_FRAGMENT_SHADER_WINDOW_H
#define WGPU_SHADER_TOY_FRAGMENT_SHADER_WINDOW_H

#include <imgui.h>
#include "gpu/Window.h"
#include "Preferences.h"
#include "FragmentShader.h"

using namespace pongasoft;

namespace pongasoft::gpu {
using vec2f = ImVec2;
using f32 = float;
using i32 = int;
}

namespace shader_toy {

class FragmentShaderWindow : public gpu::Window, public std::enable_shared_from_this<FragmentShaderWindow>
{
public:
  static constexpr auto kPreferencesSizeKey = "shader_toy::FragmentShaderWindow::Size";

public:
  struct Args
  {
    std::shared_ptr<Preferences> preferences;
  };

public:
  FragmentShaderWindow(std::shared_ptr<gpu::GPU> iGPU, Window::Args const &iWindowArgs, Args const &iMainWindowArgs);
  ~FragmentShaderWindow() override;

  void beforeFrame() override;

  void compile(std::shared_ptr<FragmentShader> const &iFragmentShader);
  void setCurrentFragmentShader(std::shared_ptr<FragmentShader> iFragmentShader);

protected:
  void doRender(wgpu::RenderPassEncoder &iRenderPass) override;

public: // should be private (but used in callback...)
  void doHandleFrameBufferSizeChange(Size const &iSize) override;
  void onMousePosChange(double xpos, double ypos);
  void onShaderCompilationResult(std::shared_ptr<FragmentShader> const &iFragmentShader,
                                 wgpu::ShaderModule iShaderModule,
                                 wgpu::CompilationInfoRequestStatus iStatus,
                                 WGPUCompilationInfo const *iCompilationInfo);

private:
  void initGPU();
  void initFragmentShader(std::shared_ptr<FragmentShader> const &iFragmentShader) const;

private:
  std::shared_ptr<Preferences> fPreferences;
  Renderable::Size fFrameBufferSize;

  // Common gpu part
  wgpu::BindGroupLayout fGroup0BindGroupLayout{};
  wgpu::BindGroup fGroup0BindGroup{};
  wgpu::Buffer fShaderToyInputsBuffer{};
  wgpu::ShaderModule fVertexShaderModule{};

  std::shared_ptr<FragmentShader> fCurrentFragmentShader{};
};

}

#endif //WGPU_SHADER_TOY_FRAGMENT_SHADER_WINDOW_H
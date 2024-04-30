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
#include "Model.h"
#include "Preferences.h"

using namespace pongasoft;

namespace pongasoft::gpu {
using vec2f = ImVec2;
using f32 = float;
using i32 = int;
}

namespace shader_toy {

class FragmentShaderWindow : public gpu::Window
{
public:
  static constexpr auto kPreferencesSizeKey = "shader_toy::FragmentShaderWindow::Size";

public:
  struct Args
  {
    std::shared_ptr<Model> model;
    std::shared_ptr<Preferences> preferences;
  };

public:
  FragmentShaderWindow(std::shared_ptr<gpu::GPU> iGPU, Window::Args const &iWindowArgs, Args const &iMainWindowArgs);

  void beforeFrame() override;

protected:
  void doRender(wgpu::RenderPassEncoder &iRenderPass) override;

public: // should be private (but used in callback...)
  void doHandleFrameBufferSizeChange(Size const &iSize) override;
  void onMousePosChange(double xpos, double ypos);
  void createRenderPipeline(wgpu::CompilationInfoRequestStatus iStatus, WGPUCompilationInfo const *iCompilationInfo);

private:
  void createRenderPipeline();
  void resetTime();

private:
  struct ShaderToyInputs
  {
    gpu::vec2f size{};
    gpu::f32 time{};
    gpu::i32 frame{};
    gpu::vec2f mouse{};
  };

private:
  std::shared_ptr<Model> fModel;
  std::shared_ptr<Preferences> fPreferences;
  int fFrameCount{};
  wgpu::ShaderModule fFragmentShaderModule{};

  wgpu::RenderPipeline fRenderPipeline{};

  wgpu::BindGroup fGroup0BindGroup{};

  std::string fCurrentFragmentShader{};
  ShaderToyInputs fShaderToyInputs{};
  wgpu::Buffer fShaderToyInputsBuffer{};

};

}

#endif //WGPU_SHADER_TOY_FRAGMENT_SHADER_WINDOW_H
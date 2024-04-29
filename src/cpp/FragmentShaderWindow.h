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

using namespace pongasoft::gpu;

namespace shader_toy {

class FragmentShaderWindow : public Window
{
public:
  FragmentShaderWindow(std::shared_ptr<GPU> iGPU, Args const &iArgs, std::shared_ptr<Model> iModel);

protected:
  void doRender(wgpu::RenderPassEncoder &iRenderPass) override;

public: // should be private (but used in callback...)
  void doHandleFrameBufferSizeChange(Size const &iSize) override;
  void onMousePosChange(double xpos, double ypos);
  void createRenderPipeline(wgpu::CompilationInfoRequestStatus iStatus, WGPUCompilationInfo const *iCompilationInfo);

private:
  void createRenderPipeline();

private:
  struct ShaderToyInputs
  {
    ImVec2 size;
    ImVec2 mouse;
  };

private:
  std::shared_ptr<Model> fModel;
  std::string fFragmentShader;
  wgpu::ShaderModule fFragmentShaderModule{};

  wgpu::RenderPipeline fRenderPipeline{};

  wgpu::BindGroup fGroup0BindGroup{};

  ShaderToyInputs fShaderToyInputs{};
  wgpu::Buffer fShaderToyInputsBuffer{};

};

}

#endif //WGPU_SHADER_TOY_FRAGMENT_SHADER_WINDOW_H
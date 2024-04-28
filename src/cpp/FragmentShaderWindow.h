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

#include "gpu/Window.h"

using namespace pongasoft::gpu;

namespace shader_toy {

class FragmentShaderWindow : public Window
{
public:
  FragmentShaderWindow(std::shared_ptr<GPU> iGPU,
                       Size const &iSize,
                       char const *title,
                       char const *iCanvasSelector,
                       char const *iCanvasResizeSelector = nullptr,
                       char const *iHandleSelector = nullptr);

protected:
  void doRender(wgpu::RenderPassEncoder &iRenderPass) override;

private:
  void createRenderPipeline();

private:
  wgpu::RenderPipeline fRenderPipeline{};
};

}

#endif //WGPU_SHADER_TOY_FRAGMENT_SHADER_WINDOW_H
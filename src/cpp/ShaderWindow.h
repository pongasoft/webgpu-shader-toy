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

#ifndef WGPU_SHADER_TOY_SHADER_WINDOW_H
#define WGPU_SHADER_TOY_SHADER_WINDOW_H

#include <imgui.h>
#include "GPU.h"

namespace shader_toy {

class ShaderWindow
{
public:
  explicit ShaderWindow(ImVec2 const &iSize) : fSize{iSize} {}

  void init(GPU &iGPU);
  void render(GPU &iGPU);

  inline ImTextureID getTextureView() const { return reinterpret_cast<ImTextureID>(fTextureView.Get()); }
  constexpr ImVec2 const &getSize() const { return fSize; }

private:
  void createTexture(GPU &iGPU, ImVec2 const &iSize);
  void createRenderPipeline(GPU &iGPU);

private:
  ImVec2 fSize;

  wgpu::Texture fTexture{};
  wgpu::TextureView fTextureView{};
  wgpu::RenderPipeline fRenderPipeline{};
};

}

#endif //WGPU_SHADER_TOY_SHADER_WINDOW_H
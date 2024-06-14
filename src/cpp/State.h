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

#ifndef WGPU_SHADER_TOY_STATE_H
#define WGPU_SHADER_TOY_STATE_H

#include "gpu/Renderable.h"
#include <string>
#include <vector>
#include <optional>
#include <imgui.h>

namespace shader_toy {

using namespace pongasoft;

struct Shader
{
  std::string fName;
  std::string fCode;
};

struct State
{
  int fFormatVersion{1};
  gpu::Renderable::Size fMainWindowSize{};
  gpu::Renderable::Size fFragmentShaderWindowSize{};
  bool fDarkStyle{true};
  bool fHiDPIAware{true};
  std::string fAspectRatio{"Free"};
  std::vector<Shader> fShaders{};
  std::optional<std::string> fCurrentShader{};
  std::optional<Shader> findCurrentShader() const {
    if(fCurrentShader)
    {
      auto iter = std::find_if(fShaders.begin(), fShaders.end(), [name = *fCurrentShader](auto const &shader) { return shader.fName == name; });
      if(iter != fShaders.end())
        return *iter;
    }
    return std::nullopt;
  }
};

}

#endif //WGPU_SHADER_TOY_STATE_H

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
  std::optional<std::string> fEditedCode;
  gpu::Renderable::Size fWindowSize;
};

struct State
{
  struct Settings
  {
    gpu::Renderable::Size fMainWindowSize{};
    gpu::Renderable::Size fFragmentShaderWindowSize{};
    bool fDarkStyle{true};
    bool fHiDPIAware{true};
    bool fLayoutManual{false};
    bool fLayoutSwapped{false};
    float fFontSize{13.0f};
    float fLineSpacing{1.0f};
    bool fCodeShowWhiteSpace{false};
    std::string fScreenshotMimeType{"image/png"};
    int fScreenshotQualityPercent{85};
    std::string fProjectFilename{"WebGPUShaderToy.json"};
    bool fBrowserAutoSave{true};
  };

  struct Shaders
  {
    std::vector<Shader> fList{};
    std::optional<std::string> fCurrent{};
  };

  int fFormatVersion{1};
  Settings fSettings{};
  Shaders fShaders{};
};

}

#endif //WGPU_SHADER_TOY_STATE_H

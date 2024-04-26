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

#ifndef WGPU_SHADER_TOY_APPLICATION_H
#define WGPU_SHADER_TOY_APPLICATION_H

#include "GPU.h"
#include "ShaderWindow.h"
#include <GLFW/glfw3.h>
#include <vector>
#include <optional>

namespace shader_toy {

class Application
{
public:
  using action_t = std::function<void(Application *)>;

public:
  Application() = default;
  ~Application();

  void init(std::string_view iImGuiCanvasSelector);
  void mainLoop();
  constexpr bool running() const { return fRunning; }
  void asyncOnFramebufferSizeChange(ImVec2 const &iSize) { fNewFrameBufferSize = iSize; }

  void newFrame();

  void deferNextFrame(action_t iAction) { if(iAction) fNewFrameActions.emplace_back(std::move(iAction)); }

private:
  static GLFWwindow *initGLFW(std::string_view iImGuiCanvasSelector);
  void initWebGPU(std::string_view iImGuiCanvasSelector);
  void initImGui();
  void renderImGui(wgpu::RenderPassEncoder &renderPass);
  void onFramebufferSizeChange(ImVec2 const &iSize);

private:
  std::unique_ptr<GPU> fGPU{};
  ShaderWindow fShaderWindow{ImVec2{500, 500}};
  GLFWwindow *fImGuiWindow{};
  ImGuiContext *fImGuiContext{};
  bool fRunning{true};

  std::optional<ImVec2> fNewFrameBufferSize{};
  std::vector<action_t> fNewFrameActions{};
};

}

#endif //WGPU_SHADER_TOY_APPLICATION_H
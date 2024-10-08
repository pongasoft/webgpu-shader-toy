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

#include "gpu/GPU.h"
#include "gpu/ImGuiWindow.h"
#include "FragmentShaderWindow.h"
#include <GLFW/glfw3.h>
#include <vector>

namespace shader_toy {

using namespace pongasoft::gpu;

class Application
{
public:
  Application();
  ~Application();

  template<IsRenderable R, typename... Args>
  std::shared_ptr<R> registerRenderable(Args&&... args);

  void mainLoop();
  constexpr bool running() const { return fRunning; }

private:
  std::shared_ptr<GPU> fGPU{};
  std::vector<std::shared_ptr<Renderable>> fRenderableList{};
  bool fRunning{true};
};

//------------------------------------------------------------------------
// Application::registerRenderable
//------------------------------------------------------------------------
template<IsRenderable R, typename... Args>
std::shared_ptr<R> Application::registerRenderable(Args &&... args)
{
  std::shared_ptr<R> renderable = std::make_unique<R>(fGPU, std::forward<Args>(args)...);
  fRenderableList.emplace_back(renderable);
  return renderable;
}

}

#endif //WGPU_SHADER_TOY_APPLICATION_H
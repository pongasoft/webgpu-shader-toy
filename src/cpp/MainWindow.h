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

#ifndef WGPU_SHADER_TOY_MAIN_WINDOW_H
#define WGPU_SHADER_TOY_MAIN_WINDOW_H

#include "gpu/ImGuiWindow.h"
#include "Model.h"
#include "Preferences.h"
#include "FragmentShaderWindow.h"
#include <optional>
#include <string>
#include <map>

using namespace pongasoft::gpu;

namespace shader_toy {

class MainWindow : public ImGuiWindow
{
public:
  static constexpr auto kPreferencesSizeKey = "shader_toy::MainWindow::Size";

public:
  struct Args
  {
    Window::Args fragmentShaderWindow;
    std::shared_ptr<Preferences> preferences;
  };

public:
  MainWindow(std::shared_ptr<GPU> iGPU, Window::Args const &iWindowArgs, Args const &iMainWindowArgs);

  ~MainWindow() override;

  void beforeFrame() override;
  void render() override;

  void doHandleFrameBufferSizeChange(Size const &iSize) override;
  void onNewFragmentShader(char const *iFilename, char const *iContent);

protected:
  void doRender() override;

private:
  void deleteFragmentShader(std::string const &iName);
  void reset();

private:
  Renderable::Size fDefaultSize;
  std::shared_ptr<Preferences> fPreferences;
  std::shared_ptr<Model> fModel;

  FragmentShaderWindow fFragmentShaderWindow;
  Renderable::Size fDefaultFragmentShaderWindowSize;

  std::optional<AspectRatio> fAspectRatioRequest{};
  bool fResetRequest{};

  std::optional<std::string> fFragmentShaderFilename{};
  std::map<std::string, std::string> fFragmentShaders{};
  std::vector<std::string> fFragmentShaderTabs{};
  std::string fCurrentFragmentShaderName{};
  std::optional<std::string> fCurrentFragmentShaderNameRequest{};
};

}

#endif //WGPU_SHADER_TOY_MAIN_WINDOW_H
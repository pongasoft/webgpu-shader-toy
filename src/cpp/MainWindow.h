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
    State defaultState;
    State state;
    std::shared_ptr<Preferences> preferences;
  };

public:
  MainWindow(std::shared_ptr<GPU> iGPU, Window::Args const &iWindowArgs, Args const &iMainWindowArgs);

  ~MainWindow() override;

  void beforeFrame() override;
  void render() override;

  void afterFrame() override;

  void onNewFragmentShader(Shader const &iShader);
  void saveState();

protected:
  void doRender() override;

private:
  void deleteFragmentShader(std::string const &iName);
  void reset();
  void initFromState(State const &iState);
  void setStyle(bool iDarkStyle);
  State computeState() const;
  void doRenderMainMenuBar();
  void doRenderControlsSection();
  void doRenderShaderSection();

private:
  std::shared_ptr<Preferences> fPreferences;
  State fDefaultState;
  std::string fLastComputedState;
  double fLastComputedStateTime;
  bool fDarkStyle{true};
  float fLineSpacing{1.0f};
  bool fCodeShowWhiteSpace{false};

  std::shared_ptr<FragmentShaderWindow> fFragmentShaderWindow;

  std::string fCurrentAspectRatio{"Free"};

  std::optional<AspectRatio> fAspectRatioRequest{};
  bool fResetRequest{};

  std::map<std::string, std::shared_ptr<FragmentShader>> fFragmentShaders{};
  std::vector<std::string> fFragmentShaderTabs{};
  std::shared_ptr<FragmentShader>fCurrentFragmentShader{};
  TextEditor fTextEditor{};

  std::optional<std::string> fCurrentFragmentShaderNameRequest{};
};

}

#endif //WGPU_SHADER_TOY_MAIN_WINDOW_H
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
#include "gui/Dialog.h"
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

  void onNewFile(char const *iFilename, char const *iContent);
  void onNewFragmentShader(Shader const &iShader);
  void saveState();
  void onClipboardString(void *iRequestUserData, char const *iClipboardString);

protected:
  void doRender() override;

private:
  void onNewFragmentShader(std::shared_ptr<FragmentShader> iFragmentShader);
  std::shared_ptr<FragmentShader> deleteFragmentShader(std::string const &iName);
  void reset();
  void initFromState(State const &iState);
  void setStyle(bool iDarkStyle);
  State computeState() const;
  void renderDialog();
  void renderMainMenuBar();
  void renderShaderMenu(TextEditor iEditor, std::string const &iNewNode, bool iEdited);
  void renderSettingsMenu();
  void renderControlsSection();
  void renderShaderSection(bool iEditorHasFocus);
  void compile(std::string const &iNewCode);
  void promptNewEmtpyShader();
  void promptRenameCurrentShader();
  void promptExportShader(std::string const &iFilename, std::string const &iContent);
  void promptExportProject();
  void promptShaderWindowSize();
  void renameShader(std::string const &iOldName, std::string const &iNewName);
  inline bool hasDialog() const { return fCurrentDialog != nullptr || !fDialogs.empty(); }
  gui::Dialog &newDialog(std::string iTitle, bool iHighPriority = false);

private:
  std::shared_ptr<Preferences> fPreferences;
  State fDefaultState;
  std::string fLastComputedState;
  double fLastComputedStateTime;
  bool fDarkStyle{true};
  float fLineSpacing{1.0f};
  bool fCodeShowWhiteSpace{false};

  std::shared_ptr<FragmentShaderWindow> fFragmentShaderWindow;

//  std::string fCurrentAspectRatio{"Free"};
//  std::optional<AspectRatio> fAspectRatioRequest{};
  bool fResetRequest{};

  std::map<std::string, std::shared_ptr<FragmentShader>> fFragmentShaders{};
  std::vector<std::string> fFragmentShaderTabs{};
  std::shared_ptr<FragmentShader>fCurrentFragmentShader{};
  TextEditor fTextEditor{};

  std::vector<std::unique_ptr<gui::Dialog>> fDialogs{};
  std::unique_ptr<gui::Dialog> fCurrentDialog{};

  std::optional<std::string> fCurrentFragmentShaderNameRequest{};
};

}

#endif //WGPU_SHADER_TOY_MAIN_WINDOW_H
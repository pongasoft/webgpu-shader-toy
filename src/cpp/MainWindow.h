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

namespace image::format {

struct Format
{
  std::string fMimeType;
  std::string fExtension;
  std::string fDescription;
  bool fHasQuality;
};

constexpr auto kPNG = Format { .fMimeType = "image/png", .fExtension = "png", .fDescription = "PNG", .fHasQuality = false};
constexpr auto kJPG = Format { .fMimeType = "image/jpeg", .fExtension = "jpg", .fDescription = "JPEG", .fHasQuality = true};
constexpr auto kWEBP = Format { .fMimeType = "image/webp", .fExtension = "webp", .fDescription = "WebP", .fHasQuality = true};

constexpr Format kAll[] = {kPNG, kJPG, kWEBP};

Format getFormatFromMimeType(std::string const &iMimeType);

}

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
  void maybeNewFragmentShader(std::string const &iTitle, std::string const &iOkButton, Shader const &iShader);
  void saveState();
  void onClipboardString(void *iRequestUserData, char const *iClipboardString);

protected:
  void doRender() override;

private:
  void onNewFragmentShader(Shader const &iShader);
  void onNewFragmentShader(std::shared_ptr<FragmentShader> iFragmentShader);
  std::shared_ptr<FragmentShader> deleteFragmentShader(std::string const &iName);
  void setCurrentFragmentShader(std::shared_ptr<FragmentShader> iFragmentShader);
  void reset();
  void initFromState(State const &iState);
  void setStyle(bool iDarkStyle);
  void setFontSize(float iFontSize);
  void requestFontSize(float iFontSize);
  void setManualLayout(bool iManualLayout);
  State computeState() const;
  void renderDialog();
  void renderMainMenuBar();
  void renderShaderMenu(TextEditor &iEditor, std::string const &iNewCode, bool iEdited);
  void renderSettingsMenu();
  void renderControlsSection();
  void renderTimeControls();
  void renderShaderSection(bool iEditorHasFocus);
  void compile(std::string const &iNewCode);
  void promptNewEmtpyShader();
  void promptRenameCurrentShader();
  void promptDuplicateShader(std::string const &iShaderName);
  void promptShaderName(std::string const &iTitle,
                        std::string const &iOkButtonName,
                        std::string const &iShaderName,
                        std::function<void(std::string const &)> iOkAction,
                        std::shared_ptr<FragmentShader> iShader = nullptr);
  void promptExportShader(std::string const &iFilename, std::string const &iContent);
  void promptExportProject();
  void promptShaderFrameSize();
  void promptSaveCurrentFragmentShaderScreenshot();
  void saveCurrentFragmentShaderScreenshot(std::string const &iFilename);
  void renameShader(std::string const &iOldName, std::string const &iNewName);
  void resizeShader(Renderable::Size const &iSize, bool iApplyToAll);
  void newAboutDialog();
  void newHelpDialog();
  void help() const;
  inline bool hasDialog() const { return fCurrentDialog != nullptr || !fDialogs.empty(); }
  gui::Dialog &newDialog(std::string iTitle, bool iHighPriority = false);

private:
  std::shared_ptr<Preferences> fPreferences;
  State fDefaultState;
  std::string fLastComputedState;
  double fLastComputedStateTime;
  bool fDarkStyle{true};
  bool fManualLayout{false};
  float fLineSpacing{1.0f};
  float fFontSize{};
  bool fCodeShowWhiteSpace{false};
  image::format::Format fScreenshotFormat{image::format::kPNG};
  int fScreenshotQualityPercent{85};

  std::shared_ptr<FragmentShaderWindow> fFragmentShaderWindow;

//  std::string fCurrentAspectRatio{"Free"};
//  std::optional<AspectRatio> fAspectRatioRequest{};
  bool fResetRequest{};
  std::optional<float> fSetFontSizeRequest{};

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
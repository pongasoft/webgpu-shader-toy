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
#include "utils/UndoManager.h"
#include <optional>
#include <string>
#include <map>
#include <GLFW/emscripten_glfw3.h>

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

class MainWindow;

template<typename R>
class MainWindowAction : public pongasoft::utils::ExecutableAction<R>
{
public:
  void initTarget(MainWindow *iMainWindow) { fMainWindow = iMainWindow; }

protected:
  MainWindow *fMainWindow;
};

template<typename T>
concept IsMainWindowAction = requires {
  typename T::result_t;
  requires std::is_base_of_v<MainWindowAction<typename T::result_t>, T>;
};

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

  int onFile(char const *iName);
  void onNewContent(int iToken, char const *iName, char const *iContent, char const *iError);
  void maybeNewFragmentShader(std::string const &iTitle, std::string const &iOkButton, Shader const &iShader);
  void saveState();

  State computeState() const;
  State::Settings computeStateSettings() const;
  State::Shaders computeStateShaders() const;

  std::shared_ptr<FragmentShader> addFragmentShaderAction(std::unique_ptr<FragmentShader> iFragmentShader, int iPosition = -1);
  std::pair<std::shared_ptr<FragmentShader>, int> removeFragmentShaderAction(std::string const &iName);
  void renameShaderAction(std::string const &iOldName, std::string const &iNewName);
  void initFromStateAction(State const &iState);
  void initFromStateAction(State::Settings const &iSettings);
  void initFromStateAction(State::Shaders const &iShaders);

protected:
  void doRender() override;

private:
  using gui_action_t = std::function<void()>;

  struct NewContentRequest
  {
    struct File{std::string name;};
    struct URL{std::string url;};

    using Source = std::variant<File, URL>;

    explicit NewContentRequest(Source iSource);
    constexpr bool isFile() const { return std::holds_alternative<File>(fSource); }
    constexpr bool isURL() const { return std::holds_alternative<URL>(fSource); }
    constexpr std::string const &getValue() const { return isFile() ? std::get<File>(fSource).name : std::get<URL>(fSource).url; }

    int fToken;
    Source fSource;
  };

private:

  template<IsMainWindowAction T, class... Args >
  typename T::result_t executeAction(Args&&... args);

  void onNewFragmentShader(Shader const &iShader);
  void onNewFragmentShader(std::unique_ptr<FragmentShader> iFragmentShader);
  int removeFragmentShader(std::string const &iName);
  void removeAllFragmentShaders();
  void setCurrentFragmentShader(std::shared_ptr<FragmentShader> iFragmentShader);
  void resetSettings();
  void resetShaders();
  void resetAll();
  void initFromState(std::optional<State::Settings> iSettings, std::optional<State::Shaders> iShaders, std::string iDescription);
  void loadFromState(std::string const &iFilename, State const &iState);
  void importFromDisk();
  void promptImportFromURL();
  void importFromURL(std::string const &iURL);
  void onNewFile(char const *iName, char const *iContent);
  void onURLImported(std::string const &iURL, char const *iName, char const *iContent);
  void setStyle(bool iDarkStyle);
  void setFontSize(float iFontSize);
  void requestFontSize(float iFontSize);
  void setManualLayout(bool iManualLayout, std::optional<Size> iLeftPaneSize = std::nullopt, std::optional<Size> iRightPaneSize = std::nullopt);
  void switchToManualLayout();
  void switchToAutomaticLayout();
  void swapLayout();
  void setWindowOrder();
  void renderDialog();
  void renderMainMenuBar();
  void renderShaderMenu(TextEditor &iEditor, std::string const &iNewCode, bool iEdited);
  void renderSettingsMenu();
  void renderControlsSection();
  void renderTimeControls();
  void renderShaderSection(bool iEditorHasFocus);
  void renderHistory();
  void renderExampleMenu();
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
  int newContentRequest(NewContentRequest::Source iSource);
  void newAboutDialog();
  void newHelpDialog();
  void help() const;
  inline bool hasDialog() const { return fCurrentDialog != nullptr || !fDialogs.empty(); }
  template<typename State>
  gui::Dialog<State> &newDialog(std::string iTitle, State const &iState);
  gui::DialogNoState &newDialog(std::string iTitle);
  char const *getShortcutString(char const *iKey, char const *iFormat = "%s + %s");

  void deferBeforeImGuiFrame(gui_action_t iAction) { if(iAction) fBeforeImGuiFrameActions.emplace_back(std::move(iAction)); }

  std::shared_ptr<FragmentShader> findFragmentShaderByName(std::string const &iName) const;

private:
  std::shared_ptr<Preferences> fPreferences;
  State fDefaultState;
  std::string fLastComputedState;
  double fLastComputedStateTime;
  bool fDarkStyle{true};
  bool fLayoutManual{false};
  bool fLayoutSwapped{false};
  float fLineSpacing{1.0f};
  float fFontSize{};
  bool fCodeShowWhiteSpace{false};
  image::format::Format fScreenshotFormat{image::format::kPNG};
  int fScreenshotQualityPercent{85};

  std::shared_ptr<FragmentShaderWindow> fFragmentShaderWindow;

//  std::string fCurrentAspectRatio{"Free"};
//  std::optional<AspectRatio> fAspectRatioRequest{};
  std::optional<float> fSetFontSizeRequest{};

  std::vector<gui_action_t> fBeforeImGuiFrameActions{};

  std::vector<std::shared_ptr<FragmentShader>> fFragmentShaders{};
  std::shared_ptr<FragmentShader>fCurrentFragmentShader{};

  std::vector<std::unique_ptr<gui::IDialog>> fDialogs{};
  std::unique_ptr<gui::IDialog> fCurrentDialog{};

  std::optional<std::string> fCurrentFragmentShaderNameRequest{};

  pongasoft::utils::UndoManager fUndoManager{};

  std::optional<NewContentRequest> fNewContentRequest{};

  // UI
  ImVec2 fIconButtonSize{};
};

}

#endif //WGPU_SHADER_TOY_MAIN_WINDOW_H
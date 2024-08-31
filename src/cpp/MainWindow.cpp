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

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <GLFW/glfw3.h>
#include "MainWindow.h"
#include "gui/WstGui.h"
#include "JetBrainsMono-Regular.cpp"
#include "IconsFontWGPUShaderToy.h"
#include "IconsFontWGPUShaderToy.cpp"
#include "Errors.h"
#include "utils/DataManager.h"
#include <iostream>
#include <ranges>
#include <utility>
#include <emscripten.h>
#include <version.h>


namespace shader_toy {
extern "C" {
using OnNewFileHandler = void (*)(MainWindow *iMainWindow, char const *iFilename, char const *iContent);
using OnBeforeUnloadHandler = void (*)(MainWindow *iMainWindow);

void wgpu_shader_toy_install_handlers(OnNewFileHandler iOnNewFileHandler,
                                      OnBeforeUnloadHandler iOnBeforeUnloadHandler,
                                      MainWindow *iMainWindow);

void wgpu_shader_toy_uninstall_handlers();
void wgpu_shader_toy_open_file_dialog();
void wgpu_shader_toy_export_content(char const *iFilename, char const *iContent);

void wgpu_shader_toy_print_stack_trace(char const *iMessage);

}

namespace callbacks {
//------------------------------------------------------------------------
// callbacks::OnNewFileCallback
//------------------------------------------------------------------------
void OnNewFileCallback(MainWindow *iMainWindow, char const *iFilename, char const *iContent)
{
  iMainWindow->onNewFile(iFilename, iContent);
}

//------------------------------------------------------------------------
// callbacks::OnBeforeUnload
//------------------------------------------------------------------------
void OnBeforeUnload(MainWindow *iMainWindow)
{
  iMainWindow->saveState();
}

}

// Some shader examples
extern std::vector<Shader> kFragmentShaderExamples;

// Empty shader
auto constexpr kEmptyShader = R"(@fragment
fn fragmentMain(@builtin(position) pos: vec4f) -> @location(0) vec4f {
    return vec4f(0.5, 0.5, 0.5, 1);
}
)";

//// kAspectRatios
//static std::vector<std::pair<std::string, Window::AspectRatio>> kAspectRatios{
//  {"Free", Window::AspectRatio{GLFW_DONT_CARE, GLFW_DONT_CARE}},
//  {"1x1", Window::AspectRatio{1, 1}},
//  {"16x9", Window::AspectRatio{16, 9}},
//  {"9x16", Window::AspectRatio{9, 16}},
//  {"4x3", Window::AspectRatio{4, 3}},
//  {"3x4", Window::AspectRatio{3, 4}},
//};

namespace impl {

//------------------------------------------------------------------------
// impl::mergeFontAwesome
//------------------------------------------------------------------------
static void mergeFontAwesome(float iSize)
{
  static auto kFontData = utils::DataManager::loadCompressedBase85(IconsFontWGPUShaderToy_compressed_data_base85);

  auto &io = ImGui::GetIO();
  static const ImWchar icons_ranges[] = {fa::kMin, fa::kMax16, 0};
  ImFontConfig icons_config;
  icons_config.GlyphOffset = {0, 1};
  icons_config.MergeMode = true;
  icons_config.PixelSnapH = true;
  icons_config.OversampleH = 2;
  icons_config.FontDataOwnedByAtlas = false;
  icons_config.GlyphMinAdvanceX = iSize; // to make it monospace

  icons_config.FontDataOwnedByAtlas = false;
  io.Fonts->AddFontFromMemoryTTF(kFontData.data(),
                                 static_cast<int>(kFontData.size()),
                                 iSize,
                                 &icons_config,
                                 icons_ranges);
}

}

//------------------------------------------------------------------------
// JSSetStyle
//------------------------------------------------------------------------
EM_JS(void, JSSetStyle, (bool iDarkStyle), { Module.wst_set_style(iDarkStyle); })

//------------------------------------------------------------------------
// JSSetLayout
//------------------------------------------------------------------------
EM_JS(void, JSSetLayout, (bool iManualLayout, int iLeftPaneWidth, int iRightPaneWidth), {
  Module.wst_set_layout(iManualLayout, iLeftPaneWidth, iRightPaneWidth);
})

//------------------------------------------------------------------------
// JSSetWindowOrder
//------------------------------------------------------------------------
EM_JS(void, JSSetWindowOrder, (GLFWwindow *iLeftWindow, GLFWwindow *iRightWindow), {
  Module.wst_set_window_order(iLeftWindow, iRightWindow);}
)

// wst_set_window_order

//------------------------------------------------------------------------
// MainWindow::MainWindow
//------------------------------------------------------------------------
MainWindow::MainWindow(std::shared_ptr<GPU> iGPU, Window::Args const &iWindowArgs,
                       Args const &iMainWindowArgs) : ImGuiWindow(std::move(iGPU), iWindowArgs),
                                                      fPreferences{iMainWindowArgs.preferences},
                                                      fDefaultState{iMainWindowArgs.defaultState},
                                                      fLastComputedState{Preferences::serialize(iMainWindowArgs.state)},
                                                      fLastComputedStateTime{glfwGetTime()},
                                                      fFragmentShaderWindow{
                                                        std::make_unique<FragmentShaderWindow>(fGPU,
                                                                                               iMainWindowArgs.fragmentShaderWindow)
                                                      }
{
  initFromStateAction(iMainWindowArgs.state);
  setFontSize(iMainWindowArgs.state.fSettings.fFontSize);
  fSetFontSizeRequest = std::nullopt;

  wgpu_shader_toy_install_handlers(callbacks::OnNewFileCallback,
                                   callbacks::OnBeforeUnload,
                                   this);
}

//------------------------------------------------------------------------
// MainWindow::findFragmentShaderByName
//------------------------------------------------------------------------
std::shared_ptr<FragmentShader> MainWindow::findFragmentShaderByName(std::string const &iName) const
{
  auto it = std::find_if(fFragmentShaders.begin(),
                         fFragmentShaders.end(),
                         [&iName](auto &f) { return f->getName() == iName; });
  if(it != fFragmentShaders.end())
    return *it;
  else
    return nullptr;
}

//------------------------------------------------------------------------
// MainWindow::setFontSize
//------------------------------------------------------------------------
void MainWindow::setFontSize(float iFontSize)
{
  static auto kFontData = utils::DataManager::loadCompressedBase85(JetBrainsMonoRegular_compressed_data_base85);

  if(fFontSize == iFontSize)
    return;

  fFontSize = iFontSize;

  auto &io = ImGui::GetIO();
  io.Fonts->Clear();
  ImFontConfig fontConfig;
  fontConfig.OversampleH = 2;
  float fontScale; float dummy;
  glfwGetWindowContentScale(fWindow, &fontScale, &dummy);
  auto const size = fFontSize * fontScale;
  fontConfig.FontDataOwnedByAtlas = false;
  io.Fonts->AddFontFromMemoryTTF(kFontData.data(), static_cast<int>(kFontData.size()), size, &fontConfig);
  impl::mergeFontAwesome(size);
  io.FontGlobalScale = 1.0f / fontScale;
}


//------------------------------------------------------------------------
// MainWindow::requestFontSize
//------------------------------------------------------------------------
void MainWindow::requestFontSize(float iFontSize)
{
  iFontSize = std::clamp(iFontSize, 8.0f, 30.0f);
  if(fFontSize != iFontSize)
    fSetFontSizeRequest = iFontSize;
}

//------------------------------------------------------------------------
// MainWindow::~MainWindow
//------------------------------------------------------------------------
MainWindow::~MainWindow()
{
  wgpu_shader_toy_uninstall_handlers();
}

namespace impl {

//------------------------------------------------------------------------
// impl::lineCount
//------------------------------------------------------------------------
inline long lineCount(std::string const &s)
{
  return std::count(s.begin(), s.end(), '\n');
}

}

#ifndef NDEBUG
static bool kShowDemoWindow = false;
#endif

//------------------------------------------------------------------------
// MainWindow::newDialog
//------------------------------------------------------------------------
template<typename State>
gui::Dialog<State> &MainWindow::newDialog(std::string iTitle, State const &iState)
{
  auto dialog = std::make_unique<gui::Dialog<State>>(std::move(iTitle), iState);
  auto &res = *dialog.get();
  fDialogs.emplace_back(std::move(dialog));
  return res;
}


//------------------------------------------------------------------------
// MainWindow::newDialog
//------------------------------------------------------------------------
gui::DialogNoState &MainWindow::newDialog(std::string iTitle)
{
  return newDialog<gui::VoidState>(std::move(iTitle), {});
}

//------------------------------------------------------------------------
// MainWindow::renderSettingsMenu
//------------------------------------------------------------------------
void MainWindow::renderSettingsMenu()
{
  if(ImGui::BeginMenu("Style"))
  {
    std::optional<bool> newDarkStyle{};
    if(ImGui::MenuItem("Dark", nullptr, fDarkStyle))
      newDarkStyle = true;
    if(ImGui::MenuItem("Light", nullptr, !fDarkStyle))
      newDarkStyle = false;
    if(newDarkStyle)
      setStyle(*newDarkStyle);
    ImGui::EndMenu();
  }
  if(ImGui::MenuItem("Font Size"))
  {
    newDialog("Font Size")
      .content([this] {
        if(ImGui::Button(" + "))
          requestFontSize(fFontSize + 1.0f);
        ImGui::SameLine();
        if(ImGui::Button(" - "))
          requestFontSize(fFontSize - 1.0f);
        ImGui::SameLine();
        ImGui::Text("%.0fpx", fFontSize);
      })
      .buttonOk()
      .button("Cancel", [fontSize = fFontSize, this] { requestFontSize(fontSize); });
  }
  if(ImGui::BeginMenu("Code"))
  {
    if(ImGui::MenuItem("Line Spacing"))
    {
      newDialog("Line Spacing")
        .content([this] { ImGui::SliderFloat("###line_spacing", &fLineSpacing, 1.0f, 2.0f); })
        .buttonOk()
        .button("Cancel", [lineSpacing = fLineSpacing, this] { fLineSpacing = lineSpacing; });
    }
    ImGui::MenuItem("Show White Space", nullptr, &fCodeShowWhiteSpace);
    ImGui::EndMenu();
  }
  if(ImGui::BeginMenu("Resolution"))
  {
    if(ImGui::MenuItem("Hi DPI", nullptr, fFragmentShaderWindow->isHiDPIAware()))
    {
      fFragmentShaderWindow->toggleHiDPIAwareness();
    }
//    if(ImGui::BeginMenu("Aspect Ratio"))
//    {
//      for(auto &[name, aspectRatio]: kAspectRatios)
//      {
//        if(ImGui::MenuItem(name.c_str(), nullptr, name == fCurrentAspectRatio))
//        {
//          fCurrentAspectRatio = name;
//          fAspectRatioRequest = aspectRatio;
//        }
//      }
//      ImGui::EndMenu();
//    }
    ImGui::EndMenu();
  }
  if(ImGui::BeginMenu("Layout"))
  {
    if(ImGui::MenuItem("Manual", nullptr, fLayoutManual))
    {
      if(!fLayoutManual)
        switchToManualLayout();
    }
    if(ImGui::MenuItem("Automatic", nullptr, !fLayoutManual))
    {
      if(fLayoutManual)
        switchToAutomaticLayout();
    }
    ImGui::Separator();
    if(ImGui::MenuItem("Swap"))
      swapLayout();
    ImGui::EndMenu();
  }
}

//------------------------------------------------------------------------
// MainWindow::renderMainMenuBar
//------------------------------------------------------------------------
void MainWindow::renderMainMenuBar()
{
  auto padding = ImGui::GetStyle().FramePadding;
  padding.y *= 2.0f;
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, padding);
  if(ImGui::BeginMainMenuBar())
  {
    if(ImGui::BeginMenu(fa::kBars))
//    if(ImGui::BeginMenu("WebGPU Shader Toy"))
    {
      if(ImGui::MenuItem("About"))
        newAboutDialog();
      if(ImGui::MenuItem("Help"))
        newHelpDialog();
      ImGui::SeparatorText("Settings");
      renderSettingsMenu();
      ImGui::SeparatorText("Project");
      if(ImGui::MenuItem("Save (browser)"))
        saveState();
      if(ImGui::MenuItem("Export (disk)"))
        promptExportProject();
      if(ImGui::MenuItem("Import (disk)"))
        wgpu_shader_toy_open_file_dialog();
      ImGui::Separator();
      if(ImGui::BeginMenu("Reset"))
      {
        if(ImGui::MenuItem("Settings"))
          deferBeforeImGuiFrame([this] { resetSettings(); });
        if(ImGui::MenuItem("Shaders"))
          deferBeforeImGuiFrame([this] { resetShaders(); });
        if(ImGui::MenuItem("All"))
          deferBeforeImGuiFrame([this] { resetAll(); });
        ImGui::EndMenu();
      }
#ifndef NDEBUG
      ImGui::Separator();
      if(ImGui::MenuItem("Quit"))
        stop();
#endif
      ImGui::EndMenu();
    }

    ImGui::Text("|");

    ImGui::BeginDisabled(fCurrentFragmentShader == nullptr);
    if(ImGui::BeginMenu("Shader"))
    {
      // defined later...
      ImGui::EndMenu();
    }
    ImGui::EndDisabled();

    if(ImGui::BeginMenu("History"))
    {
      renderHistory();
      ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Examples"))
    {
      for(auto &shader: kFragmentShaderExamples)
      {
        if(ImGui::MenuItem(shader.fName.c_str()))
        {
          maybeNewFragmentShader("Load Example", "Add", shader);
        }
      }
      ImGui::EndMenu();
    }
#ifndef NDEBUG
    if(ImGui::BeginMenu("Dev"))
    {
      ImGui::MenuItem("Demo", nullptr, &kShowDemoWindow);
      ImGui::EndMenu();
    }
#endif

    ImGui::EndMainMenuBar();
  }
  ImGui::PopStyleVar();
}
//------------------------------------------------------------------------
// MainWindow::renderTimeControls
//------------------------------------------------------------------------
void MainWindow::renderTimeControls()
{
  // Reset time
  if(ImGui::Button(fa::kClockRotateLeft, fIconButtonSize))
    fCurrentFragmentShader->setStartTime(getCurrentTime());

  ImGui::SameLine();

  auto const isKeyAlt = gui::WstGui::IsKeyAlt();
  auto const frameCount = isKeyAlt ? 1 : 12;

  // Previous frame
  ImGui::BeginDisabled(fCurrentFragmentShader->getInputs().frame == 0);
  {
    ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
    auto button = ImGui::Button(isKeyAlt ? fa::kBackward : fa::kBackwardFast, fIconButtonSize);
    if(ImGui::IsItemDeactivated())
      fCurrentFragmentShader->stopManualTime(getCurrentTime());
    else
    {
      if(button || ImGui::IsItemActivated())
        fCurrentFragmentShader->previousFrame(getCurrentTime(), frameCount);
    }
    ImGui::PopItemFlag();
  }
  ImGui::EndDisabled();

  ImGui::SameLine();

  // Play / Pause
  if(ImGui::Button(fCurrentFragmentShader->isRunning() ? fa::kCirclePause : fa::kCirclePlay, fIconButtonSize))
    fCurrentFragmentShader->toggleRunning(getCurrentTime());

  ImGui::SameLine();

  // Next frame
  ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
  {
    auto button = ImGui::Button(isKeyAlt ? fa::kForward : fa::kForwardFast, fIconButtonSize);
    if(ImGui::IsItemDeactivated())
      fCurrentFragmentShader->stopManualTime(getCurrentTime());
    else
    {
      if(button || ImGui::IsItemActivated())
        fCurrentFragmentShader->nextFrame(getCurrentTime(), frameCount);
    }
  }
  ImGui::PopItemFlag();
}

//------------------------------------------------------------------------
// MainWindow::renderControlsSection
//------------------------------------------------------------------------
void MainWindow::renderControlsSection()
{
  // [Section] Controls
  ImGui::SeparatorText("Controls");

  ImGui::BeginDisabled(!fCurrentFragmentShader->isEnabled());
  {
    // Time controls
    renderTimeControls();

    ImGui::SameLine();

    auto const isKeyAlt = gui::WstGui::IsKeyAlt();

    // Screenshot
    if(isKeyAlt)
    {
      if(ImGui::Button(fa::kCamera, fIconButtonSize))
        promptSaveCurrentFragmentShaderScreenshot();
    }
    else
    {
      if(ImGui::Button(fa::kCameraPolaroid, fIconButtonSize))
        saveCurrentFragmentShaderScreenshot(fCurrentFragmentShader->getName());
    }

    ImGui::SameLine();

    // Fullscreen
    if(ImGui::Button(isKeyAlt ? fa::kExpand : fa::kExpandWide, fIconButtonSize))
      fFragmentShaderWindow->requestFullscreen(!isKeyAlt);
  }
  ImGui::EndDisabled();

  ImGui::SameLine();

  ImGui::Text("|");

  ImGui::SameLine();

  if(ImGui::Button(fa::kPowerOff, fIconButtonSize))
    fCurrentFragmentShader->toggleEnabled();

  ImGui::SameLine();

  ImGui::Text("| %s | %.3f (%.1f FPS)",
              fCurrentFragmentShader->getStatus(),
              1000.0f / ImGui::GetIO().Framerate,
              ImGui::GetIO().Framerate);
}


//------------------------------------------------------------------------
// MainWindow::compile
//------------------------------------------------------------------------
void MainWindow::compile(std::string const &iNewCode)
{
  WST_INTERNAL_ASSERT(fCurrentFragmentShader != nullptr);
  fCurrentFragmentShader->updateCode(iNewCode);
  setCurrentFragmentShader(fCurrentFragmentShader);
}

//------------------------------------------------------------------------
// MainWindow::promptNewEmtpyShader
//------------------------------------------------------------------------
void MainWindow::promptNewEmtpyShader()
{
  maybeNewFragmentShader("New Shader", "Create", {"", kEmptyShader});
}

//------------------------------------------------------------------------
// MainWindow::promptRenameCurrentShader
//------------------------------------------------------------------------
void MainWindow::promptRenameCurrentShader()
{
  if(!fCurrentFragmentShader)
    return;

  auto const currentName = fCurrentFragmentShader->getName();

  promptShaderName("Rename Shader",
                   "Rename",
                   fCurrentFragmentShader->getName(),
                   [currentName, this](auto const &iNewName) {
                     renameShader(currentName, iNewName);
                   },
                   fCurrentFragmentShader);
}


//------------------------------------------------------------------------
// MainWindow::promptDuplicateShader
//------------------------------------------------------------------------
void MainWindow::promptDuplicateShader(std::string const &iShaderName)
{
  if(auto shader = findFragmentShaderByName(iShaderName); shader)
  {
    maybeNewFragmentShader(fmt::printf("Duplicate %s", iShaderName),
                           "Duplicate",
                           {"", shader->getCode()});
  }
}

//------------------------------------------------------------------------
// MainWindow::promptShaderName
//------------------------------------------------------------------------
void MainWindow::promptShaderName(std::string const &iTitle,
                                  std::string const &iOkButtonName,
                                  std::string const &iShaderName,
                                  std::function<void(std::string const &)> iOkAction,
                                  std::shared_ptr<FragmentShader> iShader)
{
  newDialog(iTitle, iShaderName)
    .content([this, shader = std::move(iShader), okButtonName = iOkButtonName] (auto &iDialog) {
      auto &newShaderName = iDialog.state();
      iDialog.initKeyboardFocusHere();
      ImGui::InputText("###name", &newShaderName);
      if(auto oldShader = findFragmentShaderByName(newShaderName); oldShader && oldShader != shader)
      {
        ImGui::SeparatorText("!!! Warning !!!");
        ImGui::Text("Duplicate name detected.");
        ImGui::Text("Continuing will override the content of the shader.");
        iDialog.button(0).fLabel = "Override";
      }
      else
        iDialog.button(0).fLabel = okButtonName;

      iDialog.button(0).fEnabled = !newShaderName.empty();
    })
    .button(iOkButtonName, [action = std::move(iOkAction)] (auto &iDialog) { action(iDialog.state()); }, true)
    .buttonCancel()
    ;
}

//------------------------------------------------------------------------
// MainWindow::resizeShader
//------------------------------------------------------------------------
void MainWindow::resizeShader(Renderable::Size const &iSize, bool iApplyToAll)
{
  deferBeforeImGuiFrame([this, iSize, iApplyToAll]() {
    if(!fLayoutManual)
      setManualLayout(true);
    fFragmentShaderWindow->resize(iSize);
    if(iApplyToAll)
    {
      for(auto &shader: fFragmentShaders)
        shader->setWindowSize(iSize);
    }
  });
}

//------------------------------------------------------------------------
// MainWindow::promptShaderFrameSize
//------------------------------------------------------------------------
void MainWindow::promptShaderFrameSize()
{
  auto resizeAll = fFragmentShaders.size() > 1;

  auto &dialog = newDialog("Shader Frame Size", fFragmentShaderWindow->getSize())
    .content([resizeAll] (auto &iDialog) {
      auto &size = iDialog.state();
      ImGui::SeparatorText("Size (width x height)");
      iDialog.initKeyboardFocusHere();
      ImGui::InputInt2("###size", &size.width);
      ImGui::SetItemDefaultFocus();
      iDialog.button(0).fEnabled = size.width > 0 && size.height > 0;
      if(resizeAll)
        iDialog.button(1).fEnabled = iDialog.button(0).fEnabled;
    })
    .button("Resize", [this] (auto &iDialog) { resizeShader(iDialog.state(), false); }, true)
  ;

  if(resizeAll)
    dialog.button(fmt::printf("Resize All (%d)", fFragmentShaders.size()), [this] (auto &iDialog) { resizeShader(iDialog.state(), true); });

  dialog.allowDismissDialog();
  dialog.buttonCancel();
}


namespace impl {

//------------------------------------------------------------------------
// impl::ends_with
//------------------------------------------------------------------------
inline bool ends_with(std::string const &s, std::string const &iSuffix)
{
  if(s.size() < iSuffix.size())
    return false;
  return s.substr(s.size() - iSuffix.size()) == iSuffix;
}

}

//------------------------------------------------------------------------
// MainWindow::promptExportShader
//------------------------------------------------------------------------
void MainWindow::promptExportShader(std::string const &iFilename, std::string const &iContent)
{
  auto filename = impl::ends_with(iFilename, ".wgsl") ? iFilename : fmt::printf("%s.wgsl", iFilename);

  newDialog("Export Shader", filename)
    .content([] (auto &iDialog) {
      ImGui::SeparatorText("Filename");
      iDialog.initKeyboardFocusHere();
      ImGui::InputText("###name", &iDialog.state());
      iDialog.button(0).fEnabled = !iDialog.state().empty();
    })
    .button("Export", [content = iContent] (auto &iDialog) {
      wgpu_shader_toy_export_content(iDialog.state().c_str(), content.c_str());
    }, true)
    .buttonCancel()
    ;
}


//------------------------------------------------------------------------
// MainWindow::promptExportShader
//------------------------------------------------------------------------
void MainWindow::promptExportProject()
{
  newDialog("Export Project", std::string("WebGPUShaderToy.json"))
    .content([] (auto &iDialog) {
      ImGui::SeparatorText("Filename");
      iDialog.initKeyboardFocusHere();
      ImGui::InputText("###name", &iDialog.state());
      iDialog.button(0).fEnabled = !iDialog.state().empty();
    })
    .button("Export", [this] (auto &iDialog) {
      wgpu_shader_toy_export_content(iDialog.state().c_str(), fPreferences->serialize(computeState()).c_str());
    }, true)
    .buttonCancel()
    ;
}

//------------------------------------------------------------------------
// MainWindow::saveCurrentFragmentShaderScreenshot
//------------------------------------------------------------------------
void MainWindow::saveCurrentFragmentShaderScreenshot(std::string const &iFilename)
{
  fFragmentShaderWindow->saveScreenshot(fmt::printf("%s.%s", iFilename, fScreenshotFormat.fExtension),
                                        fScreenshotFormat.fMimeType,
                                        static_cast<float>(fScreenshotQualityPercent) / 100.0f);
}

//------------------------------------------------------------------------
// MainWindow::promptSaveCurrentFragmentShaderScreenshot
//------------------------------------------------------------------------
void MainWindow::promptSaveCurrentFragmentShaderScreenshot()
{
  if(!fCurrentFragmentShader)
    return;

  newDialog("Screenshot", fCurrentFragmentShader->getName())
    .content([this] (auto &iDialog) {
      ImGui::SeparatorText("Filename");
      auto label = fmt::printf(".%s###name", fScreenshotFormat.fExtension);
      iDialog.initKeyboardFocusHere();
      ImGui::InputText(label.c_str(), &iDialog.state());
      iDialog.button(0).fEnabled = !iDialog.state().empty();
      ImGui::SeparatorText("Format");
      if(ImGui::BeginCombo("Format", fScreenshotFormat.fDescription.c_str()))
      {
        for(auto &format: image::format::kAll)
        {
          if(ImGui::Selectable(format.fDescription.c_str(), fScreenshotFormat.fMimeType == format.fMimeType))
            fScreenshotFormat = format;
        }
        ImGui::EndCombo();
      }
      if(fScreenshotFormat.fHasQuality)
        ImGui::SliderInt("Quality", &fScreenshotQualityPercent, 1, 100, "%d%%");

      ImGui::SeparatorText("Time Controls");

      if(fCurrentFragmentShader)
        renderTimeControls();
    })
    .button(ICON_FA_Camera " Screenshot", [this] (auto &iDialog) {
      saveCurrentFragmentShaderScreenshot(iDialog.state());
    }, true)
    .allowDismissDialog()
    .buttonCancel();
}


//------------------------------------------------------------------------
// MainWindow::renderShaderMenu
//------------------------------------------------------------------------
void MainWindow::renderShaderMenu(TextEditor &iEditor, std::string const &iNewCode, bool iEdited)
{
  if(ImGui::BeginMenu("Shader"))
  {
    // -- Code ------
    ImGui::SeparatorText("Shader");

    if(ImGui::MenuItem(ICON_FA_Hammer " Compile", getShortcutString("D"), false, iEdited))
      compile(iNewCode);
    if(ImGui::MenuItem("Rename"))
      promptRenameCurrentShader();
    if(ImGui::MenuItem("Duplicate"))
      promptDuplicateShader(fCurrentFragmentShader->getName());
    if(ImGui::MenuItem("Export"))
      promptExportShader(fCurrentFragmentShader->getName(), iNewCode);

    // -- Edit ------
    ImGui::SeparatorText("Edit");

    if(ImGui::MenuItem("Undo", getShortcutString("Z")))
      iEditor.Undo();
    if(ImGui::MenuItem("Redo", getShortcutString("Z", "Shift + %s + %s")))
      iEditor.Redo();
    ImGui::Separator();
    if(ImGui::MenuItem("Select All", getShortcutString("A", "Shift + %s + %s")))
      iEditor.SelectAll();

    // -- Frame ------
    ImGui::SeparatorText("Frame");

    if(ImGui::MenuItem(ICON_FA_PowerOff " Enabled", nullptr, fCurrentFragmentShader->isEnabled()))
      fCurrentFragmentShader->toggleEnabled();
    if(ImGui::MenuItem("Resize"))
      promptShaderFrameSize();
    if(ImGui::MenuItem(ICON_FA_Camera " Screenshot"))
      promptSaveCurrentFragmentShaderScreenshot();

    ImGui::EndMenu();
  }
}

//------------------------------------------------------------------------
// MainWindow::renderShaderSection
//------------------------------------------------------------------------
void MainWindow::renderShaderSection(bool iEditorHasFocus)
{
  // [Section] Shader
  ImGui::SeparatorText("Shader");

  if(ImGui::BeginTabBar(fCurrentFragmentShader->getName().c_str()))
  {
    auto &editor = fCurrentFragmentShader->edit();
    editor.SetPalette(fDarkStyle ? TextEditor::PaletteId::Dark : TextEditor::PaletteId::Light);
    editor.SetLineSpacing(fLineSpacing);
    editor.SetShowWhitespacesEnabled(fCodeShowWhiteSpace);

    auto newCode = editor.GetText();
    auto edited = newCode != fCurrentFragmentShader->getCode();

    // [Keyboard shortcut]
    if(iEditorHasFocus && ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_D))
    {
      if(edited)
        compile(newCode);
    }
    if(ImGui::BeginMainMenuBar())
    {
      renderShaderMenu(editor, newCode, edited);
      ImGui::EndMainMenuBar();
    }

    // [TabItem] Code
    if(ImGui::BeginTabItem("Code"))
    {
      // [Child] Menu / toolbar for text editor
      const bool hasCompilationError = fCurrentFragmentShader->hasCompilationError();
      long lines = std::min(hasCompilationError ? impl::lineCount(fCurrentFragmentShader->getCompilationErrorMessage()) - 1 : 1, 10L);
      ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImGui::GetStyle().Colors[ImGuiCol_ChildBg]);
      if(hasCompilationError)
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(editor.GetErrorMarkerColor()));
      ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 1),
                                          ImVec2(FLT_MAX, ImGui::GetTextLineHeightWithSpacing() * static_cast<float>(lines)));
      if(ImGui::BeginChild("Menu Bar", {}, 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_HorizontalScrollbar))
      {
        if(ImGui::BeginMenuBar())
        {
          if(!edited && hasCompilationError)
            editor.AddErrorMarker(fCurrentFragmentShader->getCompilationErrorLine(), fCurrentFragmentShader->getCompilationErrorMessage());
          else
            editor.ClearErrorMarkers();
          int lineCount, columnCount;
          editor.GetCursorPosition(lineCount, columnCount);
          ImGui::Text("%d/%d | %d lines", lineCount + 1, columnCount + 1, editor.GetLineCount());
          ImGui::BeginDisabled(!edited);
          if(ImGui::Button(ICON_FA_Hammer " Compile"))
            compile(newCode);
          ImGui::EndDisabled();
          ImGui::EndMenuBar();
        }
        if(fCurrentFragmentShader->hasCompilationError())
        {
          ImGui::Text("%s", fCurrentFragmentShader->getCompilationErrorMessage().c_str());
        }
      }
      ImGui::EndChild();
      ImGui::PopStyleColor(hasCompilationError ? 2 : 1);

      // [Editor] Render
      editor.Render("Code", iEditorHasFocus, {}, 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_HorizontalScrollbar);

      ImGui::EndTabItem();
    }

    // [TabItem] Inputs
    if(ImGui::BeginTabItem("Inputs"))
    {
      auto &inputs = fCurrentFragmentShader->getInputs();
      ImGui::Text(FragmentShader::kHeaderTemplate,
                  static_cast<int>(inputs.size.x), static_cast<int>(inputs.size.y), inputs.size.z, inputs.size.w, // size: vec4f
                  static_cast<int>(inputs.mouse.x), static_cast<int>(inputs.mouse.y), static_cast<int>(inputs.mouse.z), static_cast<int>(inputs.mouse.w), // mouse: vec4f
                  inputs.time, // time: f32
                  inputs.frame // frame: i32
      );
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
}

//------------------------------------------------------------------------
// MainWindow::renderDialog
//------------------------------------------------------------------------
void MainWindow::renderDialog()
{
  if(!fCurrentDialog)
  {
    if(fDialogs.empty())
      return;
    fCurrentDialog = std::move(fDialogs[0]);
    fDialogs.erase(fDialogs.begin());
  }
  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  fCurrentDialog->render();
  if(!fCurrentDialog->isOpen())
  {
    ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
    fCurrentDialog = nullptr;
  }
}

//------------------------------------------------------------------------
// MainWindow::doRender
//------------------------------------------------------------------------
void MainWindow::doRender()
{
  fIconButtonSize = {ImGui::CalcTextSize(fa::kCameraPolaroid).x + 2 * ImGui::GetStyle().ItemInnerSpacing.x, 0};

  renderMainMenuBar();

  bool isDialogOpen = hasDialog();

  if(isDialogOpen)
    renderDialog();

  // The main window occupies the full available space
  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos);
  ImGui::SetNextWindowSize(ImGui::GetMainViewport()->WorkSize);
  if(ImGui::Begin("WebGPU Shader Toy", nullptr,
                  ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar))
  {
    // [TabBar] One tab per shader
    if(ImGui::BeginTabBar("Fragment Shaders"))
    {
      if(!fFragmentShaders.empty())
      {
        auto selectedFragmentShader = fFragmentShaders[0];
        std::shared_ptr<FragmentShader> fragmentShaderToRemove{};
        for(auto &shader: fFragmentShaders)
        {
          auto const &fragmentShaderName = shader->getName();
          bool open = true;
          auto flags = ImGuiTabItemFlags_None;
          if(fCurrentFragmentShaderNameRequest && *fCurrentFragmentShaderNameRequest == fragmentShaderName)
          {
            flags = ImGuiTabItemFlags_SetSelected;
            selectedFragmentShader = shader;
          }
          if(ImGui::BeginTabItem(fragmentShaderName.c_str(), &open, flags))
          {
            if (ImGui::BeginPopupContextItem())
            {
              if(ImGui::MenuItem("Rename"))
                promptRenameCurrentShader();
              if(ImGui::MenuItem("Duplicate"))
                promptDuplicateShader(fragmentShaderName);

              ImGui::EndPopup();
            }
            if(!fCurrentFragmentShaderNameRequest)
              selectedFragmentShader = shader;
            ImGui::EndTabItem();
          }
          if(!open)
            fragmentShaderToRemove = shader;
        }
        fCurrentFragmentShaderNameRequest = std::nullopt;
        if(fragmentShaderToRemove)
          removeFragmentShader(fragmentShaderToRemove->getName());
        else
        {
          WST_INTERNAL_ASSERT(fCurrentFragmentShader != nullptr);
          if(fCurrentFragmentShader != selectedFragmentShader)
            setCurrentFragmentShader(selectedFragmentShader);
        }
      }
      // + to add a new shader (from file)
      if(ImGui::TabItemButton("+"))
        ImGui::OpenPopup("Add Shader");

      if(ImGui::BeginPopup("Add Shader"))
      {
        if(ImGui::MenuItem("New"))
          promptNewEmtpyShader();
        if(ImGui::MenuItem("Import"))
          wgpu_shader_toy_open_file_dialog();
        if(ImGui::BeginMenu("Examples"))
        {
          for(auto &shader: kFragmentShaderExamples)
          {
            if(ImGui::MenuItem(shader.fName.c_str()))
            {
              maybeNewFragmentShader("Load Example", "Add", shader);
            }
          }
          ImGui::EndMenu();
        }
        ImGui::EndPopup();
      }

      ImGui::EndTabBar();
    }

    if(fCurrentFragmentShader)
    {
      renderControlsSection();
      auto const editorHasFocus = !ImGui::IsAnyItemActive() && !isDialogOpen;
      renderShaderSection(editorHasFocus);
    }
    else
    {
      ImGui::Text("Click on [+] to add a shader or drag and drop a shader file here");
    }
  }
  ImGui::End();

#ifndef NDEBUG
  if(kShowDemoWindow)
    ImGui::ShowDemoWindow(&kShowDemoWindow);
#endif
}

//------------------------------------------------------------------------
// MainWindow::onNewFragmentShader
//------------------------------------------------------------------------
void MainWindow::onNewFragmentShader(Shader const &iShader)
{
  auto shader = iShader;
  if(auto fs = findFragmentShaderByName(iShader.fName); fs)
    shader.fWindowSize = fs->getWindowSize();
  else
    shader.fWindowSize = fFragmentShaderWindow->getSize();
  onNewFragmentShader(std::make_unique<FragmentShader>(shader));
}


//------------------------------------------------------------------------
// MainWindow::maybeNewFragmentShader
//------------------------------------------------------------------------
void MainWindow::maybeNewFragmentShader(std::string const &iTitle, std::string const &iOkButton, Shader const &iShader)
{
  if(iShader.fName.empty() || findFragmentShaderByName(iShader.fName))
  {
    promptShaderName(iTitle, iOkButton, iShader.fName,
                     [shader = iShader, this] (std::string const &iShaderName) mutable {
                       shader.fName = iShaderName;
                       onNewFragmentShader(shader);
                     });
  }
  else
    onNewFragmentShader(iShader);
}

//------------------------------------------------------------------------
// MainWindow::setCurrentFragmentShader
//------------------------------------------------------------------------
void MainWindow::setCurrentFragmentShader(std::shared_ptr<FragmentShader> iFragmentShader)
{
  fCurrentFragmentShader = std::move(iFragmentShader);
  if(fLayoutManual)
    fFragmentShaderWindow->resize(fCurrentFragmentShader->getWindowSize());
  else
    fCurrentFragmentShader->setWindowSize(fFragmentShaderWindow->getSize());
  fFragmentShaderWindow->setCurrentFragmentShader(fCurrentFragmentShader);
  auto title = fmt::printf("WebGPU Shader Toy | %s", fCurrentFragmentShader->getName());
  setTitle(title);
  fFragmentShaderWindow->setTitle(title);
}

//------------------------------------------------------------------------
// MainWindow::beforeFrame
//------------------------------------------------------------------------
void MainWindow::beforeFrame()
{
  auto actions = std::move(fBeforeImGuiFrameActions);
  for(auto &action: actions)
    action();

  ImGuiWindow::beforeFrame();

  if(fSetFontSizeRequest)
  {
    setFontSize(*fSetFontSizeRequest);
    fSetFontSizeRequest = std::nullopt;
    // forcing rebuild of device (where the font is created)
    doHandleFrameBufferSizeChange(getFrameBufferSize());
  }

//  if(fAspectRatioRequest)
//  {
//    fFragmentShaderWindow->setAspectRatio(*fAspectRatioRequest);
//    fAspectRatioRequest = std::nullopt;
//  }
  fFragmentShaderWindow->beforeFrame();
}

//------------------------------------------------------------------------
// MainWindow::afterFrame
//------------------------------------------------------------------------
void MainWindow::afterFrame()
{
  Renderable::afterFrame();
  auto time = glfwGetTime();
  // check every 10s
  if(fLastComputedStateTime + 10.0 < time)
  {
//    printf("Checking... [%f], [%f]\n", fLastComputedStateTime, time);
    auto state = computeState();
    auto serializedState = Preferences::serialize(state);
    if(serializedState != fLastComputedState)
    {
//      printf("Different... \n[%s] \n!=\n [%s]\n", fLastComputedState.c_str(), serializedState.c_str());
      fLastComputedState = serializedState;
      fPreferences->storeState(Preferences::kStateKey, state);
    }
    fLastComputedStateTime = time;
  }
}

//------------------------------------------------------------------------
// MainWindow::render
//------------------------------------------------------------------------
void MainWindow::render()
{
  Renderable::render();
  fFragmentShaderWindow->render();
}

//------------------------------------------------------------------------
// MainWindow::saveState
//------------------------------------------------------------------------
void MainWindow::saveState()
{
  fPreferences->storeState(Preferences::kStateKey, computeState());
}

//------------------------------------------------------------------------
// MainWindow::computeStateSettings
//------------------------------------------------------------------------
State::Settings MainWindow::computeStateSettings() const
{
  return {
    .fMainWindowSize = getSize(),
    .fFragmentShaderWindowSize = fFragmentShaderWindow->getSize(),
    .fDarkStyle = fDarkStyle,
    .fHiDPIAware = fFragmentShaderWindow->isHiDPIAware(),
    .fLayoutManual = fLayoutManual,
    .fLayoutSwapped = fLayoutSwapped,
    .fFontSize = fFontSize,
    .fLineSpacing = fLineSpacing,
    .fCodeShowWhiteSpace = fCodeShowWhiteSpace,
    .fScreenshotMimeType = fScreenshotFormat.fMimeType,
    .fScreenshotQualityPercent = fScreenshotQualityPercent,
  };
}

//------------------------------------------------------------------------
// MainWindow::computeStateShaders
//------------------------------------------------------------------------
State::Shaders MainWindow::computeStateShaders() const
{
  State::Shaders state{
    .fCurrent = fCurrentFragmentShader
                ? std::optional<std::string>(fCurrentFragmentShader->getName())
                : std::nullopt
  };

  for(auto const &shader: fFragmentShaders)
  {
    state.fList.emplace_back(Shader{
      .fName = shader->getName(),
      .fCode = shader->getCode(),
      .fEditedCode = shader->getEditedCode(),
      .fWindowSize = shader->getWindowSize()}
    );
  }

  return state;
}

//------------------------------------------------------------------------
// MainWindow::computeState
//------------------------------------------------------------------------
State MainWindow::computeState() const
{
  return {
    .fSettings = computeStateSettings(),
    .fShaders = computeStateShaders()
  };
}

//------------------------------------------------------------------------
// MainWindow::setStyle
//------------------------------------------------------------------------
void MainWindow::setStyle(bool iDarkStyle)
{
  fDarkStyle = iDarkStyle;
  auto style = ImGui::GetStyle();
  if(fDarkStyle)
    ImGui::StyleColorsDark(&style);
  else
    ImGui::StyleColorsLight(&style);
  ImGui::GetStyle() = style;
  JSSetStyle(iDarkStyle);
}

//------------------------------------------------------------------------
// MainWindow::setManualLayout
//------------------------------------------------------------------------
void MainWindow::setManualLayout(bool iManualLayout, std::optional<Size> iLeftPaneSize, std::optional<Size> iRightPaneSize)
{
  fLayoutManual = iManualLayout;
  auto leftPaneWidth = iLeftPaneSize ? iLeftPaneSize->width : getSize().width;
  auto rightPaneWidth = iRightPaneSize ? iRightPaneSize->width : fFragmentShaderWindow->getSize().width;
  JSSetLayout(iManualLayout, leftPaneWidth, rightPaneWidth);
}


//------------------------------------------------------------------------
// MainWindow::switchToManualLayout
//------------------------------------------------------------------------
void MainWindow::switchToManualLayout()
{
  deferBeforeImGuiFrame([this] { setManualLayout(true); });
}

//------------------------------------------------------------------------
// MainWindow::switchToAutomaticLayout
//------------------------------------------------------------------------
void MainWindow::switchToAutomaticLayout()
{
  // when switching to automatic, we use the same size for both panes to make it even split
  deferBeforeImGuiFrame([this, size = getSize()] { setManualLayout(false, size, size); });
}

//------------------------------------------------------------------------
// MainWindow::swapLayout
//------------------------------------------------------------------------
void MainWindow::swapLayout()
{
  fLayoutSwapped = !fLayoutSwapped;
  setWindowOrder();
}

//------------------------------------------------------------------------
// MainWindow::setWindowOrder
//------------------------------------------------------------------------
void MainWindow::setWindowOrder()
{
  auto leftWindow = asOpaquePtr();
  auto rightWindow = fFragmentShaderWindow->asOpaquePtr();

  if(fLayoutSwapped)
    std::swap(leftWindow, rightWindow);

  deferBeforeImGuiFrame([leftWindow, rightWindow] { JSSetWindowOrder(leftWindow, rightWindow); });
}

//------------------------------------------------------------------------
// MainWindow::onNewFile
//------------------------------------------------------------------------
void MainWindow::onNewFile(char const *iFilename, char const *iContent)
{
  if(iFilename == nullptr || iContent == nullptr)
    return;

  std::string filename = iFilename;
  if(impl::ends_with(filename, ".json"))
    loadFromState(filename, Preferences::deserialize(iContent, State{.fSettings = computeStateSettings()}));
  else
  {
    // remove the extension...
    if(impl::ends_with(filename, ".wgsl"))
      filename = filename.substr(0, filename.find_last_of('.'));

    maybeNewFragmentShader("Import Shader", "Continue", {filename, iContent});
  }
}

//------------------------------------------------------------------------
// MainWindow::newAboutDialog
//------------------------------------------------------------------------
void MainWindow::newAboutDialog()
{
  newDialog("WebGPU Shader Toy | About")
    .content([] {
      ImGui::SeparatorText("About");
      ImGui::Text("WebGPU Shader Toy is a tool developed by pongasoft.");
      ImGui::Text("Its main purpose is to experiment with WebGPU fragment shaders.");
      ImGui::Text("And to have fun while doing it :)");
      ImGui::SeparatorText("Versions");
      ImGui::Text("Version:    %s", kFullVersion);
      ImGui::Text("emscripten: %d.%d.%d", __EMSCRIPTEN_major__, __EMSCRIPTEN_minor__, __EMSCRIPTEN_tiny__);
      ImGui::Text("ImGui:      %s", IMGUI_VERSION);
      ImGui::Text("GLFW:       %s", glfwGetVersionString());
    })
    .allowDismissDialog()
    .buttonOk();
}

//------------------------------------------------------------------------
// MainWindow::newHelpDialog
//------------------------------------------------------------------------
void MainWindow::newHelpDialog()
{
  newDialog("Help")
    .content([this]() { help(); })
    .allowDismissDialog()
    .buttonOk();
}

//------------------------------------------------------------------------
// MainWindow::help
//------------------------------------------------------------------------
void MainWindow::help() const
{
  using help_t = std::vector <std::tuple<char const *, std::vector<char const *>>>;

  static const help_t kIcons = {
    {fa::kClockRotateLeft, {"Reset time/frame"}},
    {fa::kBackwardFast, {"Steps backward in time (-12 frames) | Hold to repeat"}},
    {ICON_FA_CirclePause " / " ICON_FA_CirclePlay, {"Pause/Play time/frame"}},
    {fa::kForwardFast, {"Steps forward in time (+12 frames) | Hold to repeat"}},
    {fa::kCameraPolaroid, {"Take an instant screenshot"}},
    {fa::kExpandWide, {"Enter fullscreen (widescreen) (ESC to exit)"}},
    {fa::kPowerOff, {"Disable/Enable shader rendering"}},
  };

  static const help_t kAltIcons = {
    {fa::kBackward, {"Steps backward in time (-1 frames) | Hold to repeat"}},
    {fa::kForward, {"Steps forward in time (+1 frame) | Hold to repeat"}},
    {fa::kCamera, {"Open the menu to take a screenshot (choose format)"}},
    {fa::kExpand, {"Enter fullscreen (ESC to exit)"}},
  };

  // for the time being we support only Windows shortcuts because the Command key does not work properly in
  // browsers
  static const help_t kShortcuts = {
    {"Ctrl + D", {"Compile the shader"}},
    {"Ctrl + C", {"Copy selection"}},
    {"Ctrl + X", {"Cut selection / Cut Line (no selection)"}},
    {"Ctrl + V", {"Paste"}},
    {"Ctrl + Z", {"Undo"}},
    {"Ctrl + Shift + Z", {"Redo"}},
    {"Ctrl + Shift + A", {"Select All"}},
    {"Ctrl + [ or ]", {"Indentation change"}},
    {"Ctrl + /", {"Toggle line comment"}},
    {"Ctrl + A", {"Beginning of line"}},
    {"Ctrl + E", {"End of line"}},
    {"Home or End", {"Beginning or End of line"}},
    {"<Nav. Key>", {"Arrows, Home, End, PgUp, PgDn: move cursor"}},
    {"Shift + <Nav. Key>", {"Select text"}},
  };

  static const help_t kAppleShortcuts = {
    {"Cmd + D", {"Compile the shader"}},
    {"Cmd + C", {"Copy selection"}},
    {"Cmd + X", {"Cut selection / Cut Line (no selection)"}},
    {"Cmd + V", {"Paste"}},
    {"Cmd + Z", {"Undo"}},
    {"Cmd + Shift + Z", {"Redo"}},
    {"Cmd + Shift + A", {"Select All"}},
    {"Cmd + [ or ]", {"Indentation change"}},
    {"Cmd + /", {"Toggle line comment"}},
    {"Cmd|Ctrl + A", {"Beginning of line"}},
    {"Cmd|Ctrl + E", {"End of line"}},
    {"Home or End", {"Beginning or End of line"}},
    {"<Nav. Key>", {"Arrows, Home, End, PgUp, PgDn: move cursor"}},
    {"Shift + <Nav. Key>", {"Select text"}},
  };

  static const help_t kShaderInputs = {
    {"ShaderToyInputs", {
                          "struct ShaderToyInputs {",
                          "  size:  vec4f,",
                          "  mouse: vec4f,",
                          "  time:  f32,",
                          "  frame: i32,",
                          "};",
                          "@group(0) @binding(0)",
                          "var<uniform> inputs: ShaderToyInputs;"
                        }},
    {"inputs.size.xy", {"size of the viewport (in pixels)"}},
    {"inputs.size.zw", {"scale ((1.0,1.0) for low res)"}},
    {"inputs.mouse.xy", {"mouse position (in viewport coordinates)"}},
    {"inputs.mouse.zw", {"position where LMB was pressed ((-1,-1) if not pressed)"}},
    {"inputs.time", {"time in seconds (since start/reset)"}},
    {"inputs.frame", {"frame count (since start/reset)"}},
  };


  auto renderHelp = [](help_t const &iHelp, char const *iSectionName, char const *iColumnName) {
    ImGui::SeparatorText(iSectionName);
    if(ImGui::BeginTable(iSectionName, 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV))
    {
      ImGui::TableSetupColumn(iColumnName);
      ImGui::TableSetupColumn("Description");
      ImGui::TableHeadersRow();

      for(auto &entry: iHelp)
      {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(std::get<0>(entry));
        ImGui::TableSetColumnIndex(1);
        for(auto const &item: std::get<1>(entry))
        {
          ImGui::TextUnformatted(item);
        }
      }
      ImGui::EndTable();
    }
  };

  renderHelp(kIcons, "Icons", "Icons");
  renderHelp(kAltIcons, "Alternative Icons (Hold Alt Key)", "Icons");
  renderHelp(fIsRuntimePlatformApple ? kAppleShortcuts : kShortcuts, "Editor Keyboard Shortcuts", "Shortcuts");
  renderHelp(kShaderInputs, "Shader Inputs", "Inputs");

}

//------------------------------------------------------------------------
// MainWindow::getShortcutString
//------------------------------------------------------------------------
char const *MainWindow::getShortcutString(char const *iKey, char const *iFormat)
{
  static char kShortcut[256]{};
  snprintf(kShortcut, 256, iFormat, fIsRuntimePlatformApple ? "Cmd" : "Ctrl", iKey);
  return kShortcut;
}

namespace impl {
//------------------------------------------------------------------------
// impl::RenderUndoAction
//------------------------------------------------------------------------
bool RenderUndoAction(pongasoft::utils::Action const *iAction, bool iSelected)
{
  bool res = false;

  ImGui::PushID(iAction);
  if(ImGui::Selectable(iAction->fDescription.c_str(), iSelected))
    res = true;
  ImGui::PopID();

//  if(ReGui::ShowQuickView())
//  {
//    if(auto c = dynamic_cast<const CompositeAction *>(iAction))
//    {
//      ReGui::ToolTip([c] { RenderUndoAction(c); });
//    }
//    else
//    {
//      ReGui::ToolTip([] { ImGui::TextUnformatted("No details"); });
//    }
//  }


  return res;
}

}

//------------------------------------------------------------------------
// MainWindow::renderHistory
//------------------------------------------------------------------------
void MainWindow::renderHistory()
{
  if(ImGui::MenuItem("Undo", nullptr, nullptr, fUndoManager.hasUndoHistory()))
    deferBeforeImGuiFrame([mgr = &fUndoManager] { mgr->undoLastAction(); });
  if(ImGui::MenuItem("Redo", nullptr, nullptr, fUndoManager.hasRedoHistory()))
    deferBeforeImGuiFrame([mgr = &fUndoManager] { mgr->redoLastAction(); });
  if(ImGui::MenuItem("Clear", nullptr, nullptr, fUndoManager.hasHistory()))
    fUndoManager.clear();
  ImGui::SeparatorText("History");
  auto const &undoHistory = fUndoManager.getUndoHistory();
  auto const &redoHistory = fUndoManager.getRedoHistory();
  if(redoHistory.empty() && undoHistory.empty())
  {
    ImGui::TextUnformatted("<empty>");
  }
  else
  {
    pongasoft::utils::Action *undoAction = nullptr;
    pongasoft::utils::Action *redoAction = nullptr;
    if(!redoHistory.empty())
    {
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
      for(const auto &i : redoHistory)
      {
        auto action = i.get();
        if(impl::RenderUndoAction(action, false))
          redoAction = action;
      }
      ImGui::PopStyleVar();
    }
    if(!undoHistory.empty())
    {
      auto currentUndoAction = fUndoManager.getLastUndoAction();
      for(const auto &i : std::ranges::reverse_view(undoHistory))
      {
        auto action = i.get();
        if(impl::RenderUndoAction(action, currentUndoAction == action))
          undoAction = action;
      }
    }
    if(ImGui::Selectable("<empty>", fUndoManager.getLastUndoAction() == nullptr))
      deferBeforeImGuiFrame([mgr = &fUndoManager] { mgr->undoAll(); });
    if(undoAction)
      deferBeforeImGuiFrame([mgr = &fUndoManager, undoAction] { mgr->undoUntil(undoAction); });
    if(redoAction)
      deferBeforeImGuiFrame([mgr = &fUndoManager, redoAction] { mgr->redoUntil(redoAction); });
  }
}

//------------------------------------------------------------------------
// image::format::getFormatFromMimeType
//------------------------------------------------------------------------
image::format::Format image::format::getFormatFromMimeType(std::string const &iMimeType)
{
  auto iter = std::find_if(std::begin(image::format::kAll),
                           std::end(image::format::kAll),
                           [&iMimeType](auto &f) { return f.fMimeType == iMimeType; });
  if(iter != std::end(image::format::kAll))
    return *iter;
  else
    return image::format::kPNG;
}

}
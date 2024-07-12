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
#include "JetBrainsMono-Regular.cpp"
#include "IconsFontWGPUShaderToy.h"
#include "IconsFontWGPUShaderToy.cpp"
#include "Errors.h"
#include "utils/DataManager.h"
#include <iostream>
#include <emscripten.h>
#include <version.h>


namespace shader_toy {
extern "C" {
using OnNewFileHandler = void (*)(MainWindow *iMainWindow, char const *iFilename, char const *iContent);
using OnBeforeUnloadHandler = void (*)(MainWindow *iMainWindow);
using OnClipboardStringHandler = void (*)(MainWindow *iMainWindow, void *iRequestUserData, char const *iClipboardString);

void wgpu_shader_toy_install_handlers(OnNewFileHandler iOnNewFileHandler,
                                      OnBeforeUnloadHandler iOnBeforeUnloadHandler,
                                      OnClipboardStringHandler iOnClipboardStringHandler,
                                      MainWindow *iMainWindow);

void wgpu_shader_toy_uninstall_handlers();
void wgpu_shader_toy_open_file_dialog();
void wgpu_shader_toy_get_clipboard_string(void *iUserData);
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

//------------------------------------------------------------------------
// callbacks::OnClipboardStringCallback
//------------------------------------------------------------------------
void OnClipboardStringCallback(MainWindow *iMainWindow, void *iRequestUserData, char const *iClipboardString)
{
  iMainWindow->onClipboardString(iRequestUserData, iClipboardString);
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
EM_JS(void, JSSetStyle, (bool iDarkStyle), {
  Module.wst_set_style(iDarkStyle);
})

//------------------------------------------------------------------------
// JSSetStyle
//------------------------------------------------------------------------
EM_JS(void, JSSetManualLayout, (bool iManualLayout), {
  Module.wst_set_manual_layout(iManualLayout);
})


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
  initFromState(iMainWindowArgs.state);

  setFontSize(iMainWindowArgs.state.fFontSize);
  fSetFontSizeRequest = std::nullopt;

  wgpu_shader_toy_install_handlers(callbacks::OnNewFileCallback,
                                   callbacks::OnBeforeUnload,
                                   callbacks::OnClipboardStringCallback,
                                   this);
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
      .lambda([this] {
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
        .lambda([this] { ImGui::SliderFloat("###line_spacing", &fLineSpacing, 1.0f, 2.0f); })
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
    if(ImGui::MenuItem("Manual", nullptr, fManualLayout))
    {
      if(!fManualLayout)
        setManualLayout(true);
    }
    if(ImGui::MenuItem("Automatic", nullptr, !fManualLayout))
    {
      if(fManualLayout)
        setManualLayout(false);
    }
    ImGui::EndMenu();
  }
}

//------------------------------------------------------------------------
// MainWindow::renderMainMenuBar
//------------------------------------------------------------------------
void MainWindow::renderMainMenuBar()
{
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
      if(ImGui::MenuItem("Reset"))
        fResetRequest = true;
#ifndef NDEBUG
      ImGui::Separator();
      if(ImGui::MenuItem("Quit"))
        stop();
#endif
      ImGui::EndMenu();
    }

    ImGui::Text("|");

    if(fCurrentFragmentShader)
    {
      if(ImGui::BeginMenu("Shader"))
      {
        // defined later...
        ImGui::EndMenu();
      }
    }

    if(ImGui::BeginMenu("Examples"))
    {
      for(auto &shader: kFragmentShaderExamples)
      {
        if(ImGui::MenuItem(shader.fName.c_str()))
        {
          onNewFragmentShader(shader);
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
}
//------------------------------------------------------------------------
// MainWindow::renderTimeControls
//------------------------------------------------------------------------
void MainWindow::renderTimeControls()
{
  // Reset time
  if(ImGui::Button(fa::kClockRotateLeft))
    fCurrentFragmentShader->setStartTime(getCurrentTime());

  ImGui::SameLine();

  // Previous frame
  ImGui::BeginDisabled(fCurrentFragmentShader->getInputs().frame == 0);
  ImGui::PushButtonRepeat(true);
  if(ImGui::Button(fa::kBackwardStep))
    fCurrentFragmentShader->previousFrame(getCurrentTime(), ImGui::GetIO().KeyAlt ? 1 : 12);
  ImGui::PopButtonRepeat();
  ImGui::EndDisabled();

  ImGui::SameLine();

  // Play / Pause
  if(ImGui::Button(fCurrentFragmentShader->isRunning() ? fa::kCirclePause : fa::kCirclePlay))
    fCurrentFragmentShader->toggleRunning(getCurrentTime());

  ImGui::SameLine();

  // Next frame
  ImGui::PushButtonRepeat(true);
  if(ImGui::Button(fa::kForwardStep))
    fCurrentFragmentShader->nextFrame(getCurrentTime(), ImGui::GetIO().KeyAlt ? 1 : 12);
  ImGui::PopButtonRepeat();
}

//------------------------------------------------------------------------
// MainWindow::renderControlsSection
//------------------------------------------------------------------------
void MainWindow::renderControlsSection()
{
  // [Section] Controls
  ImGui::SeparatorText("Controls");

  // Time controls
  renderTimeControls();

  ImGui::SameLine();

  // Screenshot
  if(ImGui::Button(fa::kCamera))
    maybePromptSaveCurrentFragmentShaderScreenshot();

  ImGui::SameLine();

  // Fullscreen
  if(ImGui::Button(fa::kExpand))
    fFragmentShaderWindow->requestFullscreen();

  ImGui::SameLine();

  ImGui::Text("%.3f (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
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
  static std::string kShaderName{};

  kShaderName.clear();

  newDialog("New Shader | Name")
    .lambda([] {
      ImGui::InputText("###name", &kShaderName);
    })
    .button("Create", [this] {
      onNewFragmentShader({kShaderName, kEmptyShader});
    }, true)
    .buttonCancel()
    ;
}

//------------------------------------------------------------------------
// MainWindow::promptRenameCurrentShader
//------------------------------------------------------------------------
void MainWindow::promptRenameCurrentShader()
{
  static std::string kNewShaderName{};

  if(!fCurrentFragmentShader)
    return;

  auto const currentName = fCurrentFragmentShader->getName();

  kNewShaderName = fCurrentFragmentShader->getName();

  newDialog("Rename Shader")
    .lambda([] {
      ImGui::InputText("###name", &kNewShaderName);
    })
    .button("Rename", [currentName, this] {
      renameShader(currentName, kNewShaderName);
    }, true)
    .buttonCancel()
    ;
}

//------------------------------------------------------------------------
// MainWindow::promptShaderWindowSize
//------------------------------------------------------------------------
void MainWindow::promptShaderWindowSize()
{
  static Renderable::Size kSize{};

  kSize = fFragmentShaderWindow->getSize();

  newDialog("Shader Window Size")
    .lambda([] {
      ImGui::InputInt2("size", &kSize.width);
      ImGui::SetItemDefaultFocus();
    })
    .button("Resize", [this] {
      if(!fManualLayout)
        setManualLayout(true);
      fFragmentShaderWindow->resize(kSize);
    }, false)
    .buttonCancel()
    ;
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
  static std::string kShaderFilename{};

  kShaderFilename = impl::ends_with(iFilename, ".wgsl") ? iFilename : fmt::printf("%s.wgsl", iFilename);

  newDialog("Export Shader")
    .preContentMessage("Enter/Modify the filename")
    .lambda([] {
      ImGui::InputText("###name", &kShaderFilename);
    })
    .button("Export", [content = iContent] {
      wgpu_shader_toy_export_content(kShaderFilename.c_str(), content.c_str());
    }, true)
    .buttonCancel()
    ;
}


//------------------------------------------------------------------------
// MainWindow::promptExportShader
//------------------------------------------------------------------------
void MainWindow::promptExportProject()
{
  static std::string kProjectName{};

  kProjectName = "WebGPUShaderToy.json";

  newDialog("Export Project")
    .preContentMessage("Enter/Modify the filename")
    .lambda([] {
      ImGui::InputText("###name", &kProjectName);
    })
    .button("Export", [this] {
      wgpu_shader_toy_export_content(kProjectName.c_str(), fPreferences->serialize(computeState()).c_str());
    }, true)
    .buttonCancel()
    ;

}

//------------------------------------------------------------------------
// MainWindow::renameShader
//------------------------------------------------------------------------
void MainWindow::renameShader(std::string const &iOldName, std::string const &iNewName)
{
  auto oldShader = deleteFragmentShader(iOldName);
  if(oldShader)
  {
    oldShader->setName(iNewName);
    onNewFragmentShader(std::move(oldShader));
  }
}

namespace image::format {

struct Info
{
  std::string fMimeType;
  std::string fExtension;
  std::string fDescription;
  bool fHasQuality;
};

constexpr auto kPNG = Info { .fMimeType = "image/png", .fExtension = "png", .fDescription = "PNG", .fHasQuality = false};
constexpr auto kJPG = Info { .fMimeType = "image/jpeg", .fExtension = "jpg", .fDescription = "JPEG", .fHasQuality = true};
constexpr auto kWEBP = Info { .fMimeType = "image/webp", .fExtension = "webp", .fDescription = "WebP", .fHasQuality = true};

constexpr Info kAll[] = {kPNG, kJPG, kWEBP};

}


//------------------------------------------------------------------------
// MainWindow::maybePromptSaveCurrentFragmentShaderScreenshot
//------------------------------------------------------------------------
void MainWindow::maybePromptSaveCurrentFragmentShaderScreenshot()
{
  static std::string kFilename{};
  static auto kFormat = image::format::kPNG;
  static float kQuality = 0.85;

  if(!fCurrentFragmentShader)
    return;

  kFilename = fCurrentFragmentShader->getName();

  if(ImGui::GetIO().KeyAlt)
  {
    // bypasses prompt and saves using previous/default parameters
    fFragmentShaderWindow->saveScreenshot(fmt::printf("%s.%s", kFilename, kFormat.fExtension), kFormat.fMimeType, kQuality);
  }
  else
  {
    newDialog("Screenshot")
      .lambda([this] {
        ImGui::SeparatorText("Filename");
        auto label = fmt::printf(".%s###name", kFormat.fExtension);
        ImGui::InputText(label.c_str(), &kFilename);
        ImGui::SeparatorText("Format");
        if(ImGui::BeginCombo("Format", kFormat.fDescription.c_str()))
        {
          for(auto &format: image::format::kAll)
          {
            if(ImGui::Selectable(format.fDescription.c_str(), kFormat.fMimeType == format.fMimeType))
              kFormat = format;
          }
          ImGui::EndCombo();
        }
        if(kFormat.fHasQuality)
        {
          auto quality = static_cast<int>(kQuality * 100);
          if(ImGui::SliderInt("Quality", &quality, 1, 100, "%d%%"))
            kQuality = static_cast<float>(quality) / 100.f;
        }
        ImGui::SeparatorText("Time Controls");
        if(fCurrentFragmentShader)
          renderTimeControls();
      })
      .button(ICON_FA_Camera " Screenshot", [this] {
        fFragmentShaderWindow->saveScreenshot(fmt::printf("%s.%s", kFilename, kFormat.fExtension), kFormat.fMimeType, kQuality);
      }, true)
      .buttonCancel()
      ;
  }

}

//------------------------------------------------------------------------
// MainWindow::renderShaderMenu
//------------------------------------------------------------------------
void MainWindow::renderShaderMenu(TextEditor &iEditor, std::string const &iNewNode, bool iEdited)
{
  if(ImGui::BeginMenu("Shader"))
  {
    ImGui::SeparatorText("Code");
    if(ImGui::MenuItem(ICON_FA_Hammer " Compile", "Ctrl + SPACE", false, iEdited))
      compile(iNewNode);
    ImGui::SeparatorText("Edit");
    if(ImGui::MenuItem("Copy", "Ctrl + C"))
      iEditor.Copy();
    if(ImGui::MenuItem("Cut", "Ctrl + X"))
      iEditor.Cut();
    if(ImGui::MenuItem("Paste", "Ctrl + V"))
      iEditor.OnKeyboardPaste();
    if(ImGui::MenuItem("Undo", "Ctrl + Z"))
      iEditor.Undo();
    if(ImGui::MenuItem("Redo", "Shift + Ctrl + Z"))
      iEditor.Redo();
    ImGui::SeparatorText("Misc");
    if(ImGui::MenuItem("Rename"))
      promptRenameCurrentShader();
    if(ImGui::MenuItem("Resize"))
      promptShaderWindowSize();
    if(ImGui::MenuItem("Export (to disk)"))
      promptExportShader(fCurrentFragmentShader->getName(), iNewNode);
//            if(ImGui::MenuItem("Revert Changes", nullptr, false, edited))
//              editor.SetText(fCurrentFragmentShader->getCode());
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
    if(iEditorHasFocus && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Space))
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
      long lines = hasCompilationError ? impl::lineCount(fCurrentFragmentShader->getCompilationErrorMessage()) + 1 : 1;
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
        ImGui::EndChild();
      }
      ImGui::PopStyleColor(hasCompilationError ? 2 : 1);

      // [Editor] Render
      editor.SetOnKeyboardPasteHandler([](TextEditor &iEditor) { wgpu_shader_toy_get_clipboard_string(&iEditor); });
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

  fCurrentDialog->render();
  if(!fCurrentDialog->isOpen())
    fCurrentDialog = nullptr;
}

//------------------------------------------------------------------------
// MainWindow::doRender
//------------------------------------------------------------------------
void MainWindow::doRender()
{
  auto const editorHasFocus = !ImGui::IsAnyItemActive() && !hasDialog();

  renderMainMenuBar();

  if(hasDialog())
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
      if(!fFragmentShaderTabs.empty())
      {
        std::string selectedFragmentShader = fFragmentShaderTabs[0];
        std::optional<std::string> fragmentShaderToDelete{};
        for(auto &fragmentShaderName: fFragmentShaderTabs)
        {
          bool open = true;
          auto flags = ImGuiTabItemFlags_None;
          if(fCurrentFragmentShaderNameRequest && *fCurrentFragmentShaderNameRequest == fragmentShaderName)
          {
            flags = ImGuiTabItemFlags_SetSelected;
            selectedFragmentShader = fragmentShaderName;
          }
          if(ImGui::BeginTabItem(fragmentShaderName.c_str(), &open, flags))
          {
            if(!fCurrentFragmentShaderNameRequest)
              selectedFragmentShader = fragmentShaderName;
            ImGui::EndTabItem();
          }
          if(!open)
            fragmentShaderToDelete = fragmentShaderName;
        }
        fCurrentFragmentShaderNameRequest = std::nullopt;
        if(fragmentShaderToDelete)
          deleteFragmentShader(*fragmentShaderToDelete);
        else
        {
          WST_INTERNAL_ASSERT(fCurrentFragmentShader != nullptr);
          if(fCurrentFragmentShader->getName() != selectedFragmentShader)
            setCurrentFragmentShader(fFragmentShaders[selectedFragmentShader]);
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
              onNewFragmentShader(shader);
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
void MainWindow::onNewFragmentShader(std::shared_ptr<FragmentShader> iFragmentShader)
{
  setCurrentFragmentShader(std::move(iFragmentShader));
  fFragmentShaders[fCurrentFragmentShader->getName()] = fCurrentFragmentShader;
  if(std::ranges::find(fFragmentShaderTabs, fCurrentFragmentShader->getName()) == fFragmentShaderTabs.end())
  {
    fFragmentShaderTabs.emplace_back(fCurrentFragmentShader->getName());
  }
  fCurrentFragmentShaderNameRequest = fCurrentFragmentShader->getName();
}

//------------------------------------------------------------------------
// MainWindow::onNewFragmentShader
//------------------------------------------------------------------------
void MainWindow::onNewFragmentShader(Shader const &iShader)
{
  auto shader = iShader;
  if(fFragmentShaders.contains(iShader.fName))
    shader.fWindowSize = fFragmentShaders[iShader.fName]->getWindowSize();
  else
    shader.fWindowSize = fFragmentShaderWindow->getSize();
  onNewFragmentShader(std::make_shared<FragmentShader>(shader));
}

//------------------------------------------------------------------------
// MainWindow::deleteFragmentShader
//------------------------------------------------------------------------
std::shared_ptr<FragmentShader> MainWindow::deleteFragmentShader(std::string const &iName)
{
  fFragmentShaderTabs.erase(std::remove(fFragmentShaderTabs.begin(), fFragmentShaderTabs.end(), iName),
                            fFragmentShaderTabs.end());
  std::shared_ptr<FragmentShader> oldShader{};
  auto iter = fFragmentShaders.find(iName);
  if(iter != fFragmentShaders.end())
  {
    oldShader = iter->second;
    fFragmentShaders.erase(iter);
  }
  if(!fFragmentShaders.empty())
  {
    if(!fCurrentFragmentShader || fCurrentFragmentShader->getName() != iName)
    {
      setCurrentFragmentShader(fFragmentShaders[fFragmentShaderTabs[0]]);
    }
  }
  else
  {
    fCurrentFragmentShader = nullptr;
    setTitle("WebGPU Shader Toy");
    fFragmentShaderWindow->setTitle("WebGPU Shader Toy");
  }

  return oldShader;
}


//------------------------------------------------------------------------
// MainWindow::setCurrentFragmentShader
//------------------------------------------------------------------------
void MainWindow::setCurrentFragmentShader(std::shared_ptr<FragmentShader> iFragmentShader)
{
  fCurrentFragmentShader = std::move(iFragmentShader);
  if(!fManualLayout)
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
  if(fResetRequest)
  {
    reset();
    fResetRequest = false;
  }

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
// MainWindow::reset
//------------------------------------------------------------------------
void MainWindow::reset()
{
  initFromState(fDefaultState);
}

//------------------------------------------------------------------------
// MainWindow::saveState
//------------------------------------------------------------------------
void MainWindow::saveState()
{
  fPreferences->storeState(Preferences::kStateKey, computeState());
}

//------------------------------------------------------------------------
// MainWindow::computeState
//------------------------------------------------------------------------
State MainWindow::computeState() const
{
  State state{
    .fMainWindowSize = getSize(),
    .fFragmentShaderWindowSize = fFragmentShaderWindow->getSize(),
    .fDarkStyle = fDarkStyle,
    .fHiDPIAware = fFragmentShaderWindow->isHiDPIAware(),
    .fManualLayout = fManualLayout,
    .fFontSize = fFontSize,
    .fLineSpacing = fLineSpacing,
    .fCodeShowWhiteSpace = fCodeShowWhiteSpace,
//    .fAspectRatio = fCurrentAspectRatio,
    .fCurrentShader = fCurrentFragmentShader
                      ? std::optional<std::string>(fCurrentFragmentShader->getName())
                      : std::nullopt
  };

  for(auto &shaderName: fFragmentShaderTabs)
  {
    auto const &shader = fFragmentShaders.at(shaderName);
    state.fShaders.emplace_back(Shader{.fName = shaderName, .fCode = shader->getCode(), .fWindowSize = shader->getWindowSize()});
  }

  return state;
}

//------------------------------------------------------------------------
// MainWindow::initFromState
//------------------------------------------------------------------------
void MainWindow::initFromState(State const &iState)
{
  requestFontSize(iState.fFontSize);
  setStyle(iState.fDarkStyle);
  setManualLayout(iState.fManualLayout);
  resize(iState.fMainWindowSize);
  if(fFragmentShaderWindow->isHiDPIAware() != iState.fHiDPIAware)
    fFragmentShaderWindow->toggleHiDPIAwareness();
  fLineSpacing = iState.fLineSpacing;
  fCodeShowWhiteSpace = iState.fCodeShowWhiteSpace;
//  if(fCurrentAspectRatio != iState.fAspectRatio)
//  {
//    auto iter = std::find_if(kAspectRatios.begin(), kAspectRatios.end(),
//                             [ar = iState.fAspectRatio](auto const &p) { return p.first == ar; });
//    if(iter != kAspectRatios.end())
//    {
//      fCurrentAspectRatio = iState.fAspectRatio;
//      fAspectRatioRequest = iter->second;
//    }
//  }
  fFragmentShaderWindow->resize(iState.fFragmentShaderWindowSize);
  fFragmentShaders.clear();
  fFragmentShaderTabs.clear();
  fCurrentFragmentShader = nullptr;

  for(auto &shader: iState.fShaders)
  {
    auto fragmentShader = std::make_shared<FragmentShader>(shader);
    if(!fFragmentShaders.contains(shader.fName))
    {
      fFragmentShaders[shader.fName] = fragmentShader;
      fFragmentShaderTabs.emplace_back(shader.fName);
      if(iState.fCurrentShader && iState.fCurrentShader.value() == shader.fName)
      {
        setCurrentFragmentShader(fragmentShader);
        fCurrentFragmentShaderNameRequest = shader.fName;
      }
    }
  }
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
void MainWindow::setManualLayout(bool iManualLayout)
{
  fManualLayout = iManualLayout;
  JSSetManualLayout(iManualLayout);
}

//------------------------------------------------------------------------
// MainWindow::newDialog
//------------------------------------------------------------------------
gui::Dialog &MainWindow::newDialog(std::string iTitle, bool iHighPriority)
{
  auto dialog = std::make_unique<gui::Dialog>(std::move(iTitle));
  if(iHighPriority)
  {
    fCurrentDialog = std::move(dialog);
    return *fCurrentDialog;
  }
  else
  {
    fDialogs.emplace_back(std::move(dialog));
    return *fDialogs[fDialogs.size() - 1];
  }
}

//------------------------------------------------------------------------
// MainWindow::onClipboardString
//------------------------------------------------------------------------
void MainWindow::onClipboardString(void *iRequestUserData, char const *iClipboardString)
{
  if(fCurrentFragmentShader)
  {
    auto &editor = fCurrentFragmentShader->edit();
    if(&editor == iRequestUserData)
    {
      if(iClipboardString)
        editor.Paste(iClipboardString);
      else
        editor.Paste();
    }
  }
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
    initFromState(Preferences::deserialize(iContent, fDefaultState));
  else
    onNewFragmentShader({filename, iContent});
}

//------------------------------------------------------------------------
// MainWindow::newAboutDialog
//------------------------------------------------------------------------
void MainWindow::newAboutDialog()
{
  newDialog("WebGPU Shader Toy | About")
    .lambda([] {
      ImGui::SeparatorText("About");
      ImGui::Text("WebGPU Shader Toy is a tool developed by pongasoft.");
      ImGui::Text("Its main purpose is to experiment with WebGPU fragment shaders");
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
    .lambda([this]() { help(); })
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
    {fa::kBackwardStep, {"Steps backward in time (-12 frames) | Hold to repeat"}},
    {ICON_FA_BackwardStep " + " "Alt", {"Steps backward in time (-1 frame) | Hold to repeat"}},
    {ICON_FA_CirclePause " / " ICON_FA_CirclePlay, {"Pause/Play time/frame"}},
    {fa::kForwardStep, {"Steps forward in time (+12 frames) | Hold to repeat"}},
    {ICON_FA_ForwardStep " + " "Alt", {"Steps forward in time (+1 frame) | Hold to repeat"}},
    {fa::kCamera, {"Take a screenshot | Use Alt to bypass prompt"}},
    {fa::kExpand, {"Enter fullscreen (ESC to exit)"}},
  };

  // for the time being we support only Windows shortcuts because the Command key does not work properly in
  // browsers
  static const help_t kShortcuts = {
    {"Ctrl + SPACE", {"Compile the shader"}},
    {"Ctrl + C", {"Copy selection"}},
    {"Ctrl + X", {"Cut selection / Cut Line (no selection)"}},
    {"Ctrl + V", {"Paste"}},
    {"Ctrl + Z", {"Undo"}},
    {"Ctrl + Shift + Z", {"Redo"}},
    {"Ctrl + A", {"Select All"}},
    {"Ctrl + [ or ]", {"Indentation change"}},
    {"Ctrl + /", {"Toggle line comment"}},
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
                          "@group(0) @binding(0) var<uniform> inputs: ShaderToyInputs;"
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
  renderHelp(kShortcuts, "Keyboard Shortcuts", "Shortcuts");
  renderHelp(kShaderInputs, "Shader Inputs", "Inputs");

}

}
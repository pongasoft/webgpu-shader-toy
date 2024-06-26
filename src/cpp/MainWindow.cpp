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
#include <GLFW/glfw3.h>
#include "MainWindow.h"
#include "JetBrainsMono-Regular.cpp"
#include "Errors.h"
#include <ranges>
#include <iostream>
#include <sstream>

namespace shader_toy {
extern "C" {
using OnNewFragmentShaderHandler = void (*)(MainWindow *iMainWindow, char const *iFilename, char const *iContent);
using OnBeforeUnloadHandler = void (*)(MainWindow *iMainWindow);

void wgpu_shader_toy_install_handlers(OnNewFragmentShaderHandler iOnNewFragmentShaderHandler,
                                      OnBeforeUnloadHandler iOnBeforeUnloadHandler, MainWindow *iMainWindow);

void wgpu_shader_toy_uninstall_handlers();

void wgpu_shader_toy_open_file_dialog();
}

namespace callbacks {
//------------------------------------------------------------------------
// callbacks::OnNewFragmentShaderCallback
//------------------------------------------------------------------------
void OnNewFragmentShaderCallback(MainWindow *iMainWindow, char const *iFilename, char const *iContent)
{
  iMainWindow->onNewFragmentShader({iFilename, iContent});
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

// kAspectRatios
static std::vector<std::pair<std::string, Window::AspectRatio>> kAspectRatios{
  {"Free", Window::AspectRatio{GLFW_DONT_CARE, GLFW_DONT_CARE}},
  {"1x1", Window::AspectRatio{1, 1}},
  {"16x9", Window::AspectRatio{16, 9}},
  {"9x16", Window::AspectRatio{9, 16}},
  {"4x3", Window::AspectRatio{4, 3}},
  {"3x4", Window::AspectRatio{3, 4}},
};

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

  auto &io = ImGui::GetIO();
  io.Fonts->Clear();
  ImFontConfig fontConfig;
  fontConfig.OversampleH = 2;
  float fontScale; float dummy;
  glfwGetWindowContentScale(fWindow, &fontScale, &dummy);
  io.Fonts->AddFontFromMemoryCompressedBase85TTF(JetBrainsMonoRegular_compressed_data_base85, 13.0f * fontScale, &fontConfig);
  io.FontGlobalScale = 1.0f / fontScale;
  wgpu_shader_toy_install_handlers(callbacks::OnNewFragmentShaderCallback, callbacks::OnBeforeUnload, this);
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
// MainWindow::doRenderMainMenuBar
//------------------------------------------------------------------------
void MainWindow::doRenderMainMenuBar()
{
  if(ImGui::BeginMainMenuBar())
  {
    if(ImGui::BeginMenu("WebGPU Shader Toy"))
    {
      if(ImGui::MenuItem("Reset"))
        fResetRequest = true;
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
      if(ImGui::BeginMenu("Code"))
      {
        if(ImGui::BeginMenu("Line Spacing"))
        {
          ImGui::SliderFloat("###line_spacing", &fLineSpacing, 1.0f, 2.0f);
          ImGui::EndMenu();
        }
        ImGui::MenuItem("Show White Space", nullptr, &fCodeShowWhiteSpace);
        ImGui::EndMenu();
      }
      if(ImGui::MenuItem("Save"))
        saveState();
      if(ImGui::MenuItem("Quit"))
        stop();
      ImGui::EndMenu();
    }
    if(ImGui::BeginMenu("Window"))
    {
      if(ImGui::MenuItem("Hi DPI", nullptr, fFragmentShaderWindow->isHiDPIAware()))
      {
        fFragmentShaderWindow->toggleHiDPIAwareness();
      }
      if(ImGui::BeginMenu("Aspect Ratio"))
      {
        for(auto &[name, aspectRatio]: kAspectRatios)
        {
          if(ImGui::MenuItem(name.c_str(), nullptr, name == fCurrentAspectRatio))
          {
            fCurrentAspectRatio = name;
            fAspectRatioRequest = aspectRatio;
          }
        }
        ImGui::EndMenu();
      }
      ImGui::EndMenu();
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
// MainWindow::doRenderControlsSection
//------------------------------------------------------------------------
void MainWindow::doRenderControlsSection()
{
  // [Section] Controls
  ImGui::SeparatorText("Controls");

  if(ImGui::Button("Reset Time"))
  {
    fCurrentFragmentShader->setStartTime(getCurrentTime());
  }
  ImGui::SameLine();
  if(ImGui::Button(fCurrentFragmentShader->isRunning() ? "Pause" : "Play"))
  {
    fCurrentFragmentShader->toggleRunning(getCurrentTime());
  }
  ImGui::SameLine();
  if(ImGui::Button("Fullscreen"))
  {
    fFragmentShaderWindow->requestFullscreen();
  }
}

//------------------------------------------------------------------------
// MainWindow::doRenderShaderSection
//------------------------------------------------------------------------
void MainWindow::doRenderShaderSection()
{
  // [Section] Shader
  ImGui::SeparatorText("Shader");

  if(ImGui::BeginTabBar(fCurrentFragmentShader->getName().c_str()))
  {
    // [TabItem] Code
    if(ImGui::BeginTabItem("Code"))
    {
      auto &editor = fCurrentFragmentShader->edit();
      editor.SetPalette(fDarkStyle ? TextEditor::PaletteId::Dark : TextEditor::PaletteId::Light);
      editor.SetLineSpacing(fLineSpacing);
      editor.SetShowWhitespacesEnabled(fCodeShowWhiteSpace);

      // [Child] Menu / toolbar for text editor
      bool hasCompilationError = fCurrentFragmentShader->hasCompilationError();
      long lines = hasCompilationError ? impl::lineCount(fCurrentFragmentShader->getCompilationError()) + 1 : 1;
      ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImGui::GetStyle().Colors[ImGuiCol_ChildBg]);
      ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 1),
                                          ImVec2(FLT_MAX, ImGui::GetTextLineHeightWithSpacing() * static_cast<float>(lines)));
      if(ImGui::BeginChild("Menu Bar", {}, 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_HorizontalScrollbar))
      {
        if(ImGui::BeginMenuBar())
        {
          auto newCode = editor.GetText();
          auto edited = newCode != fCurrentFragmentShader->getCode();
          if(ImGui::BeginMenu("M"))
          {
            if(ImGui::MenuItem("Apply Changes", nullptr, false, edited))
            {
              fCurrentFragmentShader->updateCode(newCode);
              fFragmentShaderWindow->setCurrentFragmentShader(fCurrentFragmentShader);
            }
            if(ImGui::MenuItem("Revert Changes", nullptr, false, edited))
            {
              editor.SetText(fCurrentFragmentShader->getCode());
            }
            ImGui::EndMenu();
          }
          int lineCount, columnCount;
          editor.GetCursorPosition(lineCount, columnCount);
          ImGui::Text("%d/%d | %d lines", lineCount + 1, columnCount + 1, editor.GetLineCount());
          if(ImGui::Button("C"))
          {
            glfwSetClipboardString(fWindow, fCurrentFragmentShader->getCode().c_str());
          }
          if(edited)
          {
            if(ImGui::Button("A"))
            {
              fCurrentFragmentShader->updateCode(newCode);
              fFragmentShaderWindow->setCurrentFragmentShader(fCurrentFragmentShader);
            }
          }
          ImGui::EndMenuBar();
        }
        if(fCurrentFragmentShader->hasCompilationError())
        {
          ImGui::Text("%s", fCurrentFragmentShader->getCompilationError().c_str());
        }
        ImGui::EndChild();
      }
      ImGui::PopStyleColor();

      editor.Render("Code", true, {}, 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_HorizontalScrollbar);

      ImGui::EndTabItem();
    }

    // [TabItem] Inputs
    if(ImGui::BeginTabItem("Inputs"))
    {
      auto &inputs = fCurrentFragmentShader->getInputs();
      ImGui::Text(FragmentShader::kHeaderTemplate,
                  static_cast<int>(inputs.size.x), static_cast<int>(inputs.size.y), static_cast<int>(inputs.size.z), static_cast<int>(inputs.size.w), // size: vec4f
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
// MainWindow::doRender
//------------------------------------------------------------------------
void MainWindow::doRender()
{
  doRenderMainMenuBar();

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
            fCurrentFragmentShaderNameRequest = std::nullopt;
          }
          if(ImGui::BeginTabItem(fragmentShaderName.c_str(), &open, flags))
          {
            selectedFragmentShader = fragmentShaderName;
            ImGui::EndTabItem();
          }
          if(!open)
            fragmentShaderToDelete = fragmentShaderName;
        }
        if(fragmentShaderToDelete)
          deleteFragmentShader(*fragmentShaderToDelete);
        else
        {
          ASSERT(fCurrentFragmentShader != nullptr);
          if(fCurrentFragmentShader->getName() != selectedFragmentShader)
          {
            fCurrentFragmentShader = fFragmentShaders[selectedFragmentShader];
            fFragmentShaderWindow->setCurrentFragmentShader(fCurrentFragmentShader);
          }
        }
      }
      // + to add a new shader (from file)
      if(ImGui::TabItemButton("+"))
      {
        wgpu_shader_toy_open_file_dialog();
      }
      ImGui::EndTabBar();
    }

    if(fCurrentFragmentShader)
    {
      doRenderControlsSection();
      doRenderShaderSection();
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
  fCurrentFragmentShader = std::make_shared<FragmentShader>(iShader);
  fFragmentShaders[fCurrentFragmentShader->getName()] = fCurrentFragmentShader;
  if(std::ranges::find(fFragmentShaderTabs, fCurrentFragmentShader->getName()) == fFragmentShaderTabs.end())
  {
    fFragmentShaderTabs.emplace_back(fCurrentFragmentShader->getName());
  }
  fFragmentShaderWindow->setCurrentFragmentShader(fCurrentFragmentShader);
  fCurrentFragmentShaderNameRequest = fCurrentFragmentShader->getName();
}

//------------------------------------------------------------------------
// MainWindow::deleteFragmentShader
//------------------------------------------------------------------------
void MainWindow::deleteFragmentShader(std::string const &iName)
{
  fFragmentShaderTabs.erase(std::remove(fFragmentShaderTabs.begin(), fFragmentShaderTabs.end(), iName),
                            fFragmentShaderTabs.end());
  fFragmentShaders.erase(iName);
  if(!fFragmentShaders.empty())
  {
    if(!fCurrentFragmentShader || fCurrentFragmentShader->getName() != iName)
    {
      fCurrentFragmentShader = fFragmentShaders[fFragmentShaderTabs[0]];
      fFragmentShaderWindow->setCurrentFragmentShader(fCurrentFragmentShader);
    }
  }
  else
    fCurrentFragmentShader = nullptr;
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

  Window::beforeFrame();
  if(fAspectRatioRequest)
  {
    fFragmentShaderWindow->setAspectRatio(*fAspectRatioRequest);
    fAspectRatioRequest = std::nullopt;
  }
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
    .fLineSpacing = fLineSpacing,
    .fCodeShowWhiteSpace = fCodeShowWhiteSpace,
    .fAspectRatio = fCurrentAspectRatio,
    .fCurrentShader = fCurrentFragmentShader
                      ? std::optional<std::string>(fCurrentFragmentShader->getName())
                      : std::nullopt
  };

  for(auto &shaderName: fFragmentShaderTabs)
  {
    state.fShaders.emplace_back(Shader{.fName = shaderName, .fCode = fFragmentShaders.at(shaderName)->getCode()});
  }

  return state;
}

//------------------------------------------------------------------------
// MainWindow::initFromState
//------------------------------------------------------------------------
void MainWindow::initFromState(State const &iState)
{
  setStyle(iState.fDarkStyle);
  resize(iState.fMainWindowSize);
  if(fFragmentShaderWindow->isHiDPIAware() != iState.fHiDPIAware)
    fFragmentShaderWindow->toggleHiDPIAwareness();
  fLineSpacing = iState.fLineSpacing;
  fCodeShowWhiteSpace = iState.fCodeShowWhiteSpace;
  if(fCurrentAspectRatio != iState.fAspectRatio)
  {
    auto iter = std::find_if(kAspectRatios.begin(), kAspectRatios.end(),
                             [ar = iState.fAspectRatio](auto const &p) { return p.first == ar; });
    if(iter != kAspectRatios.end())
    {
      fCurrentAspectRatio = iState.fAspectRatio;
      fAspectRatioRequest = iter->second;
    }
  }
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
        printf("Detected current fragment shader: %s\n", shader.fName.c_str());
        fCurrentFragmentShader = fragmentShader;
        fFragmentShaderWindow->setCurrentFragmentShader(fCurrentFragmentShader);
        fCurrentFragmentShaderNameRequest = fCurrentFragmentShader->getName();
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
}
}
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
#include <ranges>
#include <iostream>
#include <sstream>

namespace shader_toy {

extern "C" {
using OnNewFragmentShaderHandler = void (*)(MainWindow *iMainWindow, char const *iFilename, char const *iContent);
void wgpu_shader_toy_install_new_fragment_shader_handler(OnNewFragmentShaderHandler iHandler, MainWindow *iMainWindow);
void wgpu_shader_toy_uninstall_new_fragment_shader_handler();
void wgpu_shader_toy_open_file_dialog();
}

namespace callbacks {

//------------------------------------------------------------------------
// callbacks::OnNewFragmentShaderCallback
//------------------------------------------------------------------------
void OnNewFragmentShaderCallback(MainWindow *iMainWindow, char const *iFilename, char const *iContent)
{
  iMainWindow->onNewFragmentShader(iFilename, iContent);
}

}

constexpr char kHelloWorldFragmentShaderCode[] = R"(@fragment
fn fragmentMain(@builtin(position) pos: vec4f) -> @location(0) vec4f {
    return vec4f(pos.xy / inputs.size, 0, 1);
}
)";

constexpr char kTutorialFragmentShaderCode[] = R"(
fn isInCircle(center: vec2f, radius: f32, point: vec2f) -> bool {
    let delta = point - center;
    return (delta.x*delta.x + delta.y*delta.y) <= (radius*radius);
}

@fragment
fn fragmentMain(@builtin(position) pos: vec4f) -> @location(0) vec4f {
  let period = 5.0;
  let half_period = period / 2.0;
  let cycle_value = inputs.time % period;
  let b = half_period - abs(cycle_value - half_period);
  var color = vec4f(b / half_period, pos.xy / inputs.size, 1);
  if(isInCircle(inputs.mouse, 50.0, pos.xy)) {
    color.a = 0.8;
  }
  return color;
}
)";

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
MainWindow::MainWindow(std::shared_ptr<GPU> iGPU, Window::Args const &iWindowArgs, Args const &iMainWindowArgs) :
  ImGuiWindow(std::move(iGPU), iWindowArgs),
  fDefaultSize{iWindowArgs.size},
  fPreferences{iMainWindowArgs.preferences},
  fFragmentShaderWindow{std::make_unique<FragmentShaderWindow>(fGPU,
                                                               iMainWindowArgs.fragmentShaderWindow,
                                                               FragmentShaderWindow::Args{
                                                                 .preferences = fPreferences})},
  fDefaultFragmentShaderWindowSize{iMainWindowArgs.fragmentShaderWindow.size}
{
  // adjust size according to preferences
  resize(fPreferences->loadSize(kPreferencesSizeKey, iWindowArgs.size));

  auto &io = ImGui::GetIO();
  io.Fonts->Clear();
  ImFontConfig fontConfig;
  fontConfig.OversampleH = 2;
  float fontScale; float dummy;
  glfwGetWindowContentScale(fWindow, &fontScale, &dummy);
  io.Fonts->AddFontFromMemoryCompressedBase85TTF(JetBrainsMonoRegular_compressed_data_base85, 13.0f * fontScale, &fontConfig);
  io.FontGlobalScale = 1.0f / fontScale;
  wgpu_shader_toy_install_new_fragment_shader_handler(callbacks::OnNewFragmentShaderCallback, this);
  onNewFragmentShader("Hello World", kHelloWorldFragmentShaderCode);
}

//------------------------------------------------------------------------
// MainWindow::~MainWindow
//------------------------------------------------------------------------
MainWindow::~MainWindow()
{
  wgpu_shader_toy_uninstall_new_fragment_shader_handler();
}

//------------------------------------------------------------------------
// MainWindow::doRender
//------------------------------------------------------------------------
void MainWindow::doRender()
{
  if(ImGui::BeginMainMenuBar())
  {
    if(ImGui::BeginMenu("WebGPU Shader Toy"))
    {
      if(ImGui::MenuItem("Reset"))
        fResetRequest = true;
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
    ImGui::EndMainMenuBar();
  }

  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos);
  ImGui::SetNextWindowSize(ImGui::GetMainViewport()->WorkSize);
  if(ImGui::Begin("WebGPU Shader Toy", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar))
  {
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
          if(fCurrentFragmentShader->getName() != selectedFragmentShader)
          {
            fCurrentFragmentShader = fFragmentShaders[selectedFragmentShader];
            fFragmentShaderWindow->setCurrentFragmentShader(fCurrentFragmentShader);
          }
        }
      }
      if(ImGui::TabItemButton("+"))
      {
        wgpu_shader_toy_open_file_dialog();
      }
      ImGui::EndTabBar();
    }

    if(ImGui::Button("Reset Time"))
    {
      fCurrentFragmentShader->setStartTime(getCurrentTime());
    }
    ImGui::SameLine();
    if(ImGui::Button("Pause/Resume"))
    {
      fCurrentFragmentShader->toggleRunning(getCurrentTime());
    }

    ImGui::DragFloat4("customFloat1", &fCurrentFragmentShader->getCustomFloat1().x, 0.005, 0.0, 1.0);
    ImGui::ColorEdit4("customColor1", &fCurrentFragmentShader->getCustomColor1().x);

    if(fCurrentFragmentShader)
    {
      if(ImGui::BeginTabBar(fCurrentFragmentShader->getName().c_str()))
      {
        if(ImGui::BeginTabItem("Code"))
        {
          ImGui::Text("%s", fCurrentFragmentShader->getCode().c_str());
          ImGui::EndTabItem();
        }
        if(ImGui::BeginTabItem("Inputs"))
        {
          auto &inputs = fCurrentFragmentShader->getInputs();
          ImGui::Text(FragmentShader::kHeaderTemplate,
                      static_cast<int>(inputs.size.x), static_cast<int>(inputs.size.y), // size: vec2f
                      static_cast<int>(inputs.mouse.x), static_cast<int>(inputs.mouse.y), // mouse: vec2f
                      inputs.customFloat1.x, inputs.customFloat1.y, inputs.customFloat1.z, inputs.customFloat1.w, // customFloat1: vec4f
                      inputs.customColor1.x, inputs.customColor1.y, inputs.customColor1.z, inputs.customColor1.w, // customColor1: vec4f
                      inputs.time, // time: f32
                      inputs.frame // frame: i32
          );
          ImGui::SeparatorText("customFloat1");
          ImGui::PushID("customFloat1");
          ImGui::SliderFloat("x", &fCurrentFragmentShader->getCustomFloat1().x, 0.0, 1.0, "%.4f");
          ImGui::SliderFloat("y", &fCurrentFragmentShader->getCustomFloat1().y, 0.0, 1.0, "%.4f");
          ImGui::SliderFloat("z", &fCurrentFragmentShader->getCustomFloat1().z, 0.0, 1.0, "%.4f");
          ImGui::SliderFloat("w", &fCurrentFragmentShader->getCustomFloat1().w, 0.0, 1.0, "%.4f");
          ImGui::PopID();
          ImGui::SeparatorText("customColor1");
          ImGui::PushID("customColor1");
          ImGui::SliderFloat("r", &fCurrentFragmentShader->getCustomColor1().x, 0.0, 1.0, "%.4f");
          ImGui::SliderFloat("g", &fCurrentFragmentShader->getCustomColor1().y, 0.0, 1.0, "%.4f");
          ImGui::SliderFloat("b", &fCurrentFragmentShader->getCustomColor1().z, 0.0, 1.0, "%.4f");
          ImGui::SliderFloat("a", &fCurrentFragmentShader->getCustomColor1().w, 0.0, 1.0, "%.4f");
          ImGui::PopID();
          ImGui::EndTabItem();
        }
        if(fCurrentFragmentShader->hasCompilationError())
        {
          auto flags = ImGuiTabItemFlags_None;
          if(fCurrentFragmentShaderErrorRequest)
          {
            flags = ImGuiTabItemFlags_SetSelected;
            fCurrentFragmentShaderErrorRequest = false;
          }
          if(ImGui::BeginTabItem("Errors", nullptr, flags))
          {
            ImGui::Text("%s", fCurrentFragmentShader->getCompilationError().c_str());
            ImGui::EndTabItem();
          }
          if(ImGui::BeginTabItem("Full Code"))
          {
            std::istringstream fullCode(FragmentShader::kHeader + fCurrentFragmentShader->getCode());
            std::string line;
            int lineNumber = 1;
            while(std::getline(fullCode, line))
            {
              ImGui::Text("[%d] %s", lineNumber++, line.c_str());
            }
            ImGui::EndTabItem();
          }
        }
        ImGui::EndTabBar();
      }
    }
    else
    {
      ImGui::Text("Click on [+] to add a shader or drag and drop a shader file here");
    }
  }
  ImGui::End();
}

//------------------------------------------------------------------------
// MainWindow::doHandleFrameBufferSizeChange
//------------------------------------------------------------------------
void MainWindow::doHandleFrameBufferSizeChange(Renderable::Size const &iSize)
{
  ImGuiWindow::doHandleFrameBufferSizeChange(iSize);
  int w, h;
  glfwGetWindowSize(fWindow, &w, &h);
  fPreferences->storeSize(kPreferencesSizeKey, {w, h});
}

//------------------------------------------------------------------------
// MainWindow::onNewFragmentShader
//------------------------------------------------------------------------
void MainWindow::onNewFragmentShader(char const *iFilename, char const *iContent)
{
  fCurrentFragmentShader = std::make_shared<FragmentShader>(iFilename, iContent);
  fFragmentShaders[fCurrentFragmentShader->getName()] = fCurrentFragmentShader;
  if(std::ranges::find(fFragmentShaderTabs, fCurrentFragmentShader->getName()) == fFragmentShaderTabs.end())
  {
    fFragmentShaderTabs.emplace_back(fCurrentFragmentShader->getName());
  }
  fFragmentShaderWindow->setCurrentFragmentShader(fCurrentFragmentShader);
  fCurrentFragmentShaderNameRequest = fCurrentFragmentShader->getName();
  fCurrentFragmentShaderErrorRequest = true;
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
  resize(fDefaultSize);
  fFragmentShaderWindow->resize(fDefaultFragmentShaderWindowSize);
  fFragmentShaders.clear();
  fFragmentShaderTabs.clear();
  fCurrentFragmentShader = nullptr;
  onNewFragmentShader("Hello World", kHelloWorldFragmentShaderCode);
}

}
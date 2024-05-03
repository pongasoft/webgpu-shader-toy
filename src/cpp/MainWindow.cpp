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

constexpr auto kHelloWorldFragmentShader = "Hello World";

//------------------------------------------------------------------------
// MainWindow::MainWindow
//------------------------------------------------------------------------
MainWindow::MainWindow(std::shared_ptr<GPU> iGPU, Window::Args const &iWindowArgs, Args const &iMainWindowArgs) :
  ImGuiWindow(std::move(iGPU), iWindowArgs),
  fDefaultSize{iWindowArgs.size},
  fPreferences{iMainWindowArgs.preferences},
  fModel{std::make_unique<Model>()},
  fFragmentShaderWindow{fGPU,
                        iMainWindowArgs.fragmentShaderWindow,
                        FragmentShaderWindow::Args{
                          .model = fModel,
                          .preferences = fPreferences}},
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

  fFragmentShaders[kHelloWorldFragmentShader] = kDefaultFragmentShader;
  fFragmentShaderTabs.emplace_back(kHelloWorldFragmentShader);
  fCurrentFragmentShaderName = kHelloWorldFragmentShader;
}

//------------------------------------------------------------------------
// MainWindow::~MainWindow
//------------------------------------------------------------------------
MainWindow::~MainWindow()
{
  wgpu_shader_toy_uninstall_new_fragment_shader_handler();
}

constexpr char kShader2[] = R"(
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

//------------------------------------------------------------------------
// MainWindow::doRender
//------------------------------------------------------------------------
void MainWindow::doRender()
{
  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos);
  ImGui::SetNextWindowSize(ImGui::GetMainViewport()->WorkSize);

  if(ImGui::Begin("WebGPU Shader Toy", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar))
  {
    if(ImGui::BeginTabBar("Fragment Shaders"))
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
      if(ImGui::TabItemButton("+"))
      {
        wgpu_shader_toy_open_file_dialog();
      }
      if(fragmentShaderToDelete)
        deleteFragmentShader(*fragmentShaderToDelete);
      else
      {
        if(fCurrentFragmentShaderName != selectedFragmentShader)
        {
          fCurrentFragmentShaderName = selectedFragmentShader;
          fModel->setFragmentShader(fFragmentShaders[fCurrentFragmentShaderName]);
        }
      }
      ImGui::EndTabBar();
    }

    ImGui::Text("aspect_ratio");
    ImGui::SameLine();
    if(ImGui::Button("Free"))
      fAspectRatioRequest = AspectRatio{GLFW_DONT_CARE, GLFW_DONT_CARE};
    ImGui::SameLine();
    if(ImGui::Button("16:9"))
      fAspectRatioRequest = AspectRatio{16, 9};
    ImGui::SameLine();
    if(ImGui::Button("4:3"))
      fAspectRatioRequest = AspectRatio{4,3};
    ImGui::SameLine();
    if(ImGui::Button("1:1"))
      fAspectRatioRequest = AspectRatio{1,1};

    ImGui::SeparatorText("Shader");

    if(ImGui::TreeNode("Shader Inputs"))
    {
      ImGui::Text("%s", kFragmentShaderHeader);
      ImGui::TreePop();
    }

    ImGui::Text("%s", fModel->getFragmentShader().c_str());

    ImGui::Separator();

    if(ImGui::Button("Reset"))
      fResetRequest = true;

    ImGui::Separator();

    if(ImGui::Button("Exit"))
      stop();

    if(fModel->fFragmentShaderError)
    {
      ImGui::Text("%s", fModel->fFragmentShaderError->c_str());
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
  fCurrentFragmentShaderName = iFilename;
  fCurrentFragmentShaderNameRequest = iFilename;
  if(std::ranges::find(fFragmentShaderTabs, fCurrentFragmentShaderName) == fFragmentShaderTabs.end())
  {
    fFragmentShaderTabs.emplace_back(fCurrentFragmentShaderName);
  }
  fModel->setFragmentShader(iContent);
  fFragmentShaders[fCurrentFragmentShaderName] = iContent;
}

//------------------------------------------------------------------------
// MainWindow::deleteFragmentShader
//------------------------------------------------------------------------
void MainWindow::deleteFragmentShader(std::string const &iName)
{
  fFragmentShaderTabs.erase(std::remove(fFragmentShaderTabs.begin(), fFragmentShaderTabs.end(), iName),
                            fFragmentShaderTabs.end());
  fFragmentShaders.erase(iName);
  if(fFragmentShaders.empty())
  {
    onNewFragmentShader(kHelloWorldFragmentShader, kDefaultFragmentShader);
  }
  else
  {
    if(fCurrentFragmentShaderName == iName)
    {
      fCurrentFragmentShaderName = fFragmentShaderTabs[0];
      fCurrentFragmentShaderNameRequest = fCurrentFragmentShaderName;
    }
  }
  fModel->setFragmentShader(fFragmentShaders[fCurrentFragmentShaderName]);
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
    fFragmentShaderWindow.setAspectRatio(*fAspectRatioRequest);
    fAspectRatioRequest = std::nullopt;
  }
  fFragmentShaderWindow.beforeFrame();
}

//------------------------------------------------------------------------
// MainWindow::render
//------------------------------------------------------------------------
void MainWindow::render()
{
  Renderable::render();
  fFragmentShaderWindow.render();
}

//------------------------------------------------------------------------
// MainWindow::reset
//------------------------------------------------------------------------
void MainWindow::reset()
{
  resize(fDefaultSize);
  fFragmentShaderWindow.resize(fDefaultFragmentShaderWindowSize);
}

}
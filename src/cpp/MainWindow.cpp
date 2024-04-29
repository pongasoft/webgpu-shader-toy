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

namespace shader_toy {

//------------------------------------------------------------------------
// MainWindow::MainWindow
//------------------------------------------------------------------------
MainWindow::MainWindow(std::shared_ptr<GPU> iGPU, Args const &iArgs, std::shared_ptr<Model> iModel) :
  ImGuiWindow(std::move(iGPU), iArgs),
  fModel{std::move(iModel)}
{
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
    ImGui::Text("shader");
    ImGui::SameLine();
    if(ImGui::Button("Default"))
      fModel->fFragmentShader = kDefaultFragmentShader;
    ImGui::SameLine();
    if(ImGui::Button("Shader2"))
      fModel->fFragmentShader = kShader2;

    ImGui::Text("aspect_ratio");
    ImGui::SameLine();
    if(ImGui::Button("Free"))
      fModel->fAspectRatioRequest = Model::AspectRatio{GLFW_DONT_CARE, GLFW_DONT_CARE};
    ImGui::SameLine();
    if(ImGui::Button("16:9"))
      fModel->fAspectRatioRequest = Model::AspectRatio{16, 9};
    ImGui::SameLine();
    if(ImGui::Button("4:3"))
      fModel->fAspectRatioRequest = Model::AspectRatio{4,3};
    ImGui::SameLine();
    if(ImGui::Button("1:1"))
      fModel->fAspectRatioRequest = Model::AspectRatio{1,1};

    ImGui::SeparatorText("Shader");

    if(ImGui::TreeNode("Shader Inputs"))
    {
      ImGui::Text("%s", kFragmentShaderHeader);
      ImGui::TreePop();
    }

    ImGui::Text("%s", fModel->fFragmentShader.c_str());

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
}
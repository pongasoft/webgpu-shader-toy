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

#include <backends/imgui_impl_wgpu.h>
#include <backends/imgui_impl_glfw.h>
#include <GLFW/emscripten_glfw3.h>
#include "ImGuiWindow.h"

namespace pongasoft::gpu {

//------------------------------------------------------------------------
// ImGuiWindow::ImGuiWindow
//------------------------------------------------------------------------
ImGuiWindow::ImGuiWindow(std::shared_ptr<GPU> iGPU, Args const &iArgs) :
  Window(std::move(iGPU), iArgs)
{
  IMGUI_CHECKVERSION();
  fImGuiContext = ImGui::CreateContext();

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOther(fWindow, true);
  ImGui_ImplWGPU_InitInfo init_info;
  init_info.Device = fGPU->getDevice().Get();
  init_info.RenderTargetFormat = static_cast<WGPUTextureFormat>(fPreferredFormat);
  ImGui_ImplWGPU_Init(&init_info);

  ImGuiIO& io = ImGui::GetIO();
  io.ConfigMacOSXBehaviors = fIsRuntimePlatformApple;

  // essentially disable repeat on Meta + Key
  emscripten::glfw3::SetSuperPlusKeyTimeouts(10, 10);
}

//------------------------------------------------------------------------
// ImGuiWindow::~ImGuiWindow
//------------------------------------------------------------------------
ImGuiWindow::~ImGuiWindow()
{
  if(fImGuiContext)
  {
    ImGui_ImplWGPU_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
  }
}

//------------------------------------------------------------------------
// ImGuiWindow::doHandleFrameBufferSizeChange
//------------------------------------------------------------------------
void ImGuiWindow::doHandleFrameBufferSizeChange(Renderable::Size const &iSize)
{
  ImGui_ImplWGPU_InvalidateDeviceObjects();
  Window::doHandleFrameBufferSizeChange(iSize);
  ImGui_ImplWGPU_CreateDeviceObjects();
}

//------------------------------------------------------------------------
// ImGuiWindow::doRender
//------------------------------------------------------------------------
void ImGuiWindow::doRender(wgpu::RenderPassEncoder &iRenderPass)
{
  ImGui::SetCurrentContext(fImGuiContext);

  ImGui_ImplWGPU_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  doRender();

  ImGui::EndFrame();
  ImGui::Render();
  ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), iRenderPass.Get());

}

//------------------------------------------------------------------------
// ImGuiWindow::beforeFrame
//------------------------------------------------------------------------
void ImGuiWindow::beforeFrame()
{
  Window::beforeFrame();
  ImGui::SetCurrentContext(fImGuiContext);
}

}
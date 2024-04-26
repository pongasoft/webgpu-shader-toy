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

#include <GLFW/emscripten_glfw3.h>
#include <backends/imgui_impl_wgpu.h>
#include <backends/imgui_impl_glfw.h>
#include "Application.h"
#include "Errors.h"

namespace shader_toy {

//------------------------------------------------------------------------
// consoleErrorHandler
//------------------------------------------------------------------------
void consoleErrorHandler(int iErrorCode, char const *iErrorMessage)
{
  printf("glfwError: %d | %s\n", iErrorCode, iErrorMessage);
}

namespace callbacks {

//------------------------------------------------------------------------
// callbacks::onFrameBufferSizeChange
//------------------------------------------------------------------------
void onFrameBufferSizeChange(GLFWwindow *window, int width, int height)
{
  auto application = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
  application->asyncOnFramebufferSizeChange({static_cast<float>(width), static_cast<float>(height)});
}

}


//------------------------------------------------------------------------
// Application::~Application
//------------------------------------------------------------------------
shader_toy::Application::~Application()
{
  if(fImGuiContext)
  {
    ImGui_ImplWGPU_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
  }

  if(fImGuiWindow)
    glfwDestroyWindow(fImGuiWindow);

  glfwTerminate();
}

//------------------------------------------------------------------------
// Application::init
//------------------------------------------------------------------------
void Application::init(std::string_view iImGuiCanvasSelector)
{
  fImGuiWindow = initGLFW(iImGuiCanvasSelector);
  // makes the canvas resizable and match the full window size
  emscripten_glfw_make_canvas_resizable(fImGuiWindow, "window", nullptr);
  glfwSetWindowUserPointer(fImGuiWindow, this);
  initWebGPU(iImGuiCanvasSelector);
  initImGui();
  fShaderWindow.init(*fGPU);
  glfwShowWindow(fImGuiWindow);
}

//------------------------------------------------------------------------
// Application::initGLFW
//------------------------------------------------------------------------
GLFWwindow *Application::initGLFW(std::string_view iImGuiCanvasSelector)
{
  // set a callback for errors otherwise if there is a problem, we won't know
  glfwSetErrorCallback(consoleErrorHandler);

  // print the version on the console
  printf("%s\n", glfwGetVersionString());

  // initialize the library
  if(!glfwInit())
    return nullptr;

  // no OpenGL (use WebGPU)
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  // setting the association window <-> canvas
  emscripten_glfw_set_next_window_canvas_selector(iImGuiCanvasSelector.data());

  // create the only window
  auto window = glfwCreateWindow(320, 200, "WebGPU Shader Toy", nullptr, nullptr);
  SHADER_TOY_ASSERT(window != nullptr, "Cannot create GLFW Window");

  return window;
}

//------------------------------------------------------------------------
// Application::initWebGPU
//------------------------------------------------------------------------
void Application::initWebGPU(std::string_view iImGuiCanvasSelector)
{
  fGPU = shader_toy::GPU::create();
  fGPU->createSurface(fImGuiWindow, iImGuiCanvasSelector);

  // will initialize the swapchain on first frame
  int w, h;
  glfwGetFramebufferSize(fImGuiWindow, &w, &h);
  callbacks::onFrameBufferSizeChange(fImGuiWindow, w, h);
}

//------------------------------------------------------------------------
// Application::initImGui
//------------------------------------------------------------------------
void Application::initImGui()
{
  // Setup Dear ImGui context
  fImGuiContext = ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
//  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

#ifdef IMGUI_ENABLE_DOCKING
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigDockingWithShift = false;
#endif

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOther(fImGuiWindow, true);
  ImGui_ImplWGPU_InitInfo init_info;
  init_info.Device = fGPU->getDevice().Get();
  init_info.RenderTargetFormat = static_cast<WGPUTextureFormat>(fGPU->getPreferredFormat());
  ImGui_ImplWGPU_Init(&init_info);
}

//------------------------------------------------------------------------
// Application::onFramebufferSizeChange
//------------------------------------------------------------------------
void Application::onFramebufferSizeChange(ImVec2 const &iSize)
{
  ImGui_ImplWGPU_InvalidateDeviceObjects();
  fGPU->createSwapChain(static_cast<int>(iSize.x), static_cast<int>(iSize.y));
  ImGui_ImplWGPU_CreateDeviceObjects();
}

//------------------------------------------------------------------------
// Application::newFrame
//------------------------------------------------------------------------
void Application::newFrame()
{
//  if(!fNewFrameActions.empty())
//  {
//    auto actions = std::move(fNewFrameActions);
//    for(auto &action: actions)
//      action(this);
//  }
}

//------------------------------------------------------------------------
// Application::mainLoop
//------------------------------------------------------------------------
void Application::mainLoop()
{
  static ImVec4 clear_color = ImVec4(153.f/255.f, 153.f/255.f, 153.f/255.f, 1.00f);

  // handle size change before everything
  if(fNewFrameBufferSize)
  {
    onFramebufferSizeChange(*fNewFrameBufferSize);
    fNewFrameBufferSize = std::nullopt;
  }

  newFrame();

  glfwPollEvents();

  fGPU->beginFrame();
  fShaderWindow.render(*fGPU);
  fGPU->renderPass({.r = clear_color.x, .g = clear_color.y, .b = clear_color.z, .a = clear_color.w},
                   [this](wgpu::RenderPassEncoder &pass) { renderImGui(pass); });
  fGPU->endFrame();

  fRunning = !glfwWindowShouldClose(fImGuiWindow);
}

//------------------------------------------------------------------------
// Application::renderImGui
//------------------------------------------------------------------------
void Application::renderImGui(wgpu::RenderPassEncoder &renderPass)
{
  ImGui_ImplWGPU_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  if(ImGui::Begin("Hello, world!"))
  {
    ImGui::Text("This is some useful text.");
    if(ImGui::Button("Exit"))
      glfwSetWindowShouldClose(fImGuiWindow, GLFW_TRUE);

    auto &io = ImGui::GetIO();
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
  }
  ImGui::End();

  if(ImGui::Begin("Shader"))
  {
    ImGui::Image(fShaderWindow.getTextureView(), fShaderWindow.getSize());
  }
  ImGui::End();

  ImGui::EndFrame();
  ImGui::Render();
  ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass.Get());
}


}
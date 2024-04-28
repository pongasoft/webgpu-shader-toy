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

//------------------------------------------------------------------------
// Application::Application
//------------------------------------------------------------------------
Application::Application()
{
  // set a callback for errors otherwise if there is a problem, we won't know
  glfwSetErrorCallback(consoleErrorHandler);

  // print the version on the console
  printf("%s\n", glfwGetVersionString());

  // initialize the library
  ASSERT(glfwInit() == GLFW_TRUE);

  // no OpenGL (use WebGPU)
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  fGPU = GPU::create();

  fFragmentShaderWindow = std::make_unique<FragmentShaderWindow>(fGPU,
                                                                 Renderable::Size{320, 200},
                                                                 "WebGPU Shader Toy | fragment shader",
                                                                 "#canvas2", "#canvas2-container", "#canvas2-handle");

  fFragmentShaderWindow->show();

  fControlsWindow = std::make_unique<ImGuiWindow>(fGPU,
                                                  Renderable::Size{320, 200},
                                                  "WebGPU Shader Toy",
                                                  "#canvas1", "#canvas1-container", "#canvas1-handle");

  fControlsWindow->show();
}


//------------------------------------------------------------------------
// Application::~Application
//------------------------------------------------------------------------
shader_toy::Application::~Application()
{
  fControlsWindow = nullptr;
  fFragmentShaderWindow = nullptr;
  fGPU = nullptr;
  glfwTerminate();
}

//------------------------------------------------------------------------
// Application::mainLoop
//------------------------------------------------------------------------
void Application::mainLoop()
{
  fControlsWindow->handleFramebufferSizeChange();
  fFragmentShaderWindow->handleFramebufferSizeChange();

  glfwPollEvents();

  fGPU->beginFrame();
  fControlsWindow->render([this]() { renderControlsWindow(); });
  fFragmentShaderWindow->render();
  fGPU->endFrame();

  fRunning = fControlsWindow->running() && fFragmentShaderWindow->running();
//  fRunning = false;
}

//------------------------------------------------------------------------
// Application::renderControlsWindow
//------------------------------------------------------------------------
void Application::renderControlsWindow()
{
  static ImVec4 kClearColor = ImVec4(153.f/255.f, 153.f/255.f, 153.f/255.f, 1.00f);

  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos);
  ImGui::SetNextWindowSize(ImGui::GetMainViewport()->WorkSize);
  if(ImGui::Begin("Hello, world!", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
  {
    ImGui::Text("This is some useful text.");

    ImGui::ColorEdit3("Background", &kClearColor.x);

    if(ImGui::Button("Exit"))
      fControlsWindow->stop();

    auto &io = ImGui::GetIO();
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
  }
  fControlsWindow->setClearColor({kClearColor.x, kClearColor.y, kClearColor.z, kClearColor.w});
  ImGui::End();

//  if(ImGui::Begin("Shader"))
//  {
//    ImGui::Image(fShaderWindow.getTextureView(), fShaderWindow.getSize());
//  }
//  ImGui::End();
}


}
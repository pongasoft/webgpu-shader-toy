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

#include <GLFW/glfw3.h>
#include "Application.h"
#include "Errors.h"
#include <emscripten/version.h>

#include <utility>

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
Application::Application(std::shared_ptr<GPU> iGPU) : fGPU{std::move(iGPU)}
{
  // set a callback for errors otherwise if there is a problem, we won't know
  glfwSetErrorCallback(consoleErrorHandler);

  // print the version on the console
  printf("GLFW: %s\n", glfwGetVersionString());
  printf("Emscripten: %d.%d.%d\n", __EMSCRIPTEN_major__, __EMSCRIPTEN_minor__, __EMSCRIPTEN_tiny__);

  // initialize the library
  WST_INTERNAL_ASSERT(glfwInit() == GLFW_TRUE);

  // no OpenGL (use WebGPU)
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
}

//------------------------------------------------------------------------
// Application::asyncCreate
//------------------------------------------------------------------------
std::future<std::unique_ptr<Application>> Application::asyncCreate(std::function<void(std::string_view)> onError)
{
  auto promise = std::make_shared<std::promise<std::unique_ptr<Application>>>();
  auto future = promise->get_future();

  GPU::asyncCreate([p = std::move(promise)](std::shared_ptr<GPU> iGPU) {
    p->set_value(std::make_unique<Application>(std::move(iGPU)));
  }, std::move(onError));

  return future;
}

//------------------------------------------------------------------------
// Application::~Application
//------------------------------------------------------------------------
Application::~Application()
{
  fRenderableList.clear();
  fGPU = nullptr;
  glfwTerminate();
}

//------------------------------------------------------------------------
// Application::mainLoop
//------------------------------------------------------------------------
void Application::mainLoop()
{
  glfwPollEvents();

  try
  {
    // This makes sure that we control exactly when the asynchronous callbacks using
    // wgpu::CallbackMode::AllowProcessEvents are triggered (shader compilation)
    fGPU->pollEvents();

    // beforeFrame
    for(auto &r: fRenderableList)
    {
      r->beforeFrame(); WST_INTERNAL_ASSERT(!fGPU->hasError());
    }

    // GPU -> beginFrame
    fGPU->beginFrame(); WST_INTERNAL_ASSERT(!fGPU->hasError());

    // render
    for(auto &r: fRenderableList)
    {
      r->render(); WST_INTERNAL_ASSERT(!fGPU->hasError());
    }

    // GPU -> endFrame
    fGPU->endFrame(); WST_INTERNAL_ASSERT(!fGPU->hasError());

    // afterFrame
    for(auto &r: fRenderableList)
    {
      r->afterFrame(); WST_INTERNAL_ASSERT(!fGPU->hasError());
    }

    fRunning = true;
    for(auto &r: fRenderableList)
      fRunning &= r->running();
  }
  catch(pongasoft::Exception const &e)
  {
    auto gpuError = fGPU->consumeError();
    if(gpuError)
    {
      fRunning = false;
      printf("[WebGPU] %s error | %s\n", GPU::errorTypeAsString(gpuError->fType), gpuError->fMessage.c_str());
    }
    fRunning = false;
  }

}
}
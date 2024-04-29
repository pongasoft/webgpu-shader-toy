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
}


//------------------------------------------------------------------------
// Application::~Application
//------------------------------------------------------------------------
shader_toy::Application::~Application()
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
  try
  {
    for(auto &r: fRenderableList)
    {
      r->beforeFrame();
      ASSERT(!fGPU->hasError());
    }

    glfwPollEvents();

    fGPU->beginFrame();
    ASSERT(!fGPU->hasError());
    for(auto &r: fRenderableList)
    {
      r->render();
      ASSERT(!fGPU->hasError());
    }
    fGPU->endFrame();
    ASSERT(!fGPU->hasError());

    for(auto &r: fRenderableList)
    {
      r->afterFrame();
      ASSERT(!fGPU->hasError());
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
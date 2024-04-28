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

#include "Window.h"
#include "../Errors.h"
#include <GLFW/glfw3.h>
#include <GLFW/emscripten_glfw3.h>

namespace pongasoft::gpu {

namespace callbacks {

//------------------------------------------------------------------------
// callbacks::onFrameBufferSizeChange
//------------------------------------------------------------------------
void onFrameBufferSizeChange(GLFWwindow *window, int width, int height)
{
  auto application = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
  application->asyncOnFramebufferSizeChange({width, height});
}

}

//------------------------------------------------------------------------
// Window::Window
//------------------------------------------------------------------------
Window::Window(std::shared_ptr<GPU> iGPU,
               Size const &iSize,
               char const *title,
               char const *iCanvasSelector,
               char const *iCanvasResizeSelector,
               char const *iHandleSelector) : Renderable(std::move(iGPU))
{
  emscripten_glfw_set_next_window_canvas_selector(iCanvasSelector);

  fWindow = glfwCreateWindow(iSize.fWidth, iSize.fHeight, title, nullptr, nullptr);
  ASSERT(fWindow != nullptr, "Cannot create GLFW Window [%s]", title);

  emscripten_glfw_make_canvas_resizable(fWindow, iCanvasResizeSelector, iHandleSelector);

  glfwSetWindowUserPointer(fWindow, this);

  wgpu::SurfaceDescriptorFromCanvasHTMLSelector html_surface_desc = {};
  html_surface_desc.selector = iCanvasSelector;

  wgpu::SurfaceDescriptor surface_desc = { .nextInChain = &html_surface_desc };

  fSurface = fGPU->getInstance().CreateSurface(&surface_desc);
  ASSERT(fSurface != nullptr, "Cannot create surface for [%s]", iCanvasSelector);
  initPreferredFormat(fSurface.GetPreferredFormat(nullptr));

  // will initialize the swapchain on first frame
  int w, h;
  glfwGetFramebufferSize(fWindow, &w, &h);
  callbacks::onFrameBufferSizeChange(fWindow, w, h);
}

//------------------------------------------------------------------------
// Window::~Window
//------------------------------------------------------------------------
Window::~Window()
{
  if(fWindow)
    glfwDestroyWindow(fWindow);
}

//------------------------------------------------------------------------
// Window::beforeFrame
//------------------------------------------------------------------------
void Window::beforeFrame()
{
  handleFramebufferSizeChange();
}

//------------------------------------------------------------------------
// Window::handleFramebufferSizeChange
//------------------------------------------------------------------------
void Window::handleFramebufferSizeChange()
{
  if(fNewFrameBufferSize)
  {
    doHandleFrameBufferSizeChange(*fNewFrameBufferSize);
    fNewFrameBufferSize = std::nullopt;
  }
}

//------------------------------------------------------------------------
// Window::doHandleFrameBufferSizeChange
//------------------------------------------------------------------------
void Window::doHandleFrameBufferSizeChange(Size const &iSize)
{
  createSwapChain(iSize);
}

//------------------------------------------------------------------------
// Window::createSwapChain
//------------------------------------------------------------------------
void Window::createSwapChain(Renderable::Size const &iSize)
{
  wgpu::SwapChainDescriptor scDesc{
    .usage = wgpu::TextureUsage::RenderAttachment,
    .format = fPreferredFormat,
    .width = static_cast<uint32_t>(iSize.fWidth),
    .height = static_cast<uint32_t>(iSize.fHeight),
    .presentMode = wgpu::PresentMode::Fifo};
  fSwapChain = fGPU->getDevice().CreateSwapChain(fSurface, &scDesc);
}

//------------------------------------------------------------------------
// Window::getTextureView
//------------------------------------------------------------------------
wgpu::TextureView Window::getTextureView() const
{
  return fSwapChain.GetCurrentTextureView();
}

//------------------------------------------------------------------------
// Window::show
//------------------------------------------------------------------------
void Window::show()
{
  glfwShowWindow(fWindow);
}

//------------------------------------------------------------------------
// Window::running
//------------------------------------------------------------------------
bool Window::running() const
{
  return !glfwWindowShouldClose(fWindow);
}

//------------------------------------------------------------------------
// Window::stop
//------------------------------------------------------------------------
void Window::stop()
{
  glfwSetWindowShouldClose(fWindow, GLFW_TRUE);
}

}
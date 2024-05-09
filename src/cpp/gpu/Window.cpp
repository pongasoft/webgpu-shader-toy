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
  auto w = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
  w->asyncOnFramebufferSizeChange({width, height});
}

}

//------------------------------------------------------------------------
// Window::Window
//------------------------------------------------------------------------
Window::Window(std::shared_ptr<GPU> iGPU, Args const &iArgs) : Renderable(std::move(iGPU))
{
  emscripten_glfw_set_next_window_canvas_selector(iArgs.canvas.selector);

  fWindow = glfwCreateWindow(iArgs.size.width, iArgs.size.height, iArgs.title, nullptr, nullptr);
  ASSERT(fWindow != nullptr, "Cannot create GLFW Window [%s]", iArgs.title);

  emscripten_glfw_make_canvas_resizable(fWindow, iArgs.canvas.resizeSelector, iArgs.canvas.handleSelector);

  resize(iArgs.size);

  glfwSetWindowUserPointer(fWindow, this);

  wgpu::SurfaceDescriptorFromCanvasHTMLSelector html_surface_desc = {};
  html_surface_desc.selector = iArgs.canvas.selector;

  wgpu::SurfaceDescriptor surface_desc = { .nextInChain = &html_surface_desc };

  fSurface = fGPU->getInstance().CreateSurface(&surface_desc);
  ASSERT(fSurface != nullptr, "Cannot create surface for [%s]", iArgs.canvas.selector);
  initPreferredFormat(fSurface.GetPreferredFormat(nullptr));

  // will initialize the swapchain on first frame
  int w, h;
  glfwGetFramebufferSize(fWindow, &w, &h);
  callbacks::onFrameBufferSizeChange(fWindow, w, h);
  glfwSetFramebufferSizeCallback(fWindow, callbacks::onFrameBufferSizeChange);
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
// Window::getFrameBufferSize
//------------------------------------------------------------------------
Renderable::Size Window::getFrameBufferSize() const
{
  Renderable::Size size;
  glfwGetFramebufferSize(fWindow, &size.width, &size.height);
  return size;
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
    .width = static_cast<uint32_t>(iSize.width),
    .height = static_cast<uint32_t>(iSize.height),
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

//------------------------------------------------------------------------
// Window::resize
//------------------------------------------------------------------------
void Window::resize(Renderable::Size const &iSize)
{
  glfwSetWindowSize(fWindow, iSize.width, iSize.height);
}

//------------------------------------------------------------------------
// Window::setAspectRatio
//------------------------------------------------------------------------
void Window::setAspectRatio(AspectRatio const &iAspectRatio)
{
  glfwSetWindowAspectRatio(fWindow, iAspectRatio.numerator, iAspectRatio.denominator);
}

//------------------------------------------------------------------------
// Window::isHiDPIAware
//------------------------------------------------------------------------
bool Window::isHiDPIAware() const
{
  return glfwGetWindowAttrib(fWindow, GLFW_SCALE_FRAMEBUFFER) == GLFW_TRUE;
}

//------------------------------------------------------------------------
// Window::toggleHiDPIAwareness
//------------------------------------------------------------------------
void Window::toggleHiDPIAwareness()
{
  glfwSetWindowAttrib(fWindow, GLFW_SCALE_FRAMEBUFFER, isHiDPIAware() ? GLFW_FALSE : GLFW_TRUE);
}

//------------------------------------------------------------------------
// Window::getCurrentTime
//------------------------------------------------------------------------
double Window::getCurrentTime()
{
  return glfwGetTime();
}

}
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

#ifndef WGPU_SHADER_TOY_GPU_WINDOW_H
#define WGPU_SHADER_TOY_GPU_WINDOW_H

#include "Renderable.h"
#include <optional>

struct GLFWwindow;

namespace pongasoft::gpu {

class Window: public Renderable
{
public:
  struct Args
  {
    Size size{320, 200};
    char const *title{"undefined"};
    struct {
      char const *selector{};
      char const *resizeSelector{};
      char const *handleSelector{};
    } canvas{};
  };
public:
  Window(std::shared_ptr<GPU> iGPU, Args const &iArgs);
  ~Window() override;

  void beforeFrame() override;

  void handleFramebufferSizeChange();

  void asyncOnFramebufferSizeChange(Size const &iSize) { fNewFrameBufferSize = iSize; }

  virtual void doHandleFrameBufferSizeChange(Size const &iSize);

  void show();
  bool running() const override;
  void stop();

  void resize(Size const &iSize);

protected:
  wgpu::TextureView getTextureView() const override;

private:
  void createSwapChain(Size const &iSize);

protected:
  GLFWwindow *fWindow{};

private:
  std::optional<Size> fNewFrameBufferSize{};
  wgpu::Surface fSurface{};
  wgpu::SwapChain fSwapChain{};
};

}

#endif //WGPU_SHADER_TOY_GPU_WINDOW_H

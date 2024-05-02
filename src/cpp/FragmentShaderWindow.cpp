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

#include "FragmentShaderWindow.h"
#include "Errors.h"
#include <GLFW/glfw3.h>

#include <utility>

namespace shader_toy {

constexpr char kVertexShader[] = R"(
@vertex
fn vertexMain(@builtin(vertex_index) i : u32) -> @builtin(position) vec4f {
    const pos = array(vec2f(-1, 1), vec2f(-1, -1), vec2f(1, -1), vec2f(-1, 1), vec2f(1, -1), vec2f(1, 1));
    return vec4f(pos[i], 0, 1);
}
)";

namespace callbacks {

//------------------------------------------------------------------------
// callbacks::onFrameBufferSizeChange
//------------------------------------------------------------------------
void onCursorPosChange(GLFWwindow *window, double xpos, double ypos)
{
  auto w = reinterpret_cast<FragmentShaderWindow *>(glfwGetWindowUserPointer(window));
  w->onMousePosChange(xpos, ypos);
}

//------------------------------------------------------------------------
// callbacks::onShaderCompilationResult
// implementation note: API is using webgpu.h style, not wegpu_cpp.h
//------------------------------------------------------------------------
void onShaderCompilationResult(WGPUCompilationInfoRequestStatus iStatus, struct WGPUCompilationInfo const *iCompilationInfo, void *iUserdata)
{
  printf("onShaderCompilation %d\n", iStatus);
  auto fsw = reinterpret_cast<FragmentShaderWindow *>(iUserdata);
  fsw->createRenderPipeline(static_cast<wgpu::CompilationInfoRequestStatus>(iStatus), iCompilationInfo);
}

}

//------------------------------------------------------------------------
// FragmentShaderWindow::FragmentShaderWindow
//------------------------------------------------------------------------
FragmentShaderWindow::FragmentShaderWindow(std::shared_ptr<gpu::GPU> iGPU, Window::Args const &iWindowArgs, Args const &iMainWindowArgs) :
  Window(std::move(iGPU), iWindowArgs),
  fModel{iMainWindowArgs.model},
  fPreferences{iMainWindowArgs.preferences}
{
  glfwSetCursorPosCallback(fWindow, callbacks::onCursorPosChange);
}

#define MEMALIGN(_SIZE,_ALIGN)        (((_SIZE) + ((_ALIGN) - 1)) & ~((_ALIGN) - 1))    // Memory align (copied from IM_ALIGN() macro).

//------------------------------------------------------------------------
// FragmentShaderWindow::createRenderPipeline
//------------------------------------------------------------------------
void FragmentShaderWindow::createRenderPipeline(wgpu::CompilationInfoRequestStatus iStatus,
                                                WGPUCompilationInfo const *iCompilationInfo)
{
  if(iStatus != wgpu::CompilationInfoRequestStatus::Success)
  {
    printf("Fragment shader compilation error\n");
    if(iCompilationInfo)
    {
      for(auto i = 0; i < iCompilationInfo->messageCount; i++)
      {
        printf("Message[%d] | %s", i, iCompilationInfo->messages[0].message);
      }
    }
    return;
  }

  if(fGPU->hasError())
  {
    auto error = fGPU->consumeError();
    printf("createRenderPipeline GPU in error => %s\n", gpu::GPU::errorTypeAsString(error->fType));
    fModel->fFragmentShaderError = fmt::printf("[%s]: %s", gpu::GPU::errorTypeAsString(error->fType), error->fMessage);
    return;
  }

  auto device = fGPU->getDevice();

  // vertex shader
  wgpu::ShaderModuleWGSLDescriptor vertexShaderModuleWGSLDescriptor{};
  vertexShaderModuleWGSLDescriptor.code = kVertexShader;
  wgpu::ShaderModuleDescriptor vertexShaderModuleDescriptor{
    .nextInChain = &vertexShaderModuleWGSLDescriptor,
    .label = "FragmentShaderWindow | Vertex Shader"
  };
  wgpu::ShaderModule vertexShaderModule = device.CreateShaderModule(&vertexShaderModuleDescriptor);

  wgpu::BlendState blendState {
    .color {
      .operation = wgpu::BlendOperation::Add,
      .srcFactor = wgpu::BlendFactor::SrcAlpha,
      .dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha,
    },
    // note: copied from imgui (!= from learn webgpu)
    .alpha {
      .operation = wgpu::BlendOperation::Add,
      .srcFactor = wgpu::BlendFactor::SrcAlpha,
      .dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha,
    }
  };

  wgpu::ColorTargetState colorTargetState{.format = fPreferredFormat, .blend = &blendState};

  wgpu::FragmentState fragmentState{
    .module = fFragmentShaderModule,
    .entryPoint = "fragmentMain",
    .constantCount = 0,
    .constants = nullptr,
    .targetCount = 1,
    .targets = &colorTargetState
  };

  // Defining group(0)
  wgpu::BindGroupLayoutEntry group0BindGroupLayoutEntries[1] = {};
  // @group(0) @binding(0) var<uniform> stInputs: ShaderToyInputs;
  group0BindGroupLayoutEntries[0].binding = 0;
  group0BindGroupLayoutEntries[0].visibility = wgpu::ShaderStage::Fragment;
  group0BindGroupLayoutEntries[0].buffer.type = wgpu::BufferBindingType::Uniform;

  wgpu::BindGroupLayoutDescriptor group0BindGroupLayoutDescriptor = {
    .entryCount = 1,
    .entries = group0BindGroupLayoutEntries
  };

  wgpu::BindGroupLayout layout = device.CreateBindGroupLayout(&group0BindGroupLayoutDescriptor);

  wgpu::PipelineLayoutDescriptor pipeLineLayoutDescriptor = {
    .label = "Fragment Shader Pipeline Layout",
    .bindGroupLayoutCount = 1,
    .bindGroupLayouts = &layout
  };

  wgpu::RenderPipelineDescriptor renderPipelineDescriptor{
    .label = "Fragment Shader Pipeline",
    .layout = device.CreatePipelineLayout(&pipeLineLayoutDescriptor),
    .vertex{
      .module = vertexShaderModule,
      .entryPoint = "vertexMain"
    },
    .primitive = wgpu::PrimitiveState{},
    .multisample = wgpu::MultisampleState{},
    .fragment = &fragmentState,
  };

  wgpu::BufferDescriptor desc{
    .label = "Fragment Shader | ShaderToyInputs Buffer",
    .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
    .size = MEMALIGN(sizeof(ShaderToyInputs), 16)
  };
  fShaderToyInputsBuffer = device.CreateBuffer(&desc);

  wgpu::BindGroupEntry group0BindGroupEntries[] = {
    { .binding = 0, .buffer = fShaderToyInputsBuffer, .size = MEMALIGN(sizeof(ShaderToyInputs), 16) },
  };

  wgpu::BindGroupDescriptor group0BindGroupDescriptor = {
    .label = "Fragment Shader Group0 Bind Group",
    .layout = layout,
    .entryCount = 1,
    .entries = group0BindGroupEntries
  };

  fGroup0BindGroup = device.CreateBindGroup(&group0BindGroupDescriptor);

  fRenderPipeline = device.CreateRenderPipeline(&renderPipelineDescriptor);
  ASSERT(fRenderPipeline != nullptr, "Cannot create render pipeline");
}
//------------------------------------------------------------------------
// FragmentShaderWindow::createRenderPipeline
//------------------------------------------------------------------------
void FragmentShaderWindow::createRenderPipeline()
{
  fRenderPipeline = nullptr;

  fCurrentFragmentShader = fModel->getFragmentShader();

  auto shader = std::string(kFragmentShaderHeader) + fCurrentFragmentShader;

  // fragment shader
  wgpu::ShaderModuleWGSLDescriptor fragmentShaderModuleWGSLDescriptor{};
  fragmentShaderModuleWGSLDescriptor.code = shader.c_str();
  wgpu::ShaderModuleDescriptor fragmentShaderModuleDescriptor{
    .nextInChain = &fragmentShaderModuleWGSLDescriptor,
    .label = "FragmentShaderWindow | Fragment Shader"
  };
  fModel->fFragmentShaderError = std::nullopt;
  fFragmentShaderModule = fGPU->getDevice().CreateShaderModule(&fragmentShaderModuleDescriptor);
  fFragmentShaderModule.GetCompilationInfo(callbacks::onShaderCompilationResult, this);
}

//------------------------------------------------------------------------
// FragmentShaderWindow::beforeFrame
//------------------------------------------------------------------------
void FragmentShaderWindow::beforeFrame()
{
  if(fModel->fAspectRatioRequest)
  {
    auto aspectRatio = *fModel->fAspectRatioRequest;
    fModel->fAspectRatioRequest = std::nullopt;
    glfwSetWindowAspectRatio(fWindow, aspectRatio.numerator, aspectRatio.denominator);
  }

  Window::beforeFrame();

  if(fCurrentFragmentShader != fModel->getFragmentShader())
  {
    createRenderPipeline();
    resetTime();
  }
  fShaderToyInputs.frame++;
  fShaderToyInputs.time = static_cast<gpu::f32>(glfwGetTime());
}

//------------------------------------------------------------------------
// FragmentShaderWindow::doRender
//------------------------------------------------------------------------
void FragmentShaderWindow::doRender(wgpu::RenderPassEncoder &iRenderPass)
{
  if(fRenderPipeline)
  {
    fGPU->getDevice().GetQueue().WriteBuffer(fShaderToyInputsBuffer, 0, &fShaderToyInputs, sizeof(ShaderToyInputs));
    iRenderPass.SetPipeline(fRenderPipeline);
    iRenderPass.SetBindGroup(0, fGroup0BindGroup);
    iRenderPass.Draw(6);
  }
}

//------------------------------------------------------------------------
// FragmentShaderWindow::doHandleFrameBufferSizeChange
//------------------------------------------------------------------------
void FragmentShaderWindow::doHandleFrameBufferSizeChange(Renderable::Size const &iSize)
{
  Window::doHandleFrameBufferSizeChange(iSize);
  fShaderToyInputs.size = {static_cast<float>(iSize.width), static_cast<float>(iSize.height)};
  int w, h;
  glfwGetWindowSize(fWindow, &w, &h);
  fPreferences->storeSize(kPreferencesSizeKey, {w, h});
}

//------------------------------------------------------------------------
// FragmentShaderWindow::onMousePosChange
//------------------------------------------------------------------------
void FragmentShaderWindow::onMousePosChange(double xpos, double ypos)
{
  float xScale, yScale;
  glfwGetWindowContentScale(fWindow, &xScale, &yScale);
  fShaderToyInputs.mouse = {static_cast<float>(xpos * xScale), static_cast<float>(ypos * yScale)};
}

//------------------------------------------------------------------------
// FragmentShaderWindow::resetTime
//------------------------------------------------------------------------
void FragmentShaderWindow::resetTime()
{
  fShaderToyInputs.frame = 0;
  glfwSetTime(0);
}


}
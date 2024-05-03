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
#include "FragmentShader.h"
#include "Errors.h"
#include <GLFW/glfw3.h>
#include <map>

#include <utility>

extern "C" {
void wgpu_shader_toy_print_stack_trace(char const *iMessage);
}

namespace shader_toy {

constexpr char kVertexShader[] = R"(
@vertex
fn vertexMain(@builtin(vertex_index) i : u32) -> @builtin(position) vec4f {
    const pos = array(vec2f(-1, 1), vec2f(-1, -1), vec2f(1, -1), vec2f(-1, 1), vec2f(1, -1), vec2f(1, 1));
    return vec4f(pos[i], 0, 1);
}
)";

namespace callbacks {


struct FragmentShaderCompilationRequest
{
  std::shared_ptr<FragmentShaderWindow> fFragmentShaderWindow;
  std::shared_ptr<FragmentShader> fFragmentShader;
  wgpu::ShaderModule fShaderModule{};
};

static auto kLastCompilationKey = 0;
static std::map<void *, std::shared_ptr<FragmentShaderCompilationRequest>> kFragmentShaderCompilationRequests{};

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
  if(kFragmentShaderCompilationRequests.find(iUserdata) != kFragmentShaderCompilationRequests.end())
  {
    auto request = kFragmentShaderCompilationRequests[iUserdata];
    printf("onShaderCompilation [%s] %d\n", request->fFragmentShader->getName().c_str(), iStatus);
    kFragmentShaderCompilationRequests.erase(iUserdata);
    request->fFragmentShaderWindow->onShaderCompilationResult(request->fFragmentShader,
                                                              request->fShaderModule,
                                                              static_cast<wgpu::CompilationInfoRequestStatus>(iStatus),
                                                              iCompilationInfo);
  }
}

}

//------------------------------------------------------------------------
// FragmentShaderWindow::FragmentShaderWindow
//------------------------------------------------------------------------
FragmentShaderWindow::FragmentShaderWindow(std::shared_ptr<gpu::GPU> iGPU, Window::Args const &iWindowArgs, Args const &iMainWindowArgs) :
  Window(std::move(iGPU), iWindowArgs),
  fPreferences{iMainWindowArgs.preferences}
{
  // adjust size according to preferences
  resize(fPreferences->loadSize(kPreferencesSizeKey, iWindowArgs.size));
  fFrameBufferSize = getFrameBufferSize();
  glfwSetCursorPosCallback(fWindow, callbacks::onCursorPosChange);
  initGPU();
}

//------------------------------------------------------------------------
// FragmentShaderWindow::~FragmentShaderWindow
//------------------------------------------------------------------------
FragmentShaderWindow::~FragmentShaderWindow()
{
  callbacks::kFragmentShaderCompilationRequests.clear();
}

#define MEMALIGN(_SIZE,_ALIGN)        (((_SIZE) + ((_ALIGN) - 1)) & ~((_ALIGN) - 1))    // Memory align (copied from IM_ALIGN() macro).

//------------------------------------------------------------------------
// FragmentShaderWindow::initGPU
//------------------------------------------------------------------------
void FragmentShaderWindow::initGPU()
{
  auto device = fGPU->getDevice();

  // Defining group(0)
  wgpu::BindGroupLayoutEntry group0BindGroupLayoutEntries[1] = {};
  // @group(0) @binding(0) var<uniform> inputs: ShaderToyInputs;
  group0BindGroupLayoutEntries[0].binding = 0;
  group0BindGroupLayoutEntries[0].visibility = wgpu::ShaderStage::Fragment;
  group0BindGroupLayoutEntries[0].buffer.type = wgpu::BufferBindingType::Uniform;

  wgpu::BindGroupLayoutDescriptor group0BindGroupLayoutDescriptor = {
    .entryCount = 1,
    .entries = group0BindGroupLayoutEntries
  };

  fGroup0BindGroupLayout = device.CreateBindGroupLayout(&group0BindGroupLayoutDescriptor);

  wgpu::BufferDescriptor desc{
    .label = "FragmentShaderWindow | ShaderToyInputs Buffer",
    .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
    .size = MEMALIGN(sizeof(FragmentShader::ShaderToyInputs), 16)
  };
  fShaderToyInputsBuffer = device.CreateBuffer(&desc);

  wgpu::BindGroupEntry group0BindGroupEntries[] = {
    { .binding = 0, .buffer = fShaderToyInputsBuffer, .size = MEMALIGN(sizeof(FragmentShader::ShaderToyInputs), 16) },
  };

  wgpu::BindGroupDescriptor group0BindGroupDescriptor = {
    .label = "Fragment Shader Group0 Bind Group",
    .layout = fGroup0BindGroupLayout,
    .entryCount = 1,
    .entries = group0BindGroupEntries
  };

  fGroup0BindGroup = device.CreateBindGroup(&group0BindGroupDescriptor);

  // vertex shader
  wgpu::ShaderModuleWGSLDescriptor vertexShaderModuleWGSLDescriptor{};
  vertexShaderModuleWGSLDescriptor.code = kVertexShader;
  wgpu::ShaderModuleDescriptor vertexShaderModuleDescriptor{
    .nextInChain = &vertexShaderModuleWGSLDescriptor,
    .label = "FragmentShaderWindow | Vertex Shader"
  };
  fVertexShaderModule = device.CreateShaderModule(&vertexShaderModuleDescriptor);
}



//------------------------------------------------------------------------
// FragmentShaderWindow::compile
//------------------------------------------------------------------------
void FragmentShaderWindow::compile(std::shared_ptr<FragmentShader> const &iFragmentShader)
{
//  wgpu_shader_toy_print_stack_trace("FragmentShaderWindow::compile");
  auto shader = std::string(FragmentShader::kHeader) + iFragmentShader->getCode();

  // fragment shader
  wgpu::ShaderModuleWGSLDescriptor fragmentShaderModuleWGSLDescriptor{};
  fragmentShaderModuleWGSLDescriptor.code = shader.c_str();
  wgpu::ShaderModuleDescriptor fragmentShaderModuleDescriptor{
    .nextInChain = &fragmentShaderModuleWGSLDescriptor,
    .label = "FragmentShaderWindow | Fragment Shader"
  };

  iFragmentShader->fState = FragmentShader::State::Compiling{};

  auto shaderModule = fGPU->getDevice().CreateShaderModule(&fragmentShaderModuleDescriptor);

  auto key = reinterpret_cast<void *>(callbacks::kLastCompilationKey++);

  callbacks::kFragmentShaderCompilationRequests[key] =
    std::make_unique<callbacks::FragmentShaderCompilationRequest>(callbacks::FragmentShaderCompilationRequest{
      .fFragmentShaderWindow = shared_from_this(),
      .fFragmentShader = iFragmentShader,
      .fShaderModule = shaderModule
    });

  shaderModule.GetCompilationInfo(callbacks::onShaderCompilationResult, key);
}

//------------------------------------------------------------------------
// FragmentShaderWindow::onShaderCompilationResult
//------------------------------------------------------------------------
void FragmentShaderWindow::onShaderCompilationResult(std::shared_ptr<FragmentShader> const &iFragmentShader,
                                                     wgpu::ShaderModule iShaderModule,
                                                     wgpu::CompilationInfoRequestStatus iStatus,
                                                     WGPUCompilationInfo const *iCompilationInfo)
{
//  wgpu_shader_toy_print_stack_trace("FragmentShaderWindow::onShaderCompilationResult");

  // sanity check
  ASSERT(iFragmentShader->isCompiling());

  // at this time, it always returns success even when failure...
//  if(iStatus != wgpu::CompilationInfoRequestStatus::Success)
//  {
//    printf("Fragment shader compilation error\n");
//    if(iCompilationInfo)
//    {
//      for(auto i = 0; i < iCompilationInfo->messageCount; i++)
//      {
//        printf("Message[%d] | %s", i, iCompilationInfo->messages[0].message);
//      }
//    }
//    return;
//  }

  // compilation resulted in error
  if(fGPU->hasError())
  {
    auto error = fGPU->consumeError();
    printf("onShaderCompilationResult (%s) GPU in error => %s\n",
           iFragmentShader->getName().c_str(),
           gpu::GPU::errorTypeAsString(error->fType));
    iFragmentShader->fState = FragmentShader::State::CompiledInError{.fErrorMessage = fmt::printf("[%s]: %s", gpu::GPU::errorTypeAsString(error->fType), error->fMessage) };
    return;
  }

  auto device = fGPU->getDevice();

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
    .module = std::move(iShaderModule),
    .entryPoint = "fragmentMain",
    .constantCount = 0,
    .constants = nullptr,
    .targetCount = 1,
    .targets = &colorTargetState
  };

  wgpu::PipelineLayoutDescriptor pipeLineLayoutDescriptor = {
    .label = "Fragment Shader Pipeline Layout",
    .bindGroupLayoutCount = 1,
    .bindGroupLayouts = &fGroup0BindGroupLayout
  };

  wgpu::RenderPipelineDescriptor renderPipelineDescriptor{
    .label = "Fragment Shader Pipeline",
    .layout = device.CreatePipelineLayout(&pipeLineLayoutDescriptor),
    .vertex{
      .module = fVertexShaderModule,
      .entryPoint = "vertexMain"
    },
    .primitive = wgpu::PrimitiveState{},
    .multisample = wgpu::MultisampleState{},
    .fragment = &fragmentState,
  };

  auto pipeline = device.CreateRenderPipeline(&renderPipelineDescriptor);
  ASSERT(pipeline != nullptr, "Cannot create render pipeline");

  iFragmentShader->fState = FragmentShader::State::Compiled{.fRenderPipeline = std::move(pipeline)};
  if(fCurrentFragmentShader == iFragmentShader)
    initCurrentFragmentShader();
}

//------------------------------------------------------------------------
// FragmentShaderWindow::setCurrentFragmentShader
//------------------------------------------------------------------------
void FragmentShaderWindow::setCurrentFragmentShader(std::shared_ptr<FragmentShader> iFragmentShader)
{
  fCurrentFragmentShader = std::move(iFragmentShader);
  if(fCurrentFragmentShader->isNotCompiled())
    compile(fCurrentFragmentShader);
  else
  {
    if(fCurrentFragmentShader->isCompiled())
      initCurrentFragmentShader();
  }
}

//------------------------------------------------------------------------
// FragmentShaderWindow::initCurrentFragmentShader
//------------------------------------------------------------------------
void FragmentShaderWindow::initCurrentFragmentShader()
{
  if(fCurrentFragmentShader)
  {
    fCurrentFragmentShader->fStartTime = glfwGetTime();
    fCurrentFragmentShader->fInputs.size = {static_cast<float>(fFrameBufferSize.width), static_cast<float>(fFrameBufferSize.height)};
    fCurrentFragmentShader->fInputs.frame = 0;
  }
}

//------------------------------------------------------------------------
// FragmentShaderWindow::beforeFrame
//------------------------------------------------------------------------
void FragmentShaderWindow::beforeFrame()
{
  Window::beforeFrame();

  if(fCurrentFragmentShader && fCurrentFragmentShader->isCompiled())
  {
    fCurrentFragmentShader->fInputs.frame++;
    fCurrentFragmentShader->fInputs.time = static_cast<gpu::f32>(glfwGetTime() - fCurrentFragmentShader->fStartTime);
  }
}

//------------------------------------------------------------------------
// FragmentShaderWindow::doRender
//------------------------------------------------------------------------
void FragmentShaderWindow::doRender(wgpu::RenderPassEncoder &iRenderPass)
{
  if(fCurrentFragmentShader && fCurrentFragmentShader->isCompiled())
  {
    fGPU->getDevice().GetQueue().WriteBuffer(fShaderToyInputsBuffer, 0, &fCurrentFragmentShader->fInputs, sizeof(ShaderToyInputs));
    iRenderPass.SetPipeline(fCurrentFragmentShader->getRenderPipeline());
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
  fFrameBufferSize = iSize;
  if(fCurrentFragmentShader)
    fCurrentFragmentShader->fInputs.size = {static_cast<float>(fFrameBufferSize.width), static_cast<float>(fFrameBufferSize.height)};
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
  if(fCurrentFragmentShader)
    fCurrentFragmentShader->fInputs.mouse = {static_cast<float>(xpos * xScale), static_cast<float>(ypos * yScale)};
}


}
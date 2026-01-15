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

//extern "C" {
//void wgpu_shader_toy_print_stack_trace(char const *iMessage);
//}

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

// Only one at a time...
static std::unique_ptr<FragmentShaderCompilationRequest> kFragmentShaderCompilationRequest{};

//------------------------------------------------------------------------
// callbacks::onFrameBufferSizeChange
//------------------------------------------------------------------------
void onCursorPosChange(GLFWwindow *window, double xpos, double ypos)
{
  auto w = reinterpret_cast<FragmentShaderWindow *>(glfwGetWindowUserPointer(window));
  w->onMousePosChange(xpos, ypos);
}

//------------------------------------------------------------------------
// callbacks::onContentScaleChange
//------------------------------------------------------------------------
void onContentScaleChange(GLFWwindow *window, float xScale, float yScale)
{
//  printf("onContentScaleChange %f%f\n", xScale, yScale);
  auto w = reinterpret_cast<FragmentShaderWindow *>(glfwGetWindowUserPointer(window));
  w->onContentScaleChange({xScale, yScale});

}

//------------------------------------------------------------------------
// callbacks::onShaderCompilationResult
// implementation note: API is using webgpu.h style, not wegpu_cpp.h
//------------------------------------------------------------------------
void onShaderCompilationResult(WGPUCompilationInfoRequestStatus iStatus,
                               WGPUCompilationInfo const *iCompilationInfo,
                               void *iUserData)
{
  WST_INTERNAL_ASSERT(kFragmentShaderCompilationRequest && iUserData == kFragmentShaderCompilationRequest.get());
  auto request = std::move(kFragmentShaderCompilationRequest);
  request->fFragmentShaderWindow->onShaderCompilationResult(request->fFragmentShader,
                                                            request->fShaderModule,
                                                            static_cast<wgpu::CompilationInfoRequestStatus>(iStatus),
                                                            iCompilationInfo);
}

}

//------------------------------------------------------------------------
// FragmentShaderWindow::FragmentShaderWindow
//------------------------------------------------------------------------
FragmentShaderWindow::FragmentShaderWindow(std::shared_ptr<gpu::GPU> iGPU, Window::Args const &iWindowArgs) :
  Window(std::move(iGPU), iWindowArgs)
{
  fFrameBufferSize = getFrameBufferSize();
  glfwSetCursorPosCallback(fWindow, callbacks::onCursorPosChange);
  glfwGetWindowContentScale(fWindow, &fContentScale.x, &fContentScale.y);
  glfwSetWindowContentScaleCallback(fWindow, callbacks::onContentScaleChange);
  initGPU();
}

//------------------------------------------------------------------------
// FragmentShaderWindow::~FragmentShaderWindow
//------------------------------------------------------------------------
FragmentShaderWindow::~FragmentShaderWindow()
{
  callbacks::kFragmentShaderCompilationRequest = nullptr;
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
  wgpu::ShaderSourceWGSL vertexShaderSource{};
  vertexShaderSource.code = kVertexShader;
  wgpu::ShaderModuleDescriptor vertexShaderModuleDescriptor{
    .nextInChain = &vertexShaderSource,
    .label = "FragmentShaderWindow | Vertex Shader"
  };
  fVertexShaderModule = device.CreateShaderModule(&vertexShaderModuleDescriptor);
}



//------------------------------------------------------------------------
// FragmentShaderWindow::compile
//------------------------------------------------------------------------
void FragmentShaderWindow::compile(std::shared_ptr<FragmentShader> iFragmentShader)
{
  if(callbacks::kFragmentShaderCompilationRequest)
  {
    // there is already a shader being compiled... enqueuing until it completes
    if(!iFragmentShader->isCompilationPending())
    {
      iFragmentShader->fState = FragmentShader::State::CompilationPending{};
      fPendingCompilationRequests.emplace_back(std::move(iFragmentShader));
    }
    return;
  }

  auto shader = std::string(FragmentShader::kHeader) + iFragmentShader->getCode();

  // fragment shader
  wgpu::ShaderSourceWGSL fragmentShaderSource{};
  fragmentShaderSource.code = shader.c_str();
  wgpu::ShaderModuleDescriptor fragmentShaderModuleDescriptor{
    .nextInChain = &fragmentShaderSource,
    .label = "FragmentShaderWindow | Fragment Shader"
  };

  iFragmentShader->fState = FragmentShader::State::Compiling{};

  auto shaderModule = fGPU->getDevice().CreateShaderModule(&fragmentShaderModuleDescriptor);

  callbacks::kFragmentShaderCompilationRequest =
    std::make_unique<callbacks::FragmentShaderCompilationRequest>(callbacks::FragmentShaderCompilationRequest{
      .fFragmentShaderWindow = shared_from_this(),
      .fFragmentShader = std::move(iFragmentShader),
      .fShaderModule = shaderModule
    });

  shaderModule.GetCompilationInfo(wgpu::CallbackMode::AllowProcessEvents,
                                  [](wgpu::CompilationInfoRequestStatus iStatus,
                                     const wgpu::CompilationInfo* iCompilationInfo) {
                                    callbacks::onShaderCompilationResult(static_cast<WGPUCompilationInfoRequestStatus>(iStatus),
                                                                         reinterpret_cast<const WGPUCompilationInfo*>(iCompilationInfo),
                                                                         callbacks::kFragmentShaderCompilationRequest.get());
                                  });
}

namespace impl {

//------------------------------------------------------------------------
// impl::computeErrorState
//------------------------------------------------------------------------
std::optional<FragmentShader::State::CompiledInError> computeErrorState(wgpu::CompilationInfoRequestStatus iStatus,
                                                                        WGPUCompilationInfo const *iCompilationInfo)
{
  static auto kHeaderLineCount =
    std::count(std::begin(FragmentShader::kHeader), std::end(FragmentShader::kHeader), '\n') + 1;

  if(iStatus != wgpu::CompilationInfoRequestStatus::Success || !iCompilationInfo)
  {
    return FragmentShader::State::CompiledInError{.fErrorMessage = "Unknown error while compiling the shader"};
  }

  // Handling errors
  for(auto i = 0; i < iCompilationInfo->messageCount; i++)
  {
    auto &message = iCompilationInfo->messages[i];
    if(message.type == WGPUCompilationMessageType_Error)
    {
      auto lineNumber = static_cast<int>(message.lineNum - kHeaderLineCount + 1);
      auto columnNumber = static_cast<int>(message.linePos);
      return FragmentShader::State::CompiledInError{
        .fErrorMessage = fmt::printf("Compilation error: :%d:%d %.*s",
                                     lineNumber,
                                     columnNumber,
                                     static_cast<int>(message.message.length),
                                     message.message.data),
        .fErrorLine = lineNumber,
        .fErrorColumn = columnNumber
      };
    }
  }

  return std::nullopt;
}

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

  // it is possible (rare) that compile() was called again before this async callback is called
  if(iFragmentShader->isCompiling())
  {
    if(auto errorState = impl::computeErrorState(iStatus, iCompilationInfo))
    {
      iFragmentShader->setCompilationError(errorState.value());
      fGPU->consumeError();
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

    // Once the code does not have any error, there could still be a problem
    // if the main entry point (fragmentMain) is missing because the user renamed it
    // or simply cleared the file
    device.PushErrorScope(wgpu::ErrorFilter::Validation);
    auto pipeline = device.CreateRenderPipeline(&renderPipelineDescriptor);
    WST_INTERNAL_ASSERT(pipeline != nullptr, "Cannot create render pipeline");
    device.PopErrorScope(wgpu::CallbackMode::AllowProcessEvents,
                         [iFragmentShader, pipeline, this](wgpu::PopErrorScopeStatus iPopStatus,
                                                           wgpu::ErrorType iErrorType,
                                                           char const *iErrorMessage) {
                           if(iPopStatus == wgpu::PopErrorScopeStatus::Success && iErrorType != wgpu::ErrorType::NoError)
                           {
                             iFragmentShader->setCompilationError({
                               .fErrorMessage = fmt::printf("Validation error: Make sure there is a function called fragmentMain\n%s", iErrorMessage)
                             });
                           }
                           else
                           {
                             iFragmentShader->fState = FragmentShader::State::Compiled{.fRenderPipeline = pipeline};
                             initFragmentShader(iFragmentShader);
                           }
                         });
  }

  // scheduling the next one if there is a pending one
  if(!fPendingCompilationRequests.empty())
  {
    auto shader = fPendingCompilationRequests.front();
    fPendingCompilationRequests.erase(fPendingCompilationRequests.begin());
    shader->fState = FragmentShader::State::NotCompiled{};
    compile(std::move(shader));
  }
}

//------------------------------------------------------------------------
// FragmentShaderWindow::setCurrentFragmentShader
//------------------------------------------------------------------------
void FragmentShaderWindow::setCurrentFragmentShader(std::shared_ptr<FragmentShader> iFragmentShader)
{
  fCurrentFragmentShader = std::move(iFragmentShader);
  if(fCurrentFragmentShader)
  {
    resize(fCurrentFragmentShader->getWindowSize());
  }
}


//------------------------------------------------------------------------
// FragmentShaderWindow::initFragmentShader
//------------------------------------------------------------------------
void FragmentShaderWindow::initFragmentShader(std::shared_ptr<FragmentShader> const &iFragmentShader) const
{
  if(iFragmentShader)
    iFragmentShader->resetTime();
}

//------------------------------------------------------------------------
// FragmentShaderWindow::beforeFrame
//------------------------------------------------------------------------
void FragmentShaderWindow::beforeFrame()
{
  Window::beforeFrame();

  auto currentTime = getCurrentTime();
  auto deltaTime = currentTime - fLastFrameCurrentTime;
  fLastFrameCurrentTime = currentTime;

  if(glfwGetMouseButton(fWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
  {
    if(fMouseClick.x == -1)
    {
      double x,y;
      glfwGetCursorPos(fWindow, &x, &y);
      fMouseClick = adjustSize({static_cast<float>(x), static_cast<float>(y)});
    }
  }
  else
    fMouseClick = {-1, -1};

  if(fCurrentFragmentShader && fCurrentFragmentShader->isEnabled())
  {
    if(fCurrentFragmentShader->isCompiled())
    {
      glfwGetWindowContentScale(fWindow, &fContentScale.x, &fContentScale.y);

      fCurrentFragmentShader->tickTime(deltaTime);

      fCurrentFragmentShader->fInputs.size = {
        static_cast<float>(fFrameBufferSize.width), static_cast<float>(fFrameBufferSize.height),
        fContentScale.x, fContentScale.y
      };
      fCurrentFragmentShader->fInputs.mouse.z = fMouseClick.x;
      fCurrentFragmentShader->fInputs.mouse.w = fMouseClick.y;
    }

    if(fCurrentFragmentShader->isNotCompiled())
      compile(fCurrentFragmentShader);
  }
}

//------------------------------------------------------------------------
// FragmentShaderWindow::doRender
//------------------------------------------------------------------------
void FragmentShaderWindow::doRender(wgpu::RenderPassEncoder &iRenderPass)
{
  if(fCurrentFragmentShader && fCurrentFragmentShader->isEnabled() && fCurrentFragmentShader->isCompiled())
  {
    fGPU->getDevice().GetQueue().WriteBuffer(fShaderToyInputsBuffer,
                                             0,
                                             &fCurrentFragmentShader->fInputs,
                                             sizeof(FragmentShader::ShaderToyInputs));
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
  if(fCurrentFragmentShader && fCurrentFragmentShader->isEnabled())
  {
    fCurrentFragmentShader->fInputs.size.x = static_cast<float>(fFrameBufferSize.width);
    fCurrentFragmentShader->fInputs.size.y = static_cast<float>(fFrameBufferSize.height);
    fCurrentFragmentShader->setWindowSize(getSize());
  }
}

//------------------------------------------------------------------------
// FragmentShaderWindow::onMousePosChange
//------------------------------------------------------------------------
void FragmentShaderWindow::onMousePosChange(double xpos, double ypos)
{
  if(fCurrentFragmentShader && fCurrentFragmentShader->isEnabled())
  {
    auto pos = adjustSize({static_cast<float>(xpos), static_cast<float>(ypos)});
    if(glfwGetWindowAttrib(fWindow, GLFW_HOVERED) == GLFW_TRUE)
    {
      fCurrentFragmentShader->fInputs.mouse.x = std::clamp(pos.x, 0.0f, fCurrentFragmentShader->fInputs.size.x);
      fCurrentFragmentShader->fInputs.mouse.y = std::clamp(pos.y, 0.0f, fCurrentFragmentShader->fInputs.size.y);
    }
  }
}


}
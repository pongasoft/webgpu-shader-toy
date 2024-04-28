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

namespace shader_toy {

constexpr char shaderCode[] = R"(
@vertex
fn vertexMain(@builtin(vertex_index) i : u32) -> @builtin(position) vec4f {
    const pos = array(vec2f(-1, 1), vec2f(-1, -1), vec2f(1, -1), vec2f(-1, 1), vec2f(1, -1), vec2f(1, 1));
    return vec4f(pos[i], 0, 1);
}

@group(0) @binding(0) var<uniform> iSize: vec4f; // x,y WindowSize; z,w FrameBufferSize

@fragment
fn fragmentMain(@builtin(position) pos: vec4f) -> @location(0) vec4f {
    var color = vec4f(0);
    if(pos.x > 100) {
      color = vec4f(1,0,0,1);
    }
    return vec4f(pos.x / iSize.z, pos.y / iSize.w, 0, 1);
}
)";

//------------------------------------------------------------------------
// FragmentShaderWindow::FragmentShaderWindow
//------------------------------------------------------------------------
FragmentShaderWindow::FragmentShaderWindow(std::shared_ptr<GPU> iGPU, Args const &iArgs) :
  Window(std::move(iGPU), iArgs)
{
  createRenderPipeline();
}

#define MEMALIGN(_SIZE,_ALIGN)        (((_SIZE) + ((_ALIGN) - 1)) & ~((_ALIGN) - 1))    // Memory align (copied from IM_ALIGN() macro).

//------------------------------------------------------------------------
// FragmentShaderWindow::createRenderPipeline
//------------------------------------------------------------------------
void FragmentShaderWindow::createRenderPipeline()
{
  wgpu::ShaderModuleWGSLDescriptor wgslDesc{};
  wgslDesc.code = shaderCode;

  auto device = fGPU->getDevice();

  wgpu::ShaderModuleDescriptor shaderModuleDescriptor{.nextInChain = &wgslDesc};
  wgpu::ShaderModule shaderModule = device.CreateShaderModule(&shaderModuleDescriptor);

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
    .module = shaderModule,
    .entryPoint = "fragmentMain",
    .constantCount = 0,
    .constants = nullptr,
    .targetCount = 1,
    .targets = &colorTargetState
  };

  // Defining group(0)
  wgpu::BindGroupLayoutEntry group0BindGroupLayoutEntries[1] = {};
  // @group(0) @binding(0) var<uniforms> iSize: vec4f; // x,y WindowSize; z,w FrameBufferSize
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

  wgpu::RenderPipelineDescriptor descriptor{
    .label = "Fragment Shader Pipeline",
    .layout = device.CreatePipelineLayout(&pipeLineLayoutDescriptor),
    .vertex{
      .module = shaderModule,
      .entryPoint = "vertexMain"
    },
    .primitive = wgpu::PrimitiveState{},
    .multisample = wgpu::MultisampleState{},
    .fragment = &fragmentState,
  };

  fRenderPipeline = device.CreateRenderPipeline(&descriptor);
  ASSERT(fRenderPipeline != nullptr, "Cannot create render pipeline");

  wgpu::BufferDescriptor desc{
    .label = "Fragment Shader Uniform Buffer @group(0) @binding(0)",
    .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
    .size = MEMALIGN(sizeof(float) * 4, 16)
  };
  fSizeBuffer = device.CreateBuffer(&desc);

  wgpu::BindGroupEntry group0BindGroupEntries[] = {
    { .binding = 0, .buffer = fSizeBuffer, .size = MEMALIGN(sizeof(float) * 4, 16) },
  };

  wgpu::BindGroupDescriptor group0BindGroupDescriptor = {
    .label = "Fragment Shader Group0 Bind Group",
    .layout = layout,
    .entryCount = 1,
    .entries = group0BindGroupEntries
  };

  fGroup0BindGroup = device.CreateBindGroup(&group0BindGroupDescriptor);

  updateSizeBuffer();
}

//------------------------------------------------------------------------
// FragmentShaderWindow::doRender
//------------------------------------------------------------------------
void FragmentShaderWindow::doRender(wgpu::RenderPassEncoder &iRenderPass)
{
  iRenderPass.SetPipeline(fRenderPipeline);
  iRenderPass.SetBindGroup(0, fGroup0BindGroup);
  iRenderPass.Draw(6);
}

//------------------------------------------------------------------------
// FragmentShaderWindow::updateSizeBuffer
//------------------------------------------------------------------------
void FragmentShaderWindow::updateSizeBuffer()
{
  int w,h, fbw, fbh;
  glfwGetWindowSize(fWindow, &w, &h);
  glfwGetFramebufferSize(fWindow, &fbw, &fbh);
  float size[4]{static_cast<float>(w), static_cast<float>(h), static_cast<float>(fbw), static_cast<float>(fbh)};
  fGPU->getDevice().GetQueue().WriteBuffer(fSizeBuffer, 0, size, sizeof(float) * 4);
}

//------------------------------------------------------------------------
// FragmentShaderWindow::doHandleFrameBufferSizeChange
//------------------------------------------------------------------------
void FragmentShaderWindow::doHandleFrameBufferSizeChange(Renderable::Size const &iSize)
{
  Window::doHandleFrameBufferSizeChange(iSize);
  updateSizeBuffer();
}

}
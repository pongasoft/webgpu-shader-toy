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

namespace shader_toy {

constexpr char shaderCode[] = R"(
@vertex
fn vertexMain(@builtin(vertex_index) i : u32) -> @builtin(position) vec4f {
    const pos = array(vec2f(-1, 1), vec2f(-1, -1), vec2f(1, -1), vec2f(-1, 1), vec2f(1, -1), vec2f(1, 1));
    return vec4f(pos[i], 0, 1);
}
@fragment
fn fragmentMain(@builtin(position) pos: vec4f) -> @location(0) vec4f {
    let color = (pos + vec4f(1, 1, 1, 1)) / 2.0;
    return vec4f(pos.x, 0, 0, 1);
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

//------------------------------------------------------------------------
// FragmentShaderWindow::createRenderPipeline
//------------------------------------------------------------------------
void FragmentShaderWindow::createRenderPipeline()
{
  wgpu::ShaderModuleWGSLDescriptor wgslDesc{};
  wgslDesc.code = shaderCode;

  wgpu::ShaderModuleDescriptor shaderModuleDescriptor{.nextInChain = &wgslDesc};
  wgpu::ShaderModule shaderModule = fGPU->getDevice().CreateShaderModule(&shaderModuleDescriptor);

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


  wgpu::RenderPipelineDescriptor descriptor{
    .label = "Fragment Shader Pipeline",
    .vertex{
      .module = shaderModule,
      .entryPoint = "vertexMain"
    },
    .primitive = wgpu::PrimitiveState{},
    .multisample = wgpu::MultisampleState{},
    .fragment = &fragmentState,
  };

  fRenderPipeline = fGPU->getDevice().CreateRenderPipeline(&descriptor);
  ASSERT(fRenderPipeline != nullptr, "Cannot create render pipeline");
}

//------------------------------------------------------------------------
// FragmentShaderWindow::doRender
//------------------------------------------------------------------------
void FragmentShaderWindow::doRender(wgpu::RenderPassEncoder &iRenderPass)
{
  iRenderPass.SetPipeline(fRenderPipeline);
  iRenderPass.Draw(6);
}

}
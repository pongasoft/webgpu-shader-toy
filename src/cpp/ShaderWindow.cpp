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

#include "ShaderWindow.h"
#include "Errors.h"

namespace shader_toy {

constexpr char shaderCode[] = R"(
    @vertex fn vertexMain(@builtin(vertex_index) i : u32) -> @builtin(position) vec4f {
        const pos = array(vec2f(-1, 1), vec2f(-1, -1), vec2f(1, -1), vec2f(-1, 1), vec2f(1, -1), vec2f(1, 1));
        return vec4f(pos[i], 0, 1);
    }
    @fragment fn fragmentMain() -> @location(0) vec4f {
        return vec4f(1, 0, 0, 1);
    }
)";

//------------------------------------------------------------------------
// Shader::init
//------------------------------------------------------------------------
void ShaderWindow::init(GPU &iGPU)
{
  createTexture(iGPU, fSize);
  createRenderPipeline(iGPU);
}

//------------------------------------------------------------------------
// Shader::createTexture
//------------------------------------------------------------------------
void ShaderWindow::createTexture(GPU &iGPU, ImVec2 const &iSize)
{
  if(fTexture)
    fTexture.Destroy();

  wgpu::TextureDescriptor tex_desc = {
    .label = "LearnWGPU Main Texture <path>", // use iName.data()
    .usage = wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::RenderAttachment,
    .dimension = wgpu::TextureDimension::e2D,
    .size = {.width = static_cast<uint32_t>(iSize.x), .height = static_cast<uint32_t>(iSize.y)},
    .format = wgpu::TextureFormat::RGBA8Unorm,
    .viewFormats = nullptr
  };
  fTexture = iGPU.getDevice().CreateTexture(&tex_desc);
  SHADER_TOY_ASSERT(fTexture != nullptr, "Cannot create shader texture");
  wgpu::TextureViewDescriptor tex_view_desc = {
    .label = "LearnWGPU Main Texture View <path>",
    .format = tex_desc.format,
    .dimension = wgpu::TextureViewDimension::e2D,
  };
  fTextureView = fTexture.CreateView(&tex_view_desc);
  SHADER_TOY_ASSERT(fTexture != nullptr, "Cannot create shader texture view");
  fSize = iSize;
}

//------------------------------------------------------------------------
// Shader::createRenderPipeline
//------------------------------------------------------------------------
void ShaderWindow::createRenderPipeline(GPU &iGPU)
{
  wgpu::ShaderModuleWGSLDescriptor wgslDesc{};
  wgslDesc.code = shaderCode;

  wgpu::ShaderModuleDescriptor shaderModuleDescriptor{.nextInChain = &wgslDesc};
  wgpu::ShaderModule shaderModule = iGPU.getDevice().CreateShaderModule(&shaderModuleDescriptor);

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

  wgpu::ColorTargetState colorTargetState{.format = wgpu::TextureFormat::RGBA8Unorm, .blend = &blendState};

  wgpu::FragmentState fragmentState{
    .module = shaderModule,
    .entryPoint = "fragmentMain",
    .constantCount = 0,
    .constants = nullptr,
    .targetCount = 1,
    .targets = &colorTargetState
  };


  wgpu::RenderPipelineDescriptor descriptor{
    .label = "LearnWebGPU Main Pipeline",
    .vertex{
      .module = shaderModule,
      .entryPoint = "vertexMain"
    },
    .primitive = wgpu::PrimitiveState{},
    .multisample = wgpu::MultisampleState{},
    .fragment = &fragmentState,
  };

  fRenderPipeline = iGPU.getDevice().CreateRenderPipeline(&descriptor);
  SHADER_TOY_ASSERT(fRenderPipeline != nullptr, "Cannot create render pipeline");

}

//------------------------------------------------------------------------
// Shader::render
//------------------------------------------------------------------------
void ShaderWindow::render(GPU &iGPU)
{
  iGPU.renderPass(wgpu::Color{}, [pipeline = fRenderPipeline](wgpu::RenderPassEncoder &iRenderPass) {
                    iRenderPass.SetPipeline(pipeline);
                    iRenderPass.Draw(6);
                  },
                  fTextureView);
}

}
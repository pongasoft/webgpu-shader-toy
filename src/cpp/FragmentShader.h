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

#ifndef WGPU_SHADER_TOY_FRAGMENT_SHADER_H
#define WGPU_SHADER_TOY_FRAGMENT_SHADER_H

#include <imgui.h>
#include <string>
#include <variant>
#include <webgpu/webgpu_cpp.h>

namespace pongasoft::gpu {
using vec2f = ImVec2;
using vec4f = ImVec4;
using f32 = float;
using i32 = int;
}

using namespace pongasoft;

namespace shader_toy {

class FragmentShader
{
public:
  static constexpr char kHeader[] = R"(// Begin ShaderToy Header
struct ShaderToyInputs {
  size:         vec2f, // size of the viewport (in pixels)
  mouse:        vec2f, // mouse position (in viewport coordinates [0 ... size.x, 0 ... size.y])
  customFloat1: vec4f, // custom float 1 ([0, 1] range)
  customColor1: vec4f, // custom color 1 ([0, 1] range)
  time:         f32,   // time in seconds (since start/reset)
  frame:        i32,   // frame count (since start/reset)
};

@group(0) @binding(0) var<uniform> inputs: ShaderToyInputs;
// End ShaderToy Header

)";

  static constexpr char kHeaderTemplate[] = R"(struct ShaderToyInputs {
  size:         vec2f, [%d, %d]
  mouse:        vec2f, [%d, %d]
  customFloat1: vec4f, [%.3f, %.3f, %.3f, %.3f]
  customColor1: vec4f, [%.3f, %.3f, %.3f, %.3f]
  time:         f32,   [%.2f]
  frame:        i32,   [%d]
};)";

  struct ShaderToyInputs
  {
    gpu::vec2f size{};
    gpu::vec2f mouse{};
    gpu::vec4f customFloat1{};
    gpu::vec4f customColor1{0, 0, 0, 1.0f};
    gpu::f32 time{};
    gpu::i32 frame{};
  };

  struct State {
    enum class NotCompiled {};
    enum class Compiling{};
    struct CompiledInError { std::string fErrorMessage; };
    struct Compiled { wgpu::RenderPipeline fRenderPipeline; };
  };

  using state_t = std::variant<State::NotCompiled, State::Compiling, State::CompiledInError, State::Compiled>;

public:
  FragmentShader(std::string_view iName, std::string_view iCode) : fName{iName}, fCode{iCode} {}

  inline ShaderToyInputs const &getInputs() const { return fInputs; }

  std::string const &getName() const { return fName; }
  std::string const &getCode() const { return fCode; }

  constexpr bool hasCompilationError() const { return std::holds_alternative<FragmentShader::State::CompiledInError>(fState); }
  inline std::string getCompilationError() const { return std::get<FragmentShader::State::CompiledInError>(fState).fErrorMessage; }
  constexpr bool isCompiled() const { return std::holds_alternative<FragmentShader::State::Compiled>(fState); }

  void setStartTime(double iTime) { fStartTime = iTime; fInputs.time = 0; fInputs.frame = 0; }
  void toggleRunning(double iCurrentTime) {
    if(fRunning)
      fInputs.time = static_cast<gpu::f32>(iCurrentTime - fStartTime);
    else
      fStartTime = iCurrentTime - fInputs.time;
    fRunning = ! fRunning;
  }
  constexpr bool isRunning() const { return fRunning; }

  gpu::vec4f &getCustomFloat1() { return fInputs.customFloat1; };
  gpu::vec4f &getCustomColor1() { return fInputs.customColor1; };

  friend class FragmentShaderWindow;

private:
  constexpr bool isCompiling() const { return std::holds_alternative<FragmentShader::State::Compiling>(fState); }
  constexpr bool isNotCompiled() const { return std::holds_alternative<FragmentShader::State::NotCompiled>(fState); }
  inline wgpu::RenderPipeline getRenderPipeline() const { return std::get<FragmentShader::State::Compiled>(fState).fRenderPipeline; }

private:
  std::string fName;
  std::string fCode;

  ShaderToyInputs fInputs{};

  state_t fState{State::NotCompiled{}};
  double fStartTime{};

  bool fRunning{true};
};

}


#endif //WGPU_SHADER_TOY_FRAGMENT_SHADER_H
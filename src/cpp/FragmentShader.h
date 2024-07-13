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
#include <optional>
#include <webgpu/webgpu_cpp.h>
#include "TextEditor.h"

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
  size:  vec4f,
  mouse: vec4f,
  time:  f32,
  frame: i32,
};

@group(0) @binding(0) var<uniform> inputs: ShaderToyInputs;
// End ShaderToy Header

)";

  static constexpr char kHeaderTemplate[] = R"(struct ShaderToyInputs {
  size:         vec4f, [%d, %d, %.2f, %.2f]
  mouse:        vec4f, [%d, %d, %d, %d]
  time:         f32,   [%.2f]
  frame:        i32,   [%d]
};)";

  struct ShaderToyInputs
  {
    gpu::vec4f size{};
    gpu::vec4f mouse{};
    gpu::f32 time{};
    gpu::i32 frame{};
  };

  struct State {
    enum class NotCompiled {};
    enum class Compiling{};
    struct CompiledInError { std::string fErrorMessage; int fErrorLine{-1}; int fErrorColumn{}; };
    struct Compiled { wgpu::RenderPipeline fRenderPipeline; };
  };

  using state_t = std::variant<State::NotCompiled, State::Compiling, State::CompiledInError, State::Compiled>;

public:
  explicit FragmentShader(Shader const &iShader) : fName{iShader.fName}, fCode{iShader.fCode}, fWindowSize{iShader.fWindowSize} {}

  inline ShaderToyInputs const &getInputs() const { return fInputs; }

  std::string const &getName() const { return fName; }
  void setName(std::string iName) { fName = std::move(iName); }
  std::string const &getCode() const { return fCode; }
  gpu::Renderable::Size const &getWindowSize() const { return fWindowSize; }
  void setWindowSize(gpu::Renderable::Size const &iSize) { fWindowSize = iSize; }

  constexpr bool hasCompilationError() const { return std::holds_alternative<FragmentShader::State::CompiledInError>(fState); }
  inline std::string getCompilationErrorMessage() const { return std::get<FragmentShader::State::CompiledInError>(fState).fErrorMessage; }
  inline int getCompilationErrorLine() const { return std::get<FragmentShader::State::CompiledInError>(fState).fErrorLine; }
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

  constexpr bool isEnabled() const { return fEnabled; }
  constexpr void toggleEnabled() { fEnabled = !fEnabled; }

  constexpr char const* getStatus() const
  {
    if(isCompiled())
    {
      if(isEnabled())
        return isRunning() ? "Running" : "Paused";
      else
        return "Disabled";
    }
    else
    {
      if(hasCompilationError())
        return "Error";
      else
        return "Compiling...";
    }
  }

  void nextFrame(double iCurrentTime, int frameCount = 1)
  {
    if(fRunning)
    {
      fRunning = false;
      frameCount = 1;
    }

    fStartTime = iCurrentTime - fInputs.time;
    fInputs.frame += frameCount;
    fInputs.time = static_cast<gpu::f32>(iCurrentTime - fStartTime) + static_cast<float>(frameCount) / 60.0f;
  }

  void previousFrame(double iCurrentTime, int frameCount = 1)
  {
    if(fRunning)
    {
      fRunning = false;
      frameCount = 1;
    }

    fStartTime = iCurrentTime - fInputs.time;
    fInputs.frame = std::max(0, fInputs.frame - frameCount);
    fInputs.time =
      std::max(0.0f, static_cast<gpu::f32>(iCurrentTime - fStartTime) - static_cast<float>(frameCount) / 60.0f);
  }

  TextEditor &edit() {
    if(!fTextEditor)
    {
      fTextEditor = TextEditor{};
      fTextEditor->SetLanguageDefinition(TextEditor::LanguageDefinitionId::None);
      fTextEditor->SetText(fCode);
      fTextEditor->SetShowWhitespacesEnabled(false);
    }
    return fTextEditor.value();
  }

  void updateCode(std::string iCode)
  {
    fCode = std::move(iCode);
    fState = State::NotCompiled{};
  }

  friend class FragmentShaderWindow;

private:
  constexpr bool isCompiling() const { return std::holds_alternative<FragmentShader::State::Compiling>(fState); }
  constexpr bool isNotCompiled() const { return std::holds_alternative<FragmentShader::State::NotCompiled>(fState); }
  inline wgpu::RenderPipeline getRenderPipeline() const { return std::get<FragmentShader::State::Compiled>(fState).fRenderPipeline; }

private:
  std::string fName;
  std::string fCode;
  gpu::Renderable::Size fWindowSize;

  ShaderToyInputs fInputs{};

  state_t fState{State::NotCompiled{}};
  double fStartTime{};

  std::optional<TextEditor> fTextEditor{};

  bool fRunning{true};
  bool fEnabled{true};
};

}


#endif //WGPU_SHADER_TOY_FRAGMENT_SHADER_H
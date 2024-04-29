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

#ifndef WGPU_SHADER_TOY_MODEL_H
#define WGPU_SHADER_TOY_MODEL_H

#include <optional>
#include <string>

namespace shader_toy {

constexpr char kFragmentShaderHeader[] = R"(
struct ShaderToyInputs {
  size: vec2f,  // size of the viewport (in pixels)
  mouse: vec2f, // mouse position (in viewport coordinates [0 ... size.x, 0 ... size.y])
};

@group(0) @binding(0) var<uniform> inputs: ShaderToyInputs;
)";

constexpr char kDefaultFragmentShader[] = R"(
@fragment
fn fragmentMain(@builtin(position) pos: vec4f) -> @location(0) vec4f {
    return vec4f(pos.xy / inputs.size, 0, 1);
}
)";


struct Model
{
  std::optional<std::string> fFragmentShaderError{};
  std::string fFragmentShader{kDefaultFragmentShader};
};

}

#endif //WGPU_SHADER_TOY_MODEL_H
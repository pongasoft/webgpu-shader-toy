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

#include <string>
#include <utility>
#include <vector>

namespace shader_toy {

std::vector<std::pair<std::string, std::string>> kFragmentShaderExamples{
  // Hello World
  {"Hello World", R"(@fragment
fn fragmentMain(@builtin(position) pos: vec4f) -> @location(0) vec4f {
    return vec4f(pos.xy / inputs.size, 0, 1);
}
)"},

  // Tutorial
  {"Tutorial", R"(@fragment
fn fragmentMain(@builtin(position) pos: vec4f) -> @location(0) vec4f {
  let period = 5.0;
  let half_period = period / 2.0;
  let cycle_value = inputs.time % period;
  let b = half_period - abs(cycle_value - half_period);
  var color = vec4f(b / half_period, pos.xy / inputs.size, 1);
  if(length(pos.xy - inputs.mouse) <= 50.0) {
    color.a = 0.8;
  }
  return color;
}
)"},

  // Shader Art
  {"Shader Art", R"(fn palette(t: f32) -> vec3f {
  const a = vec3f(0.5, 0.5, 0.5);
  const b = vec3f(0.5, 0.5, 0.5);
  const c = vec3f(1.0, 1.0, 1.0);
  const d = vec3f(0.263, 0.416, 0.557);
  return a + (b * cos(6.28318 * ((c*t) + d)));
}

@fragment
fn fragmentMain(@builtin(position) pos: vec4f) -> @location(0) vec4f {
  var uv = (pos.xy * 2.0 - inputs.size) / inputs.size.y;
  let uv0 = uv;
  var finalColor = vec3f(0.0);

  for(var i = 0; i < 4; i++) {
    uv = fract(uv * 1.5) - 0.5;

    var d = length(uv) * exp(-length(uv0));

    var col = palette(length(uv0) + f32(i) * 0.4 + inputs.time * 0.4);

    d = sin(d * 8.0 + inputs.time) / 8.0;
    d = abs(d);

    d = pow(0.01 / d, 1.2);

    finalColor += col * d;
  }

  return vec4f(finalColor, 1.0);
}

// Ported from Shader Art Coding (https://www.youtube.com/watch?v=f4s1h2YETNY)
)"},
};

}
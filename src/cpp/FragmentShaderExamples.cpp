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

#include <vector>
#include "State.h"


namespace shader_toy {

std::vector<Shader> kBuiltInFragmentShaderExamples{
  // Hello World
  {"Hello World", R"(@fragment
fn fragmentMain(@builtin(position) pos: vec4f) -> @location(0) vec4f {
    return vec4f(pos.xy / inputs.size.xy, 0.5, 1);
}
)"},

  // Tutorial
  {"Tutorial", R"(/*
- You must define a function called fragmentMain which must return a color (vec4f)

- This function is called for each pixel:
  - pos.xy represents the coordinates of the pixel
  - (0, 0) is the top-left corner
  - (inputs.size.x, inputs.size.y) is the bottom right corner

- Inputs are available to the function and are defined this way:
  struct ShaderToyInputs {
    size:  vec4f,
    mouse: vec4f,
    time:  f32,
    frame: i32,
  };
  @group(0) @binding(0) var<uniform> inputs: ShaderToyInputs;

  - inputs.size.xy: the size of the viewport (in pixels)
  - inputs.size.zw: the scale ((1.0,1.0) for low res)
  - inputs.mouse.xy: the mouse position (in viewport coordinates [0 ... inputs.size.x, 0 ... inputs.size.y])
  - inputs.mouse.zw: the position where the LMB was pressed or (-1, -1) if not pressed
  - inputs.time: time in seconds (since start/reset)
  - inputs.frame: frame count (since start/reset)
*/
@fragment
fn fragmentMain(@builtin(position) pos: vec4f) -> @location(0) vec4f {
  // using inputs.time to "animate" the red color
  // sin(x) is in [-1, +1] range => converting to [0, 1] range
  let red = (sin(inputs.time) + 1.0) / 2.0;

  // using pos.xy / inputs.xy for blue (range [0, 1])
  var color = vec4f(red, pos.xy / inputs.size.xy, 1);

  // detecting if the left mouse button is pressed
  if(inputs.mouse.z > -1) {
    // accounting for low/hi res so that the circle "looks" the same
    let circleRadius = 25.0 * inputs.size.z;

    // checking if the pixel is in the circle
    if(circleSDF(pos.xy, inputs.mouse.xy, circleRadius) <= 0) {
      // change the opacity if the pixel is inside a circle where the mouse is clicked
      color.a = 0.8;
    }
  }

  // must return a color (vec4f(red, green, blue, alpha))
  return color;
}

/*
 Circle SDF (Signed Distance Function): distance between point and circle
 - returns < 0 if the point is inside the circle
 - returns 0 if the point is on the circle surface
 - returns > 0 if the point is outside the circle
*/
fn circleSDF(point: vec2f, center: vec2f, radius: f32) -> f32 {
    return length(point - center) - radius;
}
)"},
};

std::vector<std::pair<std::string, std::string>> kExternalFragmentShaderExamples {
  {"Fire", "/shaders/Fire.wgsl"},
  {"Marble", "/shaders/Marble.wgsl"},
  {"Shader Art 1", "/shaders/Shader%20Art%201.wgsl"},
  {"Shader Art 2", "/shaders/Shader%20Art%202.wgsl"},
};
}
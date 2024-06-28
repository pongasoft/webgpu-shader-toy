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

std::vector<Shader> kFragmentShaderExamples{
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
  - inputs.size.zw: the scale ((1,1) for low res, (2,2) for hi-res)
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

  // Shader Art 1
  {"Shader Art 1", R"(fn palette(t: f32) -> vec3f {
  const a = vec3f(0.5, 0.5, 0.5);
  const b = vec3f(0.5, 0.5, 0.5);
  const c = vec3f(1.0, 1.0, 1.0);
  const d = vec3f(0.263, 0.416, 0.557);
  return a + (b * cos(6.28318 * ((c*t) + d)));
}

@fragment
fn fragmentMain(@builtin(position) pos: vec4f) -> @location(0) vec4f {
  var uv = (pos.xy * 2.0 - inputs.size.xy) / inputs.size.y;
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

  // Shader Art 2
  {"Shader Art 2", R"(fn palette(t: f32) -> vec3f {
  const a = vec3f(0.5, 0.5, 0.5);
  const b = vec3f(0.5, 0.5, 0.5);
  const c = vec3f(1.0, 1.0, 1.0);
  const d = vec3f(0.263, 0.416, 0.557);
  return a + (b * cos(6.28318 * ((c*t) + d)));
}

fn sdOctahedron(p: vec3f, s: f32) -> f32 {
  let q = abs(p);
  return (q.x + q.y + q.z - s) * 0.57735027;
}

fn rot2D(angle: f32) -> mat2x2<f32> {
  let s = sin(angle);
  let c = cos(angle);
  return mat2x2<f32>(c, -s, s, c);
}

fn fmod(a: f32, b: f32) -> f32 {
    return a - floor(a / b) * b;
}

fn map(p: vec3f) -> f32 {
  var q = p;

  q.z += inputs.time * .4f;

  q.x = fract(q.x) - 0.5;
  q.y = fract(q.y) - 0.5;
  q.z = fmod(q.z, 0.25f) - 0.125f;

  let box = sdOctahedron(q, 0.15f);

  return box;
}

@fragment
fn fragmentMain(@builtin(position) pos: vec4f) -> @location(0) vec4f {
  let iResolution = inputs.size.xy; // match naming in video
  let iTime = inputs.time; // match naming in video
  let fragCoord = vec2f(pos.x, iResolution.y - pos.y); // match naming in video + glsl coordinates
  var uv = (fragCoord * 2.0 - iResolution.xy) / iResolution.y;

  // Initialization
  let ro = vec3f(0, 0, -3);
  let rd = normalize(vec3f(uv, 1));
  var col = vec3f(0);

  var t = f32(0);

  let m = vec2f(cos(iTime * 0.2), sin(iTime * 0.2));

  // Raymarching
  var i = 0;
  for(; i < 80; i++)
  {
    var p = ro + rd * t;

	  p = vec3f(p.xy * rot2D(t * 0.2 * m.x), p.z);

	  p.y += sin(t * (m.y + 1.0) * 0.5) * 0.35;

    var d = map(p);

    t += d;

	  if(d < 0.001 || t > 100.0) { break; }
  }

  // Coloring
  col = palette(t * 0.04f + f32(i) * 0.005);

  return vec4f(col, 1.0);
}

// Ported from Shader Art Coding (https://youtu.be/khblXafu7iA?si=2DLA9nY-aR9_FpYi)
)"},
};

}
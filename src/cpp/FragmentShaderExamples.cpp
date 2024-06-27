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
  {"Tutorial", R"(@fragment
fn fragmentMain(@builtin(position) pos: vec4f) -> @location(0) vec4f {
  let red = (sin(inputs.time) + 1.0) / 2.0; // range [0, 1]
  var color = vec4f(red, pos.xy / inputs.size.xy, 1);
  if(inputs.mouse.z > -1 && length(pos.xy - inputs.mouse.xy) <= 25.0 * inputs.size.z) {
    color.a = 0.8;
  }
  return color;
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
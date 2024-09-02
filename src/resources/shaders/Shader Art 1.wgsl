fn palette(t: f32) -> vec3f {
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

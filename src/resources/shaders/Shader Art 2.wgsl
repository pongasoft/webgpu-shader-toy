fn palette(t: f32) -> vec3f {
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

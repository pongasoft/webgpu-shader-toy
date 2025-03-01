// ported from https://www.shadertoy.com/view/4ttGWM
fn rand(n: vec2f) -> f32 {
  return fract(sin(cos(dot(n, vec2f(12.9898,12.1414)))) * 83758.5453);
}

fn noise(n: vec2f) -> f32 {
  const d = vec2f(0.0, 1.0);
  var b = floor(n);
  var f = smoothstep(vec2f(0.0), vec2f(1.0), fract(n));
  return mix(mix(rand(b), rand(b + d.yx), f.x), mix(rand(b + d.xy), rand(b + d.yy), f.x), f.y);
}

fn fbm(n: vec2f) -> f32 {
  var total = 0.0;
  var amplitude = inputs.size.x / inputs.size.y * 0.5;
  var vn = n;
  for (var i : i32 = 0; i <5; i++) {
      total += noise(vn) * amplitude;
      vn += vn*1.7;
      amplitude *= 0.47;
  }
  return total;
}

@fragment
fn fragmentMain(@builtin(position) pos: vec4f) -> @location(0) vec4f {
  const c1 = vec3f(0.5, 0.0, 0.1);
  const c2 = vec3f(0.9, 0.1, 0.0);
  const c3 = vec3f(0.2, 0.1, 0.7);
  const c4 = vec3f(1.0, 0.9, 0.1);
  const c5 = vec3f(0.1);
  const c6 = vec3f(0.9);

  var iTime = inputs.time;
  var fragCoord = vec2f(pos.x, inputs.size.y - pos.y);
  var iResolution = inputs.size;

  const speed = vec2f(0.1, 0.9);
  var shift = 1.327+sin(iTime*2.0)/2.4;
  const alpha = 1.0;
    
	var dist = 3.5-sin(iTime*0.4)/1.89;
    
  var uv = fragCoord.xy / iResolution.xy;
  var p = fragCoord.xy * dist / iResolution.xx;
  p += sin(p.yx*4.0+vec2f(.2,-.3)*iTime)*0.04;
  p += sin(p.yx*8.0+vec2f(.6,.1)*iTime)*0.01;
    
  p.x -= iTime/1.1;
  var q = fbm(p - iTime * 0.3+1.0*sin(iTime+0.5)/2.0);
  var qb = fbm(p - iTime * 0.4+0.1*cos(iTime)/2.0);
  var q2 = fbm(p - iTime * 0.44 - 5.0*cos(iTime)/2.0) - 6.0;
  var q3 = fbm(p - iTime * 0.9 - 10.0*cos(iTime)/15.0) - 4.0;
  var q4 = fbm(p - iTime * 1.4 - 20.0*sin(iTime)/14.0)+2.0;
  q = (q + qb - .4 * q2 - 2.0 *q3  + .6*q4)/3.8;
  var r = vec2f(fbm(p + q /2.0 + iTime * speed.x - p.x - p.y), fbm(p + q - iTime * speed.y));
  var c = mix(c1, c2, fbm(p + r)) + mix(c3, c4, r.x) - mix(c5, c6, r.y);
  var color = vec3f(1.0/(pow(c+1.61,vec3f(4.0))) * cos(shift * fragCoord.y / iResolution.y));
    
  color = vec3f(1.0,.2,.05)/(pow((r.y+r.y)* max(.0,p.y)+0.1, 4.0));;
//  color += (texture(iChannel0,uv*0.6+vec2f(.5,.1)).xyz*0.01*pow((r.y+r.y)*.65,5.0)+0.055)*mix( vec3f(.9,.4,.3),vec3f(.7,.5,.2), uv.y);
  color = color/(1.0+max(vec3f(0),color));

  return vec4f(color.x, color.y, color.z, alpha);
 }

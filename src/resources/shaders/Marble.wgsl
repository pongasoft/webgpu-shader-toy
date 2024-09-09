// Ported from https://www.shadertoy.com/view/MtX3Ws
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// Created by S. Guillitte 2015

const zoom=1.5;

fn cmul( a: vec2f, b: vec2f ) -> vec2f {
  return vec2f( a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x );
}
fn csqr( a: vec2f ) -> vec2f {
  return vec2f( a.x*a.x - a.y*a.y, 2.*a.x*a.y  );
}


fn rot(a: f32) -> mat2x2f {
	return mat2x2f(cos(a),sin(a),-sin(a),cos(a));	
}

fn iSphere( ro: vec3f, rd: vec3f, sph: vec4f ) -> vec2f
{
	var oc = ro - sph.xyz;
	var b = dot( oc, rd );
	var c = dot( oc, oc ) - sph.w*sph.w;
	var h = b*b - c;
	if( h<0.0 ) { return vec2f(-1.0); }
	h = sqrt(h);
	return vec2f(-b-h, -b+h );
}

fn map(aP: vec3f) -> f32 {
	
	var res = 0.;
	var p = aP;
	
  var c = p;
	for (var i: i32 = 0; i < 10; i++) {
    p =.7*abs(p)/dot(p,p) -.7;
    let tmp = csqr(p.yz);
    p = vec3f(p.x, tmp);
    p=p.zxy;
    res += exp(-19. * abs(dot(p,c)));
	}
	return res/2.;
}



fn raymarch(ro: vec3f, rd: vec3f, tminmax: vec2f ) -> vec3f
{
  var t = tminmax.x;
  var dt = .02;
  //var dt = .2 - .195*cos(inputs.time*.05);//animated
  var col= vec3(0.);
  var c = 0.;
  for(var i: i32 =0; i<64; i++ )
	{
    t+=dt*exp(-2.*c);
    if(t>tminmax.y) { break; }
              
    c = map(ro+t*rd);               
        
    //col = .99*col+ .08*vec3f(c*c, c, c*c*c);//green	
    col = .99*col+ .08*vec3f(c*c*c, c*c, c);//blue
  }    
  return col;
}


@fragment
fn fragmentMain(@builtin(position) pos: vec4f) -> @location(0) vec4f {
  var iTime = inputs.time;
  var fragCoord = vec2f(pos.x, inputs.size.y - pos.y);
  var iResolution = inputs.size;
  var iMouse = inputs.mouse;
  
  var time = iTime;
  var q = fragCoord.xy / iResolution.xy;
  var p = -1.0 + 2.0 * q;
  p.x *= iResolution.x/iResolution.y;
  var m = vec2(0.);
	if( iMouse.z>0.0 ) {
	  m = iMouse.xy/iResolution.xy*3.14;
	}
  m-=.5;

  // camera
  var ro = zoom*vec3f(4.);
  let tmp1 = ro.yz * rot(m.y);
  ro = vec3f(ro.x, tmp1);
  let tmp2 = ro.xz * rot(m.x+ 0.1*time);
  ro = vec3f(tmp2.x, ro.y, tmp2.y);
  var ta = vec3f( 0.0 , 0.0, 0.0 );
  var ww = normalize( ta - ro );
  var uu = normalize( cross(ww,vec3f(0.0,1.0,0.0) ) );
  var vv = normalize( cross(uu,ww));
  var rd = normalize( p.x*uu + p.y*vv + 4.0*ww );

    
  var tmm = iSphere( ro, rd, vec4f(0.,0.,0.,2.) );

	// raymarch
  var col = raymarch(ro,rd,tmm);
  if (tmm.x<0.) {
    // col = texture(iChannel0, rd).rgb;
    col = vec3f(0);
  }
  else {
//     vec3 nor=(ro+tmm.x*rd)/2.;
//     nor = reflect(rd, nor);        
//     float fre = pow(.5+ clamp(dot(nor,rd),0.0,1.0), 3. )*1.3;
//     col += texture(iChannel0, nor).rgb * fre;
  }
	
	// shade
    
  col =  .5 *(log(1.+col));
  //col = clamp(col,0.,1.);
  return vec4f( col, 1.0 );
}

uniform sampler2D img;
uniform float     stepx;
uniform float     stepy;

//nvidia's half is a 16 bit float and can bring some speed improvements
//without affecting quality
#ifndef __GLSL_CG_DATA_TYPES
  #define half3 vec3
  #define half4 vec4
#endif

#if (HAS_FLOAT_TEXTURE)
uniform sampler1D kernelTex;

half4 weight(float pos)
{
  return texture1D(kernelTex, pos);
}
#else
uniform sampler2D kernelTex;

half4 weight(float pos)
{
  //row 0 contains the high byte, row 1 contains the low byte
  return (texture2D(kernelTex, vec2(pos, 0.0)) * 256.0 + texture2D(kernelTex, vec2(pos, 1.0))) / 128.5 - 1.0;
}
#endif

half3 pixel(float xpos, float ypos)
{
  return texture2D(img, vec2(xpos, ypos)).rgb;
}

half3 line (float ypos, vec4 xpos, half4 linetaps)
{
  half3  pixels;

  pixels  = pixel(xpos.r, ypos) * linetaps.r;
  pixels += pixel(xpos.g, ypos) * linetaps.g;
  pixels += pixel(xpos.b, ypos) * linetaps.b;
  pixels += pixel(xpos.a, ypos) * linetaps.a;

  return pixels;
}

void main()
{
  float xf = fract(gl_TexCoord[0].x / stepx);
  float yf = fract(gl_TexCoord[0].y / stepy);

  half4 linetaps   = weight(1.0 - xf);
  half4 columntaps = weight(1.0 - yf);

  vec4 xpos = vec4(
      (-0.5 - xf) * stepx + gl_TexCoord[0].x,
      ( 0.5 - xf) * stepx + gl_TexCoord[0].x,
      ( 1.5 - xf) * stepx + gl_TexCoord[0].x,
      ( 2.5 - xf) * stepx + gl_TexCoord[0].x);

  half3 output;

  output  = line((-0.5 - yf) * stepy + gl_TexCoord[0].y, xpos, linetaps) * columntaps.r;
  output += line(( 0.5 - yf) * stepy + gl_TexCoord[0].y, xpos, linetaps) * columntaps.g;
  output += line(( 1.5 - yf) * stepy + gl_TexCoord[0].y, xpos, linetaps) * columntaps.b;
  output += line(( 2.5 - yf) * stepy + gl_TexCoord[0].y, xpos, linetaps) * columntaps.a;

  gl_FragColor.rgb = output;
  gl_FragColor.a = gl_Color.a;
}


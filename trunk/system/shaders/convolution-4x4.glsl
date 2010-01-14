uniform sampler2D img;
uniform float     stepx;
uniform float     stepy;
uniform sampler2D kernelTex;

vec4 weight(float pos)
{
  return texture2D(kernelTex, vec2(pos, 0.5));
}

vec4 pixel(float xpos, float ypos)
{
  return texture2D(img, vec2(xpos, ypos));
}

vec4 line (float ypos, vec4 xpos, vec4 linetaps)
{
  vec4  pixels;

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

  vec4 linetaps   = weight(1.0 - xf);
  vec4 columntaps = weight(1.0 - yf);

  vec4 xpos = vec4(
      (-0.5 - xf) * stepx + gl_TexCoord[0].x,
      ( 0.5 - xf) * stepx + gl_TexCoord[0].x,
      ( 1.5 - xf) * stepx + gl_TexCoord[0].x,
      ( 2.5 - xf) * stepx + gl_TexCoord[0].x);

  gl_FragColor  = line((-0.5 - yf) * stepy + gl_TexCoord[0].y, xpos, linetaps) * columntaps.r;
  gl_FragColor += line(( 0.5 - yf) * stepy + gl_TexCoord[0].y, xpos, linetaps) * columntaps.g;
  gl_FragColor += line(( 1.5 - yf) * stepy + gl_TexCoord[0].y, xpos, linetaps) * columntaps.b;
  gl_FragColor += line(( 2.5 - yf) * stepy + gl_TexCoord[0].y, xpos, linetaps) * columntaps.a;

  gl_FragColor.a = gl_Color.a;
}


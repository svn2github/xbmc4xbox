/*
* XBoxMediaCenter
* Shader Classes
* Copyright (c) 2007 d4rk
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#ifdef HAS_SDL_OPENGL

#include "YUV2RGBShader.h"
#include <string>

using namespace Shaders;
using namespace std;

//////////////////////////////////////////////////////////////////////
// BaseYUV2RGBShader - base class for YUV2RGB shaders
//////////////////////////////////////////////////////////////////////

BaseYUV2RGBShader::BaseYUV2RGBShader()
{
  m_width    = 0;
  m_height   = 0;
  m_stepX    = 0;
  m_stepY    = 0;
  m_field    = 0;
  m_yTexUnit = 0;
  m_uTexUnit = 0;
  m_vTexUnit = 0;

  // shader attribute handles
  m_hYTex  = 0;
  m_hUTex  = 0;
  m_hVTex  = 0;
  m_hStepX = 0;
  m_hStepY = 0;
  m_hField = 0;

  // default passthrough vertex shader
  string shaderv = 
    "void main()"
    "{"
    "gl_TexCoord[0] = gl_MultiTexCoord0;"
    "gl_TexCoord[1] = gl_MultiTexCoord1;"
    "gl_TexCoord[2] = gl_MultiTexCoord2;"
    "gl_TexCoord[3] = gl_MultiTexCoord3;"
    "gl_Position = ftransform();"
    "}";
  SetVertexShaderSource(shaderv);
}


//////////////////////////////////////////////////////////////////////
// YUV2RGBProgressiveShader - YUV2RGB with no deinterlacing
// Use for weave deinterlacing / progressive
//////////////////////////////////////////////////////////////////////

YUV2RGBProgressiveShader::YUV2RGBProgressiveShader()
{
  string shaderf = 
    "uniform sampler2D ytex;"
    "uniform sampler2D utex;"
    "uniform sampler2D vtex;"
    "void main()"
    "{"
    "vec4 yuv, rgb;"
    "mat4 yuvmat = { 1.0,      1.0,     1.0,     0.0, "
    "                0,       -0.39465, 2.03211, 0.0, "
    "                1.13983, -0.58060, 0.0,     0.0, "
    "                0.0,     0.0,      0.0,     0.0 }; "
    "yuv.rgba = vec4(texture2D(ytex, gl_TexCoord[0].xy).r,"
    "                0.436 * (texture2D(utex, gl_TexCoord[1].xy).r * 2.0 - 1.0),"
    "                0.615 * (texture2D(vtex, gl_TexCoord[2].xy).r * 2.0 - 1.0),"
    "                0.0);"
    "rgb = yuvmat * yuv;"
    "rgb.a = 1.0;"
    "gl_FragColor = rgb;"
    "}";
  SetPixelShaderSource(shaderf);
}

void YUV2RGBProgressiveShader::OnCompiledAndLinked()
{
  // obtain shader attribute handles on successfull compilation
  m_hYTex = glGetUniformLocation(ProgramHandle(), "ytex");
  m_hUTex = glGetUniformLocation(ProgramHandle(), "utex");
  m_hVTex = glGetUniformLocation(ProgramHandle(), "vtex");
}

bool YUV2RGBProgressiveShader::OnEnabled()
{
  // set shader attributes once enabled
  glUniform1i(m_hYTex, m_yTexUnit);
  glUniform1i(m_hUTex, m_uTexUnit);
  glUniform1i(m_hVTex, m_vTexUnit);
}


//////////////////////////////////////////////////////////////////////
// YUV2RGBBobShader - YUV2RGB with Bob deinterlacing
//////////////////////////////////////////////////////////////////////

YUV2RGBBobShader::YUV2RGBBobShader()
{
  string shaderf = 
    "uniform sampler2D ytex;"
    "uniform sampler2D utex;"
    "uniform sampler2D vtex;"
    "uniform float stepX, stepY;"
    "uniform int field;"
    "void main()"
    "{"
    "vec4 yuv, rgb;"
    "vec2 offsetY, offsetUV;"
    "float temp1 = mod(gl_TexCoord[0].y, 2*stepY);"

    "offsetY  = gl_TexCoord[0].xy ;"
    "offsetUV = gl_TexCoord[1].xy ;"
    "offsetY.y  -= (temp1 - stepY/2 + float(field)*stepY);"
    "offsetUV.y -= (temp1 - stepY/2 + float(field)*stepY)/2;"
    "mat4 yuvmat = { 1.0,      1.0,     1.0,     0.0, "
    "                0,       -0.39465, 2.03211, 0.0, "
    "                1.13983, -0.58060, 0.0,     0.0, "
    "                0.0,     0.0,      0.0,     0.0 }; "
    "yuv = vec4(texture2D(ytex, offsetY).r,"
    "           texture2D(utex, offsetUV).r,"
    "           texture2D(vtex, offsetUV).r,"
    "           0.0);"
    "yuv.gba = vec3(0.436 * (yuv.g * 2.0 - 1.0),"
    "           0.615 * (yuv.b * 2.0 - 1.0), 0);"        
    "rgb = yuvmat * yuv;"
    "rgb.a = 1.0;"
    "gl_FragColor = rgb;"
    "}";
  SetPixelShaderSource(shaderf);  
}

void YUV2RGBBobShader::OnCompiledAndLinked()
{
  // obtain shader attribute handles on successfull compilation
  m_hYTex = glGetUniformLocation(ProgramHandle(), "ytex");
  m_hUTex = glGetUniformLocation(ProgramHandle(), "utex");
  m_hVTex = glGetUniformLocation(ProgramHandle(), "vtex");
  m_hStepX = glGetUniformLocation(ProgramHandle(), "stepX");
  m_hStepY = glGetUniformLocation(ProgramHandle(), "stepY");
  m_hField = glGetUniformLocation(ProgramHandle(), "field");
}

bool YUV2RGBBobShader::OnEnabled()
{
  // set shader attributes once enabled
  glUniform1i(m_hYTex, m_yTexUnit);
  glUniform1i(m_hUTex, m_uTexUnit);
  glUniform1i(m_hVTex, m_vTexUnit);
  glUniform1i(m_hField, m_field);
  glUniform1f(m_hStepX, 1.0 / (float)m_width);
  glUniform1f(m_hStepY, 1.0 / (float)m_height);
}


#endif // HAS_SDL_OPENGL

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


#include "include.h"
#include "../RenderFlags.h"
#include "YUV2RGBShader.h"
#include <string>
#include <sstream>

#ifdef HAS_SDL_OPENGL

using namespace Shaders;
using namespace std;

//
// Transformation matrixes for different colorspaces.
//
static float yuv_coef_bt601[4][4] = 
{
    { 1.0,      1.0,     1.0,     0.0 },
    { 0.0,     -0.344,   1.773,   0.0 },
    { 1.403,   -0.714,   0.0,     0.0 },
    { 0.0,      0.0,     0.0,     0.0 } 
};

static float yuv_coef_bt709[4][4] =
{
    { 1.0,      1.0,     1.0,     0.0 },
    { 0.0,     -0.1870,  1.8556,  0.0 },
    { 1.5701,  -0.4664,  0.0,     0.0 },
    { 0.0,      0.0,     0.0,     0.0 }
};

static float yuv_coef_ebu[4][4] = 
{
    { 1.0,      1.0,     1.0,     0.0 },
    { 0.0,     -0.3960,  2.029,   0.0 },
    { 1.140,   -0.581,   0.0,     0.0 },
    { 0.0,      0.0,     0.0,     0.0 }
};

static float yuv_coef_smtp240m[4][4] =
{
    { 1.0,      1.0,     1.0,     0.0 },
    { 0.0,     -0.2253,  1.8270,  0.0 },
    { 1.5756,  -0.5000,  0.0,     0.0 },
    { 0.0,      0.0,     0.0,     0.0 }
};

//////////////////////////////////////////////////////////////////////
// BaseYUV2RGBGLSLShader - base class for GLSL YUV2RGB shaders
//////////////////////////////////////////////////////////////////////

BaseYUV2RGBGLSLShader::BaseYUV2RGBGLSLShader(unsigned flags)
{
  m_width      = 1;
  m_height     = 1;
  m_stepX      = 0;
  m_stepY      = 0;
  m_field      = 0;
  m_yTexUnit   = 0;
  m_uTexUnit   = 0;
  m_vTexUnit   = 0;
  m_flags      = flags;

  // shader attribute handles
  m_hYTex  = -1;
  m_hUTex  = -1;
  m_hVTex  = -1;
  m_hStepX = -1;
  m_hStepY = -1;
  m_hField = -1;

  // default passthrough vertex shader
  string shaderv = 
    "void main()"
    "{"
    "gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;"
    "gl_TexCoord[1] = gl_TextureMatrix[1] * gl_MultiTexCoord1;"
    "gl_TexCoord[2] = gl_TextureMatrix[2] * gl_MultiTexCoord2;"
    "gl_TexCoord[3] = gl_TextureMatrix[3] * gl_MultiTexCoord3;"
    "gl_Position = ftransform();"
    "}";
  SetVertexShaderSource(shaderv);
}

string BaseYUV2RGBGLSLShader::BuildYUVMatrix()
{
  // Pick the matrix.
  float (*matrix)[4];
  switch(CONF_FLAGS_YUVCOEF_MASK(m_flags))
  {
    case CONF_FLAGS_YUVCOEF_240M:
      matrix = yuv_coef_smtp240m; break;
    case CONF_FLAGS_YUVCOEF_BT709:
      matrix = yuv_coef_bt709; break;
    case CONF_FLAGS_YUVCOEF_BT601:    
      matrix = yuv_coef_bt601; break;
    case CONF_FLAGS_YUVCOEF_EBU:
      matrix = yuv_coef_ebu; break;
    default:
      matrix = yuv_coef_bt601; break;
  }
  
  // Convert to GLSL language.
  stringstream strStream;
  strStream << "mat4( \n";
  for (int x=0; x<4; x++)
  {
    strStream << "vec4(";
    
    for (int y=0; y<4; y++)
    {
      strStream << matrix[x][y];
      if (y != 3)
        strStream << ", ";
    }
    
    strStream << ")";
    if (x != 3)
      strStream << ", ";
    strStream << " \n";
  }
  
  strStream << "); \n";
  return strStream.str();
}


//////////////////////////////////////////////////////////////////////
// BaseYUV2RGBGLSLShader - base class for GLSL YUV2RGB shaders
//////////////////////////////////////////////////////////////////////

BaseYUV2RGBARBShader::BaseYUV2RGBARBShader(unsigned flags)
{
  m_width         = 1;
  m_height        = 1;
  m_stepX         = 0;
  m_stepY         = 0;
  m_field         = 0;
  m_yTexUnit      = 0;
  m_uTexUnit      = 0;
  m_vTexUnit      = 0;
  m_flags         = flags;

  // shader attribute handles
  m_hYTex  = -1;
  m_hUTex  = -1;
  m_hVTex  = -1;
  m_hStepX = -1;
  m_hStepY = -1;
  m_hField = -1;
}

//////////////////////////////////////////////////////////////////////
// YUV2RGBProgressiveShader - YUV2RGB with no deinterlacing
// Use for weave deinterlacing / progressive
//////////////////////////////////////////////////////////////////////

YUV2RGBProgressiveShader::YUV2RGBProgressiveShader(bool rect, unsigned flags)
  : BaseYUV2RGBGLSLShader(flags)
{
  string target = "";
  if (rect)
    target="Rect";
  string shaderf = 
    "uniform sampler2D"+target+" ytex;\n"
    "uniform sampler2D"+target+" utex;\n"
    "uniform sampler2D"+target+" vtex;\n"
    "void main()\n"
    "{\n"
    "vec4 yuv, rgb;\n"
    "mat4 yuvmat = " + BuildYUVMatrix();
  
  if (!(flags & CONF_FLAGS_YUV_FULLRANGE))
  {
    shaderf +=
      "yuv.rgba = vec4(((texture2D"+target+"(ytex, gl_TexCoord[0].xy).r - 16.0/256.0) * 1.164383562),\n"
      "                ((texture2D"+target+"(utex, gl_TexCoord[1].xy).r - 16.0/256.0) * 1.138392857 - 0.5),\n"
      "                ((texture2D"+target+"(vtex, gl_TexCoord[2].xy).r - 16.0/256.0) * 1.138392857 - 0.5),\n"
      "                0.0);\n";
  }
  else
  {
    shaderf +=
      "yuv.rgba = vec4(texture2D"+target+"(ytex, gl_TexCoord[0].xy).r,\n"
      "                0.436 * (texture2D"+target+"(utex, gl_TexCoord[1].xy).r * 2.0 - 1.0),\n"
      "                0.615 * (texture2D"+target+"(vtex, gl_TexCoord[2].xy).r * 2.0 - 1.0),\n"
      "                vec4(0.0,      0.0,     0.0,     0.0) ); \n";
  }
  shaderf +=
    "rgb = yuvmat * yuv;\n"
    "rgb.a = 1.0;\n"
    "gl_FragColor = rgb;\n"
    "}";
  SetPixelShaderSource(shaderf);
}

void YUV2RGBProgressiveShader::OnCompiledAndLinked()
{
  // obtain shader attribute handles on successfull compilation
  m_hYTex = glGetUniformLocation(ProgramHandle(), "ytex");
  m_hUTex = glGetUniformLocation(ProgramHandle(), "utex");
  m_hVTex = glGetUniformLocation(ProgramHandle(), "vtex");
  VerifyGLState();
}

bool YUV2RGBProgressiveShader::OnEnabled()
{
  // set shader attributes once enabled
  glUniform1i(m_hYTex, m_yTexUnit);
  glUniform1i(m_hUTex, m_uTexUnit);
  glUniform1i(m_hVTex, m_vTexUnit);
  VerifyGLState();
  return true;
}


//////////////////////////////////////////////////////////////////////
// YUV2RGBBobShader - YUV2RGB with Bob deinterlacing
//////////////////////////////////////////////////////////////////////

YUV2RGBBobShader::YUV2RGBBobShader(bool rect, unsigned flags)
  : BaseYUV2RGBGLSLShader(flags)
{
  string target = "";
  if (rect)
    target="Rect";
  string shaderf = 
    "uniform sampler2D"+target+" ytex;"
    "uniform sampler2D"+target+" utex;"
    "uniform sampler2D"+target+" vtex;"
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
    "mat4 yuvmat = " + BuildYUVMatrix() +
    "yuv = vec4(texture2D"+target+"(ytex, offsetY).r,"
    "           texture2D"+target+"(utex, offsetUV).r,"
    "           texture2D"+target+"(vtex, offsetUV).r,"
    "           0.0);";
    if (!(flags & CONF_FLAGS_YUV_FULLRANGE))
    {
      shaderf +=
        "yuv.rgba = vec4( (yuv.r - 16.0/256.0) * 1.164383562,"
        "           (yuv.g - 16.0/256.0) * 1.138392857 - 0.5, "
        "           (yuv.b - 16.0/256.0) * 1.138392857 - 0.5;";
    }
    else
    {
      shaderf +=
        "yuv.gba = vec3(0.436 * (yuv.g * 2.0 - 1.0),"
        "           0.615 * (yuv.b * 2.0 - 1.0), 0);";        
    }
    shaderf +=
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
  VerifyGLState();
}

bool YUV2RGBBobShader::OnEnabled()
{
  // set shader attributes once enabled
  glUniform1i(m_hYTex, m_yTexUnit);
  glUniform1i(m_hUTex, m_uTexUnit);
  glUniform1i(m_hVTex, m_vTexUnit);
  glUniform1i(m_hField, m_field);
  glUniform1f(m_hStepX, 1.0f / (float)m_width);
  glUniform1f(m_hStepY, 1.0f / (float)m_height);
  VerifyGLState();
  return true;
}

//////////////////////////////////////////////////////////////////////
// YUV2RGBProgressiveShaderARB - YUV2RGB with no deinterlacing
//////////////////////////////////////////////////////////////////////

YUV2RGBProgressiveShaderARB::YUV2RGBProgressiveShaderARB(bool rect, unsigned flags)
  : BaseYUV2RGBARBShader(flags)
{
  string source = "";
  if (!rect)
  {
    source ="!!ARBfp1.0\n"
      "PARAM c[2] = { { 0, -0.1720674, 0.88599992, 1 },\n"
      "		             { 0.70099545, -0.35706902, 0, 2 } };\n"
      "TEMP R0;\n"
      "TEMP R1;\n"
      "TEX R1.x, fragment.texcoord[2], texture[2], 2D;\n"
      "TEX R0.x, fragment.texcoord[1], texture[1], 2D;\n"
      "MUL R0.z, R0.x, c[1].w;\n"
      "MUL R0.y, R1.x, c[1].w;\n"
      "TEX R0.x, fragment.texcoord[0], texture[0], 2D;\n"
      "ADD R0.z, R0, -c[0].w;\n"
      "MAD R1.xyz, R0.z, c[0], R0.x;\n"
      "ADD R0.x, R0.y, -c[0].w;\n"
      "MAD result.color.xyz, R0.x, c[1], R1;\n"
      "MOV result.color.w, c[0];\n"
      "END\n";
  }
  else
  {
    source ="!!ARBfp1.0\n"
      "PARAM c[2] = { { 0, -0.1720674, 0.88599992, 1 },\n"
      "		             { 0.70099545, -0.35706902, 0, 2 } };\n"
      "TEMP R0;\n"
      "TEMP R1;\n"
      "TEX R1.x, fragment.texcoord[2], texture[2], RECT;\n"
      "TEX R0.x, fragment.texcoord[1], texture[1], RECT;\n"
      "MUL R0.z, R0.x, c[1].w;\n"
      "MUL R0.y, R1.x, c[1].w;\n"
      "TEX R0.x, fragment.texcoord[0], texture[0], RECT;\n"
      "ADD R0.z, R0, -c[0].w;\n"
      "MAD R1.xyz, R0.z, c[0], R0.x;\n"
      "ADD R0.x, R0.y, -c[0].w;\n"
      "MAD result.color.xyz, R0.x, c[1], R1;\n"
      "MOV result.color.w, c[0];\n"
      "END\n";
  }
  SetPixelShaderSource(source);
}

void YUV2RGBProgressiveShaderARB::OnCompiledAndLinked()
{

}

bool YUV2RGBProgressiveShaderARB::OnEnabled()
{
  return true;
}


#endif // HAS_SDL_OPENGL

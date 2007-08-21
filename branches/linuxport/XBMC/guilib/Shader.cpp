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

#include "stdafx.h"
#include <GL/glew.h>
#include "../xbmc/Settings.h"
#include "Shader.h"

#ifdef HAS_SDL_OPENGL

using namespace Shaders;
using namespace std;

//////////////////////////////////////////////////////////////////////
// CShader
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
// CVertexShader
//////////////////////////////////////////////////////////////////////

bool CVertexShader::Compile()
{
  GLint params[4]; 

  /* 
     Workaround for locale bug in nVidia's shader compiler.
     Save the current locale, set to a neutral locale while compiling and switch back afterwards.
  */
  char * currentLocale = setlocale(LC_NUMERIC, NULL);
  setlocale(LC_NUMERIC, "C");

  m_vertexShader = glCreateShader(GL_VERTEX_SHADER);
  const char *ptr = m_source.c_str();
  glShaderSource(m_vertexShader, 1, &ptr, 0);
  glCompileShader(m_vertexShader);
  glGetShaderiv(m_vertexShader, GL_COMPILE_STATUS, params);
  if (params[0]!=GL_TRUE) 
  {
    GLchar log[512];
    CLog::Log(LOGERROR, "Error compiling shader");
    glGetShaderInfoLog(m_vertexShader, 512, NULL, log);
    CLog::Log(LOGERROR, (const char*)log);
    m_lastLog = log;
    m_compiled = false;
  }
  else
  {
    GLchar log[512];
    CLog::Log(LOGDEBUG, "Shader compilation log:");
    glGetShaderInfoLog(m_vertexShader, 512, NULL, log);
    CLog::Log(LOGDEBUG, (const char*)log);
    m_lastLog = log;
    m_compiled = true;
  }
  setlocale(LC_NUMERIC, currentLocale);
}

void CVertexShader::Free()
{
  if (m_vertexShader)
    glDeleteShader(m_vertexShader); 
  m_vertexShader = 0;
}


//////////////////////////////////////////////////////////////////////
// CPixelShader
//////////////////////////////////////////////////////////////////////
bool CPixelShader::Compile()
{
  GLint params[4]; 

  Free();

  // Pixel shaders are not mandatory.
  if (m_source.length()==0)
  {
    CLog::Log(LOGNOTICE, "GL: No pixel shader, fixed pipeline in use");
    return true;
  }

  /* 
     Workaround for locale bug in nVidia's shader compiler.
     Save the current locale, set to a neutral while compiling and switch back afterwards.
  */
  char * currentLocale = setlocale(LC_NUMERIC, NULL);
  setlocale(LC_NUMERIC, "C");

  m_pixelShader = glCreateShader(GL_FRAGMENT_SHADER);
  const char *ptr = m_source.c_str();
  glShaderSource(m_pixelShader, 1, &ptr, 0);
  glCompileShader(m_pixelShader);
  glGetShaderiv(m_pixelShader, GL_COMPILE_STATUS, params);
  if (params[0]!=GL_TRUE) 
  {
    GLchar log[512];
    CLog::Log(LOGERROR, "Error compiling shader");
    glGetShaderInfoLog(m_pixelShader, 512, NULL, log);
    CLog::Log(LOGERROR, (const char*)log);
    m_lastLog = log;
    m_compiled = false;
  }
  else
  {
    GLchar log[512];
    CLog::Log(LOGDEBUG, "Shader compilation log:");
    glGetShaderInfoLog(m_pixelShader, 512, NULL, log);
    CLog::Log(LOGDEBUG, (const char*)log);
    m_lastLog = log;
    m_compiled = true;
  }
  setlocale(LC_NUMERIC, currentLocale);
}

void CPixelShader::Free()
{
  if (m_pixelShader) 
    glDeleteShader(m_pixelShader); 
  m_pixelShader = 0; 
}


//////////////////////////////////////////////////////////////////////
// CShaderProgram
//////////////////////////////////////////////////////////////////////
void CShaderProgram::Free()
{
  m_VP.Free();
  m_FP.Free();
  if (m_shaderProgram)
  {
    glDeleteProgram(m_shaderProgram);
  }
  m_shaderProgram = 0;
  m_ok = false;
  m_lastProgram = 0;
}

bool CShaderProgram::CompileAndLink()
{
  GLint params[4]; 

  // free resources
  Free();

  // compiled vertex shader
  if (!m_VP.Compile())
  {
    CLog::Log(LOGERROR, "GL: Error compiling vertex shader");
    return false;
  }

  // compile pixel shader
  if (!m_FP.Compile())
  {
    m_VP.Free();
    CLog::Log(LOGERROR, "GL: Error compiling fragment shader");
    return false;
  }
  
  // create program object
  if (!(m_shaderProgram = glCreateProgram()))
    goto error;

  // attach the vertex shader
  glAttachShader(m_shaderProgram, m_VP.Handle());

  // if we have a pixel shader, attach it. If not, fixed pipeline
  // will be used.
  if (m_FP.Handle())
    glAttachShader(m_shaderProgram, m_FP.Handle());

  // link the program
  glLinkProgram(m_shaderProgram);
  glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, params);
  if (params[0]!=GL_TRUE) 
  {
    GLchar log[512];
    CLog::Log(LOGERROR, "GL: Error linking shader");
    glGetProgramInfoLog(m_shaderProgram, 512, NULL, log);
    CLog::Log(LOGERROR, (const char*)log);
    goto error;
  }

  // validate the program
  glValidateProgram(m_shaderProgram);
  glGetProgramiv(m_shaderProgram, GL_VALIDATE_STATUS, params);
  if (params[0]!=GL_TRUE) 
  {
    GLchar log[512];
    CLog::Log(LOGERROR, "GL: Error validating shader");
    glGetProgramInfoLog(m_shaderProgram, 512, NULL, log);
    CLog::Log(LOGERROR, (const char*)log);
    goto error;
  }
  
  m_ok = true;
  OnCompiledAndLinked();
  return true;

 error:
  m_ok = false;
  Free();
  return false;
}

bool CShaderProgram::Enable()
{
  if (OK())
  {
    glGetIntegerv(GL_CURRENT_PROGRAM, &m_lastProgram);
    glUseProgram((GLuint)m_shaderProgram);
    return true;
  }
  return false;
}

void CShaderProgram::Disable()
{
  if (OK())
  {
    glUseProgram((GLuint)m_lastProgram);
  }
}

#endif

/*!
\file GUITextureGL.h
\brief 
*/

#ifndef GUILIB_GUITEXTUREGL_H
#define GUILIB_GUITEXTUREGL_H

#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUITexture.h"

class CGUITextureGL : public CGUITextureBase
{
public:
  CGUITextureGL(float posX, float posY, float width, float height, const CTextureInfo& texture);
  static void DrawQuad(const XbmcCRect &coords, color_t color, CBaseTexture *texture = NULL, const XbmcCRect *texCoords = NULL);
protected:
  void Begin();
  void Draw(float *x, float *y, float *z, const XbmcCRect &texture, const XbmcCRect &diffuse, color_t color, int orientation);
  void End();
};

#endif

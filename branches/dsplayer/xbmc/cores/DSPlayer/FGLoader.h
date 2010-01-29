/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#pragma once
#include "streams.h"
#include "FgFilter.h"
#include "File.h"
#include "FileItem.h"
#include "tinyXML/tinyxml.h"
#include <list>
#include "SystemInfo.h" //g_sysinfo
#include "GUISettings.h"//g_guiSettings

enum DIRECTSHOW_RENDERER
{
    DIRECTSHOW_RENDERER_VMR9 = 1,
    DIRECTSHOW_RENDERER_EVR = 2,
};

class CFGLoader : public CCritSec
{
public:
  CFGLoader();
  virtual ~CFGLoader();


  HRESULT    LoadConfig(IFilterGraph2* fg,CStdString configFile);
  HRESULT    LoadFilterRules(const CFileItem& pFileItem);
  HRESULT    InsertSourceFilter(const CFileItem& pFileItem,TiXmlElement *pRule);
  HRESULT    InsertSplitter(TiXmlElement *pRule);
  HRESULT    InsertAudioDecoder(TiXmlElement *pRule);
  HRESULT    InsertVideoDecoder(TiXmlElement *pRule);
  HRESULT    InsertAudioRenderer();
  HRESULT    InsertVideoRenderer();
  HRESULT    InsertAutoLoad();
  
  IBaseFilter* GetSplitter() { return m_SplitterF; };

  CStdString GetVideoDecInfo(){return  m_pStrVideodec;};
  CStdString GetAudioDecInfo(){return  m_pStrAudiodec;};
  CStdString GetSourceFilterInfo(){return  m_pStrSource;};
  CStdString GetSplitterFilterInfo(){return  m_pStrSplitter;};
  CStdString GetAudioRenderer(){return  m_pStrAudioRenderer;};
protected:
  IFilterGraph2*            m_pGraphBuilder;
  CStdString                m_xbmcConfigFilePath;
  CStdString                m_pStrVideodec;
  CStdString                m_pStrAudiodec;
  CStdString                m_pStrAudioRenderer;
  CStdString                m_pStrSource;
  CStdString                m_pStrSplitter;
  std::list<CFGFilterFile*> m_configFilter;
  XFILE::CFile              m_File;
  IBaseFilter*              m_SourceF;
  IBaseFilter*              m_SplitterF;
  DIRECTSHOW_RENDERER       m_CurrentRenderer;
  CFGFilterVideoRenderer*   m_pFGF;

};


#include "../../../stdafx.h"
#include "pyutil.h"
#include <wchar.h>
#include "skininfo.h"

static int iPyGUILockRef = 0;
static TiXmlDocument pySkinReferences;
static int iPyLoadedSkinReferences = 0;

#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")

namespace PYXBMC
{
	int PyGetUnicodeString(string& buf, PyObject* pObject, int pos)
	{
    // TODO: UTF-8: Does python use UTF-16?
    //              Do we need to convert from the string charset to UTF-8
    //              for non-unicode data?
		if(PyUnicode_Check(pObject))
		{
      CStdString utf8String;
      g_charsetConverter.utf16toUTF8(PyUnicode_AsUnicode(pObject), utf8String);
      buf = utf8String;
			return 1;
		}
		if(PyString_Check(pObject))
		{
			buf = PyString_AsString(pObject);
			return 1;
		}
		// object is not an unicode ar normal string
		buf = "";
		if (pos != -1) PyErr_Format(PyExc_TypeError, "argument %.200i must be unicode or str", pos);
		return 0;
	}

	void PyGUILock()
	{
		if (iPyGUILockRef == 0) g_graphicsContext.Lock();
		iPyGUILockRef++;
	}

	void PyGUIUnlock()
	{
		if (iPyGUILockRef > 0)
		{
			iPyGUILockRef--;
			if (iPyGUILockRef == 0) g_graphicsContext.Unlock();	
		}
	}

	/*
	 * Looks in references.xml for image name
	 * If none exist return default image name
	 * iPyLoadedSkinReferences:
	 * - 0 xml not loaded
	 * - 1 tried to load xml, but it failed
	 * - 2 xml loaded
	 */
	const char* PyGetDefaultImage(char* cControlType, char* cTextureType, char* cDefault)
	{
		if (iPyLoadedSkinReferences == 0)
		{
			// xml not loaded. We only try to load references.xml once
			RESOLUTION res;
			string strPath = g_SkinInfo.GetSkinPath("references.xml", &res);
			if (pySkinReferences.LoadFile(strPath.c_str()))	iPyLoadedSkinReferences = true;
			else return cDefault; // return default value
				
		}
//		else if (iPyLoadedSkinReferences == 1) return cDefault;

		TiXmlElement *pControls = pySkinReferences.RootElement();

		TiXmlNode *pNode = NULL;
		TiXmlElement *pElement = NULL;

		//find control element
		while(pNode = pControls->IterateChildren("control", pNode))
		{
			pElement = pNode->FirstChildElement("type");
			if (pElement) if (!strcmp(pElement->FirstChild()->Value(), cControlType)) break;
		}
		if (!pElement) return cDefault;

		// get texture element
    CStdString element = cTextureType;
    // TODO: Once skin version goes to 2.0, we must check every reference
    // to this function and make them all lower case.
    if (g_SkinInfo.GetVersion() >= 1.85)
      element.ToLower();
		TiXmlElement *pTexture = pNode->FirstChildElement(element.c_str());
		if (pTexture)
		{
			// found our textureType
			pNode = pTexture->FirstChild();
			if (pNode && pNode->Value()[0] != '-') return pNode->Value();
			else
			{
				//set default
				return cDefault;
			}
		}
		return cDefault;
	}

	bool PyWindowIsNull(void* pWindow)
	{
		if (pWindow == NULL)
		{
			PyErr_SetString(PyExc_SystemError, "Error: Window is NULL, this is not possible :-)");
			return true;
		}
		return false;
	}
}

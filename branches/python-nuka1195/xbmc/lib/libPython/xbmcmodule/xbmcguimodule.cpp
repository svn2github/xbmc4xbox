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

#include "stdafx.h"
#include "lib/libPython/Python/Include/Python.h"
#include "lib/libPython/Python/Include/structmember.h"
#ifdef _LINUX
#include "../XBPythonDll.h"
#endif
#include "control.h"
#include "window.h"
#include "dialog.h"
#include "winxml.h"
#include "pyutil.h"
#include "action.h"
#include "GUIWindowManager.h"
#include "GUIListItem.h"

#ifndef __GNUC__
#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")
#endif

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  // lock() method
  PyDoc_STRVAR(lock__doc__,
    "lock() -- Lock the gui until xbmcgui.unlock() is called.\n"
    "\n"
    "*Note, This will improve performance when doing a lot of gui manipulation at once.\n"
    "       The main program (xbmc itself) will freeze until xbmcgui.unlock() is called.\n"
    "\n"
    "example:\n"
    "  - xbmcgui.lock()\n");

  PyObject* XBMCGUI_Lock(PyObject *self, PyObject *args)
  {
    PyGUILock();
    Py_INCREF(Py_None);
    return Py_None;
  }

  // unlock() method
  PyDoc_STRVAR(unlock__doc__,
    "unlock() -- Unlock the gui from a lock() call.\n"
    "\n"
    "example:\n"
    "  - xbmcgui.unlock()\n");

  PyObject* XBMCGUI_Unlock(PyObject *self, PyObject *args)
  {
    PyGUIUnlock();
    Py_INCREF(Py_None);
    return Py_None;
  }

  // getCurrentWindowId() method
  PyDoc_STRVAR(getCurrentWindowId__doc__,
    "getCurrentWindowId() -- Returns the id for the current 'active' window as an integer.\n"
    "\n"
    "example:\n"
    "  - wid = xbmcgui.getCurrentWindowId()\n");

  PyObject* XBMCGUI_GetCurrentWindowId(PyObject *self, PyObject *args)
  {
    PyGUILock();
    DWORD dwId = m_gWindowManager.GetActiveWindow();
    PyGUIUnlock();
    return Py_BuildValue("l", dwId);
  }

  // getCurrentWindowDialogId() method
  PyDoc_STRVAR(getCurrentWindowDialogId__doc__,
    "getCurrentWindowDialogId() -- Returns the id for the current 'active' dialog as an integer.\n"
    "\n"
    "example:\n"
    "  - wid = xbmcgui.getCurrentWindowDialogId()\n");

  PyObject* XBMCGUI_GetCurrentWindowDialogId(PyObject *self, PyObject *args)
  {
    PyGUILock();
    DWORD dwId = m_gWindowManager.GetTopMostModalDialogID();
    PyGUIUnlock();
    return Py_BuildValue("l", dwId);
  }

  // define c functions to be used in python here
  PyMethodDef xbmcGuiMethods[] = {
    {"lock", (PyCFunction)XBMCGUI_Lock, METH_VARARGS, lock__doc__},
    {"unlock", (PyCFunction)XBMCGUI_Unlock, METH_VARARGS, unlock__doc__},
    {"getCurrentWindowId", (PyCFunction)XBMCGUI_GetCurrentWindowId, METH_VARARGS, getCurrentWindowId__doc__},
    {"getCurrentWindowDialogId", (PyCFunction)XBMCGUI_GetCurrentWindowDialogId, METH_VARARGS, getCurrentWindowDialogId__doc__},
    {NULL, NULL, 0, NULL}
  };

  PyDoc_STRVAR(xbmcgui_module_documentation,
    //  "XBMC GUI Module"
    //  "\n"
      "");

  PyMODINIT_FUNC
  InitGUITypes(void)
  {
    initWindow_Type();
    initWindowDialog_Type();
    initWindowXML_Type();
    initWindowXMLDialog_Type();
    initListItem_Type();
    initControl_Type();
    initControlSpin_Type();
    initControlLabel_Type();
    initControlFadeLabel_Type();
    initControlTextBox_Type();
    initControlButton_Type();
    initControlCheckMark_Type();
    initControlProgress_Type();
    initControlList_Type();
    initControlImage_Type();
    initControlGroup_Type();
    initDialog_Type();
    initDialogProgress_Type();
    initAction_Type();
    initControlRadioButton_Type();

    if (PyType_Ready(&Window_Type) < 0 ||
        PyType_Ready(&WindowDialog_Type) < 0 ||
        PyType_Ready(&WindowXML_Type) < 0 ||
        PyType_Ready(&WindowXMLDialog_Type) < 0 ||
        PyType_Ready(&ListItem_Type) < 0 ||
        PyType_Ready(&Control_Type) < 0 ||
        PyType_Ready(&ControlSpin_Type) < 0 ||
        PyType_Ready(&ControlLabel_Type) < 0 ||
        PyType_Ready(&ControlFadeLabel_Type) < 0 ||
        PyType_Ready(&ControlTextBox_Type) < 0 ||
        PyType_Ready(&ControlButton_Type) < 0 ||
        PyType_Ready(&ControlCheckMark_Type) < 0 ||
        PyType_Ready(&ControlList_Type) < 0 ||
        PyType_Ready(&ControlImage_Type) < 0 ||
        PyType_Ready(&ControlProgress_Type) < 0 ||
        PyType_Ready(&ControlGroup_Type) < 0 ||
        PyType_Ready(&Dialog_Type) < 0 ||
        PyType_Ready(&DialogProgress_Type) < 0 ||
        PyType_Ready(&ControlRadioButton_Type) < 0 ||
        PyType_Ready(&Action_Type) < 0)
      return;

  }

  PyMODINIT_FUNC
  DeinitGUIModule(void)
  {
    Py_DECREF(&Window_Type);
    Py_DECREF(&WindowDialog_Type);
    Py_DECREF(&WindowXML_Type);
    Py_DECREF(&WindowXMLDialog_Type);
    Py_DECREF(&ListItem_Type);
    Py_DECREF(&Control_Type);
    Py_DECREF(&ControlSpin_Type);
    Py_DECREF(&ControlLabel_Type);
    Py_DECREF(&ControlFadeLabel_Type);
    Py_DECREF(&ControlTextBox_Type);
    Py_DECREF(&ControlButton_Type);
    Py_DECREF(&ControlCheckMark_Type);
    Py_DECREF(&ControlList_Type);
    Py_DECREF(&ControlImage_Type);
    Py_DECREF(&ControlProgress_Type);
    Py_DECREF(&ControlGroup_Type);
    Py_DECREF(&Dialog_Type);
    Py_DECREF(&DialogProgress_Type);
    Py_DECREF(&Action_Type);
  }

  PyMODINIT_FUNC
  InitGUIModule(void)
  {
    // init xbmc gui modules
    PyObject* pXbmcGuiModule;

    Py_INCREF(&Window_Type);
    Py_INCREF(&WindowDialog_Type);
    Py_INCREF(&WindowXML_Type);
    Py_INCREF(&WindowXMLDialog_Type);
    Py_INCREF(&ListItem_Type);
    Py_INCREF(&Control_Type);
    Py_INCREF(&ControlSpin_Type);
    Py_INCREF(&ControlLabel_Type);
    Py_INCREF(&ControlFadeLabel_Type);
    Py_INCREF(&ControlTextBox_Type);
    Py_INCREF(&ControlButton_Type);
    Py_INCREF(&ControlCheckMark_Type);
    Py_INCREF(&ControlList_Type);
    Py_INCREF(&ControlImage_Type);
    Py_INCREF(&ControlProgress_Type);
    Py_INCREF(&ControlGroup_Type);
    Py_INCREF(&Dialog_Type);
    Py_INCREF(&DialogProgress_Type);
    Py_INCREF(&Action_Type);
    Py_INCREF(&ControlRadioButton_Type);

    pXbmcGuiModule = Py_InitModule3("xbmcgui", xbmcGuiMethods, xbmcgui_module_documentation);

    if (pXbmcGuiModule == NULL) return;

    PyModule_AddObject(pXbmcGuiModule, "Window", (PyObject*)&Window_Type);
    PyModule_AddObject(pXbmcGuiModule, "WindowDialog", (PyObject*)&WindowDialog_Type);
    PyModule_AddObject(pXbmcGuiModule, "WindowXML", (PyObject*)&WindowXML_Type);
    PyModule_AddObject(pXbmcGuiModule, "WindowXMLDialog", (PyObject*)&WindowXMLDialog_Type);
    PyModule_AddObject(pXbmcGuiModule, "ListItem", (PyObject*)&ListItem_Type);
    //PyModule_AddObject(pXbmcGuiModule, "Control", (PyObject*)&Control_Type);
    //PyModule_AddObject(pXbmcGuiModule, "ControlSpin", (PyObject*)&ControlSpin_Type);
    PyModule_AddObject(pXbmcGuiModule, "ControlLabel", (PyObject*)&ControlLabel_Type);
    PyModule_AddObject(pXbmcGuiModule, "ControlFadeLabel", (PyObject*)&ControlFadeLabel_Type);
    PyModule_AddObject(pXbmcGuiModule, "ControlTextBox", (PyObject*)&ControlTextBox_Type);
    PyModule_AddObject(pXbmcGuiModule, "ControlButton", (PyObject*)&ControlButton_Type);
    PyModule_AddObject(pXbmcGuiModule, "ControlCheckMark", (PyObject*)&ControlCheckMark_Type);
    PyModule_AddObject(pXbmcGuiModule, "ControlList", (PyObject*)&ControlList_Type);
    PyModule_AddObject(pXbmcGuiModule, "ControlImage", (PyObject*)&  ControlImage_Type);
    PyModule_AddObject(pXbmcGuiModule, "ControlProgress", (PyObject*)& ControlProgress_Type);
    PyModule_AddObject(pXbmcGuiModule, "ControlGroup", (PyObject*)& ControlGroup_Type);
    PyModule_AddObject(pXbmcGuiModule, "Dialog", (PyObject *)&Dialog_Type);
    PyModule_AddObject(pXbmcGuiModule, "DialogProgress", (PyObject *)&DialogProgress_Type);
    PyModule_AddObject(pXbmcGuiModule, "Action", (PyObject *)&Action_Type);
    PyModule_AddObject(pXbmcGuiModule, "ControlRadioButton", (PyObject*)&ControlRadioButton_Type);

    PyModule_AddStringConstant(pXbmcGuiModule, "__author__",    PY_XBMC_AUTHOR);
    PyModule_AddStringConstant(pXbmcGuiModule, "__date__",      "14 July 2006");
    PyModule_AddStringConstant(pXbmcGuiModule, "__version__",    "1.2");
    PyModule_AddStringConstant(pXbmcGuiModule, "__credits__",    PY_XBMC_CREDITS);
    PyModule_AddStringConstant(pXbmcGuiModule, "__platform__",  PY_XBMC_PLATFORM);

    // icon overlay constants
    PyModule_AddIntConstant(pXbmcGuiModule, "ICON_OVERLAY_NONE", CGUIListItem::ICON_OVERLAY_NONE);
    PyModule_AddIntConstant(pXbmcGuiModule, "ICON_OVERLAY_RAR", CGUIListItem::ICON_OVERLAY_RAR);
    PyModule_AddIntConstant(pXbmcGuiModule, "ICON_OVERLAY_ZIP", CGUIListItem::ICON_OVERLAY_ZIP);
    PyModule_AddIntConstant(pXbmcGuiModule, "ICON_OVERLAY_LOCKED", CGUIListItem::ICON_OVERLAY_LOCKED);
    PyModule_AddIntConstant(pXbmcGuiModule, "ICON_OVERLAY_HAS_TRAINER", CGUIListItem::ICON_OVERLAY_HAS_TRAINER);
    PyModule_AddIntConstant(pXbmcGuiModule, "ICON_OVERLAY_TRAINED", CGUIListItem::ICON_OVERLAY_TRAINED);
    PyModule_AddIntConstant(pXbmcGuiModule, "ICON_OVERLAY_UNWATCHED", CGUIListItem::ICON_OVERLAY_UNWATCHED);
    PyModule_AddIntConstant(pXbmcGuiModule, "ICON_OVERLAY_WATCHED", CGUIListItem::ICON_OVERLAY_WATCHED);
    PyModule_AddIntConstant(pXbmcGuiModule, "ICON_OVERLAY_HD", CGUIListItem::ICON_OVERLAY_HD);
  }
}

#ifdef __cplusplus
}
#endif

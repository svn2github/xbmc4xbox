#include "stdafx.h"
#include "..\python.h"
#include "..\structmember.h"
#include "control.h"
#include "window.h"
#include "pyutil.h"

#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
	extern PyTypeObject DialogType;
	extern PyTypeObject DialogProgressType;

	PyObject* XBMCGUI_Lock(PyObject *self, PyObject *args)
	{
		PyGUILock();
		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* XBMCGUI_Unlock(PyObject *self, PyObject *args)
	{
		PyGUIUnlock();
		Py_INCREF(Py_None);
		return Py_None;
	}

	// define c functions to be used in python here
	PyMethodDef xbmcGuiMethods[] = {
		{"lock", (PyCFunction)XBMCGUI_Lock, METH_VARARGS, ""},
		{"unlock", (PyCFunction)XBMCGUI_Unlock, METH_VARARGS, ""},
		{NULL, NULL, 0, NULL}
	};

	PyMODINIT_FUNC
	initxbmcgui(void) 
	{
		// init xbmc gui modules
		PyObject* pXbmcGuiModule;

		DialogType.tp_new = PyType_GenericNew;
		DialogProgressType.tp_new = PyType_GenericNew;

		if (PyType_Ready(&Window_Type) < 0 ||
				PyType_Ready(&Control_Type) < 0 ||
				PyType_Ready(&ControlSpin_Type) < 0 ||
				PyType_Ready(&ControlLabel_Type) < 0 ||
				PyType_Ready(&ControlFadeLabel_Type) < 0 ||
				PyType_Ready(&ControlTextBox_Type) < 0 ||
				PyType_Ready(&ControlButton_Type) < 0 ||
				PyType_Ready(&ControlList_Type) < 0 ||
				PyType_Ready(&ControlImage_Type) < 0 ||
				PyType_Ready(&DialogType) < 0 ||
				PyType_Ready(&DialogProgressType) < 0)
			return;

		Py_INCREF(&Window_Type);
		Py_INCREF(&Control_Type);
		Py_INCREF(&ControlSpin_Type);
		Py_INCREF(&ControlLabel_Type);
		Py_INCREF(&ControlFadeLabel_Type);
		Py_INCREF(&ControlTextBox_Type);
		Py_INCREF(&ControlButton_Type);
		Py_INCREF(&ControlList_Type);
		Py_INCREF(&ControlImage_Type);
		Py_INCREF(&DialogType);
		Py_INCREF(&DialogProgressType);

		pXbmcGuiModule = Py_InitModule("xbmcgui", xbmcGuiMethods);
		if (pXbmcGuiModule == NULL) return;

    PyModule_AddObject(pXbmcGuiModule, "Window", (PyObject*)&Window_Type);
		//PyModule_AddObject(pXbmcGuiModule, "Control", (PyObject*)&Control_Type);
		//PyModule_AddObject(pXbmcGuiModule, "ControlSpin", (PyObject*)&ControlSpin_Type);
		PyModule_AddObject(pXbmcGuiModule, "ControlLabel", (PyObject*)&ControlLabel_Type);
		PyModule_AddObject(pXbmcGuiModule, "ControlFadeLabel", (PyObject*)&ControlFadeLabel_Type);
		PyModule_AddObject(pXbmcGuiModule, "ControlTextBox", (PyObject*)&ControlTextBox_Type);
		PyModule_AddObject(pXbmcGuiModule, "ControlButton", (PyObject*)&ControlButton_Type);
		PyModule_AddObject(pXbmcGuiModule, "ControlList", (PyObject*)&ControlList_Type);
		PyModule_AddObject(pXbmcGuiModule, "ControlImage", (PyObject*)&	ControlImage_Type);
		PyModule_AddObject(pXbmcGuiModule, "Dialog", (PyObject*)&DialogType);
		PyModule_AddObject(pXbmcGuiModule, "DialogProgress", (PyObject *)&DialogProgressType);
	}
}

#ifdef __cplusplus
}
#endif

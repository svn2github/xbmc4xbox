
#include "stdafx.h"
#include "XBPythonDll.h"
#include "../../cores/DllLoader/DllLoader.h"

// macro to make life a bit simpler

#define DLL_FUNCTION(function) #function, (void**)&p_##function

// use these if the dll exports data structs
#define DLL_OBJECT_DATA(data) #data, (void**)&pointer_##data

#define FUNCTION(function) void* (__cdecl* p_##function)();

extern "C"
{
  /*****************************************
   * python24.dll
   */

  FUNCTION(PyEval_ReleaseLock)
  FUNCTION(PyEval_AcquireLock)
  FUNCTION(PyThreadState_Get)
  FUNCTION(PyRun_SimpleString)
  FUNCTION(PyEval_InitThreads)
  FUNCTION(Py_Initialize)
  FUNCTION(Py_IsInitialized)
  FUNCTION(Py_Finalize)
  FUNCTION(Py_NewInterpreter)
  FUNCTION(Py_EndInterpreter)
  FUNCTION(PyThreadState_Swap)
  FUNCTION(PyErr_SetString)
  FUNCTION(PyThreadState_New)
  FUNCTION(PyErr_Print)
  FUNCTION(PyErr_Occurred)
  FUNCTION(PyRun_SimpleFile)
  FUNCTION(PySys_SetPath)
#ifdef _LINUX
  FUNCTION(Py_GetPath)
#endif
  FUNCTION(PyThreadState_Delete)
  FUNCTION(PyThreadState_Clear)
  FUNCTION(Py_BuildValue)
  FUNCTION(PyArg_Parse)
  FUNCTION(PyArg_ParseTuple)
  FUNCTION(PyType_IsSubtype)
  FUNCTION(PyArg_ParseTupleAndKeywords)
  FUNCTION(PyString_AsString)
  FUNCTION(Py_AddPendingCall)
  FUNCTION(PyList_GetItem)
  FUNCTION(PyList_Size)
  FUNCTION(_PyObject_New)
  FUNCTION(PyLong_AsLong)
  FUNCTION(PyLong_AsLongLong)
  FUNCTION(PyErr_Format)
#ifndef _LINUX
  FUNCTION(PyUnicodeUCS2_AsUnicode)
#else
  FUNCTION(PyUnicodeUCS4_AsUnicode)
#endif
  FUNCTION(Py_MakePendingCalls)
  FUNCTION(PyEval_SaveThread)
  FUNCTION(PyEval_RestoreThread)
  FUNCTION(PyLong_FromLong)
  FUNCTION(PyModule_AddStringConstant)
  FUNCTION(PyModule_AddObject)
  FUNCTION(Py_InitModule4)
  FUNCTION(PyInt_AsLong)
  FUNCTION(PyFloat_AsDouble)
  FUNCTION(PyString_FromString)
  FUNCTION(PyModule_AddIntConstant)
  FUNCTION(PyObject_CallFunction) // va arg
  FUNCTION(PyObject_CallMethod)
  FUNCTION(PyDict_SetItemString)
  FUNCTION(PyDict_New)
  FUNCTION(PyModule_GetDict)
  FUNCTION(PyImport_Import)
  FUNCTION(PyInt_FromLong)
  FUNCTION(PyDict_GetItemString)
  FUNCTION(PyDict_Next)
  FUNCTION(PyDict_Size)
  FUNCTION(PyType_Ready)
  FUNCTION(PyType_GenericNew)
  FUNCTION(PySys_SetArgv)
  FUNCTION(PyObject_RichCompare)
  FUNCTION(PyFloat_FromDouble)

  DATA_OBJECT(PyExc_SystemError)
  DATA_OBJECT(PyExc_ValueError)
  DATA_OBJECT(PyExc_Exception)
  DATA_OBJECT(PyExc_TypeError)
  DATA_OBJECT(PyExc_KeyboardInterrupt)
  DATA_OBJECT(PyExc_RuntimeError)
  DATA_OBJECT(PyExc_ReferenceError)

  DATA_OBJECT(_Py_NoneStruct)
  DATA_OBJECT(_Py_NotImplementedStruct)
  DATA_OBJECT(_Py_TrueStruct)
  DATA_OBJECT(_Py_ZeroStruct)
  DATA_OBJECT(PyString_Type)
  DATA_OBJECT(PyList_Type)
  DATA_OBJECT(PyLong_Type)
  DATA_OBJECT(PyInt_Type)
  DATA_OBJECT(PyUnicode_Type)
  DATA_OBJECT(PyTuple_Type)
  DATA_OBJECT(PyDict_Type)

  bool python_load_dll(LibraryLoader& dll)
  {
    bool bResult;

    bResult = (
      dll.ResolveExport(DLL_FUNCTION(PyEval_ReleaseLock)) &&
      dll.ResolveExport(DLL_FUNCTION(PyEval_AcquireLock)) &&
      dll.ResolveExport(DLL_FUNCTION(PyThreadState_Get)) &&
      dll.ResolveExport(DLL_FUNCTION(PyRun_SimpleString)) &&
      dll.ResolveExport(DLL_FUNCTION(PyEval_InitThreads)) &&
      dll.ResolveExport(DLL_FUNCTION(Py_Initialize)) &&
      dll.ResolveExport(DLL_FUNCTION(Py_IsInitialized)) &&
      dll.ResolveExport(DLL_FUNCTION(Py_Finalize)) &&
      dll.ResolveExport(DLL_FUNCTION(Py_NewInterpreter)) &&
      dll.ResolveExport(DLL_FUNCTION(Py_EndInterpreter)) &&
      dll.ResolveExport(DLL_FUNCTION(PyThreadState_Swap)) &&
      dll.ResolveExport(DLL_FUNCTION(PyErr_SetString)) &&
      dll.ResolveExport(DLL_FUNCTION(PyThreadState_New)) &&
      dll.ResolveExport(DLL_FUNCTION(PyErr_Print)) &&
      dll.ResolveExport(DLL_FUNCTION(PyErr_Occurred)) &&
      dll.ResolveExport(DLL_FUNCTION(PyRun_SimpleFile)) &&
      dll.ResolveExport(DLL_FUNCTION(PySys_SetPath)) &&
#ifdef _LINUX
      dll.ResolveExport(DLL_FUNCTION(Py_GetPath)) &&
#endif
      dll.ResolveExport(DLL_FUNCTION(PyThreadState_Delete)) &&
      dll.ResolveExport(DLL_FUNCTION(PyThreadState_Clear)) &&
      dll.ResolveExport(DLL_FUNCTION(Py_BuildValue)) &&
      dll.ResolveExport(DLL_FUNCTION(PyType_IsSubtype)) &&
      dll.ResolveExport(DLL_FUNCTION(PyArg_ParseTupleAndKeywords)) &&
      dll.ResolveExport(DLL_FUNCTION(PyString_AsString)) &&
      dll.ResolveExport(DLL_FUNCTION(Py_AddPendingCall)) &&
      dll.ResolveExport(DLL_FUNCTION(PyObject_CallMethod)) &&
      dll.ResolveExport(DLL_FUNCTION(PyList_GetItem)) &&
      dll.ResolveExport(DLL_FUNCTION(PyList_Size)) &&
      dll.ResolveExport(DLL_FUNCTION(_PyObject_New)) &&
      dll.ResolveExport(DLL_FUNCTION(PyLong_AsLong)) &&
      dll.ResolveExport(DLL_FUNCTION(PyLong_AsLongLong)) &&
      dll.ResolveExport(DLL_FUNCTION(PyErr_Format)) &&
#ifndef _LINUX
      dll.ResolveExport(DLL_FUNCTION(PyUnicodeUCS2_AsUnicode)) &&
#else
      dll.ResolveExport(DLL_FUNCTION(PyUnicodeUCS4_AsUnicode)) &&
#endif
      dll.ResolveExport(DLL_FUNCTION(Py_MakePendingCalls)) &&
      dll.ResolveExport(DLL_FUNCTION(PyEval_SaveThread)) &&
      dll.ResolveExport(DLL_FUNCTION(PyEval_RestoreThread)) &&
      dll.ResolveExport(DLL_FUNCTION(PyLong_FromLong)) &&
      dll.ResolveExport(DLL_FUNCTION(PyModule_AddStringConstant)) &&
      dll.ResolveExport(DLL_FUNCTION(PyModule_AddObject)) &&
      dll.ResolveExport(DLL_FUNCTION(Py_InitModule4)) &&
      dll.ResolveExport(DLL_FUNCTION(PyInt_AsLong)) &&
      dll.ResolveExport(DLL_FUNCTION(PyFloat_AsDouble)) &&
      dll.ResolveExport(DLL_FUNCTION(PyString_FromString)) &&
      dll.ResolveExport(DLL_FUNCTION(PyModule_AddIntConstant)) &&
      dll.ResolveExport(DLL_FUNCTION(PyObject_CallFunction)) &&
      dll.ResolveExport(DLL_FUNCTION(PyDict_SetItemString)) &&
      dll.ResolveExport(DLL_FUNCTION(PyDict_New)) &&
      dll.ResolveExport(DLL_FUNCTION(PyModule_GetDict)) &&
      dll.ResolveExport(DLL_FUNCTION(PyImport_Import)) &&
      dll.ResolveExport(DLL_FUNCTION(PyFloat_FromDouble)) &&
      dll.ResolveExport(DLL_FUNCTION(PyInt_FromLong)) &&
      dll.ResolveExport(DLL_FUNCTION(PyDict_GetItemString)) &&
      //dll.ResolveExport(DLL_FUNCTION(PyDict_GetItem)) &&
      //dll.ResolveExport(DLL_FUNCTION(PyDict_Keys)) &&
      dll.ResolveExport(DLL_FUNCTION(PyDict_Next)) &&
      dll.ResolveExport(DLL_FUNCTION(PyDict_Size)) &&
      dll.ResolveExport(DLL_FUNCTION(PyType_Ready)) &&
      dll.ResolveExport(DLL_FUNCTION(PyType_GenericNew)) &&
      dll.ResolveExport(DLL_FUNCTION(PyArg_Parse)) &&
      dll.ResolveExport(DLL_FUNCTION(PyArg_ParseTuple)) &&
      dll.ResolveExport(DLL_FUNCTION(PySys_SetArgv)) &&
      dll.ResolveExport(DLL_FUNCTION(PyObject_RichCompare)) &&

      dll.ResolveExport(DLL_OBJECT_DATA(PyExc_SystemError)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyExc_ValueError)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyExc_Exception)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyExc_TypeError)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyExc_KeyboardInterrupt)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyExc_RuntimeError)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyExc_ReferenceError)) &&

      dll.ResolveExport(DLL_OBJECT_DATA(_Py_NoneStruct)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(_Py_NotImplementedStruct)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(_Py_TrueStruct)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(_Py_ZeroStruct)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyString_Type)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyList_Type)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyLong_Type)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyInt_Type)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyUnicode_Type)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyTuple_Type)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyDict_Type)));

    return bResult;
  }
}

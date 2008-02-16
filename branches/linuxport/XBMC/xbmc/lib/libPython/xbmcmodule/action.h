#ifndef _LINUX
#include "../python/Python.h"
#else
#include <python2.4/Python.h>
#include "../XBPythonDll.h"
#endif
#include "GUIControl.h"
#include "listitem.h"

#pragma once

// python type checking
#define Action_Check(op) PyObject_TypeCheck(op, &Action_Type)
#define Action_CheckExact(op) ((op)->ob_type == &Action_Type)

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  typedef struct {
    PyObject_HEAD
    long id;
    float fAmount1;
    float fAmount2;
    float fRepeat;
    unsigned long buttonCode;
    std::string strAction;
  } Action;

  extern PyTypeObject Action_Type;

  PyObject* Action_FromAction(const CAction& action);

  void initAction_Type();

}

#ifdef __cplusplus
}
#endif

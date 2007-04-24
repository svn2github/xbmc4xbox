#include "../../../stdafx.h"
#include "action.h"
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
  PyObject* Action_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    Action *self;

    self = (Action*)type->tp_alloc(type, 0);
    if (!self) return NULL;

    //if (!PyArg_ParseTuple(args, "l", &self->action)) return NULL;
    //self->action = -1;

    self->id = -1;
    self->fAmount1 = 0.0f;
    self->fAmount2 = 0.0f;
    self->fRepeat = 0.0f;
    self->buttonCode = 0;
    self->strAction = "";

    return (PyObject*)self;
  }

  PyObject* Action_FromAction(const CAction& action)
  {
    Action* pyAction = (Action*)Action_Type.tp_alloc(&Action_Type, 0);

    if (pyAction)
    {
      pyAction->id = action.wID;
      pyAction->buttonCode = action.m_dwButtonCode;
      pyAction->fAmount1 = action.fAmount1;
      pyAction->fAmount2 = action.fAmount2;
      pyAction->fRepeat = action.fRepeat;
      pyAction->strAction = action.strAction.c_str();
    }

    return (PyObject*)pyAction;
  }

  void Action_Dealloc(Action* self)
  {
    self->ob_type->tp_free((PyObject*)self);
  }

  /* For backwards compatability whe have to check the action code
   * against an integer
   * The first argument is always an Action object
   */
  PyObject* Action_RichCompare(Action* obj1, PyObject* obj2, int method)
  {
    if (method == Py_EQ)
    {
      if (Action_Check(obj2))
      {
        // both are Action objects
        Action* a2 = (Action*)obj2;

        if (obj1->id == a2->id &&
            obj1->buttonCode == a2->buttonCode &&
            obj1->fAmount1 == a2->fAmount1 &&
            obj1->fAmount2 == a2->fAmount2 &&
            obj1->fRepeat == a2->fRepeat &&
            obj1->strAction == a2->strAction)
        {
          Py_RETURN_TRUE;
        }
        else
        {
          Py_RETURN_FALSE;
        }
      }
      else
      {
        // for backwards compatability in python scripts
        PyObject* o1 = PyLong_FromLong(obj1->id);
        return PyObject_RichCompare(o1, obj2, method);
      }
    }
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }

  // getId() Method
  PyDoc_STRVAR(getId__doc__,
    "getId() -- Returns the action's current id as a long or 0 if no action is mapped in the xml's.\n"
    "\n");

  PyObject* Action_GetId(Action* self, PyObject* args)
  {
    return Py_BuildValue("l", self->id);
  }

  // getButtonCode() Method
  PyDoc_STRVAR(getButtonCode__doc__,
    "getButtonCode() -- Returns the button code for this action.\n"
    "\n");

  PyObject* Action_GetButtonCode(Action* self, PyObject* args)
  {
    return Py_BuildValue("l", self->buttonCode);
  }

  PyDoc_STRVAR(getAmount1__doc__,
    "getAmount1() -- Returns the first amount of force applied to the thumbstick n.\n"
    "\n");

  PyDoc_STRVAR(getAmount2__doc__,
    "getAmount2() -- Returns the second amount of force applied to the thumbstick n.\n"
    "\n");

  PyObject* Action_GetAmount1(Action* self, PyObject* args)
  {
    return Py_BuildValue("f", self->fAmount1);
  }

  PyObject* Action_GetAmount2(Action* self, PyObject* args)
  {
    return Py_BuildValue("f", self->fAmount2);
  }

  PyMethodDef Action_methods[] = {
    {"getId", (PyCFunction)Action_GetId, METH_VARARGS, getId__doc__},
    {"getButtonCode", (PyCFunction)Action_GetButtonCode, METH_VARARGS, getButtonCode__doc__},
    {"getAmount1", (PyCFunction)Action_GetAmount1, METH_VARARGS, getAmount1__doc__},
    {"getAmount2", (PyCFunction)Action_GetAmount2, METH_VARARGS, getAmount2__doc__},
    {NULL, NULL, 0, NULL}
  };

  PyDoc_STRVAR(action__doc__,
    "Action class.\n"
    "\n"
    "For backwards compatibility reasons the == operator is extended so that it"
    "can compare an action with other actions and action.id with numbers"
    "  example: (action == ACTION_MOVE_LEFT)"
    "");

// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

  PyTypeObject Action_Type;

  void initAction_Type()
  {
    PyInitializeTypeObject(&Action_Type);

    Action_Type.tp_name = "xbmcgui.Action";
    Action_Type.tp_basicsize = sizeof(Action);
    Action_Type.tp_dealloc = (destructor)Action_Dealloc;
    //Action_Type.tp_compare = Action_Compare;
    Action_Type.tp_richcompare = (richcmpfunc)Action_RichCompare;
    Action_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    Action_Type.tp_doc = action__doc__;
    Action_Type.tp_methods = Action_methods;
    Action_Type.tp_base = 0;
    Action_Type.tp_new = Action_New;
  }
}

#ifdef __cplusplus
}
#endif

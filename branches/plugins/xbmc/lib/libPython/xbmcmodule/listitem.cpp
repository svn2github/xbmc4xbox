#include "stdafx.h"
#include "../python/python.h"
#include "listitem.h"
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
  PyObject* ListItem_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    ListItem *self;
    static char *keywords[] = { "label", "label2",
      "iconImage", "thumbnailImage", NULL };

    PyObject* label = NULL;
    PyObject* label2 = NULL;
    char* cIconImage = NULL;
    char* cThumbnailImage = NULL;

    // allocate new object
    self = (ListItem*)type->tp_alloc(type, 0);
    if (!self) return NULL;
      self->item = NULL;

    // parse user input
    if (!PyArg_ParseTupleAndKeywords(
      args, kwds,
      "|OOss", keywords,
      &label, &label2,
      &cIconImage, &cThumbnailImage))
    {
      Py_DECREF( self );
      return NULL;
    }

    // create CFileItem
    self->item = new CFileItem();
    if (!self->item)
    {
      Py_DECREF( self );
      return NULL;
    }
    CStdString utf8String;
    if (label && PyGetUnicodeString(utf8String, label, 1))
    {
      self->item->SetLabel( utf8String );
    }
    if (label2 && PyGetUnicodeString(utf8String, label2, 1))
    {
      self->item->SetLabel2( utf8String );
    }
    if (cIconImage)
    {
      self->item->SetIconImage( cIconImage );
    }
    if (cThumbnailImage)
    {
      self->item->SetThumbnailImage( cThumbnailImage );
    }
    return (PyObject*)self;
  }

  /*
   * allocate a new listitem. Used for c++ and not the python user
   * returns a new reference
   */
  ListItem* ListItem_FromString(string strLabel)
  {
    ListItem* self = (ListItem*)ListItem_Type.tp_alloc(&ListItem_Type, 0);
    if (!self) return NULL;

    self->item = new CFileItem(strLabel);
    if (!self->item)
    {
      Py_DECREF( self );
      return NULL;
    }

    return self;
  }

  void ListItem_Dealloc(ListItem* self)
  {
    if (self->item) delete self->item;
    self->ob_type->tp_free((PyObject*)self);
  }

  PyDoc_STRVAR(getLabel__doc__,
    "getLabel() -- Returns the listitem label.\n"
    "\n"
    "example:\n"
    "  - label = self.list.getSelectedItem().getLabel()\n");

  PyObject* ListItem_GetLabel(ListItem *self, PyObject *args)
  {
    if (!self->item) return NULL;

    PyGUILock();
    const char *cLabel =  self->item->GetLabel().c_str();
    PyGUIUnlock();

    return Py_BuildValue("s", cLabel);
  }

  PyDoc_STRVAR(getLabel2__doc__,
    "getLabel2() -- Returns the listitem's second label.\n"
    "\n"
    "example:\n"
    "  - label2 = self.list.getSelectedItem().getLabel2()\n");

  PyObject* ListItem_GetLabel2(ListItem *self, PyObject *args)
  {
    if (!self->item) return NULL;

    PyGUILock();
    const char *cLabel = self->item->GetLabel2().c_str();
    PyGUIUnlock();

    return Py_BuildValue("s", cLabel);
  }

  PyDoc_STRVAR(setLabel__doc__,
    "setLabel(label) -- Sets the listitem's label.\n"
    "\n"
    "label          : string or unicode - text string.\n"
    "\n"
    "example:\n"
    "  - self.list.getSelectedItem().setLabel('Casino Royale')\n");

  PyObject* ListItem_SetLabel(ListItem *self, PyObject *args)
  {
    PyObject* unicodeLine = NULL;
    if (!self->item) return NULL;

    if (!PyArg_ParseTuple(args, "O", &unicodeLine)) return NULL;

    string utf8Line;
    if (unicodeLine && !PyGetUnicodeString(utf8Line, unicodeLine, 1))
      return NULL;
    // set label
    PyGUILock();
    self->item->SetLabel(utf8Line);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(setLabel2__doc__,
    "setLabel2(label2) -- Sets the listitem's second label.\n"
    "\n"
    "label2         : string or unicode - text string.\n"
    "\n"
    "example:\n"
    "  - self.list.getSelectedItem().setLabel2('[pg-13]')\n");
  
  PyObject* ListItem_SetLabel2(ListItem *self, PyObject *args)
  {
    PyObject* unicodeLine = NULL;
    if (!self->item) return NULL;

    if (!PyArg_ParseTuple(args, "O", &unicodeLine)) return NULL;

    string utf8Line;
    if (unicodeLine && !PyGetUnicodeString(utf8Line, unicodeLine, 1))
      return NULL;
    // set label
    PyGUILock();
    self->item->SetLabel2(utf8Line);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(setIconImage__doc__,
    "setIconImage(icon) -- Sets the listitem's icon image.\n"
    "\n"
    "icon            : string - image filename.\n"
    "\n"
    "example:\n"
    "  - self.list.getSelectedItem().setIconImage('emailread.png')\n");

  PyObject* ListItem_SetIconImage(ListItem *self, PyObject *args)
  {
    char *cLine = NULL;
    if (!self->item) return NULL;

    if (!PyArg_ParseTuple(args, "s", &cLine)) return NULL;

    // set label
    PyGUILock();
    self->item->SetIconImage(cLine ? cLine : "");
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(setThumbnailImage__doc__,
    "setThumbnailImage(thumb) -- Sets the listitem's thumbnail image.\n"
    "\n"
    "thumb           : string - image filename.\n"
    "\n"
    "example:\n"
    "  - self.list.getSelectedItem().setThumbnailImage('emailread.png')\n");

  PyObject* ListItem_SetThumbnailImage(ListItem *self, PyObject *args)
  {
    char *cLine = NULL;
    if (!self->item) return NULL;

    if (!PyArg_ParseTuple(args, "s", &cLine)) return NULL;

    // set label
    PyGUILock();
    self->item->SetThumbnailImage(cLine ? cLine : "");
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(select__doc__,
    "select(selected) -- Sets the listitem's selected status.\n"
    "\n"
    "selected        : bool - True=selected/False=not selected\n"
    "\n"
    "example:\n"
    "  - self.list.getSelectedItem().select(True)\n");

  PyObject* ListItem_Select(ListItem *self, PyObject *args)
  {
    if (!self->item) return NULL;

    bool bOnOff = false;
    if (!PyArg_ParseTuple(args, "b", &bOnOff)) return NULL;

    PyGUILock();
    self->item->Select(bOnOff);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }
  
  PyDoc_STRVAR(isSelected__doc__,
    "isSelected() -- Returns the listitem's selected status.\n"
    "\n"
    "example:\n"
    "  - is = self.list.getSelectedItem().isSelected()\n");

  PyObject* ListItem_IsSelected(ListItem *self, PyObject *args)
  {
    if (!self->item) return NULL;

    PyGUILock();
    bool bOnOff = self->item->IsSelected();
    PyGUIUnlock();

    return Py_BuildValue("b", bOnOff);
  }
  
  PyDoc_STRVAR(setInfo__doc__,
    "setInfo(type, tags) -- Sets the listitem's media tags.\n"
    "\n"
    "type           : string - type of media(video/music/pictures).\n"
    "tags           : list - tuple pairs of tag,value.\n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "example:\n"
    "  - self.list.getSelectedItem().setInfo('video',[('Genre', 'Comedy',)])\n");

  PyObject* ListItem_SetInfo(ListItem *self, PyObject *args, PyObject *kwds)
  {
    static char *keywords[] = { "type", "tags", NULL };
    char *cType = NULL;
    PyObject *pTagList = NULL;
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      "sO",
      keywords,
      &cType,
      &pTagList))
    {
      return NULL;
    }

    if (pTagList == NULL || !PyObject_TypeCheck(pTagList, &PyList_Type))
    {
      PyErr_SetString(PyExc_TypeError, "tags object should be of type List");
      return NULL;
    }

    PyGUILock();
    for (int i = 0; i < PyList_Size(pTagList); i++)
    {
      PyObject *pTuple = NULL;
      char *cTag = NULL;
      char *cValue = NULL;
      pTuple = PyList_GetItem(pTagList, i);
      if (pTuple == NULL || !PyObject_TypeCheck(pTuple, &PyTuple_Type))
      {
        //PyErr_SetString(PyExc_TypeError, "List must only contain tuples");
        continue;//return NULL;
      }
      if (!PyArg_ParseTuple(pTuple, "ss", &cTag, &cValue))
      {
        //PyErr_SetString(PyExc_TypeError, "Error unpacking tuple found in list");
        continue;//return NULL;
      }

      if (strcmpi(cType, "video") == 0)
      {
        if (strcmpi(cTag, "genre") == 0)
          self->item->GetVideoInfoTag()->m_strGenre = cValue;
        else if (strcmpi(cTag, "year") == 0)
          self->item->GetVideoInfoTag()->m_iYear = atoi(cValue);
        else if (strcmpi(cTag, "director") == 0)
          self->item->GetVideoInfoTag()->m_strDirector = cValue;
        else if (strcmpi(cTag, "rating") == 0)
          self->item->GetVideoInfoTag()->m_strMPAARating = cValue;
        else if (strcmpi(cTag, "plot") == 0)
          self->item->GetVideoInfoTag()->m_strPlot = cValue;
        else if (strcmpi(cTag, "runtime") == 0)
          self->item->GetVideoInfoTag()->m_strRuntime = cValue;
        else if (strcmpi(cTag, "title") == 0)
          self->item->GetVideoInfoTag()->m_strTitle = cValue;
        CLog::Log(LOGDEBUG, __FUNCTION__" - Type: %s - Tag: %s - Value: %s ", cType, cTag, cValue);
      }
      else if (strcmpi(cType, "music") == 0)
      {
        CLog::Log(LOGDEBUG, __FUNCTION__" - Type: %s - Tag: %s - Value: %s ", cType, cTag, cValue);
      }
      else if (strcmpi(cType, "pictures") == 0)
      {
        CLog::Log(LOGDEBUG, __FUNCTION__" - Type: %s - Tag: %s - Value: %s ", cType, cTag, cValue);
      }
    }
    PyGUIUnlock();
  
    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(setInfoDict__doc__,
    "setInfo(type, tags) -- Sets the listitem's media tags.\n"
    "\n"
    "type           : string - type of media(video/music/pictures).\n"
    "tags           : list - tuple pairs of tag,value.\n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "example:\n"
    "  - self.list.getSelectedItem().setInfo('video',[('Genre', 'Comedy',)])\n");

  PyObject* ListItem_SetInfoDict(ListItem *self, PyObject *args, PyObject *kwds)
  {
    static char *keywords[] = { "type", "tags", NULL };
    char *cType = NULL;
    PyObject *pTagDict = NULL;
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      "sO",
      keywords,
      &cType,
      &pTagDict))
    {
      return NULL;
    }
    if (pTagDict == NULL || !PyObject_TypeCheck(pTagDict, &PyDict_Type))
    {
      PyErr_SetString(PyExc_TypeError, "tags object should be of type Dict");
      return NULL;
    }

    //////////////////////////////
    // enumerate all named values
    PyObject *pTagKeys = PyDict_Keys(pTagDict); // new
    for(int i = 0; i < PyList_Size(pTagKeys); ++i) {
        PyObject *pKey =
                PyList_GetItem(pTagKeys, i); // borrowed
        PyObject *pValue =
                PyDict_GetItem(pTagDict, pKey); // borrowed
        CLog::Log(LOGDEBUG, __FUNCTION__" - Type: %s - Tag: %s - Value: %s ", cType, pKey, pValue);
    }
    Py_DECREF(pTagKeys);
    /////////////////////////////
  
    Py_INCREF(Py_None);
    return Py_None;
  }
  PyMethodDef ListItem_methods[] = {
    {"getLabel" , (PyCFunction)ListItem_GetLabel, METH_VARARGS, getLabel__doc__},
    {"setLabel" , (PyCFunction)ListItem_SetLabel, METH_VARARGS, setLabel__doc__},
    {"getLabel2", (PyCFunction)ListItem_GetLabel2, METH_VARARGS, getLabel2__doc__},
    {"setLabel2", (PyCFunction)ListItem_SetLabel2, METH_VARARGS, setLabel2__doc__},
    {"setIconImage", (PyCFunction)ListItem_SetIconImage, METH_VARARGS, setIconImage__doc__},
    {"setThumbnailImage", (PyCFunction)ListItem_SetThumbnailImage, METH_VARARGS, setThumbnailImage__doc__},
    {"select", (PyCFunction)ListItem_Select, METH_VARARGS, select__doc__},
    {"isSelected", (PyCFunction)ListItem_IsSelected, METH_VARARGS, isSelected__doc__},
    {"setInfo", (PyCFunction)ListItem_SetInfo, METH_KEYWORDS, setInfo__doc__},
    {"setInfoDict", (PyCFunction)ListItem_SetInfoDict, METH_KEYWORDS, setInfoDict__doc__},
    {NULL, NULL, 0, NULL}
  };

  PyDoc_STRVAR(listItem__doc__,
    "ListItem class.\n"
    "\n"
    "ListItem([label, label2, iconImage, thumbnailImage]) -- Creates a new ListItem.\n"
    "\n"
    "label          : [opt] string or unicode - label1 text.\n"
    "label2         : [opt] string or unicode - label2 text.\n"
    "iconImage      : [opt] string - icon filename.\n"
    "thumbnailImage : [opt] string - thumbnail filename.\n"
    "\n"
    "example:\n"
    "  - listitem = xbmcgui.ListItem('Casino Royale', '[PG-13]', 'blank-poster.tbn', 'poster.tbn')\n");

// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

  PyTypeObject ListItem_Type;

  void initListItem_Type()
  {
    PyInitializeTypeObject(&ListItem_Type);

    ListItem_Type.tp_name = "xbmcgui.ListItem";
    ListItem_Type.tp_basicsize = sizeof(ListItem);
    ListItem_Type.tp_dealloc = (destructor)ListItem_Dealloc;
    ListItem_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    ListItem_Type.tp_doc = listItem__doc__;
    ListItem_Type.tp_methods = ListItem_methods;
    ListItem_Type.tp_base = 0;
    ListItem_Type.tp_new = ListItem_New;
  }
}

#ifdef __cplusplus
}
#endif

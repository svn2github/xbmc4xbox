#include "../../../stdafx.h"
#include "..\..\..\playlistplayer.h"
#include "..\..\..\util.h"
#include "pyplaylist.h"
#include "..\structmember.h"
#include "..\..\..\playlistfactory.h"

#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{

/* PlayListItem Functions */

	PyObject* PlayListItem_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
	{
		PlayListItem *self;

		self = (PlayListItem*)type->tp_alloc(type, 0);
		if (!self) return NULL;

		self->item = new PLAYLIST::CPlayList::CPlayListItem();

		return (PyObject*)self;
	}

	void PlayListItem_Dealloc(PlayList* self)
	{
		if (self->pPlayList) delete self->pPlayList;
		self->ob_type->tp_free((PyObject*)self);
	}

	PyDoc_STRVAR(getDescription__doc__,
		"getdescription() -- Returns the description of this PlayListItem.\n");

	PyObject* PlayListItem_GetDescription(PlayListItem *self, PyObject *key)
	{
		return Py_BuildValue("s", self->item->GetDescription().c_str());
	}

	PyDoc_STRVAR(getDuration__doc__,
		"getduration() -- Returns the duration of this PlayListItem.\n");

	PyObject* PlayListItem_GetDuration(PlayListItem *self, PyObject *key)
	{
		return Py_BuildValue("l", self->item->GetDuration());
	}

	PyDoc_STRVAR(getFilename__doc__,
		"getfilename() -- Returns the filename of this PlayListItem.\n");

	PyObject* PlayListItem_GetFileName(PlayListItem *self, PyObject *key)
	{
		return Py_BuildValue("s", self->item->GetFileName().c_str());
	}
	
/* PlayList Fucntions */

	PyObject* PlayList_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
	{
		int iNr;
		PlayList *self;
		if (!PyArg_ParseTuple(args, "i", &iNr))	return NULL;

		self = (PlayList*)type->tp_alloc(type, 0);
		if (!self) return NULL;

		// we do not create our own playlist, just using the ones from playlistplayer
		if (iNr != PLAYLIST_MUSIC &&
				iNr != PLAYLIST_MUSIC_TEMP &&
				iNr != PLAYLIST_VIDEO &&
				iNr != PLAYLIST_VIDEO_TEMP)
		{
			PyErr_SetString((PyObject*)self, "PlayList does not exist");
			return NULL;
		}

		self->pPlayList = &g_playlistPlayer.GetPlaylist(iNr);
		self->iPlayList = iNr;

		return (PyObject*)self;
	}

	void PlayList_Dealloc(PlayList* self)
	{
		self->ob_type->tp_free((PyObject*)self);
	}

	PyDoc_STRVAR(add__doc__,
		"add(filename[, description, duration]) -- Add's a new file to the playlist.\n");

	PyObject* PlayList_Add(PlayList *self, PyObject *args)
	{
		char *cFileName = NULL;
		char *cDescription = NULL;
		int iDuration = 0;
		CPlayList::CPlayListItem Item;

		if (!PyArg_ParseTuple(args, "s|sl", &cFileName, &cDescription, &iDuration))	return NULL;

		Item.SetFileName(cFileName);

		if (cDescription) Item.SetDescription(cDescription);
		else Item.SetDescription(cFileName);

		if (iDuration) Item.SetDuration(iDuration);
		else Item.SetDuration(0);

		self->pPlayList->Add(Item);

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(load__doc__,
		"load(filename) -- Load a playlist.\n"
		"\n"
		"clear current playlist and copy items from the file to this Playlist\n"
		"filename can be like .pls or .m3u ...\n"
		"returns False if unable to load playlist");

	PyObject* PlayList_Load(PlayList *self, PyObject *args)
	{
		char* cFileName = NULL;

		if (!PyArg_ParseTuple(args, "s", &cFileName))	return NULL;

		CFileItem item(cFileName);
		item.m_strPath=cFileName;

		if (item.IsPlayList())
		{
			// load playlist and copy al items to existing playlist

			// load a playlist like .m3u, .pls
			// first get correct factory to load playlist
			CPlayListFactory factory;
			auto_ptr<CPlayList> pPlayList (factory.Create(item.m_strPath));
			if ( NULL != pPlayList.get())
			{
				// load it
				if (!pPlayList->Load(item.m_strPath))
				{
					//hmmm unable to load playlist?
					return Py_BuildValue("b", false);
				}

				// clear current playlist
				g_playlistPlayer.GetPlaylist(self->iPlayList).Clear();

				// add each item of the playlist to the playlistplayer
				for (int i=0; i < (int)pPlayList->size(); ++i)
				{
					const CPlayList::CPlayListItem& playListItem =(*pPlayList)[i];
					CStdString strLabel=playListItem.GetDescription();
					if (strLabel.size()==0) 
						strLabel=CUtil::GetFileName(playListItem.GetFileName());

					CPlayList::CPlayListItem playlistItem;
					playlistItem.SetFileName(playListItem.GetFileName());
					playlistItem.SetDescription(strLabel);
					playlistItem.SetDuration(playListItem.GetDuration());

					self->pPlayList->Add(playlistItem);
				}
			}
		}
		else
		{
			// filename is not a valid playlist
			PyErr_SetString(PyExc_ValueError, "Not a valid playlist");
			return NULL;
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(remove__doc__,
		"remove(filename) -- remove an item with this filename from the playlist.\n");

	PyObject* PlayList_Remove(PlayList *self, PyObject *args)
	{
		char *cFileName;
		if (!PyArg_ParseTuple(args, "s", &cFileName))	return NULL;

		self->pPlayList->Remove(cFileName);

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(clear__doc__,
		"clear() -- clear all items in the playlist.\n");

	PyObject* PlayList_Clear(PlayList *self, PyObject *args)
	{
		self->pPlayList->Clear();
		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(shuffle__doc__,
		"shuffle() -- shuffle the playlist.\n");

	PyObject* PlayList_Shuffle(PlayList *self, PyObject *args)
	{
		self->pPlayList->Shuffle();
		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(size__doc__,
		"size() -- returns the total number of PlayListItems in this playlist.\n");

	PyObject* PlayList_Size(PlayList *self, PyObject *key)
	{
		return Py_BuildValue("i", self->pPlayList->size());
	}

	int PlayList_Length(PyObject *self)
	{
		return ((PlayList*)self)->pPlayList->size();
	}

	PyObject* PlayList_GetItem(PyObject *self, PyObject *pAttr)
	{
		long pos = -1;
		int iPlayListSize = ((PlayList*)self)->pPlayList->size();
		if (PyInt_Check(pAttr))
		{
			pos = PyInt_AS_LONG(pAttr);
			if (pos < 0) pos += iPlayListSize;
		}
		else if (PyLong_Check(pAttr))
		{
			pos = PyLong_AsLong(pAttr);
			if (pos == -1 && PyErr_Occurred()) return NULL;
			if (pos < 0) pos += iPlayListSize;
		}
		else
		{
			PyErr_SetString(PyExc_TypeError, "playlist indices must be integers");
			return NULL;
		}
		if (pos < 0 || pos >= iPlayListSize)
		{
			PyErr_SetString(PyExc_TypeError, "array out of bound");
			return NULL;
		}

		PlayListItem* item = (PlayListItem*)_PyObject_New(&PlayListItem_Type);
		//Py_INCREF(item);
		
		item->item = new PLAYLIST::CPlayList::CPlayListItem();

		CPlayList* p = ((PlayList*)self)->pPlayList;
		*item->item = (*p)[pos];

		return (PyObject*)item;
	}

  PyDoc_STRVAR(getposition__doc__,
    "getposition() -- returns the position of the current song in this playlist.\n");

  PyObject* PlayList_GetPosition(PlayList *self, PyObject *key)
  {
    return Py_BuildValue("i", g_playlistPlayer.GetCurrentSong());
  }

	PyMethodDef PlayListItem_methods[] = {
		{"getdescription", (PyCFunction)PlayListItem_GetDescription, METH_VARARGS, getDescription__doc__},
		{"getduration", (PyCFunction)PlayListItem_GetDuration, METH_VARARGS, getDuration__doc__},
		{"getfilename", (PyCFunction)PlayListItem_GetFileName, METH_VARARGS, getFilename__doc__},
		{NULL, NULL, 0, NULL}
	};

	/*
	static PyMemberDef PlayList_Members[] = {
		{"name", T_INT, offsetof(PlayList, member), READONLY, "Music PlayList"},
		{NULL}
	};
*/

	PyMappingMethods Playlist_as_mapping = {
    PlayList_Length,		/* inquiry mp_length;                 /* __len__ */
    PlayList_GetItem,		/* binaryfunc mp_subscript            /* __getitem__ */
    0,									/* objargproc mp_ass_subscript;    /* __setitem__ */
	};

	PyMethodDef PlayList_methods[] = {
		{"add", (PyCFunction)PlayList_Add, METH_VARARGS, add__doc__},
		{"load", (PyCFunction)PlayList_Load, METH_VARARGS, load__doc__},	
		{"remove", (PyCFunction)PlayList_Remove, METH_VARARGS, remove__doc__},
		{"clear", (PyCFunction)PlayList_Clear, METH_VARARGS, clear__doc__},
		{"size", (PyCFunction)PlayList_Size, METH_VARARGS, size__doc__},
		{"shuffle", (PyCFunction)PlayList_Shuffle, METH_VARARGS, shuffle__doc__},
		{"getposition", (PyCFunction)PlayList_GetPosition, METH_VARARGS, getposition__doc__},
		{NULL, NULL, 0, NULL}
	};

	PyDoc_STRVAR(playlistItem__doc__,
		"PlayListItem class.\n"
		"\n"
		"PlayListItem() -- Creates a new PlaylistItem which can be added to a PlayList.");

	PyDoc_STRVAR(playlist__doc__,
		"PlayList class.\n"
		"\n"
		"PlayList(int playlist) -- retrieve a reference from a valid xbmc playlist\n"
		"\n"
		"int playlist can be one of the next values:\n"
		"\n"
		"  0 : xbmc.PLAYLIST_MUSIC\n"
		"  1 : xbmc.PLAYLIST_MUSIC_TEMP\n"
		"  2 : xbmc.PLAYLIST_VIDEO\n"
		"  3 : xbmc.PLAYLIST_VIDEO_TEMP\n"
		"\n"
		"Use PlayList[int position] or __getitem__(int position) to get a PlayListItem.");

// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

	PyTypeObject PlayListItem_Type = {
			PyObject_HEAD_INIT(NULL)
			0,                         /*ob_size*/
			"xbmc.PlayListItem",       /*tp_name*/
			sizeof(PlayListItem),      /*tp_basicsize*/
			0,                         /*tp_itemsize*/
			(destructor)PlayListItem_Dealloc,/*tp_dealloc*/
			0,                         /*tp_print*/
			0,                         /*tp_getattr*/
			0,                         /*tp_setattr*/
			0,                         /*tp_compare*/
			0,                         /*tp_repr*/
			0,                         /*tp_as_number*/
			0,                         /*tp_as_sequence*/
			0,                         /*tp_as_mapping*/
			0,                         /*tp_hash */
			0,                         /*tp_call*/
			0,                         /*tp_str*/
			0,                         /*tp_getattro*/
			0,                         /*tp_setattro*/
			0,                         /*tp_as_buffer*/
			Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
			playlistItem__doc__,       /* tp_doc */
			0,		                     /* tp_traverse */
			0,		                     /* tp_clear */
			0,		                     /* tp_richcompare */
			0,		                     /* tp_weaklistoffset */
			0,		                     /* tp_iter */
			0,		                     /* tp_iternext */
			PlayListItem_methods,      /* tp_methods */
			0,                         /* tp_members */
			0,                         /* tp_getset */
			0,                         /* tp_base */
			0,                         /* tp_dict */
			0,                         /* tp_descr_get */
			0,                         /* tp_descr_set */
			0,                         /* tp_dictoffset */
			0,                         /* tp_init */
			0,                         /* tp_alloc */
			PlayListItem_New,          /* tp_new */
	};

	PyTypeObject PlayList_Type = {
			PyObject_HEAD_INIT(NULL)
			0,                         /*ob_size*/
			"xbmc.PlayList",           /*tp_name*/
			sizeof(PlayList),          /*tp_basicsize*/
			0,                         /*tp_itemsize*/
			(destructor)PlayList_Dealloc,/*tp_dealloc*/
			0,                         /*tp_print*/
			0,                         /*tp_getattr*/
			0,                         /*tp_setattr*/
			0,                         /*tp_compare*/
			0,                         /*tp_repr*/
			0,                         /*tp_as_number*/
			0,                         /*tp_as_sequence*/
			&Playlist_as_mapping,      /*tp_as_mapping*/
			0,                         /*tp_hash */
			0,                         /*tp_call*/
			0,                         /*tp_str*/
			0,                         /*tp_getattro*/
			0,                         /*tp_setattro*/
			0,                         /*tp_as_buffer*/
			Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
			playlist__doc__,           /* tp_doc */
			0,		                     /* tp_traverse */
			0,		                     /* tp_clear */
			0,		                     /* tp_richcompare */
			0,		                     /* tp_weaklistoffset */
			0,		                     /* tp_iter */
			0,		                     /* tp_iternext */
			PlayList_methods,          /* tp_methods */
			0,                         /* tp_members */
			0,                         /* tp_getset */
			0,                         /* tp_base */
			0,                         /* tp_dict */
			0,                         /* tp_descr_get */
			0,                         /* tp_descr_set */
			0,                         /* tp_dictoffset */
			0,                         /* tp_init */
			0,                         /* tp_alloc */
			PlayList_New,              /* tp_new */
	};
}

#ifdef __cplusplus
}
#endif

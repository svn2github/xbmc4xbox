#include "..\python.h"
#include <string>
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
	typedef struct {
    PyObject_HEAD
		std::string strDefault;
    std::string strHeading;
	} Keyboard;

	extern PyTypeObject Keyboard_Type;
}

#ifdef __cplusplus
}
#endif

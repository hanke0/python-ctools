/* Copyright 2019 ko-han. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#include <Python.h>

#include "ctools_funcs.h"
#include "ctools_lfu.h"
#include "ctools_strhash.h"

static PyMethodDef CtoolsMethods[] = {
    {"jump_consistent_hash", (PyCFunction)Ctools__jump_hash, METH_VARARGS,
     jump_consistent_hash__doc__},
    {"strhash", (PyCFunction)Ctools__strhash, METH_VARARGS | METH_KEYWORDS,
     strhash__doc__},
    {"int8_to_datetime", (PyCFunction)Ctools__int8_to_datetime, METH_O,
     int8_to_datetime__doc__},
    {NULL, NULL, 0, NULL},
};

PyDoc_STRVAR(ctools__doc__,
             "A collection of useful functions for python implement in C.");
static struct PyModuleDef ctools_module = {
    PyModuleDef_HEAD_INIT,
    "ctools",      /* m_name */
    ctools__doc__, /* m_doc */
    -1,            /* m_size */
    CtoolsMethods, /* m_methods */
    NULL,          /* m_reload */
    NULL,          /* m_traverse */
    NULL,          /* m_clear */
    NULL,          /* m_free */
};

PyMODINIT_FUNC PyInit_ctools(void) {
  PyDateTime_IMPORT;
  if (PyType_Ready(&LFUCacheType) < 0) return NULL;

  if (PyType_Ready(&LFUWrapperType) < 0) return NULL;

  PyObject *m = PyModule_Create(&ctools_module);
  if (m == NULL) return NULL;

  Py_INCREF(&LFUWrapperType);
  Py_INCREF(&LFUCacheType);
  PyModule_AddObject(m, "LFUCache", (PyObject *)&LFUCacheType);
  PyModule_AddObject(m, "LFUWrapper", (PyObject *)&LFUWrapperType);
  return m;
}

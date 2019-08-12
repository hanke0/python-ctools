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

#include "util.h"
#include "cachemap.c"
#include "channel.c"
#include "functions.c"
#include "rbtree.c"
#include "ttlcache.c"

static PyMethodDef ctools_methods[] = {
    { "jump_consistent_hash",
      Ctools__jump_hash,
      METH_VARARGS,
      jump_consistent_hash__doc__ },
    { "strhash", Ctools__strhash, METH_VARARGS, strhash__doc__ },
    { "int8_to_datetime",
      Ctools__int8_to_datetime,
      METH_O,
      int8_to_datetime__doc__ },
    { "build_with_debug", (PyCFunction)build_with_debug, METH_NOARGS, NULL },
    { NULL, NULL, 0, NULL },
};


static struct PyModuleDef _ctools_module = {
    PyModuleDef_HEAD_INIT,
    "_ctools", /* m_name */
    NULL,               /* m_doc */
    -1,                 /* m_size */
    ctools_methods,               /* m_methods */
    NULL,               /* m_reload */
    NULL,               /* m_traverse */
    NULL,               /* m_clear */
    NULL,               /* m_free */
};


static const char *
_type_name(PyTypeObject *type)
{
  const char *s = strrchr(type->tp_name, '.');
  if (s == NULL) {
    s = type->tp_name;
  }
  else {
    s++;
  }
  return s;
}

PyMODINIT_FUNC
PyInit__ctools(void)
{
  int i;
  PyObject *m;
  const char *name;
  PyDateTime_IMPORT;

  PyTypeObject* typelist[] = {
      &CacheMap_Type,
      &CacheEntry_Type,
      &Channel_Type,
      &TTLCache_Type,
      &TTLCacheEntry_Type,
      NULL,
  };

  m = PyModule_Create(&_ctools_module);
  RETURN_IF_NULL(m, NULL);

  for (i=0 ; typelist[i] != NULL ; i++) {
    if (PyType_Ready(typelist[i]) < 0) {
      Py_DECREF(m);
      return NULL;
    }
    name = _type_name(typelist[i]);
    Py_INCREF(typelist[i]);
    PyModule_AddObject(m, name, (PyObject *)typelist[i]);
  }

  return m;
}
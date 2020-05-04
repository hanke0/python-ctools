/*
Copyright (c) 2019 ko han

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

#include "module.h"

#include "Python.h"

static struct PyModuleDef _ctools = {
    PyModuleDef_HEAD_INIT,
    "ctools._ctools",                /* m_name */
    "A collection of useful tools.", /* m_doc */
    -1,                              /* m_size */
    NULL,                            /* m_methods */
    NULL,                            /* m_reload */
    NULL,                            /* m_traverse */
    NULL,                            /* m_clear */
    NULL,                            /* m_free */
};

#define CtoolsModuleInitOne(name)                                              \
  do {                                                                         \
    if (name(module)) {                                                        \
      Py_DECREF(module);                                                       \
      return NULL;                                                             \
    }                                                                          \
  } while (0)

PyMODINIT_FUNC PyInit__ctools(void) {
  PyObject *module;
  module = PyModule_Create(&_ctools);
  ReturnIfNULL(module, NULL);

  CtoolsModuleInitOne(ctools_init_cachemap);
  CtoolsModuleInitOne(ctools_init_funcs);
  CtoolsModuleInitOne(ctools_init_channel);
  CtoolsModuleInitOne(ctools_init_ttlcache);
  CtoolsModuleInitOne(ctools_init_rbtree);
  return module;
}
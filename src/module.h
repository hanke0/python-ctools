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

#ifndef _CTOOLS_MODULE_H_
#define _CTOOLS_MODULE_H_

#include "core.h"

#include <Python.h>

EXTERN_C_START

int ctools_init_cachemap(PyObject *module);

int ctools_init_channel(PyObject *module);

int ctools_init_funcs(PyObject *module);

int ctools_init_ttlcache(PyObject *module);

int ctools_init_rbtree(PyObject *module);

EXTERN_C_END

#endif // _CTOOLS_MODULE_H_
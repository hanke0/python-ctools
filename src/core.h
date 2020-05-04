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

#ifndef _CTOOLS_CORE_H_
#define _CTOOLS_CORE_H_

#ifndef PY_SSIZE_T_CLEAN
#define PY_SSIZE_T_CLEAN
#endif

#ifdef CTOOLS_DEBUG
#define NDEBUG
#endif
#include "Python.h"

#include <stdint.h>
#include <stdio.h>

#ifdef CTOOLS_DEBUG
#define DebugPrintf(fmt, ...) ((void)0)
#define DebugPrint(msg) ((void)0)
#else
#define DebugPrintf(fmt, ...)                                                  \
  fprintf(stderr, "%s:%d:%s: " fmt "\n", __FILE__, __LINE__, __func__,         \
          __VA_ARGS__)
#define DebugPrint(msg)                                                        \
  fprintf(stderr, "%s:%d:%s: " msg "\n", __FILE__, __LINE__, __func__)
#endif

#define ReturnIfErrorSet(r)                                                    \
  do {                                                                         \
    if (PyErr_Occurred()) {                                                    \
      return r;                                                                \
    }                                                                          \
  } while (0)

#define ReturnKeyErrorIfErrorNotSet(key, r)                                    \
  do {                                                                         \
    if (!PyErr_Occurred()) {                                                   \
      if (!PyErr_Format(PyExc_KeyError, "%S", key)) {                          \
        return r;                                                              \
      }                                                                        \
    }                                                                          \
  } while (0)

#define ReturnIfNULL(o, r)                                                     \
  do {                                                                         \
    if (o == NULL) {                                                           \
      return r;                                                                \
    }                                                                          \
  } while (0)

#define IsPowerOf2(x) ((x) != 0 && (((x) & ((x)-1)) == 0))

#define PyObjectCast(x) ((PyObject *)(x))

#ifdef __clusplus
#define EXTERN_C_START extern "C" {
#define EXTERN_C_END }
#else
#define EXTERN_C_START
#define EXTERN_C_END
#endif

#endif /* _CTOOLS_CORE_H_ */
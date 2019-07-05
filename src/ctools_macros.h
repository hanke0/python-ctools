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

#ifndef _CTOOLS_MACROS_H_
#define _CTOOLS_MACROS_H_

#ifdef __cplusplus
extern "C"
{
#endif
#include <stdio.h>

#ifdef NDEBUG
#define DEBUG_PRINTF(FORMAT, ...) ((void)0)
#else
#define DEBUG_PRINTF(FORMAT, ...)                                              \
  printf("%s() in %s, line %i: " FORMAT "\n",                                  \
         __func__,                                                             \
         __FILE__,                                                             \
         __LINE__,                                                             \
         __VA_ARGS__)
#endif

#define PyObject_HASHABLE(o) (PyObject_Hash((o)) == -1)

#define RETURN_IF_ERROR_SET(r)                                                 \
  if (PyErr_Occurred())                                                        \
  return r

#define RETURN_KEY_ERROR_IF_ERROR_NOT_SET(key, r)                              \
  do {                                                                         \
    if (!PyErr_Occurred()) {                                                   \
      if (!PyErr_Format(PyExc_KeyError, "%S", key))                            \
        return r;                                                              \
    }                                                                          \
  } while (0)

#define RETURN_IF_NULL(o, r)                                                   \
  if (o == NULL)                                                               \
  return r

#ifdef __cplusplus
}
#endif

#endif /* _CTOOLS_MACROS_H_ */

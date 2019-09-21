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
#ifdef _MSC_VER
#ifndef int8_t
  typedef signed __int8 int8_t;
  typedef signed __int16 int16_t;
  typedef signed __int32 int32_t;
  typedef unsigned __int8 uint8_t;
  typedef unsigned __int16 uint16_t;
  typedef unsigned __int32 uint32_t;
  typedef signed __int64 int64_t;
  typedef unsigned __int64 uint64_t;
#define INT8_MIN ((int8_t)_I8_MIN)
#define INT8_MAX _I8_MAX
#define INT16_MIN ((int16_t)_I16_MIN)
#define INT16_MAX _I16_MAX
#define INT32_MIN ((int32_t)_I32_MIN)
#define INT32_MAX _I32_MAX
#define INT64_MIN ((int64_t)_I64_MIN)
#define INT64_MAX _I64_MAX
#define UINT8_MAX _UI8_MAX
#define UINT16_MAX _UI16_MAX
#define UINT32_MAX _UI32_MAX
#define UINT64_MAX _UI64_MAX
#endif /* int8_t */
#endif /* _MSC_VER */

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

#define IS_POW_OF_2(x) ((x) != 0 && (((x) & ((x)-1)) == 0))

#define PYOBJECT_CAST(x) ((PyObject*)(x))

#ifdef __cplusplus
}
#endif

#endif /* _CTOOLS_MACROS_H_ */

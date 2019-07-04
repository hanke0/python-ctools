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
#include <datetime.h>

#include "ctools_config.h"

PyDoc_STRVAR(jump_consistent_hash__doc__,
             "jump_consistent_hash(key: int, num_buckets: int) -> int:\n\n\
    Generate a number in the range [0, num_buckets).\n\
    This function uses C bindings for speed.\n\n\
    :param key: The key to hash.\n\
    :type key: int\n\
    :param num_buckets: Number of buckets to use.\n\
    :type num_buckets: int\n\
    :return: hash number\n\
    :rtype: int\n");

static PyObject*
Ctools__jump_hash(PyObject* m, PyObject* args)
{
  uint64_t key;
  int32_t num_buckets;

  if (!PyArg_ParseTuple(args, "Ki", &key, &num_buckets))
    return NULL;

  int64_t b = -1, j = 0;
  while (j < num_buckets) {
    b = j;
    key = key * 2862933555777941757ULL + 1;
    j = (b + 1) * ((double)(1LL << 31) / ((double)((key >> 33) + 1)));
  }
  return Py_BuildValue("i", b);
}

#define PyDateTime_FromDate(year, month, day)                                  \
  PyDateTime_FromDateAndTime(year, month, day, 0, 0, 0, 0)

PyDoc_STRVAR(int8_to_datetime__doc__,
             "int8_to_datetime(date_integer) -> datetime.datetime:\n\n\
    Convert int like 20180101 to datetime.datetime(2018, 1, 1)).\n\n\
    This function uses C bindings for speed.\n\n\
    :param date_integer: The string to hash.\n\
    :type date_integer: int\n\
    :return: parsed datetime\n\
    :rtype: datetime.datetime\n");

static PyObject*
Ctools__int8_to_datetime(PyObject* m, PyObject* date_integer)
{
  register long date = PyLong_AsLong(date_integer);
  if (date > 99990101 || date < 101) {
    PyErr_SetString(PyExc_ValueError,
                    "date integer should between 00000101 and 99991231");
    return NULL;
  }
  return PyDateTime_FromDate(date / 10000, date % 10000 / 100, date % 100);
}
static unsigned int
fnv1a(const char* s, unsigned long len)
{
  unsigned int hash = 2166136261U;
  for (unsigned long i = 0; i < len; i++) {
    hash = hash ^ s[i];
    /* hash * (1 << 24 + 1 << 8 + 0x93) */
    hash +=
      (hash << 1) + (hash << 4) + (hash << 7) + (hash << 8) + (hash << 24);
  }
  return hash;
}

static unsigned int
fnv1(const char* s, unsigned long len)
{
  unsigned int hash = 2166136261U;
  for (unsigned long i = 0; i < len; i++) {
    /* hash * (1 << 24 + 1 << 8 + 0x93) */
    hash +=
      (hash << 1) + (hash << 4) + (hash << 7) + (hash << 8) + (hash << 24);
    hash = hash ^ s[i];
  }
  return hash;
}

static unsigned int
murmur_hash2(const char* str, unsigned long len)
{
  unsigned int hash, key;

  hash = 0 ^ len;
  while (len >= 4) {
    key = str[0];
    key |= str[1] << 8;
    key |= str[2] << 16;
    key |= str[3] << 24;

    key *= 0x5bd1e995;
    key ^= key >> 24;
    key *= 0x5bd1e995;

    hash *= 0x5bd1e995;
    hash ^= key;

    str += 4;
    len -= 4;
  }

  switch (len) {
    case 3:
      hash ^= str[2] << 16;
      /* fall through */
    case 2:
      hash ^= str[1] << 8;
      /* fall through */
    case 1:
      hash ^= str[0];
      hash *= 0x5bd1e995;
    default:;
  }

  hash ^= hash >> 13;
  hash *= 0x5bd1e995;
  hash ^= hash >> 15;

  return hash;
}

static unsigned int
djb2(const char* str, unsigned long len)
{
  unsigned int hash = 5381;
  for (unsigned long i = 0; i < len; i++) {
    hash = ((hash << 5) + hash) + str[i];
  }

  return hash;
}

PyDoc_STRVAR(strhash__doc__, "strhash(s, method='fnv1a') -> int:\n\n\
    hash str with consistent value.\n\n\
    This function uses C bindings for speed.\n\n\
    :param s: The string to hash.\n\
    :param method: fnv1a | fnv1 | djb2\n\
    :type s: string\n\
    :return: hash number\n\
    :rtype: int\n");

static PyObject*
Ctools__strhash(PyObject* m, PyObject* args)
{
  const char *s, *method = NULL;
  Py_ssize_t len = 0, m_len = 0;
  if (!PyArg_ParseTuple(args, "s#|s#", &s, &len, &method, &m_len))
    return NULL;
  if (method == NULL)
    return Py_BuildValue("I", fnv1a(s, len));
  switch (method[0]) {
    case 'f': {
      if (m_len == 5)
        return Py_BuildValue("I", fnv1a(s, len));
      else
        return Py_BuildValue("I", fnv1(s, len));
    }
    case 'd':
      return Py_BuildValue("I", djb2(s, len));
    case 'm':
      return Py_BuildValue("I", murmur_hash2(s, len));
    default: {
      PyErr_SetString(PyExc_ValueError, "invalid method");
      return NULL;
    }
  }
}

static PyObject*
build_with_debug(PyObject* self, PyObject* unused)
{
#ifndef NDEBUG
  Py_RETURN_TRUE;
#else
  Py_RETURN_FALSE;
#endif
}

static PyMethodDef ctools_utils_methods[] = {
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

static struct PyModuleDef _ctools_utils_module = {
  PyModuleDef_HEAD_INIT,
  "_ctools_utils",      /* m_name */
  NULL,                 /* m_doc */
  -1,                   /* m_size */
  ctools_utils_methods, /* m_methods */
  NULL,                 /* m_reload */
  NULL,                 /* m_traverse */
  NULL,                 /* m_clear */
  NULL,                 /* m_free */
};

PyMODINIT_FUNC
PyInit__ctools_utils(void)
{
  PyDateTime_IMPORT;

  return PyModule_Create(&_ctools_utils_module);
}

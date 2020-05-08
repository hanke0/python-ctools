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

#include "core.h"

#include <Python.h>
#include <datetime.h>

PyDoc_STRVAR(jump_consistent_hash__doc__,
             "jump_consistent_hash(key, num_buckets, /)\n"
             "--\n\n"
             "Generate a number in the range [0, num_buckets).\n\n"
             "Parameters\n"
             "----------\n"
             "key : int\n"
             "  The key to hash.\n"
             "num_buckets : int\n"
             "  Number of buckets to use.\n"
             "\n"
             "Returns\n"
             "-------\n"
             "int\n"
             "  hash number.\n");

static PyObject *Ctools__jump_hash(PyObject *Py_UNUSED(module),
                                   PyObject *args) {
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

PyDoc_STRVAR(
    int8_to_datetime__doc__,
    "int8_to_datetime(date_integer, /)\n"
    "--\n\n"
    "Convert integer like 20180101 to datetime.datetime(2018, 1, 1)).\n\n"
    "Parameters\n"
    "----------\n"
    "date_integer : int\n"
    "  The string to hash.\n"
    "\n"
    "Returns\n"
    "-------\n"
    "datetime.datetime\n"
    "  parsed datetime\n"
    "\n"
    "Examples\n"
    "--------\n"
    ">>> import ctools\n"
    ">>> ctools.int8_to_datetime(20010101)\n"
    "datetime.datetime(2001, 1, 1, 0, 0)\n");

static PyObject *Ctools__int8_to_datetime(PyObject *Py_UNUSED(module),
                                          PyObject *date_integer) {
  register long date = PyLong_AsLong(date_integer);
  if (date > 99990101 || date < 101) {
    PyErr_SetString(PyExc_ValueError,
                    "date integer should between 00000101 and 99991231");
    return NULL;
  }
  return PyDateTime_FromDate(date / 10000, date % 10000 / 100, date % 100);
}

static unsigned int fnv1a(const char *s, unsigned long len) {
  unsigned int hash = 2166136261U;
  for (unsigned long i = 0; i < len; i++) {
    hash = hash ^ s[i];
    /* hash * (1 << 24 + 1 << 8 + 0x93) */
    hash +=
        (hash << 1) + (hash << 4) + (hash << 7) + (hash << 8) + (hash << 24);
  }
  return hash;
}

static unsigned int fnv1(const char *s, unsigned long len) {
  unsigned int hash = 2166136261U;
  for (unsigned long i = 0; i < len; i++) {
    /* hash * (1 << 24 + 1 << 8 + 0x93) */
    hash +=
        (hash << 1) + (hash << 4) + (hash << 7) + (hash << 8) + (hash << 24);
    hash = hash ^ s[i];
  }
  return hash;
}

static unsigned int murmur_hash2(const char *str, unsigned long len) {
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

static unsigned int djb2(const char *str, unsigned long len) {
  unsigned int hash = 5381;
  for (unsigned long i = 0; i < len; i++) {
    hash = ((hash << 5) + hash) + str[i];
  }

  return hash;
}

PyDoc_STRVAR(strhash__doc__,
             "strhash(s, method='fnv1a', /)\n"
             "--\n\n"
             "Hash str with consistent value.\n"
             "\n"
             "Parameters\n"
             "----------\n"
             "s : str\n"
             "  The string to hash.\n"
             "method : {'fnv1a', 'fnv1', 'djb2', 'murmur'}, optional\n"
             "  Choice in method, default first when optional.\n"
             "\n"
             "Returns\n"
             "-------\n"
             "int\n"
             "  hash number\n"
             "\n"
             "Raises\n"
             "------\n"
             "ValueError"
             "  If method not supported.\n");

static PyObject *Ctools__strhash(PyObject *Py_UNUSED(module), PyObject *args) {
  const char *s, *method = NULL;
  Py_ssize_t len = 0, m_len = 0;
  if (!PyArg_ParseTuple(args, "s#|s#", &s, &len, &method, &m_len))
    return NULL;
  if (method == NULL)
    return Py_BuildValue("I", fnv1a(s, len));
  switch (method[0]) {
  case 'f':
    if (m_len == 5) {
      return Py_BuildValue("I", fnv1a(s, len));
    } else {
      return Py_BuildValue("I", fnv1(s, len));
    }
  case 'd':
    return Py_BuildValue("I", djb2(s, len));
  case 'm':
    return Py_BuildValue("I", murmur_hash2(s, len));
  default:
    PyErr_SetString(PyExc_ValueError, "invalid method");
    return NULL;
  }
}

static PyObject *build_with_debug(PyObject *Py_UNUSED(self),
                                  PyObject *Py_UNUSED(u)) {
#ifndef NDEBUG
  Py_RETURN_TRUE;
#else
  Py_RETURN_FALSE;
#endif
}

static PyMethodDef methods[] = {
    {"jump_consistent_hash", Ctools__jump_hash, METH_VARARGS,
     jump_consistent_hash__doc__},
    {"strhash", Ctools__strhash, METH_VARARGS, strhash__doc__},
    {"int8_to_datetime", Ctools__int8_to_datetime, METH_O,
     int8_to_datetime__doc__},
    {"build_with_debug", (PyCFunction)build_with_debug, METH_NOARGS,
     "build_with_debug()\n--\n\nReturn if build in debug."},
    {NULL, NULL, 0, NULL},
};

EXTERN_C_START
int ctools_init_funcs(PyObject *module) {
  PyDateTime_IMPORT;
  if (PyModule_AddFunctions(module, methods)) {
    return -1;
  }
  return 0;
}
EXTERN_C_END
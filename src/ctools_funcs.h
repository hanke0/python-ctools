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
#ifndef _CTOOLS_FUNCS_H
#define _CTOOLS_FUNCS_H
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

static PyObject *Ctools__jump_hash(PyObject *m, PyObject *args) {
  uint64_t key;
  int32_t num_buckets;

  if (!PyArg_ParseTuple(args, "Ki", &key, &num_buckets)) return NULL;

  int64_t b = -1, j = 0;
  while (j < num_buckets) {
    b = j;
    key = key * 2862933555777941757ULL + 1;
    j = (b + 1) * ((double)(1LL << 31) / ((double)((key >> 33) + 1)));
  }
  return Py_BuildValue("i", b);
}

#define PyDateTime_FromDate(year, month, day) \
  PyDateTime_FromDateAndTime(year, month, day, 0, 0, 0, 0)

PyDoc_STRVAR(int8_to_datetime__doc__,
             "int8_to_datetime(date_integer) -> datetime.datetime:\n\n\
    Convert int like 20180101 to datetime.datetime(2018, 1, 1)).\n\n\
    This function uses C bindings for speed.\n\n\
    :param date_integer: The string to hash.\n\
    :type date_integer: int\n\
    :return: parsed datetime\n\
    :rtype: datetime.datetime\n");

static PyObject *Ctools__int8_to_datetime(PyObject *m, PyObject *date_integer) {
  register long date = PyLong_AsLong(date_integer);
  if (date > 99990101 || date < 101) {
    PyErr_SetString(PyExc_ValueError,
                    "date integer should between 00000101 and 99991231");
    return NULL;
  }
  return PyDateTime_FromDate(date / 10000, date % 10000 / 100, date % 100);
}
#endif  // _CTOOLS_FUNCS_H

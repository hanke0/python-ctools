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
#ifndef _CTOOLS_STRHASH_H_
#define _CTOOLS_STRHASH_H_

#include "ctools_config.h"

unsigned int fnv1a(const char *s, unsigned long len) {
  unsigned int hash = 2166136261U;
  for (unsigned long i = 0; i < len; i++) {
    hash = hash ^ s[i];
    /* hash * (1 << 24 + 1 << 8 + 0x93) */
    hash +=
        (hash << 1) + (hash << 4) + (hash << 7) + (hash << 8) + (hash << 24);
  }
  return hash;
}

unsigned int fnv1(const char *s, unsigned long len) {
  unsigned int hash = 2166136261U;
  for (unsigned long i = 0; i < len; i++) {
    /* hash * (1 << 24 + 1 << 8 + 0x93) */
    hash +=
        (hash << 1) + (hash << 4) + (hash << 7) + (hash << 8) + (hash << 24);
    hash = hash ^ s[i];
  }
  return hash;
}

unsigned int murmur_hash2(const char *str, unsigned long len) {
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

unsigned int djb2(const char *str, unsigned long len) {
  unsigned int hash = 5381;
  for (unsigned long i = 0; i < len; i++) {
    hash = ((hash << 5) + hash) + str[i];
  }

  return hash;
}

PyDoc_STRVAR(strhash__doc__,
             "strhash(s, method='fnv1a') -> int:\n\n\
    hash str with consistent value.\n\n\
    This function uses C bindings for speed.\n\n\
    :param s: The string to hash.\n\
    :param method: fnv1a | fnv1 | djb2\n\
    :type s: string\n\
    :return: hash number\n\
    :rtype: int\n");

static PyObject *Ctools__strhash(PyObject *m, PyObject *args) {
  const char *s, *method = NULL;
  Py_ssize_t len = 0;
  if (!PyArg_ParseTuple(args, "s#|s", &s, &len, &method)) return NULL;
  if (method == NULL) return Py_BuildValue("I", fnv1a(s, len));
  switch (method[0]) {
    case 'f': {
      if (len == 5)
        return Py_BuildValue("I", fnv1a(s, len));
      else
        return Py_BuildValue("I", fnv1(s, len));
    }
    case 'd':
      return Py_BuildValue("I", djb2(s, len));
    case 'm':
      return Py_BuildValue("I", len);
    default: {
      PyErr_SetString(PyExc_ValueError, "invalid method");
      return NULL;
    }
  }
}

#endif  // _CTOOLS_STRHASH_H_

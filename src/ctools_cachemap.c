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
#include <time.h>

#include "ctools_config.h"
#include "ctools_macros.h"

#define CacheEntry_DEFAULT_VISITS 255U
#define CacheEntry_DECREASE_EVERY_MINUTES 10

#define CacheMap_BUCKET_NUM 8
#define CacheMap_BUCKET_SIZE 256

static inline unsigned int
time_in_minutes(void)
{
  return (unsigned int)(((uint64_t)time(NULL) / 60) & UINT32_MAX);
}

typedef struct _cacheentry
{
  /* clang-format off */
  PyObject_HEAD
  PyObject* ma_value;
  /* clang-format on */
  uint32_t last_visit;
  uint32_t visits;
} CacheMapEntry;

static PyTypeObject CacheEntry_Type;

static CacheMapEntry*
CacheEntry_New(PyObject* ma_value)
{
  CacheMapEntry* self;
  assert(ma_value);
  self = (CacheMapEntry*)PyObject_GC_New(CacheMapEntry, &CacheEntry_Type);
  RETURN_IF_NULL(self, NULL);
  self->ma_value = ma_value;
  Py_INCREF(ma_value);
  PyObject_GC_Track(self);
  return self;
}

static PyObject*
CacheEntry_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
  PyObject* ma_value;
  static char* kwlist[] = { "obj", NULL };
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &ma_value))
    return NULL;
  return (PyObject*)CacheEntry_New(ma_value);
}

#define CacheEntry_Init(self)                                                  \
  do {                                                                         \
    (self)->last_visit = time_in_minutes();                                    \
    (self)->visits = CacheEntry_DEFAULT_VISITS;                                \
  } while (0)

static int
CacheEntry_init(CacheMapEntry* self, PyObject* unused, PyObject* unused1)
{
  CacheEntry_Init(self);
  return 0;
}

static int
CacheEntry_tp_traverse(CacheMapEntry* self, visitproc visit, void* arg)
{
  Py_VISIT(self->ma_value);
  return 0;
}

static int
CacheEntry_tp_clear(CacheMapEntry* self)
{
  Py_CLEAR(self->ma_value);
  return 0;
}

static void
CacheEntry_tp_dealloc(CacheMapEntry* self)
{
  PyObject_GC_UnTrack(self);
  CacheEntry_tp_clear(self);
  PyObject_GC_Del(self);
}

#define CacheEntry_NewVisit(self)                                              \
  do {                                                                         \
    self->visits++;                                                            \
    self->last_visit = time_in_minutes();                                      \
  } while (0)

static PyObject*
CacheEntry_get_ma_value(CacheMapEntry* self)
{
  CacheEntry_NewVisit(self);
  PyObject* ma_value = self->ma_value;
  Py_INCREF(ma_value);
  return ma_value;
}

static inline unsigned int
CacheEntry_GetWeight(CacheMapEntry* self, unsigned int now)
{
  register unsigned int num, counter;
  counter = self->visits;
  num = (now - self->last_visit) / CacheEntry_DECREASE_EVERY_MINUTES;
  return num > counter ? 0 : counter - num;
}

static PyObject*
CacheEntry_get_weight(CacheMapEntry* self)
{
  return Py_BuildValue("I", CacheEntry_GetWeight(self, time_in_minutes()));
}

static PyMethodDef CacheEntry_methods[] = {
  { "get_value", (PyCFunction)CacheEntry_get_ma_value, METH_NOARGS, NULL },
  { "get_weight", (PyCFunction)CacheEntry_get_weight, METH_NOARGS, NULL },
  { NULL, NULL, 0, NULL } /* Sentinel */
};

static PyObject*
CacheEntry_repr(CacheMapEntry* self)
{
  return PyObject_Repr(self->ma_value);
}

static PyTypeObject CacheEntry_Type = {
  /* clang-format off */
  PyVarObject_HEAD_INIT(NULL, 0)
  /* clang-format on */
  "CacheMapEntry",                         /* tp_name */
  sizeof(CacheMapEntry),                   /* tp_basicsize */
  0,                                       /* tp_itemsize */
  (destructor)CacheEntry_tp_dealloc,       /* tp_dealloc */
  0,                                       /* tp_print */
  0,                                       /* tp_getattr */
  0,                                       /* tp_setattr */
  0,                                       /* tp_compare */
  (reprfunc)CacheEntry_repr,               /* tp_repr */
  0,                                       /* tp_as_number */
  0,                                       /* tp_as_sequence */
  0,                                       /* tp_as_mapping */
  0,                                       /* tp_hash */
  0,                                       /* tp_call */
  0,                                       /* tp_str */
  0,                                       /* tp_getattro */
  0,                                       /* tp_setattro */
  0,                                       /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, /* tp_flags */
  NULL,                                    /* tp_doc */
  (traverseproc)CacheEntry_tp_traverse,    /* tp_traverse */
  (inquiry)CacheEntry_tp_clear,            /* tp_clear */
  0,                                       /* tp_richcompare */
  0,                                       /* tp_weaklistoffset */
  0,                                       /* tp_iter */
  0,                                       /* tp_iternext */
  CacheEntry_methods,                      /* tp_methods */
  0,                                       /* tp_members */
  0,                                       /* tp_getset */
  0,                                       /* tp_base */
  0,                                       /* tp_dict */
  0,                                       /* tp_descr_get */
  0,                                       /* tp_descr_set */
  0,                                       /* tp_dictoffset */
  (initproc)CacheEntry_init,               /* tp_init */
  0,                                       /* tp_alloc */
  (newfunc)CacheEntry_new,                 /* tp_new */
};
/* CacheMapEntry Type Define */

typedef struct
{
  /* clang-format off */
  PyObject_HEAD
  PyObject* dict;
  /* clang-format on */
  Py_ssize_t capacity;
  Py_ssize_t hits;
  Py_ssize_t misses;
} CacheMap;

#define CacheMap_Size(self) (PyDict_Size(((CacheMap*)(self))->dict))

Py_ssize_t
CacheMap_size(CacheMap* self)
{
  return CacheMap_Size(self);
}

/* return a random number between 0 and limit inclusive. */
int
rand_integer(int limit)
{
  int divisor = RAND_MAX / (limit + 1);
  int rv;

  do {
    rv = rand() / divisor;
  } while (rv > limit);

  return rv;
}

static int
CacheMap_Contains(PyObject* self, PyObject* key)
{
  return PyDict_Contains(((CacheMap*)self)->dict, key);
}

/* Hack to implement "key in dict" */
static PySequenceMethods CacheMap_as_sequence = {
  0,                 /* sq_length */
  0,                 /* sq_concat */
  0,                 /* sq_repeat */
  0,                 /* sq_item */
  0,                 /* sq_slice */
  0,                 /* sq_ass_item */
  0,                 /* sq_ass_slice */
  CacheMap_Contains, /* sq_contains */
  0,                 /* sq_inplace_concat */
  0,                 /* sq_inplace_repeat */
};

#define CacheMap_GetItemWithError(self, key)                                   \
  ((CacheMapEntry*)PyDict_GetItemWithError(((CacheMap*)self)->dict,            \
                                           (PyObject*)(key)))

/* New Reference */
static PyObject*
CacheMap_NextEvictKey(CacheMap* self)
{
  PyObject *key = NULL, *wrapper = NULL;
  Py_ssize_t pos = 0;
  uint32_t min = 0, weight;
  PyObject* rv = NULL;
  uint32_t now = time_in_minutes();
  Py_ssize_t dict_len = CacheMap_Size(self);

  if (dict_len == 0) {
    PyErr_SetString(PyExc_KeyError, "CacheMap is empty.");
    return NULL;
  } else if (dict_len < CacheMap_BUCKET_SIZE) {
    while (PyDict_Next(self->dict, &pos, &key, &wrapper)) {
      weight = CacheEntry_GetWeight((CacheMapEntry*)wrapper, now);
      if (min == 0 || weight < min) {
        min = weight;
        rv = key;
      }
    }
  } else {
    PyObject* keylist = PyDict_Keys(self->dict);
    RETURN_IF_NULL(keylist, NULL);
    Py_ssize_t b_size = dict_len / CacheMap_BUCKET_NUM;
    for (int i = 0; i < CacheMap_BUCKET_NUM - 1; i++) {
      pos = i * b_size + rand_integer(b_size);
      /* fast get list item, borrowed reference */
      key = PyList_GET_ITEM(keylist, pos);
      wrapper = PyDict_GetItem(self->dict, key);
      weight = CacheEntry_GetWeight((CacheMapEntry*)wrapper, now);
      if (min == 0 || weight < min) {
        min = weight;
        rv = key;
      }
    }
    if ((dict_len % CacheMap_BUCKET_NUM)) {
      pos = CacheMap_BUCKET_NUM * b_size +
            (dict_len - CacheMap_BUCKET_NUM * b_size) / 2;
      wrapper = PyDict_GetItem(self->dict, PyList_GetItem(keylist, pos));
      weight = CacheEntry_GetWeight((CacheMapEntry*)wrapper, now);
      if (min == 0 || weight < min) {
        rv = key;
      }
    }
    Py_DECREF(keylist);
  }
  assert(rv);
  Py_INCREF(rv);
  return rv;
}
/* Always return Py_None */
static PyObject*
CacheMap_evict(CacheMap* self)
{
  PyObject* k = CacheMap_NextEvictKey(self);
  if (!k) {
    PyErr_Clear();
    Py_RETURN_NONE;
  }
  if (PyDict_DelItem(self->dict, k)) {
    return NULL;
  }
  Py_DECREF(k);
  Py_RETURN_NONE;
}

int
CacheMap_DelItem(CacheMap* self, PyObject* key)
{
  CacheMapEntry* entry = CacheMap_GetItemWithError(self, key);
  if (!entry) {
    RETURN_KEY_ERROR_IF_ERROR_NOT_SET(entry, -1);
    return -1;
  }
  if (PyDict_DelItem(self->dict, key) != 0) {
    Py_XINCREF(entry->ma_value);
    return -1;
  }
  return 0;
}

int
CacheMap_SetItem(CacheMap* self, PyObject* key, PyObject* value)
{
  PyObject* old_value;
  CacheMapEntry* entry;
  entry = CacheMap_GetItemWithError(self, key);
  RETURN_IF_ERROR_SET(-1);
  if (entry) {
    old_value = entry->ma_value;
    Py_INCREF(value);
    entry->ma_value = value;
    Py_DECREF(old_value);
    return 0;
  }
  if (CacheMap_Size(self) >= self->capacity) {
    if (!CacheMap_evict(self)) {
      return -1;
    }
  }

  entry = CacheEntry_New(value);
  RETURN_IF_NULL(entry, -1);
  CacheEntry_Init(entry);
  if (PyDict_SetItem(self->dict, key, (PyObject*)entry) != 0) {
    Py_DECREF(entry);
    return -1;
  }
  Py_DECREF(entry);
  return 0;
}

void
CacheMap_Clear(CacheMap* self)
{
  PyDict_Clear(self->dict);
  self->misses = 0;
  self->hits = 0;
}

static PyTypeObject CacheMap_Type;

static CacheMap*
CacheMap_New(void)
{
  CacheMap* self;
  self = (CacheMap*)PyObject_GC_New(CacheMap, &CacheMap_Type);
  RETURN_IF_NULL(self, NULL);
  if (!(self->dict = PyDict_New())) {
    Py_DECREF(self);
    return NULL;
  }
  PyObject_GC_Track(self);
  return self;
}

static PyObject*
CacheMap_tp_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
  return (PyObject*)CacheMap_New();
}

#define CacheMap_Init(self)                                                    \
  do {                                                                         \
    (self)->hits = 0;                                                          \
    (self)->misses = 0;                                                        \
  } while (0)

static int
CacheMap_tp_init(CacheMap* self, PyObject* args, PyObject* kwds)
{
  static char* kwlist[] = { "capacity", NULL };
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "n", kwlist, &self->capacity)) {
    return -1;
  }
  if (self->capacity <= 0) {
    PyErr_SetString(PyExc_ValueError, "Capacity should be a positive number");
    return -1;
  }
  CacheMap_Init(self);
  return 0;
}

static int
CacheMap_tp_traverse(CacheMap* self, visitproc visit, void* arg)
{
  Py_VISIT(self->dict);
  return 0;
}

static int
CacheMap_tp_clear(CacheMap* self)
{
  Py_CLEAR(self->dict);
  return 0;
}

static void
CacheMap_tp_dealloc(CacheMap* self)
{
  PyObject_GC_UnTrack(self);
  CacheMap_tp_clear(self);
  PyObject_GC_Del(self);
}

static PyObject*
CacheMap_repr(CacheMap* self)
{
  PyObject* s;
  PyObject* rv;
  s = PyObject_Repr(self->dict);
  RETURN_IF_NULL(s, NULL);
  rv = PyUnicode_FromFormat("CacheMap(%S)", s);
  if (!rv) {
    Py_DECREF(s);
    return NULL;
  }
  Py_DECREF(s);
  return rv;
}

/* mp_subscript: __getitem__() */
static PyObject*
CacheMap_mp_subscript(CacheMap* self, PyObject* key)
{
  CacheMapEntry* wrapper = CacheMap_GetItemWithError(self, key);
  if (!wrapper) {
    self->misses++;
    RETURN_IF_ERROR_SET(NULL);
    return PyErr_Format(PyExc_KeyError, "%S", key);
  }
  self->hits++;
  return CacheEntry_get_ma_value(wrapper);
}

/* mp_ass_subscript: __setitem__() and __delitem__() */
static int
CacheMap_mp_ass_sub(CacheMap* self, PyObject* key, PyObject* value)
{
  if (value == NULL) {
    return CacheMap_DelItem(self, key);
  } else {
    return CacheMap_SetItem(self, key, value);
  }
}

static PyMappingMethods CacheMap_as_mapping = {
  (lenfunc)CacheMap_size,             /*mp_length*/
  (binaryfunc)CacheMap_mp_subscript,  /*mp_subscript*/
  (objobjargproc)CacheMap_mp_ass_sub, /*mp_ass_subscript*/
};

static PyObject*
CacheMap_hints(CacheMap* self)
{
  return Py_BuildValue("iii", self->capacity, self->hits, self->misses);
}

static PyObject*
CacheMap_keys(CacheMap* self)
{
  return PyDict_Keys(self->dict);
}

static PyObject*
CacheMap_values(CacheMap* self)
{
  PyObject* values;
  CacheMapEntry* entry;
  Py_ssize_t size;
  values = PyDict_Values(self->dict);
  RETURN_IF_NULL(values, NULL);

  size = PyList_GET_SIZE(values);
  if (size == 0)
    return values;

  for (Py_ssize_t i = 0; i < size; i++) {
    entry = (CacheMapEntry*)PyList_GET_ITEM(values, i);
    PyList_SET_ITEM(values, i, CacheEntry_get_ma_value(entry));
    Py_DECREF(entry);
  }
  return values;
}

static PyObject*
CacheMap_items(CacheMap* self)
{
  CacheMapEntry* cacheentry;
  PyObject* items;
  PyObject* kv;
  Py_ssize_t size;

  items = PyDict_Items(self->dict);
  RETURN_IF_NULL(items, NULL);
  size = PyList_GET_SIZE(items);
  if (size == 0)
    return items;

  for (Py_ssize_t i = 0; i < size; i++) {
    kv = PyList_GET_ITEM(items, i);
    cacheentry = (CacheMapEntry*)PyTuple_GET_ITEM(kv, 1);
    Py_INCREF(cacheentry);
    PyTuple_SET_ITEM(kv, 1, CacheEntry_get_ma_value(cacheentry));
    Py_DECREF(cacheentry);
  }
  return items;
}

static PyObject*
CacheMap_get(CacheMap* self, PyObject* args, PyObject* kw)
{
  PyObject* key;
  PyObject* _default = NULL;
  CacheMapEntry* result;

  static char* kwlist[] = { "key", "default", NULL };
  if (!PyArg_ParseTupleAndKeywords(args, kw, "O|O", kwlist, &key, &_default))
    return NULL;
  result = CacheMap_GetItemWithError(self, key);
  if (!result) {
    RETURN_IF_ERROR_SET(NULL);
    if (!_default)
      Py_RETURN_NONE;
    Py_INCREF(_default);
    return _default;
  }
  Py_INCREF(result->ma_value);
  return result->ma_value;
}

static PyObject*
CacheMap_pop(CacheMap* self, PyObject* args, PyObject* kw)
{
  PyObject* key;
  PyObject* _default = NULL;
  CacheMapEntry* result;

  static char* kwlist[] = { "key", "default", NULL };
  if (!PyArg_ParseTupleAndKeywords(args, kw, "O|O", kwlist, &key, &_default))
    return NULL;
  result = CacheMap_GetItemWithError(self, key);
  if (!result) {
    RETURN_IF_ERROR_SET(NULL);
    if (!_default)
      Py_RETURN_NONE;
    Py_INCREF(_default);
    return _default;
  }
  Py_INCREF(result->ma_value);
  if (!CacheMap_DelItem(self, key)) {
    Py_XDECREF(result->ma_value);
    return NULL;
  }
  return result->ma_value;
}

static PyObject*
CacheMap_setdefault(CacheMap* self, PyObject* args, PyObject* kw)
{
  PyObject* key;
  PyObject* _default = NULL;
  CacheMapEntry* result;

  static char* kwlist[] = { "key", "default", NULL };
  if (!PyArg_ParseTupleAndKeywords(args, kw, "O|O", kwlist, &key, &_default))
    return NULL;
  result = CacheMap_GetItemWithError(self, key);
  if (result != NULL) {
    return CacheEntry_get_ma_value(result);
  }
  RETURN_IF_ERROR_SET(NULL);
  if (!_default)
    Py_RETURN_NONE;

  Py_INCREF(_default);
  if (CacheMap_SetItem(self, key, _default)) {
    Py_DECREF(_default);
    return NULL;
  }
  return _default;
}

static PyObject*
CacheMap_setnx(CacheMap* self, PyObject* args, PyObject* kw)
{
  PyObject* key;
  PyObject* _default;
  PyObject* callback;
  CacheMapEntry* result;

  static char* kwlist[] = { "key", "callback", NULL };
  if (!PyArg_ParseTupleAndKeywords(args, kw, "OO", kwlist, &key, &callback))
    return NULL;

  result = CacheMap_GetItemWithError(self, key);
  if (result) {
    return CacheEntry_get_ma_value(result);
  }

  _default = PyObject_CallFunction(callback, NULL);
  RETURN_IF_NULL(_default, NULL);
  if (CacheMap_SetItem(self, key, _default) != 0) {
    Py_XDECREF(_default);
    return NULL;
  }
  return _default;
}

static PyObject*
CacheMap_update(CacheMap* self, PyObject* args, PyObject* kwargs)
{
  PyObject *key, *value;
  PyObject* arg = NULL;
  Py_ssize_t pos = 0;

  if ((PyArg_ParseTuple(args, "|O", &arg))) {
    if (arg && PyDict_Check(arg)) {
      while (PyDict_Next(arg, &pos, &key, &value))
        if (CacheMap_SetItem(self, key, value))
          return NULL;
    }
  }

  if (kwargs != NULL && PyArg_ValidateKeywordArguments(kwargs)) {
    while (PyDict_Next(kwargs, &pos, &key, &value))
      if (CacheMap_SetItem(self, key, value))
        return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
CacheMap_set_capacity(CacheMap* self, PyObject* capacity)
{
  long long cap = PyLong_AsLongLong(capacity);
  if (cap <= 0) {
    if (PyErr_Occurred() == NULL) {
      PyErr_SetString(PyExc_ValueError,
                      "Capacity should be a positive integer");
    }
    return NULL;
  }
  if (cap < self->capacity && CacheMap_Size(self) > cap) {
    int r = CacheMap_Size(self) - cap;
    for (int i = 0; i < r; ++i) {
      if (CacheMap_evict(self) == NULL)
        return NULL;
    }
  }
  self->capacity = cap;
  Py_RETURN_NONE;
}

static PyObject*
CacheMap__storage(CacheMap* self)
{
  PyObject* dict = self->dict;
  Py_INCREF(dict);
  return dict;
}

static PyObject*
CacheMap_clear(CacheMap* self)
{
  CacheMap_Clear(self);
  Py_RETURN_NONE;
}

/* tp_methods */
static PyMethodDef CacheMap_methods[] = {
  { "evict", (PyCFunction)CacheMap_evict, METH_NOARGS, NULL },
  { "set_capacity", (PyCFunction)CacheMap_set_capacity, METH_O, NULL },
  { "hints", (PyCFunction)CacheMap_hints, METH_NOARGS, NULL },
  { "next_evict_key", (PyCFunction)CacheMap_NextEvictKey, METH_NOARGS, NULL },
  { "get", (PyCFunction)CacheMap_get, METH_VARARGS | METH_KEYWORDS, NULL },
  { "setdefault",
    (PyCFunction)CacheMap_setdefault,
    METH_VARARGS | METH_KEYWORDS,
    NULL },
  { "pop", (PyCFunction)CacheMap_pop, METH_VARARGS | METH_KEYWORDS, NULL },
  { "keys", (PyCFunction)CacheMap_keys, METH_NOARGS, NULL },
  { "values", (PyCFunction)CacheMap_values, METH_NOARGS, NULL },
  { "items", (PyCFunction)CacheMap_items, METH_NOARGS, NULL },
  { "update",
    (PyCFunction)CacheMap_update,
    METH_VARARGS | METH_KEYWORDS,
    NULL },
  { "clear", (PyCFunction)CacheMap_clear, METH_NOARGS, NULL },
  { "setnx", (PyCFunction)CacheMap_setnx, METH_VARARGS | METH_KEYWORDS, NULL },
  { "_storage", (PyCFunction)CacheMap__storage, METH_NOARGS, NULL },
  { NULL, NULL, 0, NULL } /* Sentinel */
};

static PyObject*
CacheMap_tp_iter(CacheMap* self)
{
  PyObject *keys, *it;
  keys = CacheMap_keys(self);
  if (keys == NULL)
    return NULL;
  it = PySeqIter_New(keys);
  if (it == NULL)
    return NULL;
  assert(keys);
  Py_DECREF(keys);
  return it;
}

PyDoc_STRVAR(CacheMap__doc__, "A fast CacheMap behaving much like dict.");

static PyTypeObject CacheMap_Type = {
  /* clang-format off */
  PyVarObject_HEAD_INIT(NULL, 0)
  /* clang-format on */
  "CacheMap",                              /* tp_name */
  sizeof(CacheMap),                        /* tp_basicsize */
  0,                                       /* tp_itemsize */
  (destructor)CacheMap_tp_dealloc,         /* tp_dealloc */
  0,                                       /* tp_print */
  0,                                       /* tp_getattr */
  0,                                       /* tp_setattr */
  0,                                       /* tp_compare */
  (reprfunc)CacheMap_repr,                 /* tp_repr */
  0,                                       /* tp_as_number */
  &CacheMap_as_sequence,                   /* tp_as_sequence */
  &CacheMap_as_mapping,                    /* tp_as_mapping */
  PyObject_HashNotImplemented,             /* tp_hash */
  0,                                       /* tp_call */
  0,                                       /* tp_str */
  0,                                       /* tp_getattro */
  0,                                       /* tp_setattro */
  0,                                       /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, /* tp_flags */
  CacheMap__doc__,                         /* tp_doc */
  (traverseproc)CacheMap_tp_traverse,      /* tp_traverse */
  (inquiry)CacheMap_tp_clear,              /* tp_clear */
  0,                                       /* tp_richcompare */
  0,                                       /* tp_weaklistoffset */
  (getiterfunc)CacheMap_tp_iter,           /* tp_iter */
  0,                                       /* tp_iternext */
  CacheMap_methods,                        /* tp_methods */
  0,                                       /* tp_members */
  0,                                       /* tp_getset */
  0,                                       /* tp_base */
  0,                                       /* tp_dict */
  0,                                       /* tp_descr_get */
  0,                                       /* tp_descr_set */
  0,                                       /* tp_dictoffset */
  (initproc)CacheMap_tp_init,              /* tp_init */
  0,                                       /* tp_alloc */
  (newfunc)CacheMap_tp_new,                /* tp_new */
};

static struct PyModuleDef _ctools_cachemap_module = {
  PyModuleDef_HEAD_INIT,
  "_ctools_cachemap", /* m_name */
  NULL,               /* m_doc */
  -1,                 /* m_size */
  NULL,               /* m_methods */
  NULL,               /* m_reload */
  NULL,               /* m_traverse */
  NULL,               /* m_clear */
  NULL,               /* m_free */
};

PyMODINIT_FUNC
PyInit__ctools_cachemap(void)
{
  if (PyType_Ready(&CacheMap_Type) < 0)
    return NULL;

  if (PyType_Ready(&CacheEntry_Type) < 0)
    return NULL;

  PyObject* m = PyModule_Create(&_ctools_cachemap_module);
  if (m == NULL)
    return NULL;

  Py_INCREF(&CacheEntry_Type);
  Py_INCREF(&CacheMap_Type);

  PyModule_AddObject(m, "CacheMap", (PyObject*)&CacheMap_Type);
  PyModule_AddObject(m, "CacheMapEntry", (PyObject*)&CacheEntry_Type);

  return m;
}

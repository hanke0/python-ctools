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

#define DEFAULT_TTL 60

#define NOW() ((int64_t)time(NULL))

typedef struct
{
  /* clang-format off */
  PyObject_HEAD
  PyObject *ma_value;
  /* clang-format on */
  int64_t expire;
} TTLCacheEntry;

static PyTypeObject TTLCacheEntry_Type;

static TTLCacheEntry*
TTLCacheEntry_New(PyObject* ma_value, int64_t ttl)
{
  TTLCacheEntry* self;
  assert(ma_value);
  assert(ttl > 0);
  self = (TTLCacheEntry*)PyObject_GC_New(TTLCacheEntry, &TTLCacheEntry_Type);
  RETURN_IF_NULL(self, NULL);
  self->ma_value = ma_value;
  self->expire = NOW() + ttl;
  Py_INCREF(ma_value);
  PyObject_GC_Track(self);
  return self;
}

static PyObject*
TTLCacheEntry_tp_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
  PyObject* ma_value;
  int64_t ttl;
  static char* kwlist[] = { "obj", "ttl", NULL };
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "OL", kwlist, &ma_value, &ttl))
    return NULL;
  if (ttl < 0) {
    ttl = DEFAULT_TTL;
  }
  return (PyObject*)TTLCacheEntry_New(ma_value, ttl);
}

static int
TTLCacheEntry_tp_traverse(TTLCacheEntry* self, visitproc visit, void* arg)
{
  Py_VISIT(self->ma_value);
  return 0;
}

static int
TTLCacheEntry_tp_clear(TTLCacheEntry* self)
{
  Py_CLEAR(self->ma_value);
  return 0;
}

static void
TTLCacheEntry_tp_dealloc(TTLCacheEntry* self)
{
  PyObject_GC_UnTrack(self);
  TTLCacheEntry_tp_clear(self);
  PyObject_GC_Del(self);
}

static PyObject*
TTLCacheEntry_get_ma_value(TTLCacheEntry* self)
{
  PyObject* ma_value = self->ma_value;
  Py_INCREF(ma_value);
  return ma_value;
}

static PyMethodDef TTLCacheEntry_methods[] = {
  { "get_value", (PyCFunction)TTLCacheEntry_get_ma_value, METH_NOARGS, NULL },
  { NULL, NULL, 0, NULL } /* Sentinel */
};

static PyObject*
TTLCacheEntry_repr(TTLCacheEntry* self)
{
  return PyObject_Repr(self->ma_value);
}

static PyTypeObject TTLCacheEntry_Type = {
  /* clang-format off */
    PyVarObject_HEAD_INIT(NULL, 0)
  /* clang-format on */
  "TTLCacheEntry",                         /* tp_name */
  sizeof(TTLCacheEntry),                   /* tp_basicsize */
  0,                                       /* tp_itemsize */
  (destructor)TTLCacheEntry_tp_dealloc,    /* tp_dealloc */
  0,                                       /* tp_print */
  0,                                       /* tp_getattr */
  0,                                       /* tp_setattr */
  0,                                       /* tp_compare */
  (reprfunc)TTLCacheEntry_repr,            /* tp_repr */
  0,                                       /* tp_as_number */
  0,                                       /* tp_as_sequence */
  0,                                       /* tp_as_mapping */
  PyObject_HashNotImplemented,             /* tp_hash */
  0,                                       /* tp_call */
  0,                                       /* tp_str */
  0,                                       /* tp_getattro */
  0,                                       /* tp_setattro */
  0,                                       /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, /* tp_flags */
  NULL,                                    /* tp_doc */
  (traverseproc)TTLCacheEntry_tp_traverse, /* tp_traverse */
  (inquiry)TTLCacheEntry_tp_clear,         /* tp_clear */
  0,                                       /* tp_richcompare */
  0,                                       /* tp_weaklistoffset */
  0,                                       /* tp_iter */
  0,                                       /* tp_iternext */
  TTLCacheEntry_methods,                   /* tp_methods */
  0,                                       /* tp_members */
  0,                                       /* tp_getset */
  0,                                       /* tp_base */
  0,                                       /* tp_dict */
  0,                                       /* tp_descr_get */
  0,                                       /* tp_descr_set */
  0,                                       /* tp_dictoffset */
  0,                                       /* tp_init */
  0,                                       /* tp_alloc */
  (newfunc)TTLCacheEntry_tp_new,           /* tp_new */
};
/* TTLCacheEntry Type Define */

typedef struct
{
  /* clang-format off */
  PyObject_HEAD
  PyObject *dict;
  /* clang-format on */
  int64_t default_ttl;
} TTLCache;

#define TTLCache_Size(self) (PyDict_Size(((TTLCache*)(self))->dict))

/* borrowed reference. KeyError will not be set.*/
#define TTLCache_GetItemWithError(self, key)                                   \
  ((TTLCacheEntry*)PyDict_GetItemWithError(((TTLCache*)(self))->dict,          \
                                           (PyObject*)(key)))

/* KeyError would be set if key not in cache */
static int
TTLCache_DelItem(TTLCache* self, PyObject* key)
{
  TTLCacheEntry* entry = TTLCache_GetItemWithError(self, key);
  if (!entry) {
    RETURN_KEY_ERROR_IF_ERROR_NOT_SET(key, -1);
    return -1;
  }
  if (PyDict_DelItem(self->dict, key) != 0) {
    return -1;
  }
  return 0;
}

/* borrowed reference.*/
static TTLCacheEntry*
TTLCache_GetTTLItemWithError(TTLCache* self, PyObject* key)
{
  assert(key);
  TTLCacheEntry* entry;
  int i;
  int64_t t;
  entry = TTLCache_GetItemWithError(self, key);
  RETURN_IF_NULL(entry, NULL);

  t = NOW();
  if (entry->expire < t) {
    /* key is already in cache, error would not raised */
    i = TTLCache_DelItem(self, key);
    assert(i != -1);
    return NULL;
  }
  return entry;
}

static int
TTLCache_SetItem(TTLCache* self, PyObject* key, PyObject* value)
{
  TTLCacheEntry* entry;
  entry = TTLCache_GetItemWithError(self, key);
  if (entry) {
    Py_DECREF(entry->ma_value);
    Py_INCREF(value);
    entry->ma_value = value;
    entry->expire = NOW() + self->default_ttl;
    return 0;
  }

  RETURN_IF_ERROR_SET(-1);
  entry = TTLCacheEntry_New(value, self->default_ttl);
  if (!entry) {
    return -1;
  }
  if (PyDict_SetItem(self->dict, key, (PyObject*)entry)) {
    Py_DECREF(entry);
    return -1;
  }
  Py_DECREF(entry);
  return 0;
}

static void
TTLCache_Clear(TTLCache* self)
{
  PyDict_Clear(self->dict);
}

static Py_ssize_t
TTLCache_get_size(TTLCache* self)
{
  return TTLCache_Size(self);
}

static PyTypeObject TTLCache_Type;

static TTLCache*
TTLCache_New(int64_t ttl)
{
  TTLCache* self;
  assert(ttl > 0);
  self = (TTLCache*)PyObject_GC_New(TTLCache, &TTLCache_Type);
  RETURN_IF_NULL(self, NULL);

  if (!(self->dict = PyDict_New())) {
    Py_DECREF(self);
    return NULL;
  }
  self->default_ttl = ttl;
  PyObject_GC_Track(self);
  return self;
}

static PyObject*
TTLCache_tp_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
  int64_t ttl = 0;
  static char* kwlist[] = { "ttl", NULL };
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "|L", kwlist, &ttl))
    return NULL;
  if (ttl <= 0) {
    ttl = DEFAULT_TTL;
  }
  return (PyObject*)TTLCache_New(ttl);
}

static int
TTLCache_tp_traverse(TTLCache* self, visitproc visit, void* arg)
{
  Py_VISIT(self->dict);
  return 0;
}

static int
TTLCache_tp_clear(TTLCache* self)
{
  Py_CLEAR(self->dict);
  return 0;
}

static void
TTLCache_tp_dealloc(TTLCache* self)
{
  PyObject_GC_UnTrack(self);
  TTLCache_tp_clear(self);
  PyObject_GC_Del(self);
}

static PyObject*
TTLCache_repr(TTLCache* self)
{
  PyObject* s;
  PyObject* rv;
  s = PyObject_Repr(self->dict);
  if (!s) {
    return NULL;
  }
  rv = PyUnicode_FromFormat("TTLCache(%S)", s);
  if (!rv) {
    return NULL;
  }
  Py_DECREF(s);
  return rv;
}

/* mp_subscript: __getitem__() */
static PyObject*
TTLCache_mp_subscript(TTLCache* self, PyObject* key)
{
  TTLCacheEntry* wrapper = TTLCache_GetTTLItemWithError(self, key);
  if (!wrapper) {
    RETURN_KEY_ERROR_IF_ERROR_NOT_SET(key, NULL);
    return NULL;
  }
  return TTLCacheEntry_get_ma_value(wrapper);
}

/* mp_ass_subscript: __setitem__() and __delitem__() */
static int
TTLCache_mp_ass_sub(TTLCache* self, PyObject* key, PyObject* value)
{
  if (value == NULL) {
    return TTLCache_DelItem(self, key);
  } else {
    return TTLCache_SetItem(self, key, value);
  }
}

static PyMappingMethods TTLCache_as_mapping = {
  (lenfunc)TTLCache_get_size,         /*mp_length*/
  (binaryfunc)TTLCache_mp_subscript,  /*mp_subscript*/
  (objobjargproc)TTLCache_mp_ass_sub, /*mp_ass_subscript*/
};

static int
TTLCache_Contains(PyObject* self, PyObject* key)
{
  TTLCacheEntry* entry;

  entry = TTLCache_GetTTLItemWithError((TTLCache*)self, key);
  if (!entry) {
    RETURN_IF_ERROR_SET(-1);
    return 0;
  }
  return 1;
}

/* Hack to implement "key in dict" */
static PySequenceMethods TTLCache_as_sequence = {
  0,                 /* sq_length */
  0,                 /* sq_concat */
  0,                 /* sq_repeat */
  0,                 /* sq_item */
  0,                 /* sq_slice */
  0,                 /* sq_ass_item */
  0,                 /* sq_ass_slice */
  TTLCache_Contains, /* sq_contains */
  0,                 /* sq_inplace_concat */
  0,                 /* sq_inplace_repeat */
};

static PyObject*
TTLCache_keys(TTLCache* self)
{
  return PyDict_Keys(self->dict);
}

static PyObject*
TTLCache_values(TTLCache* self)
{
  PyObject* values = PyDict_Values(self->dict);
  RETURN_IF_NULL(values, NULL);

  Py_ssize_t size = PyList_GET_SIZE(values);
  if (size == 0)
    return values;

  TTLCacheEntry* entry;
  for (Py_ssize_t i = 0; i < size; i++) {
    entry = (TTLCacheEntry*)PyList_GET_ITEM(values, i);
    PyList_SET_ITEM(values, i, TTLCacheEntry_get_ma_value(entry));
    Py_DECREF(entry);
  }
  return values;
}

static PyObject*
TTLCache_items(TTLCache* self)
{
  TTLCacheEntry* entry;
  PyObject* items = PyDict_Items(self->dict);
  PyObject* kv;
  if (!items) {
    return NULL;
  }
  Py_ssize_t size = PyList_GET_SIZE(items);
  if (size == 0)
    return items;

  for (Py_ssize_t i = 0; i < size; i++) {
    kv = PyList_GET_ITEM(items, i);
    entry = (TTLCacheEntry*)PyTuple_GET_ITEM(kv, 1);
    Py_INCREF(entry);
    PyTuple_SET_ITEM(kv, 1, TTLCacheEntry_get_ma_value(entry));
    Py_DECREF(entry);
  }
  return items;
}

static PyObject*
TTLCache_get(TTLCache* self, PyObject* args, PyObject* kw)
{
  PyObject* key;
  PyObject* _default = NULL;
  TTLCacheEntry* result;

  static char* kwlist[] = { "key", "default", NULL };
  if (!PyArg_ParseTupleAndKeywords(args, kw, "O|O", kwlist, &key, &_default))
    return NULL;
  result = TTLCache_GetTTLItemWithError((TTLCache*)self, key);
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
TTLCache_pop(TTLCache* self, PyObject* args, PyObject* kw)
{
  PyObject* key;
  PyObject* _default = NULL;
  TTLCacheEntry* result;

  static char* kwlist[] = { "key", "default", NULL };
  if (!PyArg_ParseTupleAndKeywords(args, kw, "O|O", kwlist, &key, &_default))
    return NULL;
  result = TTLCache_GetTTLItemWithError((TTLCache*)self, key);
  if (!result) {
    RETURN_IF_ERROR_SET(NULL);
    if (!_default)
      Py_RETURN_NONE;
    Py_INCREF(_default);
    return _default;
  }
  Py_INCREF(result->ma_value);
  if (!TTLCache_DelItem(self, key)) {
    Py_XDECREF(result->ma_value);
    return NULL;
  }
  return result->ma_value;
}

static PyObject*
TTLCache_setdefault(TTLCache* self, PyObject* args, PyObject* kw)
{
  PyObject* key;
  PyObject* _default = NULL;
  TTLCacheEntry* result;

  static char* kwlist[] = { "key", "default", NULL };
  if (!PyArg_ParseTupleAndKeywords(args, kw, "O|O", kwlist, &key, &_default))
    return NULL;
  result = TTLCache_GetTTLItemWithError((TTLCache*)self, key);
  if (result) {
    return TTLCacheEntry_get_ma_value(result);
  }
  RETURN_IF_ERROR_SET(NULL);
  if (!_default)
    _default = Py_None;
  Py_INCREF(_default);
  if (TTLCache_SetItem(self, key, _default)) {
    Py_DECREF(_default);
    return NULL;
  }
  return _default;
}

static PyObject*
TTLCache_setnx(TTLCache* self, PyObject* args, PyObject* kw)
{
  PyObject* key;
  PyObject *_default = NULL, *callback = NULL;
  TTLCacheEntry* result;

  static char* kwlist[] = { "key", "callback", NULL };
  if (!PyArg_ParseTupleAndKeywords(args, kw, "OO", kwlist, &key, &callback))
    return NULL;

  if (callback == NULL || !PyCallable_Check(callback)) {
    PyErr_SetString(PyExc_TypeError, "callback is not callable.");
    return NULL;
  }

  result = TTLCache_GetTTLItemWithError((TTLCache*)self, key);
  if (result) {
    return TTLCacheEntry_get_ma_value(result);
  }
  RETURN_IF_ERROR_SET(NULL);
  _default = PyObject_CallFunction(callback, NULL);
  RETURN_IF_NULL(_default, NULL);
  if (TTLCache_SetItem(self, key, _default)) {
    Py_XDECREF(_default);
    return NULL;
  }
  return _default;
}

static PyObject*
TTLCache_update(TTLCache* self, PyObject* args, PyObject* kwargs)
{
  PyObject *key, *value;
  PyObject* arg = NULL;
  Py_ssize_t pos = 0;

  if ((PyArg_ParseTuple(args, "|O", &arg))) {
    if (arg && PyDict_Check(arg)) {
      while (PyDict_Next(arg, &pos, &key, &value))
        if (TTLCache_SetItem(self, key, value))
          return NULL;
    }
  }

  if (kwargs != NULL && PyArg_ValidateKeywordArguments(kwargs)) {
    while (PyDict_Next(kwargs, &pos, &key, &value))
      if (TTLCache_SetItem(self, key, value))
        return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
TTLCache_set_default_ttl(TTLCache* self, PyObject* ttl)
{
  int64_t cap = PyLong_AsLong(ttl);
  if (cap <= 0) {
    PyObject* err = PyErr_Occurred();
    if (err == NULL) {
      PyErr_SetString(PyExc_ValueError, "ttl should be a positive integer");
    }
    return NULL;
  }

  self->default_ttl = cap;
  Py_RETURN_NONE;
}

static PyObject*
TTLCache_get_default_ttl(TTLCache* self)
{
  return PyLong_FromLongLong(self->default_ttl);
}

static PyObject*
TTLCache__storage(TTLCache* self)
{
  PyObject* dict = self->dict;
  Py_INCREF(dict);
  return dict;
}

static PyObject*
TTLCache_clear(TTLCache* self)
{
  TTLCache_Clear(self);
  Py_RETURN_NONE;
}

/* tp_methods */
static PyMethodDef TTLCache_methods[] = {
  { "set_default_ttl", (PyCFunction)TTLCache_set_default_ttl, METH_O, NULL },
  { "get", (PyCFunction)TTLCache_get, METH_VARARGS | METH_KEYWORDS, NULL },
  { "setdefault",
    (PyCFunction)TTLCache_setdefault,
    METH_VARARGS | METH_KEYWORDS,
    NULL },
  { "pop", (PyCFunction)TTLCache_pop, METH_VARARGS | METH_KEYWORDS, NULL },
  { "keys", (PyCFunction)TTLCache_keys, METH_NOARGS, NULL },
  { "values", (PyCFunction)TTLCache_values, METH_NOARGS, NULL },
  { "items", (PyCFunction)TTLCache_items, METH_NOARGS, NULL },
  { "update",
    (PyCFunction)TTLCache_update,
    METH_VARARGS | METH_KEYWORDS,
    NULL },
  { "clear", (PyCFunction)TTLCache_clear, METH_NOARGS, NULL },
  { "setnx", (PyCFunction)TTLCache_setnx, METH_VARARGS | METH_KEYWORDS, NULL },
  { "_storage", (PyCFunction)TTLCache__storage, METH_NOARGS, NULL },
  { "get_default_ttl",
    (PyCFunction)TTLCache_get_default_ttl,
    METH_NOARGS,
    NULL },
  { NULL, NULL, 0, NULL } /* Sentinel */
};

static PyObject*
TTLCache_tp_iter(TTLCache* self)
{
  PyObject *keys, *it;
  keys = TTLCache_keys(self);
  RETURN_IF_NULL(keys, NULL);
  it = PySeqIter_New(keys);
  if (it == NULL) {
    Py_DECREF(keys);
    return NULL;
  }

  assert(keys);
  Py_DECREF(keys);
  return it;
}

PyDoc_STRVAR(TTLCache__doc__, "A fast TTLCache behaving much like dict.");

static PyTypeObject TTLCache_Type = {
  /* clang-format off */
    PyVarObject_HEAD_INIT(NULL, 0)
  /* clang-format on */
  "TTLCache",                              /* tp_name */
  sizeof(TTLCache),                        /* tp_basicsize */
  0,                                       /* tp_itemsize */
  (destructor)TTLCache_tp_dealloc,         /* tp_dealloc */
  0,                                       /* tp_print */
  0,                                       /* tp_getattr */
  0,                                       /* tp_setattr */
  0,                                       /* tp_compare */
  (reprfunc)TTLCache_repr,                 /* tp_repr */
  0,                                       /* tp_as_number */
  &TTLCache_as_sequence,                   /* tp_as_sequence */
  &TTLCache_as_mapping,                    /* tp_as_mapping */
  0,                                       /* tp_hash */
  0,                                       /* tp_call */
  0,                                       /* tp_str */
  0,                                       /* tp_getattro */
  0,                                       /* tp_setattro */
  0,                                       /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, /* tp_flags */
  TTLCache__doc__,                         /* tp_doc */
  (traverseproc)TTLCache_tp_traverse,      /* tp_traverse */
  (inquiry)TTLCache_tp_clear,              /* tp_clear */
  0,                                       /* tp_richcompare */
  0,                                       /* tp_weaklistoffset */
  (getiterfunc)TTLCache_tp_iter,           /* tp_iter */
  0,                                       /* tp_iternext */
  TTLCache_methods,                        /* tp_methods */
  0,                                       /* tp_members */
  0,                                       /* tp_getset */
  0,                                       /* tp_base */
  0,                                       /* tp_dict */
  0,                                       /* tp_descr_get */
  0,                                       /* tp_descr_set */
  0,                                       /* tp_dictoffset */
  0,                                       /* tp_init */
  0,                                       /* tp_alloc */
  (newfunc)TTLCache_tp_new,                /* tp_new */
};

static struct PyModuleDef _ctools_TTLCache_module = {
  PyModuleDef_HEAD_INIT,
  "_ctools_ttlcache", /* m_name */
  NULL,               /* m_doc */
  -1,                 /* m_size */
  NULL,               /* m_methods */
  NULL,               /* m_reload */
  NULL,               /* m_traverse */
  NULL,               /* m_clear */
  NULL,               /* m_free */
};

PyMODINIT_FUNC
PyInit__ctools_ttlcache(void)
{
  if (PyType_Ready(&TTLCache_Type) < 0)
    return NULL;

  if (PyType_Ready(&TTLCacheEntry_Type) < 0)
    return NULL;

  PyObject* m = PyModule_Create(&_ctools_TTLCache_module);
  if (m == NULL)
    return NULL;

  Py_INCREF(&TTLCacheEntry_Type);
  Py_INCREF(&TTLCache_Type);

  PyModule_AddObject(m, "TTLCache", (PyObject*)&TTLCache_Type);
  PyModule_AddObject(m, "TTLCacheEntry", (PyObject*)&TTLCacheEntry_Type);

  return m;
}

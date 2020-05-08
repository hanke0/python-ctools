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
#include "pydoc.h"

#include <Python.h>
#include <time.h>

#define DEFAULT_TTL 60

#define NOW() ((int64_t)time(NULL))

/* clang-format off */
typedef struct {
  PyObject_HEAD
  PyObject *ma_value;
  int64_t expire;
} CtsTTLCacheEntry;
/* clang-format on */

static PyTypeObject TTLCacheEntry_Type;

static CtsTTLCacheEntry *TTLCacheEntry_New(PyObject *ma_value, int64_t ttl) {
  CtsTTLCacheEntry *self;
  assert(ma_value);
  assert(ttl > 0);
  self = (CtsTTLCacheEntry *)PyObject_GC_New(CtsTTLCacheEntry,
                                             &TTLCacheEntry_Type);
  ReturnIfNULL(self, NULL);
  self->ma_value = ma_value;
  self->expire = NOW() + ttl;
  Py_INCREF(ma_value);
  PyObject_GC_Track(self);
  return self;
}

static PyObject *TTLCacheEntry_tp_new(PyTypeObject *type, PyObject *args,
                                      PyObject *kwds) {
  PyObject *ma_value;
  int64_t ttl;
  static char *kwlist[] = {"obj", "ttl", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "OL", kwlist, &ma_value, &ttl))
    return NULL;
  if (ttl < 0) {
    ttl = DEFAULT_TTL;
  }
  return (PyObject *)TTLCacheEntry_New(ma_value, ttl);
}

static int TTLCacheEntry_tp_traverse(CtsTTLCacheEntry *self, visitproc visit,
                                     void *arg) {
  Py_VISIT(self->ma_value);
  return 0;
}

static int TTLCacheEntry_tp_clear(CtsTTLCacheEntry *self) {
  Py_CLEAR(self->ma_value);
  return 0;
}

static void TTLCacheEntry_tp_dealloc(CtsTTLCacheEntry *self) {
  PyObject_GC_UnTrack(self);
  TTLCacheEntry_tp_clear(self);
  PyObject_GC_Del(self);
}

static PyObject *TTLCacheEntry_get_ma_value(CtsTTLCacheEntry *self) {
  PyObject *ma_value = self->ma_value;
  Py_INCREF(ma_value);
  return ma_value;
}

static PyMethodDef TTLCacheEntry_methods[] = {
    {"get_value", (PyCFunction)TTLCacheEntry_get_ma_value, METH_NOARGS, NULL},
    {NULL, NULL, 0, NULL} /* Sentinel */
};

static PyObject *TTLCacheEntry_repr(CtsTTLCacheEntry *self) {
  return PyObject_Repr(self->ma_value);
}

static PyTypeObject TTLCacheEntry_Type = {
    /* clang-format off */
    PyVarObject_HEAD_INIT(NULL, 0)
    /* clang-format on */
    "ctools.TTLCacheEntry",                  /* tp_name */
    sizeof(CtsTTLCacheEntry),                /* tp_basicsize */
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
/* CtsTTLCacheEntry Type Define */

/* clang-format off */
typedef struct {
  PyObject_HEAD
  PyObject *dict;
  int64_t default_ttl;
} CtsTTLCache;
/* clang-format on */

#define TTLCache_Size(self) (PyDict_Size(((CtsTTLCache *)(self))->dict))

/* borrowed reference. KeyError will not be set.*/
#define TTLCache_GetItemWithError(self, key)                                   \
  ((CtsTTLCacheEntry *)PyDict_GetItemWithError(((CtsTTLCache *)(self))->dict,  \
                                               (PyObject *)(key)))

/* KeyError would be set if key not in cache */
static int TTLCache_DelItem(CtsTTLCache *self, PyObject *key) {
  return PyDict_DelItem(self->dict, key);
}

/* borrowed reference.*/
static CtsTTLCacheEntry *TTLCache_GetTTLItemWithError(CtsTTLCache *self,
                                                      PyObject *key) {
  assert(key);
  CtsTTLCacheEntry *entry;
  int i;
  int64_t t;
  entry = TTLCache_GetItemWithError(self, key);
  ReturnIfNULL(entry, NULL);

  t = NOW();
  if (entry->expire < t) {
    /* key is already in cache, error would not raised */
    i = TTLCache_DelItem(self, key);
    assert(i == 0);
    if (i != 0) {
      abort();
    }
    return NULL;
  }
  return entry;
}

static int TTLCache_SetItem(CtsTTLCache *self, PyObject *key, PyObject *value) {
  CtsTTLCacheEntry *entry;
  entry = TTLCache_GetItemWithError(self, key);
  if (entry) {
    Py_DECREF(entry->ma_value);
    Py_INCREF(value);
    entry->ma_value = value;
    entry->expire = NOW() + self->default_ttl;
    return 0;
  }

  ReturnIfErrorSet(-1);
  entry = TTLCacheEntry_New(value, self->default_ttl);
  if (!entry) {
    return -1;
  }
  if (PyDict_SetItem(self->dict, key, (PyObject *)entry)) {
    Py_DECREF(entry);
    return -1;
  }
  Py_DECREF(entry);
  return 0;
}

static void TTLCache_Clear(CtsTTLCache *self) { PyDict_Clear(self->dict); }

static Py_ssize_t TTLCache_get_size(CtsTTLCache *self) {
  return TTLCache_Size(self);
}

static PyTypeObject TTLCache_Type;

static CtsTTLCache *TTLCache_New(int64_t ttl) {
  CtsTTLCache *self;
  assert(ttl > 0);
  self = (CtsTTLCache *)PyObject_GC_New(CtsTTLCache, &TTLCache_Type);
  ReturnIfNULL(self, NULL);

  if (!(self->dict = PyDict_New())) {
    Py_DECREF(self);
    return NULL;
  }
  self->default_ttl = ttl;
  PyObject_GC_Track(self);
  return self;
}

static PyObject *TTLCache_tp_new(PyTypeObject *Py_UNUSED(type), PyObject *args,
                                 PyObject *Py_UNUSED(kwds)) {
  int64_t ttl = DEFAULT_TTL;
  if (!PyArg_ParseTuple(args, "|L", &ttl))
    return NULL;
  if (ttl <= 0) {
    PyErr_SetString(PyExc_ValueError,
                    "ttl should be a positive integer in seconds.");
    return NULL;
  }
  return (PyObject *)TTLCache_New(ttl);
}

static int TTLCache_tp_traverse(CtsTTLCache *self, visitproc visit, void *arg) {
  Py_VISIT(self->dict);
  return 0;
}

static int TTLCache_tp_clear(CtsTTLCache *self) {
  Py_CLEAR(self->dict);
  return 0;
}

static void TTLCache_tp_dealloc(CtsTTLCache *self) {
  PyObject_GC_UnTrack(self);
  TTLCache_tp_clear(self);
  PyObject_GC_Del(self);
}

static PyObject *TTLCache_repr(CtsTTLCache *self) {
  PyObject *s;
  PyObject *rv;
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
static PyObject *TTLCache_mp_subscript(CtsTTLCache *self, PyObject *key) {
  CtsTTLCacheEntry *wrapper = TTLCache_GetTTLItemWithError(self, key);
  if (!wrapper) {
    ReturnKeyErrorIfErrorNotSet(key, NULL);
    return NULL;
  }
  return TTLCacheEntry_get_ma_value(wrapper);
}

/* mp_ass_subscript: __setitem__() and __delitem__() */
static int TTLCache_mp_ass_sub(CtsTTLCache *self, PyObject *key,
                               PyObject *value) {
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

static int TTLCache_Contains(PyObject *self, PyObject *key) {
  CtsTTLCacheEntry *entry;

  entry = TTLCache_GetTTLItemWithError((CtsTTLCache *)self, key);
  if (!entry) {
    ReturnIfErrorSet(-1);
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

static PyObject *TTLCache_keys(CtsTTLCache *self) {
  return PyDict_Keys(self->dict);
}

static PyObject *TTLCache_values(CtsTTLCache *self) {
  PyObject *values = PyDict_Values(self->dict);
  ReturnIfNULL(values, NULL);

  Py_ssize_t size = PyList_GET_SIZE(values);
  if (size == 0) {
    return values;
  }

  CtsTTLCacheEntry *entry;
  for (Py_ssize_t i = 0; i < size; i++) {
    entry = (CtsTTLCacheEntry *)PyList_GET_ITEM(values, i);
    PyList_SET_ITEM(values, i, TTLCacheEntry_get_ma_value(entry));
    Py_DECREF(entry);
  }
  return values;
}

static PyObject *TTLCache_items(CtsTTLCache *self) {
  CtsTTLCacheEntry *entry;
  PyObject *items = PyDict_Items(self->dict);
  PyObject *kv;
  if (!items) {
    return NULL;
  }
  Py_ssize_t size = PyList_GET_SIZE(items);
  if (size == 0) {
    return items;
  }

  for (Py_ssize_t i = 0; i < size; i++) {
    kv = PyList_GET_ITEM(items, i);
    entry = (CtsTTLCacheEntry *)PyTuple_GET_ITEM(kv, 1);
    Py_INCREF(entry);
    PyTuple_SET_ITEM(kv, 1, TTLCacheEntry_get_ma_value(entry));
    Py_DECREF(entry);
  }
  return items;
}

static PyObject *TTLCache_get(CtsTTLCache *self, PyObject *args, PyObject *kw) {
  PyObject *key;
  PyObject *_default = NULL;
  CtsTTLCacheEntry *result;

  static char *kwlist[] = {"key", "default", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kw, "O|O", kwlist, &key, &_default))
    return NULL;
  result = TTLCache_GetTTLItemWithError((CtsTTLCache *)self, key);
  if (!result) {
    ReturnIfErrorSet(NULL);
    if (!_default) {
      Py_RETURN_NONE;
    }
    Py_INCREF(_default);
    return _default;
  }
  Py_INCREF(result->ma_value);
  return result->ma_value;
}

static PyObject *TTLCache_pop(CtsTTLCache *self, PyObject *args, PyObject *kw) {
  PyObject *key;
  PyObject *_default = NULL;
  CtsTTLCacheEntry *result;

  static char *kwlist[] = {"key", "default", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kw, "O|O", kwlist, &key, &_default))
    return NULL;
  result = TTLCache_GetTTLItemWithError((CtsTTLCache *)self, key);
  if (!result) {
    ReturnIfErrorSet(NULL);
    if (!_default) {
      Py_RETURN_NONE;
    }
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

static PyObject *TTLCache_popitem(CtsTTLCache *self,
                                  PyObject *Py_UNUSED(args)) {
  PyObject *keys, *key, *value, *tuple;
  CtsTTLCacheEntry *value_entry;
  Py_ssize_t size;
  int err;
  keys = PyDict_Keys(self->dict);
  ReturnIfNULL(keys, NULL);
  size = PyList_Size(keys);
  if (size < 0) {
    Py_DECREF(keys);
    return NULL;
  }
  if (size == 0) {
    PyErr_SetString(PyExc_KeyError, "popitem(): mapping is empty");
    return NULL;
  }
  key = PyList_GetItem(keys, 0);
  Py_DECREF(keys);
  /* keys is not usable start from here */
  if (key == NULL) {
    return NULL;
  }
  value_entry = TTLCache_GetItemWithError(self, key);
  if (value_entry == NULL) {
    if (!PyErr_Occurred()) {
      PyErr_Format(PyExc_KeyError, "%S", key);
    }
    return NULL;
  }
  Py_INCREF(key);
  value = TTLCacheEntry_get_ma_value(value_entry);
  tuple = PyTuple_New(2);
  if (!tuple) {
    Py_DECREF(key);
    Py_DECREF(value);
    return NULL;
  }
  err = PyTuple_SetItem(tuple, 0, key);
  if (err) {
    Py_DECREF(key);
    Py_DECREF(value);
    Py_DECREF(tuple);
    return NULL;
  }
  err = PyTuple_SetItem(tuple, 1, value);
  if (err) {
    Py_DECREF(value);
    Py_DECREF(tuple);
    return NULL;
  }
  TTLCache_DelItem(self, key);
  PyErr_Clear(); /* Don't care del error */
  return tuple;
}

static PyObject *TTLCache_setdefault(CtsTTLCache *self, PyObject *args,
                                     PyObject *kw) {
  PyObject *key;
  PyObject *_default = NULL;
  CtsTTLCacheEntry *result;

  static char *kwlist[] = {"key", "default", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kw, "O|O", kwlist, &key, &_default))
    return NULL;
  result = TTLCache_GetTTLItemWithError((CtsTTLCache *)self, key);
  if (result) {
    return TTLCacheEntry_get_ma_value(result);
  }
  ReturnIfErrorSet(NULL);
  if (!_default) {
    _default = Py_None;
  }
  Py_INCREF(_default);
  if (TTLCache_SetItem(self, key, _default)) {
    Py_DECREF(_default);
    return NULL;
  }
  return _default;
}

static PyObject *TTLCache_setnx(CtsTTLCache *self, PyObject *args,
                                PyObject *kw) {
  PyObject *key;
  PyObject *_default = NULL, *callback = NULL;
  CtsTTLCacheEntry *result;

  static char *kwlist[] = {"key", "fn", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kw, "OO", kwlist, &key, &callback))
    return NULL;

  if (callback == NULL || !PyCallable_Check(callback)) {
    PyErr_SetString(PyExc_TypeError, "callback is not callable.");
    return NULL;
  }

  result = TTLCache_GetTTLItemWithError((CtsTTLCache *)self, key);
  if (result) {
    return TTLCacheEntry_get_ma_value(result);
  }
  ReturnIfErrorSet(NULL);
  _default = PyObject_CallFunctionObjArgs(callback, key, NULL);
  ReturnIfNULL(_default, NULL);
  if (TTLCache_SetItem(self, key, _default)) {
    Py_XDECREF(_default);
    return NULL;
  }
  return _default;
}

static PyObject *TTLCache_update(CtsTTLCache *self, PyObject *args,
                                 PyObject *kwargs) {
  PyObject *key, *value;
  PyObject *arg = NULL;
  Py_ssize_t pos = 0;

  if ((PyArg_ParseTuple(args, "|O", &arg))) {
    if (arg && PyDict_Check(arg)) {
      while (PyDict_Next(arg, &pos, &key, &value))
        if (TTLCache_SetItem(self, key, value)) {
          return NULL;
        }
    }
  }

  if (kwargs != NULL && PyArg_ValidateKeywordArguments(kwargs)) {
    while (PyDict_Next(kwargs, &pos, &key, &value))
      if (TTLCache_SetItem(self, key, value)) {
        return NULL;
      }
  }

  Py_RETURN_NONE;
}

static PyObject *TTLCache_set_default_ttl(CtsTTLCache *self, PyObject *ttl) {
  int64_t cap = PyLong_AsLong(ttl);
  if (cap <= 0) {
    PyObject *err = PyErr_Occurred();
    if (err == NULL) {
      PyErr_SetString(PyExc_ValueError, "ttl should be a positive integer");
    }
    return NULL;
  }

  self->default_ttl = cap;
  Py_RETURN_NONE;
}

static PyObject *TTLCache_get_default_ttl(CtsTTLCache *self) {
  return PyLong_FromLongLong(self->default_ttl);
}

static PyObject *TTLCache__storage(CtsTTLCache *self) {
  PyObject *dict = self->dict;
  Py_INCREF(dict);
  return dict;
}

static PyObject *TTLCache_clear(CtsTTLCache *self) {
  TTLCache_Clear(self);
  Py_RETURN_NONE;
}

/* tp_methods */
static PyMethodDef TTLCache_methods[] = {
    {
        "set_default_ttl",
        (PyCFunction)TTLCache_set_default_ttl,
        METH_O,
        "set_default_ttl(ttl, /)\n--\n\n"
        "Reset default ttl.\n\n"
        "Parameters\n"
        "----------\n"
        "ttl : int\n"
        "  Expire seconds.\n\n"
        "Notes\n"
        "-----\n"
        "Exist keys won't change their expire.\n",
    },
    {
        "get_default_ttl",
        (PyCFunction)TTLCache_get_default_ttl,
        METH_NOARGS,
        "get_default_ttl()\n--\n\nReturn default ttl.",
    },
    {
        "get",
        (PyCFunction)TTLCache_get,
        METH_VARARGS | METH_KEYWORDS,
        "get(key, default=None, /)\n--\n\nGet item from cache.",
    },
    {
        "setdefault",
        (PyCFunction)TTLCache_setdefault,
        METH_VARARGS | METH_KEYWORDS,
        "setdefault(key, default=None, /)\n--\n\n"
        "Get item in cache, if key not "
        "exists, set default to cache and return it.",
    },
    {
        "pop",
        (PyCFunction)TTLCache_pop,
        METH_VARARGS | METH_KEYWORDS,
        "pop(key, default=None)\n--\n\nPop item from cache.",
    },
    {
        "popitem",
        (PyCFunction)TTLCache_popitem,
        METH_NOARGS,
        "popitem()\n--\n\nRemove and return some (key, value) pair"
        "as a 2-tuple; but raise KeyError if mapping is empty.",
    },
    {
        "keys",
        (PyCFunction)TTLCache_keys,
        METH_NOARGS,
        "keys()\n--\n\nIter keys.",
    },
    {
        "values",
        (PyCFunction)TTLCache_values,
        METH_NOARGS,
        "values()\n--\n\nIter values.",
    },
    {
        "items",
        (PyCFunction)TTLCache_items,
        METH_NOARGS,
        "items()\n--\n\nIter items.",
    },
    {
        "update",
        (PyCFunction)TTLCache_update,
        METH_VARARGS | METH_KEYWORDS,
        "update(mp, **kwargs)\n--\n\n"
        "Update item to cache. Unlike dict.update, only accept a dict object.",
    },
    {"clear", (PyCFunction)TTLCache_clear, METH_NOARGS,
     "clear()\n--\n\nClear cache."},
    {
        "setnx",
        (PyCFunction)TTLCache_setnx,
        METH_VARARGS | METH_KEYWORDS,
        USUAL_SETNX_METHOD_DOC,
    },
    {"_storage", (PyCFunction)TTLCache__storage, METH_NOARGS, NULL},
    {NULL, NULL, 0, NULL} /* Sentinel */
};

static PyObject *TTLCache_tp_iter(CtsTTLCache *self) {
  PyObject *keys, *it;
  keys = TTLCache_keys(self);
  ReturnIfNULL(keys, NULL);
  it = PySeqIter_New(keys);
  Py_DECREF(keys);
  return it;
}

static PyObject *TTLCache_tp_richcompare(PyObject *self, PyObject *other,
                                         int opid) {
  switch (opid) {
  case Py_EQ:
    if (self == other) {
      Py_RETURN_TRUE;
    } else {
      Py_RETURN_FALSE;
    }
  default:
    Py_RETURN_FALSE;
  }
}

PyDoc_STRVAR(
    TTLCache__doc__,
    "TTLCache(ttl=None)\n--\n\n"
    "A mapping that keys expire and unreachable after ``ttl`` seconds.\n"
    "\n"
    "Parameters\n"
    "----------\n"
    "ttl : int, optional\n"
    "  Key will expire after this many seconds, default is 60 (1 minute).\n"
    "\n"
    "Examples\n"
    "--------\n"
    ">>> import ctools\n"
    ">>> import time\n"
    ">>> cache = ctools.TTLCache(5)\n"
    ">>> cache['foo'] = 'bar'\n"
    ">>> cache['foo']\n"
    "'bar'\n"
    ">>> time.sleep(5)\n"
    ">>> 'foo' in cache\n"
    "False\n");

static PyTypeObject TTLCache_Type = {
    /* clang-format off */
    PyVarObject_HEAD_INIT(NULL, 0)
    /* clang-format on */
    "ctools.TTLCache",                       /* tp_name */
    sizeof(CtsTTLCache),                     /* tp_basicsize */
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
    (richcmpfunc)TTLCache_tp_richcompare,    /* tp_richcompare */
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

EXTERN_C_START
int ctools_init_ttlcache(PyObject *module) {
  if (PyType_Ready(&TTLCacheEntry_Type) < 0) {
    return -1;
  }

  if (PyType_Ready(&TTLCache_Type) < 0) {
    return -1;
  }
  Py_INCREF(&TTLCacheEntry_Type);
  Py_INCREF(&TTLCache_Type);
  if (PyModule_AddObject(module, "TTLCacheEntry",
                         (PyObject *)&TTLCacheEntry_Type)) {
    goto FAIL;
  }
  if (PyModule_AddObject(module, "TTLCache", (PyObject *)&TTLCache_Type)) {
    goto FAIL;
  }

  return 0;
FAIL:
  Py_DECREF(&TTLCacheEntry_Type);
  Py_DECREF(&TTLCache_Type);
  return -1;
}
EXTERN_C_END
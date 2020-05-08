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

#define CacheEntry_DEFAULT_VISITS 255U
#define CacheEntry_DECREASE_EVERY_MINUTES 10

#define CacheMap_BUCKET_NUM 8
#define CacheMap_BUCKET_SIZE 256

static inline unsigned int time_in_minutes(void) {
  return (unsigned int)(((uint64_t)time(NULL) / 60) & UINT32_MAX);
}

typedef struct {
  /* clang-format off */
  PyObject_HEAD
  PyObject *ma_value;
  /* clang-format on */
  uint32_t last_visit;
  uint32_t visits;
} CtsCacheMapEntry;

static PyTypeObject CacheEntry_Type;

static CtsCacheMapEntry *CacheEntry_New(PyObject *ma_value) {
  CtsCacheMapEntry *self;
  assert(ma_value);
  self =
      (CtsCacheMapEntry *)PyObject_GC_New(CtsCacheMapEntry, &CacheEntry_Type);
  ReturnIfNULL(self, NULL);
  self->ma_value = ma_value;
  Py_INCREF(ma_value);
  PyObject_GC_Track(self);
  return self;
}

static PyObject *CacheEntry_new(PyTypeObject *Py_UNUSED(type), PyObject *args,
                                PyObject *kwds) {
  PyObject *ma_value;
  static char *kwlist[] = {"obj", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &ma_value)) {
    return NULL;
  }
  return (PyObject *)CacheEntry_New(ma_value);
}

#define CacheEntry_Init(self)                                                  \
  do {                                                                         \
    (self)->last_visit = time_in_minutes();                                    \
    (self)->visits = CacheEntry_DEFAULT_VISITS;                                \
  } while (0)

static int CacheEntry_init(CtsCacheMapEntry *self, PyObject *Py_UNUSED(unused1),
                           PyObject *Py_UNUSED(unused2)) {
  CacheEntry_Init(self);
  return 0;
}

static int CacheEntry_tp_traverse(CtsCacheMapEntry *self, visitproc visit,
                                  void *arg) {
  Py_VISIT(self->ma_value);
  return 0;
}

static int CacheEntry_tp_clear(CtsCacheMapEntry *self) {
  Py_CLEAR(self->ma_value);
  return 0;
}

static void CacheEntry_tp_dealloc(CtsCacheMapEntry *self) {
  PyObject_GC_UnTrack(self);
  CacheEntry_tp_clear(self);
  PyObject_GC_Del(self);
}

#define CacheEntry_NewVisit(self)                                              \
  do {                                                                         \
    self->visits++;                                                            \
    self->last_visit = time_in_minutes();                                      \
  } while (0)

static PyObject *CacheEntry_get_ma_value(CtsCacheMapEntry *self) {
  CacheEntry_NewVisit(self);
  PyObject *ma_value = self->ma_value;
  Py_INCREF(ma_value);
  return ma_value;
}

static inline unsigned int CacheEntry_GetWeight(CtsCacheMapEntry *self,
                                                unsigned int now) {
  register unsigned int num, counter;
  counter = self->visits;
  num = (now - self->last_visit) / CacheEntry_DECREASE_EVERY_MINUTES;
  return counter > num ? counter - num : 0;
}

static PyObject *CacheEntry_get_weight(CtsCacheMapEntry *self) {
  return Py_BuildValue("I", CacheEntry_GetWeight(self, time_in_minutes()));
}

static PyMethodDef CacheEntry_methods[] = {
    {"get_value", (PyCFunction)CacheEntry_get_ma_value, METH_NOARGS, NULL},
    {"get_weight", (PyCFunction)CacheEntry_get_weight, METH_NOARGS, NULL},
    {NULL, NULL, 0, NULL} /* Sentinel */
};

static PyObject *CacheEntry_repr(CtsCacheMapEntry *self) {
  return PyObject_Repr(self->ma_value);
}

static PyTypeObject CacheEntry_Type = {
    /* clang-format off */
    PyVarObject_HEAD_INIT(NULL, 0)
    "ctools.CacheMapEntry",                  /* tp_name */
    /* clang-format on */
    sizeof(CtsCacheMapEntry),                /* tp_basicsize */
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
/* CtsCacheMapEntry Type Define */

typedef struct {
  /* clang-format off */
  PyObject_HEAD
  PyObject *dict;
  /* clang-format on */
  Py_ssize_t capacity;
  Py_ssize_t hits;
  Py_ssize_t misses;
} CtsCacheMap;

#define CacheMap_Size(self) (PyDict_Size(((CtsCacheMap *)(self))->dict))

static Py_ssize_t CacheMap_size(CtsCacheMap *self) {
  return CacheMap_Size(self);
}

/* return a random number between 0 and limit inclusive. */
static int rand_integer(int limit) {
  int divisor = RAND_MAX / (limit + 1);
  int rv;

  do {
    rv = rand() / divisor;
  } while (rv > limit);

  return rv;
}

static int CacheMap_Contains(PyObject *self, PyObject *key) {
  return PyDict_Contains(((CtsCacheMap *)self)->dict, key);
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
  ((CtsCacheMapEntry *)PyDict_GetItemWithError(((CtsCacheMap *)self)->dict,    \
                                               (PyObject *)(key)))

/* New Reference */
static PyObject *CacheMap_NextEvictKey(CtsCacheMap *self) {
  PyObject *key = NULL, *wrapper = NULL;
  Py_ssize_t pos = 0;
  uint32_t min = 0, weight;
  PyObject *rv = NULL;
  uint32_t now = time_in_minutes();
  Py_ssize_t dict_len = CacheMap_Size(self);

  if (dict_len == 0) {
    PyErr_SetString(PyExc_KeyError, "CacheMap is empty.");
    return NULL;
  } else if (dict_len < CacheMap_BUCKET_SIZE) {
    while (PyDict_Next(self->dict, &pos, &key, &wrapper)) {
      weight = CacheEntry_GetWeight((CtsCacheMapEntry *)wrapper, now);
      if (min == 0 || weight < min) {
        min = weight;
        rv = key;
      }
    }
  } else {
    PyObject *keylist = PyDict_Keys(self->dict);
    ReturnIfNULL(keylist, NULL);
    Py_ssize_t b_size = dict_len / CacheMap_BUCKET_NUM;
    for (int i = 0; i < CacheMap_BUCKET_NUM - 1; i++) {
      pos = i * b_size + rand_integer(b_size);
      /* fast get list item, borrowed reference */
      key = PyList_GET_ITEM(keylist, pos);
      wrapper = PyDict_GetItem(self->dict, key);
      weight = CacheEntry_GetWeight((CtsCacheMapEntry *)wrapper, now);
      if (min == 0 || weight < min) {
        min = weight;
        rv = key;
      }
    }
    if ((dict_len % CacheMap_BUCKET_NUM)) {
      pos = CacheMap_BUCKET_NUM * b_size +
            (dict_len - CacheMap_BUCKET_NUM * b_size) / 2;
      wrapper = PyDict_GetItem(self->dict, PyList_GET_ITEM(keylist, pos));
      weight = CacheEntry_GetWeight((CtsCacheMapEntry *)wrapper, now);
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
static PyObject *CacheMap_evict(CtsCacheMap *self) {
  PyObject *k = CacheMap_NextEvictKey(self);
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

static int CacheMap_DelItem(CtsCacheMap *self, PyObject *key) {
  return PyDict_DelItem(self->dict, key);
}

static int CacheMap_SetItem(CtsCacheMap *self, PyObject *key, PyObject *value) {
  PyObject *old_value;
  CtsCacheMapEntry *entry;
  entry = CacheMap_GetItemWithError(self, key);
  ReturnIfErrorSet(-1);
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
  ReturnIfNULL(entry, -1);
  CacheEntry_Init(entry);
  if (PyDict_SetItem(self->dict, key, (PyObject *)entry) != 0) {
    Py_DECREF(entry);
    return -1;
  }
  Py_DECREF(entry);
  return 0;
}

static void CacheMap_Clear(CtsCacheMap *self) {
  PyDict_Clear(self->dict);
  self->misses = 0;
  self->hits = 0;
}

static PyTypeObject CacheMap_Type;

static CtsCacheMap *CacheMap_New(void) {
  CtsCacheMap *self;
  self = (CtsCacheMap *)PyObject_GC_New(CtsCacheMap, &CacheMap_Type);
  ReturnIfNULL(self, NULL);
  if (!(self->dict = PyDict_New())) {
    Py_DECREF(self);
    return NULL;
  }
  PyObject_GC_Track(self);
  self->hits = 0;
  self->misses = 0;
  self->capacity = INT32_MAX;
  return self;
}

static PyObject *CacheMap_tp_new(PyTypeObject *Py_UNUSED(type),
                                 PyObject *Py_UNUSED(args),
                                 PyObject *Py_UNUSED(kwds)) {
  return (PyObject *)CacheMap_New();
}

static int CacheMap_tp_init(CtsCacheMap *self, PyObject *args,
                            PyObject *Py_UNUSED(kwds)) {
  Py_ssize_t capacity = 0;
  if (!PyArg_ParseTuple(args, "|n", &capacity)) {
    return -1;
  }
  if (capacity < 0) {
    PyErr_SetString(PyExc_ValueError, "Capacity should be a positive number");
    return -1;
  }
  if (capacity > 0) {
    self->capacity = capacity;
  }
  return 0;
}

static int CacheMap_tp_traverse(CtsCacheMap *self, visitproc visit, void *arg) {
  Py_VISIT(self->dict);
  return 0;
}

static int CacheMap_tp_clear(CtsCacheMap *self) {
  Py_CLEAR(self->dict);
  return 0;
}

static void CacheMap_tp_dealloc(CtsCacheMap *self) {
  PyObject_GC_UnTrack(self);
  CacheMap_tp_clear(self);
  PyObject_GC_Del(self);
}

static PyObject *CacheMap_repr(CtsCacheMap *self) {
  PyObject *s;
  PyObject *rv;
  s = PyObject_Repr(self->dict);
  ReturnIfNULL(s, NULL);
  rv = PyUnicode_FromFormat("CacheMap(%S)", s);
  if (!rv) {
    Py_DECREF(s);
    return NULL;
  }
  Py_DECREF(s);
  return rv;
}

/* mp_subscript: __getitem__() */
static PyObject *CacheMap_mp_subscript(CtsCacheMap *self, PyObject *key) {
  CtsCacheMapEntry *wrapper = CacheMap_GetItemWithError(self, key);
  if (!wrapper) {
    self->misses++;
    ReturnIfErrorSet(NULL);
    return PyErr_Format(PyExc_KeyError, "%S", key);
  }
  self->hits++;
  return CacheEntry_get_ma_value(wrapper);
}

/* mp_ass_subscript: __setitem__() and __delitem__() */
static int CacheMap_mp_ass_sub(CtsCacheMap *self, PyObject *key,
                               PyObject *value) {
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

static PyObject *CacheMap_hit_info(CtsCacheMap *self) {
  return Py_BuildValue("iii", self->capacity, self->hits, self->misses);
}

static PyObject *CacheMap_keys(CtsCacheMap *self) {
  return PyDict_Keys(self->dict);
}

static PyObject *CacheMap_values(CtsCacheMap *self) {
  PyObject *values;
  CtsCacheMapEntry *entry;
  Py_ssize_t size;
  values = PyDict_Values(self->dict);
  ReturnIfNULL(values, NULL);

  size = PyList_GET_SIZE(values);
  if (size == 0) {
    return values;
  }

  for (Py_ssize_t i = 0; i < size; i++) {
    entry = (CtsCacheMapEntry *)PyList_GET_ITEM(values, i);
    PyList_SET_ITEM(values, i, CacheEntry_get_ma_value(entry));
    Py_DECREF(entry);
  }
  return values;
}

static PyObject *CacheMap_items(CtsCacheMap *self) {
  CtsCacheMapEntry *cacheentry;
  PyObject *items;
  PyObject *kv;
  Py_ssize_t size;

  items = PyDict_Items(self->dict);
  ReturnIfNULL(items, NULL);
  size = PyList_GET_SIZE(items);
  if (size == 0) {
    return items;
  }

  for (Py_ssize_t i = 0; i < size; i++) {
    kv = PyList_GET_ITEM(items, i);
    cacheentry = (CtsCacheMapEntry *)PyTuple_GET_ITEM(kv, 1);
    Py_INCREF(cacheentry);
    PyTuple_SET_ITEM(kv, 1, CacheEntry_get_ma_value(cacheentry));
    Py_DECREF(cacheentry);
  }
  return items;
}

static PyObject *CacheMap_get(CtsCacheMap *self, PyObject *args, PyObject *kw) {
  PyObject *key;
  PyObject *_default = NULL;
  CtsCacheMapEntry *result;

  static char *kwlist[] = {"key", "default", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kw, "O|O", kwlist, &key, &_default))
    return NULL;
  result = CacheMap_GetItemWithError(self, key);
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

static PyObject *CacheMap_pop(CtsCacheMap *self, PyObject *args, PyObject *kw) {
  PyObject *key;
  PyObject *_default = NULL;
  CtsCacheMapEntry *result;

  static char *kwlist[] = {"key", "default", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kw, "O|O", kwlist, &key, &_default)) {
    return NULL;
  }
  result = CacheMap_GetItemWithError(self, key);
  if (!result) {
    ReturnIfErrorSet(NULL);
    if (!_default) {
      Py_RETURN_NONE;
    }
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

static PyObject *CacheMap_popitem(CtsCacheMap *self,
                                  PyObject *Py_UNUSED(args)) {
  PyObject *keys, *key, *value, *tuple;
  CtsCacheMapEntry *value_entry;
  Py_ssize_t size;
  int err;
  keys = CacheMap_keys(self);
  ReturnIfNULL(keys, NULL);
  size = PyList_Size(keys);
  if (size < 0) {
    Py_DECREF(keys);
    return NULL;
  }
  if (size == 0) {
    PyErr_SetString(PyExc_KeyError, "popitem(): cache map is empty");
    return NULL;
  }
  key = PyList_GetItem(keys, 0);
  Py_DECREF(keys);
  /* keys is not usable start from here */
  if (key == NULL) {
    return NULL;
  }
  value_entry = CacheMap_GetItemWithError(self, key);
  if (value_entry == NULL) {
    if (!PyErr_Occurred()) {
      PyErr_Format(PyExc_KeyError, "%S", key);
    }
    return NULL;
  }
  Py_INCREF(key);
  value = CacheEntry_get_ma_value(value_entry);
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
  CacheMap_DelItem(self, key);
  PyErr_Clear(); /* Don't care del error */
  return tuple;
}

static PyObject *CacheMap_setdefault(CtsCacheMap *self, PyObject *args,
                                     PyObject *kw) {
  PyObject *key;
  PyObject *_default = NULL;
  CtsCacheMapEntry *result;

  static char *kwlist[] = {"key", "default", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kw, "O|O", kwlist, &key, &_default))
    return NULL;
  result = CacheMap_GetItemWithError(self, key);
  if (result != NULL) {
    return CacheEntry_get_ma_value(result);
  }
  ReturnIfErrorSet(NULL);
  if (!_default) {
    Py_RETURN_NONE;
  }

  Py_INCREF(_default);
  if (CacheMap_SetItem(self, key, _default)) {
    Py_DECREF(_default);
    return NULL;
  }
  return _default;
}

static PyObject *CacheMap_setnx(CtsCacheMap *self, PyObject *args,
                                PyObject *kw) {
  PyObject *key;
  PyObject *_default;
  PyObject *callback;
  CtsCacheMapEntry *result;

  static char *kwlist[] = {"key", "fn", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kw, "OO", kwlist, &key, &callback)) {
    return NULL;
  }

  result = CacheMap_GetItemWithError(self, key);
  if (result) {
    return CacheEntry_get_ma_value(result);
  }

  _default = PyObject_CallFunctionObjArgs(callback, key, NULL);
  ReturnIfNULL(_default, NULL);
  if (CacheMap_SetItem(self, key, _default) != 0) {
    Py_XDECREF(_default);
    return NULL;
  }
  return _default;
}

static PyObject *CacheMap_update(CtsCacheMap *self, PyObject *args,
                                 PyObject *kwargs) {
  PyObject *key, *value;
  PyObject *arg = NULL;
  Py_ssize_t pos = 0;

  if ((PyArg_ParseTuple(args, "|O", &arg))) {
    if (arg && PyDict_Check(arg)) {
      while (PyDict_Next(arg, &pos, &key, &value))
        if (CacheMap_SetItem(self, key, value)) {
          return NULL;
        }
    }
  }

  if (kwargs != NULL && PyArg_ValidateKeywordArguments(kwargs)) {
    while (PyDict_Next(kwargs, &pos, &key, &value))
      if (CacheMap_SetItem(self, key, value)) {
        return NULL;
      }
  }

  Py_RETURN_NONE;
}

static PyObject *CacheMap_set_capacity(CtsCacheMap *self, PyObject *capacity) {
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
      ReturnIfNULL(CacheMap_evict(self), NULL);
    }
  }
  self->capacity = cap;
  Py_RETURN_NONE;
}

static PyObject *CacheMap__storage(CtsCacheMap *self) {
  PyObject *dict = self->dict;
  Py_INCREF(dict);
  return dict;
}

static PyObject *CacheMap_clear(CtsCacheMap *self) {
  CacheMap_Clear(self);
  Py_RETURN_NONE;
}

/* tp_methods */
static PyMethodDef CacheMap_methods[] = {
    {"evict", (PyCFunction)CacheMap_evict, METH_NOARGS,
     "evict()\n--\n\nEvict a item. raise error if no item in cache."},
    {
        "set_capacity",
        (PyCFunction)CacheMap_set_capacity,
        METH_O,
        "set_capacity(capacity, /)\n--\n\n Reset capacity of cache.",
    },
    {"hit_info", (PyCFunction)CacheMap_hit_info, METH_NOARGS,
     "hit_info()\n--\n\nReturn capacity, hits, and misses count."},
    {"next_evict_key", (PyCFunction)CacheMap_NextEvictKey, METH_NOARGS,
     "next_evict_key()\n--\n\nReturn the most unused key."},
    {"get", (PyCFunction)CacheMap_get, METH_VARARGS | METH_KEYWORDS,
     "get(key, default=None)\n--\n\nGet item from cache."},
    {"setdefault", (PyCFunction)CacheMap_setdefault,
     METH_VARARGS | METH_KEYWORDS,
     "setdefault(key, default=None, /)\n--\n\nGet item in cache, if key not "
     "exists, set default to cache and return it."},
    {"pop", (PyCFunction)CacheMap_pop, METH_VARARGS | METH_KEYWORDS,
     "pop(key, default=None, /)\n--\n\nPop an item from cache, if key not "
     "exists return default."},
    {
        "popitem",
        (PyCFunction)CacheMap_popitem,
        METH_NOARGS,
        "popitem()\n--\n\nRemove and return some (key, value) pair"
        "as a 2-tuple; but raise KeyError if mapping is empty.",
    },
    {"keys", (PyCFunction)CacheMap_keys, METH_NOARGS,
     "keys()\n--\n\nIter keys."},
    {"values", (PyCFunction)CacheMap_values, METH_NOARGS,
     "values()\n--\n\nIter values."},
    {"items", (PyCFunction)CacheMap_items, METH_NOARGS,
     "items()\n--\n\nIter keys and values."},
    {"update", (PyCFunction)CacheMap_update, METH_VARARGS | METH_KEYWORDS,
     "update(map, /)\n--\n\nUpdate item to cache. Unlike dict.update, only "
     "accept a dict object."},
    {
        "clear",
        (PyCFunction)CacheMap_clear,
        METH_NOARGS,
        "clear()\n--\n\nClean cache.",
    },
    {
        "setnx",
        (PyCFunction)CacheMap_setnx,
        METH_VARARGS | METH_KEYWORDS,
        USUAL_SETNX_METHOD_DOC,
    },
    {"_storage", (PyCFunction)CacheMap__storage, METH_NOARGS, NULL},
    {NULL, NULL, 0, NULL} /* Sentinel */
};

static PyObject *CacheMap_tp_iter(CtsCacheMap *self) {
  PyObject *keys, *it;
  keys = CacheMap_keys(self);
  ReturnIfNULL(keys, NULL);
  it = PySeqIter_New(keys);
  Py_DECREF(keys);
  return it;
}

static PyObject *CacheMap_tp_richcompare(PyObject *self, PyObject *other,
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

PyDoc_STRVAR(CacheMap__doc__,
             "CacheMap(capacity=None, )\n--\n\n"
             "A fast LFU (least frequently used) mapping.\n"
             "\n"
             "Parameters\n"
             "----------\n"
             "capacity : int, optional\n"
             "  Max size of cache, default is  C ``INT32_MAX``.\n"
             "\n"
             "Examples\n"
             "--------\n"
             ">>> import ctools\n"
             ">>> cache = ctools.CacheMap(1)\n"
             ">>> cache['foo'] = 'bar'\n"
             ">>> cache['foo']\n"
             "'bar'\n"
             ">>> cache['bar'] = 'foo'\n"
             ">>> 'foo' in cache\n"
             "False\n");

static PyTypeObject CacheMap_Type = {
    /* clang-format off */
    PyVarObject_HEAD_INIT(NULL, 0)
    /* clang-format on */
    "ctools.CacheMap",                       /* tp_name */
    sizeof(CtsCacheMap),                     /* tp_basicsize */
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
    (richcmpfunc)CacheMap_tp_richcompare,    /* tp_richcompare */
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

EXTERN_C_START
int ctools_init_cachemap(PyObject *module) {
  if (PyType_Ready(&CacheMap_Type) < 0) {
    return -1;
  }

  if (PyType_Ready(&CacheEntry_Type) < 0) {
    return -1;
  }
  Py_INCREF(&CacheEntry_Type);
  Py_INCREF(&CacheMap_Type);
  if (PyModule_AddObject(module, "CacheMap", PyObjectCast(&CacheMap_Type))) {
    goto FAIL;
  }
  if (PyModule_AddObject(module, "CacheMapEntry",
                         PyObjectCast(&CacheEntry_Type))) {
    goto FAIL;
  }

  return 0;
FAIL:
  Py_DECREF(&CacheEntry_Type);
  Py_DECREF(&CacheMap_Type);
  return -1;
}

EXTERN_C_END

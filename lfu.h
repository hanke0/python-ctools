#ifndef _CTOOLS_LFU_H
#define _CTOOLS_LFU_H

#include <Python.h>
#include <stdlib.h>
#include <structmember.h>
#include <time.h>

#define LFU_INIT_VAL 5U
#define LFU_LOG_FACTOR 10U
#define LFU_DECAY_FACTOR 3U // minutes

#define LFU_BUCKET 8
#define LFU_BUCKET_SIZE 256

#define LFUWrapper_LastVisit(self) ((self)->lfu >> 8U)
#define LFUWrapper_Counter(self) ((self)->lfu & 255U)

/*
 * COPY FROM redis
 * 概率计算公式 1 / ((counter - LFU_INIT_VAL) * LFU_LOG_FACTOR + 1)
 */
uint32_t
lfu_log_incr(uint32_t counter)
{
    if (counter == 255)
        return 255;
    double r = (double)rand() / RAND_MAX;
    double baseval = counter - LFU_INIT_VAL;
    if (baseval < 0)
        baseval = 0;
    double p = 1.0 / (baseval * LFU_LOG_FACTOR + 1);
    if (r < p)
        counter++;
    return counter;
}

static inline uint32_t
time_in_minutes(void)
{
    return (uint32_t)(((uint64_t)time(NULL) / 60) & 16777215UL);
}

typedef struct _LFUValue {
    PyObject_HEAD
        PyObject* wrapped;
    uint32_t lfu; /* last_visit in minutes & counter, 24 bit + 8 bit */
} LFUWrapper;

static PyObject*
LFUWrapper_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
    LFUWrapper* self;
    PyObject* wrapped;
    static char* kwlist[] = { "obj", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &wrapped))
        return NULL;
    assert(wrapped);
    self = (LFUWrapper*)PyObject_GC_New(LFUWrapper, type);
    if (!self)
        return NULL;
    PyObject_GC_Track(self);
    self->wrapped = wrapped;
    Py_INCREF(wrapped);
    return (PyObject*)self;
}

static int
LFUWrapper_init(LFUWrapper* self, PyObject* args, PyObject* kwds)
{
    self->lfu = ((uint32_t)((time_in_minutes() & 16777215UL) << 8U)) | LFU_INIT_VAL;
    return 0;
}

static int
LFUWrapper_tp_traverse(LFUWrapper* self, visitproc visit, void* arg)
{
    Py_VISIT(self->wrapped);
    return 0;
}

static int
LFUWrapper_tp_clear(LFUWrapper* self)
{
    Py_CLEAR(self->wrapped);
    return 0;
}

static void
LFUWrapper_tp_dealloc(LFUWrapper* self)
{
    PyObject_GC_UnTrack(self);
    LFUWrapper_tp_clear(self);
    PyObject_GC_Del(self);
}

static PyObject*
LFUWrapper_cast_info(LFUWrapper* self)
{
    return Py_BuildValue("ii", LFUWrapper_LastVisit(self), LFUWrapper_LastVisit(self));
}

static PyObject*
LFUWrapper_wrapped(LFUWrapper* self)
{
    uint32_t last_visit = LFUWrapper_LastVisit(self);
    uint32_t counter = LFUWrapper_Counter(self);
    uint32_t t = time_in_minutes();
    uint32_t num = (t - last_visit) / LFU_DECAY_FACTOR;
    counter = lfu_log_incr(num > counter ? 0 : counter - num);
    self->lfu = (t << 8U) | counter;
    PyObject* wrapped = self->wrapped;
    Py_INCREF(wrapped);
    return wrapped;
}

static inline uint32_t
LFUWrapper_WEIGHT(LFUWrapper* self, uint32_t now)
{
    register int32_t num, counter;
    counter = LFUWrapper_Counter(self);
    num = (now - LFUWrapper_LastVisit(self)) / LFU_DECAY_FACTOR;
    return num > counter ? 0 : counter - num;
}

static PyObject*
LFUWrapper_weight(LFUWrapper* self)
{
    return Py_BuildValue("I", LFUWrapper_WEIGHT(self, time_in_minutes()));
}

static PyMethodDef LFUWrapper_methods[] = {
    { "cash_info", (PyCFunction)LFUWrapper_cast_info, METH_NOARGS, NULL },
    { "wrapped", (PyCFunction)LFUWrapper_wrapped, METH_NOARGS, NULL },
    { "weight", (PyCFunction)LFUWrapper_weight, METH_NOARGS, NULL },
    { NULL, NULL, 0, NULL } /* Sentinel */
};

static PyObject*
LFUWrapper_repr(LFUWrapper* self)
{
    return PyObject_Repr(self->wrapped);
}

PyTypeObject LFUWrapperType = {
    PyVarObject_HEAD_INIT(NULL, 0) "cools.LFUWrapper", /* tp_name */
    sizeof(LFUWrapper), /* tp_basicsize */
    0, /* tp_itemsize */
    (destructor)LFUWrapper_tp_dealloc, /* tp_dealloc */
    0, /* tp_print */
    0, /* tp_getattr */
    0, /* tp_setattr */
    0, /* tp_compare */
    (reprfunc)LFUWrapper_repr, /* tp_repr */
    0, /* tp_as_number */
    0, /* tp_as_sequence */
    0, /* tp_as_mapping */
    0, /* tp_hash */
    0, /* tp_call */
    0, /* tp_str */
    0, /* tp_getattro */
    0, /* tp_setattro */
    0, /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /* tp_flags */
    "LFU value store", /* tp_doc */
    (traverseproc)LFUWrapper_tp_traverse, /* tp_traverse */
    (inquiry)LFUWrapper_tp_clear, /* tp_clear */
    0, /* tp_richcompare */
    0, /* tp_weaklistoffset */
    0, /* tp_iter */
    0, /* tp_iternext */
    LFUWrapper_methods, /* tp_methods */
    0, /* tp_members */
    0, /* tp_getset */
    0, /* tp_base */
    0, /* tp_dict */
    0, /* tp_descr_get */
    0, /* tp_descr_set */
    0, /* tp_dictoffset */
    (initproc)LFUWrapper_init, /* tp_init */
    0, /* tp_alloc */
    (newfunc)LFUWrapper_new, /* tp_new */
};
/* LFUWrapper Type Define */

typedef struct {
    PyObject_HEAD
        PyObject* dict;
    Py_ssize_t capacity;
    Py_ssize_t hits;
    Py_ssize_t misses;
} LFUCache;

Py_ssize_t
PyLFUCache_Size(LFUCache* self)
{
    return PyDict_Size(self->dict);
}

/* return a random number between 0 and limit inclusive. */
int rand_limit(int limit)
{
    int divisor = RAND_MAX / (limit + 1);
    int rv;

    do {
        rv = rand() / divisor;
    } while (rv > limit);

    return rv;
}

static int
LFUCache_Contains(PyObject* self, PyObject* key)
{
    return PyDict_Contains(((LFUCache*)self)->dict, key);
}

/* Hack to implement "key in dict" */
static PySequenceMethods LFUCache_as_sequence = {
    0, /* sq_length */
    0, /* sq_concat */
    0, /* sq_repeat */
    0, /* sq_item */
    0, /* sq_slice */
    0, /* sq_ass_item */
    0, /* sq_ass_slice */
    LFUCache_Contains, /* sq_contains */
    0, /* sq_inplace_concat */
    0, /* sq_inplace_repeat */
};

#define PyLFUCache_GetItem(self, key) (LFUWrapper*)PyDict_GetItem(((LFUCache*)self)->dict, key)

static PyObject*
LFUCache_lfu(LFUCache* self)
{
    PyObject *key = NULL, *wrapper = NULL;
    Py_ssize_t pos = 0;
    uint32_t min = 0, weight;
    PyObject* rv = NULL;
    uint32_t now = time_in_minutes();
    Py_ssize_t dict_len = PyLFUCache_Size(self);

    if (dict_len == 0) {
        PyErr_SetString(PyExc_KeyError, "No key in dict");
        return NULL;
    } else if (dict_len < LFU_BUCKET_SIZE) {
        while (PyDict_Next(self->dict, &pos, &key, &wrapper)) {
            weight = LFUWrapper_WEIGHT((LFUWrapper*)wrapper, now);
            if (min == 0 || weight < min) {
                min = weight;
                rv = key;
            }
        }
    } else {
        PyObject* keylist = PyDict_Keys(self->dict);
        Py_ssize_t b_size = dict_len / LFU_BUCKET;
        for (int i = 0; i < LFU_BUCKET - 1; i++) {
            pos = i * b_size + rand_limit(b_size);
            key = PyList_GET_ITEM(keylist, pos);
            wrapper = PyDict_GetItem(self->dict, key);
            weight = LFUWrapper_WEIGHT((LFUWrapper*)wrapper, now);
            if (min == 0 || weight < min) {
                min = weight;
                rv = key;
            }
        }
        if ((dict_len % LFU_BUCKET)) {
            pos = LFU_BUCKET * b_size + (dict_len - LFU_BUCKET * b_size) / 2;
            wrapper = PyDict_GetItem(self->dict, PyList_GetItem(keylist, pos));
            weight = LFUWrapper_WEIGHT((LFUWrapper*)wrapper, now);
            if (min == 0 || weight < min) {
                rv = key;
            }
        }
        Py_XDECREF(keylist);
    }
    assert(rv);
    Py_INCREF(rv);
    return rv;
}

static PyObject*
LFUCache_evict(LFUCache* self)
{
    PyObject* k = LFUCache_lfu(self);
    if (!k) {
        PyErr_Clear();
        Py_RETURN_NONE;
    }
    if (PyDict_DelItem(self->dict, k)) {
        return PyErr_Format(PyExc_KeyError, "Fail to delete Key %S", k);
    }
    Py_DECREF(k);
    Py_RETURN_NONE;
}

int PyLFUCache_DelItem(LFUCache* self, PyObject* key)
{
    LFUWrapper* wrapper = PyLFUCache_GetItem(self, key);
    if (!wrapper) {
        PyErr_Format(PyExc_TypeError, "%S", key);
        return -1;
    }
    if (PyDict_DelItem(self->dict, key)) {
        Py_XINCREF(wrapper->wrapped);
        return -1;
    }
    return 0;
}

int PyLFUCache_SetItem(LFUCache* self, PyObject* key, PyObject* value)
{
    LFUWrapper* wrapper = (LFUWrapper*)PyDict_GetItem(self->dict, key);
    if (wrapper) {
        Py_DECREF(wrapper->wrapped);
        wrapper->wrapped = value;
        Py_INCREF(wrapper->wrapped);
        return 0;
    }
    if (PyLFUCache_Size(self) + 1 > self->capacity) {
        if (!LFUCache_evict(self)) {
            return -1;
        }
    }
    PyObject* args = Py_BuildValue("(O)", value);
    if (!args)
        return -1;

    wrapper = (LFUWrapper*)PyObject_CallObject((PyObject*)&LFUWrapperType, args);
    if (!wrapper) {
        Py_DECREF(args);
        return -1;
    }
    if (PyDict_SetItem(self->dict, key, (PyObject*)wrapper)) {
        Py_DECREF(args);
        Py_DECREF(wrapper);
        return -1;
    }
    Py_DECREF(args);
    Py_DECREF(wrapper);
    return 0;
}

void PyLFUCache_Clear(LFUCache* self)
{
    PyDict_Clear(self->dict);
    self->misses = 0;
    self->hits = 0;
}

static PyObject* LFUCache_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
    LFUCache* self;
    self = (LFUCache*)PyObject_GC_New(LFUCache, type);
    if (!self)
        return NULL;
    PyObject_GC_Track(self);
    if (!(self->dict = PyDict_New())) {
        Py_DECREF(self);
        return NULL;
    }
    return (PyObject*)self;
}

static int
LFUCache_init(LFUCache* self, PyObject* args, PyObject* kwds)
{
    static char* kwlist[] = { "capacity", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "n", kwlist, &self->capacity)) {
        return -1;
    }
    if (self->capacity <= 0) {
        PyErr_SetString(PyExc_ValueError, "Capacity should be a positive number");
        return -1;
    }
    self->hits = 0;
    self->misses = 0;
    return 0;
}

static int
LFUCache_tp_traverse(LFUCache* self, visitproc visit, void* arg)
{
    Py_VISIT(self->dict);
    return 0;
}

static int
LFUCache_tp_clear(LFUCache* self)
{
    Py_CLEAR(self->dict);
    return 0;
}

static void
LFUCache_tp_dealloc(LFUCache* self)
{
    PyObject_GC_UnTrack(self);
    LFUCache_tp_clear(self);
    PyObject_GC_Del(self);
}

static PyObject*
LFUCache_repr(LFUCache* self)
{
    return PyObject_Repr(self->dict);
}

/* mp_subscript: __getitem__() */
static PyObject*
LFUCache_mp_subscript(LFUCache* self, PyObject* key)
{
    LFUWrapper* wrapper = PyLFUCache_GetItem(self, key);
    if (!wrapper) {
        self->misses++;
        return PyErr_Format(PyExc_KeyError, "%S", key);
    }
    self->hits++;
    return LFUWrapper_wrapped(wrapper);
}

/* mp_ass_subscript: __setitem__() and __delitem__() */
static int
LFUCache_mp_ass_sub(LFUCache* self, PyObject* key, PyObject* value)
{
    if (value == NULL) {
        return PyLFUCache_DelItem(self, key);
    } else {
        return PyLFUCache_SetItem(self, key, value);
    }
}

static PyMappingMethods LFUCache_as_mapping = {
    (lenfunc)PyLFUCache_Size, /*mp_length*/
    (binaryfunc)LFUCache_mp_subscript, /*mp_subscript*/
    (objobjargproc)LFUCache_mp_ass_sub, /*mp_ass_subscript*/
};

static PyObject*
LFUCache_hints(LFUCache* self)
{
    return Py_BuildValue("iii", self->capacity, self->hits, self->misses);
}

static PyObject* LFUCache_keys(LFUCache* self) { return PyDict_Keys(self->dict); }

static PyObject*
LFUCache_values(LFUCache* self)
{
    LFUWrapper* wrapper;
    PyObject* values = PyDict_Values(self->dict);
    if (!values)
        return NULL;
    if (PyList_GET_SIZE(values) == 0)
        return values;

    for (Py_ssize_t i = 0; i < PyList_GET_SIZE(values); i++) {
        wrapper = (LFUWrapper*)PyList_GET_ITEM(values, i);
        Py_INCREF(wrapper);
        PyList_SET_ITEM(values, i, LFUWrapper_wrapped(wrapper));
        Py_DECREF(wrapper);
    }
    return values;
}

static PyObject* LFUCache_items(LFUCache* self)
{
    LFUWrapper* wrapper;
    PyObject* items = PyDict_Items(self->dict);
    PyObject* kv;
    if (!items) {
        return NULL;
    }
    if (PyList_GET_SIZE(items) == 0)
        return items;

    for (Py_ssize_t i = 0; i < PyList_GET_SIZE(items); i++) {
        kv = PyList_GET_ITEM(items, i);
        wrapper = (LFUWrapper*)PyTuple_GET_ITEM(kv, 1);
        Py_INCREF(wrapper);
        PyTuple_SET_ITEM(kv, 1, LFUWrapper_wrapped(wrapper));
        Py_DECREF(wrapper);
    }
    return items;
}

static PyObject*
LFUCache_get(LFUCache* self, PyObject* args, PyObject* kw)
{
    PyObject* key;
    PyObject* _default = NULL;
    LFUWrapper* result;

    static char* kwlist[] = { "key", "default", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O|O", kwlist, &key, &_default))
        return NULL;
    result = PyLFUCache_GetItem(self, key);
    if (!result) {
        if (!_default)
            Py_RETURN_NONE;
        Py_INCREF(_default);
        return _default;
    }
    Py_INCREF(result->wrapped);
    return result->wrapped;
}

static PyObject*
LFUCache_pop(LFUCache* self, PyObject* args, PyObject* kw)
{
    PyObject* key;
    PyObject* _default = NULL;
    LFUWrapper* result;

    static char* kwlist[] = { "key", "default", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O|O", kwlist, &key, &_default))
        return NULL;
    result = PyLFUCache_GetItem(self, key);
    if (!result) {
        if (!_default)
            Py_RETURN_NONE;
        Py_INCREF(_default);
        return _default;
    }
    Py_INCREF(result->wrapped);
    if (!PyLFUCache_DelItem(self, key)) {
        Py_XDECREF(result->wrapped);
        return NULL;
    }
    return result->wrapped;
}

static PyObject*
LFUCache_setdefault(LFUCache* self, PyObject* args, PyObject* kw)
{
    PyObject* key;
    PyObject* _default = NULL;
    LFUWrapper* result;

    static char* kwlist[] = { "key", "default", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O|O", kwlist, &key, &_default))
        return NULL;
    result = PyLFUCache_GetItem(self, key);
    if (!result) {
        if (!_default)
            _default = Py_None;
        Py_INCREF(_default);
        if (PyLFUCache_SetItem(self, key, _default)) {
            Py_DECREF(_default);
            return NULL;
        }
        return _default;
    }

    return LFUWrapper_wrapped(result);
}

static PyObject*
LFUCache_setnx(LFUCache* self, PyObject* args, PyObject* kw)
{
    PyObject* key;
    PyObject *_default = NULL, *callback = NULL;
    LFUWrapper* result;

    static char* kwlist[] = { "key", "callback", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "OO", kwlist, &key, &callback))
        return NULL;

    if (callback == NULL || !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "callback should be callable.");
        return NULL;
    }

    result = PyLFUCache_GetItem(self, key);
    if (!result) {
        _default = PyObject_CallFunction(callback, NULL);
        Py_INCREF(_default);
        if (PyLFUCache_SetItem(self, key, _default)) {
            Py_DECREF(_default);
            return NULL;
        }
        return _default;
    }

    return LFUWrapper_wrapped(result);
}

static PyObject*
LFUCache_update(LFUCache* self, PyObject* args, PyObject* kwargs)
{
    PyObject *key, *value;
    PyObject* arg = NULL;
    Py_ssize_t pos = 0;

    if ((PyArg_ParseTuple(args, "|O", &arg))) {
        if (arg && PyDict_Check(arg)) {
            while (PyDict_Next(arg, &pos, &key, &value))
                if (PyLFUCache_SetItem(self, key, value))
                    return NULL;
        }
    }

    if (kwargs != NULL && PyArg_ValidateKeywordArguments(kwargs)) {
        while (PyDict_Next(kwargs, &pos, &key, &value))
            if (PyLFUCache_SetItem(self, key, value))
                return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject*
LFUCache_set_capacity(LFUCache* self, PyObject* capacity)
{
    long long cap = PyLong_AsLongLong(capacity);
    if (cap <= 0) {
        PyObject* err = PyErr_Occurred();
        if (err == NULL) {
            PyErr_SetString(PyExc_ValueError,
                "Capacity should be a positive integer");
        }
        return NULL;
    }
    if (cap < self->capacity && PyLFUCache_Size(self) > cap) {
        int r = PyLFUCache_Size(self) - cap;
        for (int i = 0; i < r; ++i) {
            if (LFUCache_evict(self) == NULL)
                return NULL;
        }
    }
    self->capacity = cap;
    Py_RETURN_NONE;
}

static PyObject*
LFUCache__store(LFUCache* self)
{
    PyObject* dict = self->dict;
    Py_INCREF(dict);
    return dict;
}

static PyObject*
LFUCache_clear(LFUCache* self)
{
    PyLFUCache_Clear(self);
    Py_RETURN_NONE;
}

/* tp_methods */
static PyMethodDef LFUCache_methods[] = {
    { "evict", (PyCFunction)LFUCache_evict, METH_NOARGS, NULL },
    { "set_capacity", (PyCFunction)LFUCache_set_capacity, METH_O, NULL },
    { "hints", (PyCFunction)LFUCache_hints, METH_NOARGS, NULL },
    { "lfu", (PyCFunction)LFUCache_lfu, METH_NOARGS, NULL },
    { "get", (PyCFunction)LFUCache_get, METH_VARARGS | METH_KEYWORDS, NULL },
    { "setdefault", (PyCFunction)LFUCache_setdefault,
        METH_VARARGS | METH_KEYWORDS, NULL },
    { "pop", (PyCFunction)LFUCache_pop, METH_VARARGS | METH_KEYWORDS, NULL },
    { "keys", (PyCFunction)LFUCache_keys, METH_NOARGS, NULL },
    { "values", (PyCFunction)LFUCache_values, METH_NOARGS, NULL },
    { "items", (PyCFunction)LFUCache_items, METH_NOARGS, NULL },
    { "update", (PyCFunction)LFUCache_update,
        METH_VARARGS | METH_KEYWORDS, NULL },
    { "clear", (PyCFunction)LFUCache_clear, METH_NOARGS, NULL },
    { "setnx", (PyCFunction)LFUCache_setnx, METH_VARARGS | METH_KEYWORDS, NULL },
    { "_store", (PyCFunction)LFUCache__store, METH_NOARGS, NULL },
    { NULL, NULL, 0, NULL } /* Sentinel */
};

static PyObject*
LFUCache_tp_iter(LFUCache* self)
{
    PyObject *keys, *it;
    keys = LFUCache_keys(self);
    if (!keys)
        return NULL;
    it = PySeqIter_New(keys);
    Py_DECREF(keys);
    return it;
}

PyDoc_STRVAR(LFUCache__doc__, "A fast LFUCache behaving much like dict.");

PyTypeObject LFUCacheType = {
    PyVarObject_HEAD_INIT(NULL, 0) "ctools.LFUCache", /* tp_name */
    .tp_basicsize=sizeof(LFUCache), /* tp_basicsize */
    0, /* tp_itemsize */
    .tp_dealloc=(destructor)LFUCache_tp_dealloc, /* tp_dealloc */
    0, /* tp_print */
    0, /* tp_getattr */
    0, /* tp_setattr */
    0, /* tp_compare */
    .tp_repr=(reprfunc)LFUCache_repr, /* tp_repr */
    0, /* tp_as_number */
    &LFUCache_as_sequence, /* tp_as_sequence */
    &LFUCache_as_mapping, /* tp_as_mapping */
    0, /* tp_hash */
    0, /* tp_call */
    0, /* tp_str */
    0, /* tp_getattro */
    0, /* tp_setattro */
    0, /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /* tp_flags */
    LFUCache__doc__, /* tp_doc */
    (traverseproc)LFUCache_tp_traverse, /* tp_traverse */
    (inquiry)LFUCache_tp_clear, /* tp_clear */
    0, /* tp_richcompare */
    0, /* tp_weaklistoffset */
    (getiterfunc)LFUCache_tp_iter, /* tp_iter */
    0, /* tp_iternext */
    LFUCache_methods, /* tp_methods */
    0, /* tp_members */
    0, /* tp_getset */
    0, /* tp_base */
    0, /* tp_dict */
    0, /* tp_descr_get */
    0, /* tp_descr_set */
    0, /* tp_dictoffset */
    (initproc)LFUCache_init, /* tp_init */
    0, /* tp_alloc */
    (newfunc)LFUCache_new, /* tp_new */
};

#endif // _CTOOLS_LFU_H

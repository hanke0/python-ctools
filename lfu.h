#ifndef _CTOOLS_LFU_H
#define _CTOOLS_LFU_H

#include <Python.h>
#include <stdlib.h>
#include <time.h>

typedef struct _LFUValue {
    PyObject_HEAD PyObject* value;
    uint32_t lfu; /* last_visit in minutes & counter, 24 bit + 8 bit */
} LFUValue;

static void LFUValue_dealloc(LFUValue* self)
{
    Py_DECREF(self->value);
    PyObject_Del((PyObject*)self);
}

static PyObject* LFUValue_repr(LFUValue* self)
{
    return PyObject_Repr(self->value);
}

static PyTypeObject LFUValueType = {
    PyVarObject_HEAD_INIT(NULL, 0) "cools.LFUValue", /* tp_name */
    sizeof(LFUValue), /* tp_basicsize */
    0, /* tp_itemsize */
    (destructor)LFUValue_dealloc, /* tp_dealloc */
    0, /* tp_print */
    0, /* tp_getattr */
    0, /* tp_setattr */
    0, /* tp_compare */
    (reprfunc)LFUValue_repr, /* tp_repr */
    0, /* tp_as_number */
    0, /* tp_as_sequence */
    0, /* tp_as_mapping */
    0, /* tp_hash */
    0, /* tp_call */
    0, /* tp_str */
    0, /* tp_getattro */
    0, /* tp_setattro */
    0, /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT, /* tp_flags */
    "LFU value store", /* tp_doc */
    0, /* tp_traverse */
    0, /* tp_clear */
    0, /* tp_richcompare */
    0, /* tp_weaklistoffset */
    0, /* tp_iter */
    0, /* tp_iternext */
    0, /* tp_methods */
    0, /* tp_members */
    0, /* tp_getset */
    0, /* tp_base */
    0, /* tp_dict */
    0, /* tp_descr_get */
    0, /* tp_descr_set */
    0, /* tp_dictoffset */
    0, /* tp_init */
    0, /* tp_alloc */
    0, /* tp_new */
};

typedef struct {
    PyObject_HEAD PyObject* dict;
    Py_ssize_t capacity;
    Py_ssize_t hits;
    Py_ssize_t misses;
} LFUCache;

#define LFU_INIT_VAL 5UL
#define LFU_LOG_FACTOR 10UL
#define LFU_DECAY_FACTOR 3UL // minutes

/*
 * COPY FROM redis
 * 概率计算公式 1 / ((counter - LFU_INIT_VAL) * LFU_LOG_FACTOR + 1)
 */
uint32_t lfu_log_incr(uint32_t counter)
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

#define TimeInMinutes() ((time(NULL) / 60) & 0xffffff)

#define LFUValue_UPDATE(self)                                             \
    do {                                                                  \
        uint32_t last_visit = (self)->lfu >> 8;                           \
        uint32_t counter = (self)->lfu & 255;                             \
        uint32_t time_in_minutes = TimeInMinutes();                       \
        uint32_t num = (time_in_minutes - last_visit) / LFU_DECAY_FACTOR; \
        counter = lfu_log_incr(num > counter ? 0 : counter - num);        \
        (self)->lfu = (time_in_minutes << 8) | counter;                   \
    } while (0)

#define LFUValue_INIT(self, value)                                                    \
    do {                                                                              \
        (self)->lfu = ((uint32_t)((TimeInMinutes() & 0xffffff) << 8)) | LFU_INIT_VAL; \
        (self)->value = value;                                                        \
    } while (0)

static void LFUCache_dealloc(LFUCache* self)
{
    if (self->dict)
        Py_DECREF(self->dict);
    PyObject_Del((PyObject*)self);
}

static int LFUCache_init(LFUCache* self, PyObject* args, PyObject* kwds)
{
    static char* kwlist[] = { "capacity", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "n", kwlist, &self->capacity)) {
        PyErr_SetString(PyExc_TypeError, "Capacity needed");
        return -1;
    }
    if (self->capacity <= 0) {
        PyErr_SetString(PyExc_ValueError, "Capacity should be a positive number");
        return -1;
    }
    self->dict = PyDict_New();
    self->hits = 0;
    self->misses = 0;
    return 0;
}

static PyObject* LFUCache_repr(LFUCache* self)
{
    return PyObject_Repr(self->dict);
}

static Py_ssize_t LFUCache_length(LFUCache* self)
{
    return PyDict_Size(self->dict);
}

/* mp_subscript: __getitem__() */
static PyObject* LFUCache_mp_subscript(LFUCache* self, PyObject* key)
{
    LFUValue* value = (LFUValue*)PyDict_GetItem(self->dict, key);
    if (value == NULL) {
        self->misses++;
        PyErr_SetString(PyExc_KeyError, "Key Error");
        return NULL;
    }
    LFUValue_UPDATE(value);
    PyObject* rv = value->value;
    Py_INCREF(rv);
    self->hits++;
    return rv;
}

/* return a random number between 0 and limit inclusive.
 */
int rand_limit(int limit)
{
    int divisor = RAND_MAX / (limit + 1);
    int rv;

    do {
        rv = rand() / divisor;
    } while (rv > limit);

    return rv;
}

#define LFU_BUCKET 8
#define LFU_BUCKET_SIZE 256

static PyObject* LFUCache_lfu(LFUCache* self)
{
    PyObject *key = NULL, *value = NULL;
    Py_ssize_t pos = 0;
    uint32_t min = 0;
    LFUValue* current = NULL;
    PyObject* rv = NULL;
    uint32_t now = TimeInMinutes(), num, counter;
    Py_ssize_t dict_len = LFUCache_length(self);

    if (dict_len < LFU_BUCKET_SIZE) {
        while (PyDict_Next(self->dict, &pos, &key, &value)) {
            current = (LFUValue*)value;
            counter = current->lfu & 255;
            num = (now - (current->lfu >> 8)) / LFU_DECAY_FACTOR;
            num = num > counter ? 0 : counter - num;
            if (min == 0 || num < min) {
                min = num;
                rv = key;
            }
        }
    } else {
        PyObject* keys = PyDict_Keys(self->dict);
        Py_ssize_t b_size = dict_len / LFU_BUCKET;
        if ((dict_len % LFU_BUCKET)) b_size++;
        for (int i = 1; i < LFU_BUCKET; i++) {
            pos = (i - 1) * b_size + rand_limit(b_size);
            value = PyDict_GetItem(self->dict, PyList_GetItem(keys, pos));
            current = (LFUValue*)value;
            counter = current->lfu & 255;
            num = (now - (current->lfu >> 8)) / LFU_DECAY_FACTOR;
            num = num > counter ? 0 : counter - num;
            if (min == 0 || num < min) {
                min = num;
                rv = key;
            }else {
              Py_DECREF(key);
            }
            Py_DECREF(value);
        }
        pos = (LFU_BUCKET - 1) * b_size;
        pos = pos + rand_limit(dict_len - pos);
        value = PyDict_GetItem(self->dict, PyList_GetItem(key, pos));
        current = (LFUValue*)value;
        counter = current->lfu & 255;
        num = (now - (current->lfu >> 8)) / LFU_DECAY_FACTOR;
        num = num > counter ? 0 : counter - num;
        if (min == 0 || num < min) {
          rv = key;
        } else {
          Py_DECREF(key);
        }
        Py_DECREF(value);
        Py_DECREF(keys);
    }
    if (!rv) {
        PyErr_SetString(PyExc_KeyError, "No key in dict");
        return NULL;
    }
    return rv;
}

static PyObject* LFUCache_evict(LFUCache* self)
{
    PyObject* k = LFUCache_lfu(self);
    if (k == NULL) {
        PyErr_Clear();
        Py_RETURN_NONE;
    }
    if (PyDict_DelItem(self->dict, k) != 0) {
        return PyErr_Format(PyExc_KeyError, "Delete Not Exist Key %U", k);
    }
    Py_RETURN_NONE;
}

/* mp_ass_subscript: __setitem__() and __delitem__() */
static int LFUCache_mp_ass_sub(LFUCache* self, PyObject* key, PyObject* value)
{
    if (value == NULL) {
        return PyDict_DelItem(self->dict, key);
    } else {
        LFUValue* v = (LFUValue*)PyDict_GetItem(self->dict, key);
        if (v != NULL) {
            Py_DECREF(v->value);
            v->value = value;
            Py_INCREF(v->value);
            return 0;
        }
        PyErr_Clear();
        if (LFUCache_length(self) + 1 > self->capacity) {
            if (LFUCache_evict(self) == NULL) {
                return -1;
            }
        }
        LFUValue* LFUCache_value = PyObject_NEW(LFUValue, &LFUValueType);
        LFUValue_INIT(LFUCache_value, value);
        return PyDict_SetItem(self->dict, key, (PyObject*)LFUCache_value);
    }
}

static PyMappingMethods LFUCache_as_mapping = {
    (lenfunc)LFUCache_length, /*mp_length*/
    (binaryfunc)LFUCache_mp_subscript, /*mp_subscript*/
    (objobjargproc)LFUCache_mp_ass_sub, /*mp_ass_subscript*/
};

PyObject* LFUCache_lfu_of(LFUCache* self, PyObject* key)
{
    LFUValue* rv = (LFUValue*)PyDict_GetItem(self->dict, key);
    return Py_BuildValue("k", rv->lfu);
}

PyObject* LFUCache_hints(LFUCache* self)
{
    return Py_BuildValue("iii", self->capacity, self->hits, self->misses);
}

PyObject* LFUCache_keys(LFUCache* self) { return PyDict_Keys(self->dict); }

PyObject* LFUCache_values(LFUCache* self) { return PyDict_Values(self->dict); }

PyObject* LFUCache_items(LFUCache* self) { return PyDict_Items(self->dict); }

PyObject* LFUCache_get(LFUCache* self, PyObject* args)
{
    PyObject* key;
    PyObject* _default = NULL;
    PyObject* result;

    if (!PyArg_ParseTuple(args, "O|O", &key, &_default))
        return NULL;
    result = LFUCache_mp_subscript(self, key);
    PyErr_Clear(); /* mp_subscript sets an exception on miss. Clear it */
    if (result)
        return result;
    if (!_default)
        Py_RETURN_NONE;
    Py_INCREF(_default);
    return _default;
}

PyObject* LFUCache_pop(LFUCache* self, PyObject* args)
{
    PyObject* key;
    PyObject* _default = NULL;
    PyObject* result;

    if (!PyArg_ParseTuple(args, "O|O", &key, &_default))
        return NULL;
    result = LFUCache_mp_subscript(self, key);
    PyErr_Clear(); /* mp_subscript sets an exception on miss. Clear it */
    if (result) {
        LFUCache_mp_ass_sub(self, key, NULL);
        Py_INCREF(result);
        return result;
    }
    if (!_default)
        Py_RETURN_NONE;
    Py_INCREF(_default);
    return _default;
}

PyObject* LFUCache_setdefault(LFUCache* self, PyObject* args)
{
    PyObject* key;
    PyObject* _default = NULL;
    PyObject* result;

    if (!PyArg_ParseTuple(args, "O|O", &key, &_default))
        return NULL;
    result = LFUCache_mp_subscript(self, key);
    PyErr_Clear(); /* mp_subscript sets an exception on miss. Clear it */
    if (result)
        return result;
    if (!_default)
        _default = Py_None;
    if (LFUCache_mp_ass_sub(self, key, _default) != 0) {
        return NULL;
    }
    Py_INCREF(_default);
    return _default;
}

PyObject* LFUCache_update(LFUCache* self, PyObject* args, PyObject* kwargs)
{
    PyObject *key, *value;
    PyObject* arg = NULL;
    Py_ssize_t pos = 0;

    if ((PyArg_ParseTuple(args, "|O", &arg))) {
        if (arg && PyDict_Check(arg)) {
            while (PyDict_Next(arg, &pos, &key, &value))
                LFUCache_mp_ass_sub(self, key, value);
        }
    }

    if (kwargs != NULL && PyDict_Check(kwargs)) {
        while (PyDict_Next(kwargs, &pos, &key, &value))
            LFUCache_mp_ass_sub(self, key, value);
    }

    Py_RETURN_NONE;
}

PyObject* LFUCache_clear(LFUCache* self)
{
    PyDict_Clear(self->dict);
    self->misses = 0;
    self->hits = 0;
    Py_RETURN_NONE;
}

PyObject* LFUCache_set_capacity(LFUCache* self, PyObject* capacity)
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
    if (cap < self->capacity && LFUCache_length(self) > cap) {
        int r = LFUCache_length(self) - cap;
        for (int i = 0; i < r; ++i) {
            if (LFUCache_evict(self) == NULL)
                return NULL;
        }
    }
    self->capacity = cap;
    Py_RETURN_NONE;
}

/* tp_methods */
static PyMethodDef LFUCache_methods[] = {
    { "evict", (PyNoArgsFunction)LFUCache_evict, METH_NOARGS, NULL },
    { "set_capacity", (PyCFunction)LFUCache_set_capacity, METH_O, NULL },
    { "hints", (PyNoArgsFunction)LFUCache_hints, METH_NOARGS, NULL },
    { "lfu", (PyNoArgsFunction)LFUCache_lfu, METH_NOARGS, NULL },
    { "lfu_of", (PyCFunction)LFUCache_lfu_of, METH_O, NULL },
    { "get", (PyCFunction)LFUCache_get, METH_VARARGS | METH_KEYWORDS, NULL },
    { "setdefault", (PyCFunction)LFUCache_setdefault,
        METH_VARARGS | METH_KEYWORDS, NULL },
    { "pop", (PyCFunction)LFUCache_pop, METH_VARARGS | METH_KEYWORDS, NULL },
    { "keys", (PyNoArgsFunction)LFUCache_keys, METH_NOARGS, NULL },
    { "values", (PyNoArgsFunction)LFUCache_values, METH_NOARGS, NULL },
    { "items", (PyNoArgsFunction)LFUCache_items, METH_NOARGS, NULL },
    { "update", (PyCFunctionWithKeywords)LFUCache_update,
        METH_VARARGS | METH_KEYWORDS, NULL },
    { "clear", (PyNoArgsFunction)LFUCache_clear, METH_NOARGS, NULL },
    { NULL, NULL } /* sentinel */
};

PyDoc_STRVAR(LFUCache__doc__, "A fast LFUCache behaving much like dict.");

static PyTypeObject LFUCacheType = {
    PyVarObject_HEAD_INIT(NULL, 0) "ctools.LFUCache", /* tp_name */
    sizeof(LFUCache), /* tp_basicsize */
    0, /* tp_itemsize */
    (destructor)LFUCache_dealloc, /* tp_dealloc */
    0, /* tp_print */
    0, /* tp_getattr */
    0, /* tp_setattr */
    0, /* tp_compare */
    (reprfunc)LFUCache_repr, /* tp_repr */
    0, /* tp_as_number */
    0, /* tp_as_sequence */
    &LFUCache_as_mapping, /* tp_as_mapping */
    0, /* tp_hash */
    0, /* tp_call */
    0, /* tp_str */
    0, /* tp_getattro */
    0, /* tp_setattro */
    0, /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT, /* tp_flags */
    LFUCache__doc__, /* tp_doc */
    0, /* tp_traverse */
    0, /* tp_clear */
    0, /* tp_richcompare */
    0, /* tp_weaklistoffset */
    0, /* tp_iter */
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
    0, /* tp_new */
};

#endif // _CTOOLS_LFU_H

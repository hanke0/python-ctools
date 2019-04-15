#ifndef _CTOOLS_LFU_H
#define _CTOOLS_LFU_H

#include <Python.h>
#include <stdlib.h>
#include <time.h>

typedef struct _LFUValue {
    PyObject_HEAD
    PyObject *value;
    uint32_t lfu; /* last_visit in minutes & counter, 24 bit + 8 bit */
} LFUValue;

static void
lfu_value_dealloc(LFUValue *self) {
    Py_DECREF(self->value);
    PyObject_Del((PyObject *) self);
}

static PyObject *
lfu_value_repr(LFUValue *self) {
    return PyObject_Repr(self->value);
}

static PyTypeObject LFUValueType = {
        PyVarObject_HEAD_INIT(NULL, 0) "cools.LFUValue", /* tp_name */
        sizeof(LFUValue), /* tp_basicsize */
        0, /* tp_itemsize */
        (destructor) lfu_value_dealloc, /* tp_dealloc */
        0, /* tp_print */
        0, /* tp_getattr */
        0, /* tp_setattr */
        0, /* tp_compare */
        (reprfunc) lfu_value_repr, /* tp_repr */
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
    PyObject_HEAD
    PyObject *dict;
    Py_ssize_t capacity;
    Py_ssize_t hits;
    Py_ssize_t misses;
} LFU;

#define LFU_INIT_VAL 5UL
#define LFU_LOG_FACTOR 10UL
#define LFU_DECAY_FACTOR 3UL // minutes

/*
 * COPY FROM redis
 * 概率计算公式 1 / ((counter - LFU_INIT_VAL) * LFU_LOG_FACTOR + 1)
 */
uint32_t
lfu_log_incr(uint32_t counter) {
    if (counter == 255)
        return 255;
    double r = (double) rand() / RAND_MAX;
    double baseval = counter - LFU_INIT_VAL;
    if (baseval < 0)
        baseval = 0;
    double p = 1.0 / (baseval * LFU_LOG_FACTOR + 1);
    if (r < p)
        counter++;
    return counter;
}

#define TimeInMinutes() ((time(NULL) / 60) & 0xffffff)

#define LFU_UPDATE(self)                                                  \
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

static void
LFU_dealloc(LFU *self) {
    if (self->dict)
        Py_DECREF(self->dict);
    PyObject_Del((PyObject *) self);
}

static int
LFU_init(LFU *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"capacity", NULL};
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

static PyObject *
LFU_repr(LFU *self) {
    return PyObject_Repr(self->dict);
}

static Py_ssize_t
LFU_length(LFU *self) {
    return PyDict_Size(self->dict);
}

/* mp_subscript: __getitem__() */
static PyObject *
LFU_mp_subscript(LFU *self, PyObject *key) {
    LFUValue *value = (LFUValue *) PyDict_GetItem(self->dict, key);
    if (value == NULL) {
        self->misses++;
        PyErr_SetString(PyExc_KeyError, "Key Error");
        return NULL;
    }
    LFU_UPDATE(value);
    PyObject *rv = value->value;
    Py_INCREF(rv);
    self->hits++;
    return rv;
}

#define LFU_JUMP 5
/* return a random number between 0 and limit inclusive.
 */
int rand_limit(int limit) {
    int divisor = RAND_MAX / (limit + 1);
    int retval;

    do { 
        retval = rand() / divisor;
    } while (retval > limit);

    return retval;
}

static PyObject *
LFU_should_removed(LFU *self) {
    PyObject *key = NULL, *value = NULL;
    Py_ssize_t pos = 0;
    uint32_t min = 0;
    LFUValue *current = NULL;
    PyObject *rv = NULL;
    uint32_t now = TimeInMinutes(), num, counter;
    while (PyDict_Next(self->dict, &pos, &key, &value)) {
        if ((self->capacity > LFU_JUMP) && ((pos % LFU_JUMP) != 0))
            continue;

        current = (LFUValue *) value;

        counter = current->lfu & 255;
        num = (now - (current->lfu >> 8)) / LFU_DECAY_FACTOR;
        num = num > counter ? 0 : counter - num;
        if (min == 0 || num < min) {
            min = num;
            rv = key;
        }
    }
    if (!rv) {
        PyErr_SetString(PyExc_KeyError, "No key in dict");
        return NULL;
    }
    return rv;
}

static int
LFU_remove_one(LFU *self) {
    PyObject *k = LFU_should_removed(self);
    if (k == NULL) {
        PyErr_Clear();
    }
    if (PyDict_DelItem(self->dict, k) != 0) {
        PyErr_SetString(PyExc_KeyError, "Delete Not Exist Key");
        return -1;
    }
    return 0;
}

/* mp_ass_subscript: __setitem__() and __delitem__() */
static int
LFU_mp_ass_sub(LFU *self, PyObject *key, PyObject *value) {
    if (value == NULL) {
        return PyDict_DelItem(self->dict, key);
    } else {
        LFUValue *v = (LFUValue *) PyDict_GetItem(self->dict, key);
        if (v != NULL) {
            Py_DECREF(v->value);
            v->value = value;
            Py_INCREF(v->value);
            return 0;
        }
        PyErr_Clear();
        if (LFU_length(self) + 1 > self->capacity) {
            if (LFU_remove_one(self) != 0) {
                return -1;
            }
        }
        LFUValue *lfu_value = PyObject_NEW(LFUValue, &LFUValueType);
        LFUValue_INIT(lfu_value, value);
        return PyDict_SetItem(self->dict, key, (PyObject *) lfu_value);
    }
}

static PyMappingMethods LFU_as_mapping = {
        (lenfunc) LFU_length, /*mp_length*/
        (binaryfunc) LFU_mp_subscript, /*mp_subscript*/
        (objobjargproc) LFU_mp_ass_sub, /*mp_ass_subscript*/
};

PyObject *
LFU_lfu_of(LFU *self, PyObject *key) {
    LFUValue *rv = (LFUValue *) PyDict_GetItem(self->dict, key);
    return Py_BuildValue("k", rv->lfu);
}

PyObject *
LFU_get_hint(LFU *self) {
    return Py_BuildValue("iii", self->capacity, self->hits, self->misses);
}

PyObject *
LFU_keys(LFU *self) {
    return PyDict_Keys(self->dict);
}

PyObject *
LFU_values(LFU *self) {
    return PyDict_Values(self->dict);
}

PyObject *
LFU_items(LFU *self) {
    return PyDict_Items(self->dict);
}

PyObject *
LFU_get(LFU *self, PyObject *args) {
    PyObject *key;
    PyObject *_default = NULL;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "O|O", &key, &_default))
        return NULL;
    result = LFU_mp_subscript(self, key);
    PyErr_Clear(); /* mp_subscript sets an exception on miss. Clear it */
    if (result)
        return result;
    if (!_default)
        Py_RETURN_NONE;
    Py_INCREF(_default);
    return _default;
}

PyObject *
LFU_pop(LFU *self, PyObject *args) {
    PyObject *key;
    PyObject *_default = NULL;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "O|O", &key, &_default))
        return NULL;
    result = LFU_mp_subscript(self, key);
    PyErr_Clear(); /* mp_subscript sets an exception on miss. Clear it */
    if (result) {
        LFU_mp_ass_sub(self, key, NULL);
        Py_INCREF(result);
        return result;
    }
    if (!_default)
        Py_RETURN_NONE;
    Py_INCREF(_default);
    return _default;
}

PyObject *
LFU_setdefault(LFU *self, PyObject *args) {
    PyObject *key;
    PyObject *_default = NULL;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "O|O", &key, &_default))
        return NULL;
    result = LFU_mp_subscript(self, key);
    PyErr_Clear(); /* mp_subscript sets an exception on miss. Clear it */
    if (result)
        return result;
    if (!_default)
        _default = Py_None;
    if (LFU_mp_ass_sub(self, key, _default) != 0) {
        return NULL;
    }
    Py_INCREF(_default);
    return _default;
}

PyObject *
LFU_update(LFU *self, PyObject *args, PyObject *kwargs) {
    PyObject *key, *value;
    PyObject *arg = NULL;
    Py_ssize_t pos = 0;

    if ((PyArg_ParseTuple(args, "|O", &arg))) {
        if (arg && PyDict_Check(arg)) {
            while (PyDict_Next(arg, &pos, &key, &value))
                LFU_mp_ass_sub(self, key, value);
        }
    }

    if (kwargs != NULL && PyDict_Check(kwargs)) {
        while (PyDict_Next(kwargs, &pos, &key, &value))
            LFU_mp_ass_sub(self, key, value);
    }

    Py_RETURN_NONE;
}

PyObject *
LFU_clear(LFU *self) {
    PyDict_Clear(self->dict);
    self->misses = 0;
    self->hits = 0;
    Py_RETURN_NONE;
}

PyObject *
LFU_set_capacity(LFU *self, PyObject *capacity) {
    long long cap =  PyLong_AsLongLong(capacity);
    if (cap <= 0) {
        PyObject *err = PyErr_Occurred();
        if (err == NULL) {
            PyErr_SetString(PyExc_ValueError, "Capacity should be a positive integer");
        }
        return NULL;
    }
    if (cap < self->capacity && LFU_length(self) > cap) {
        int r = LFU_length(self) - cap;
        for (int i = 0; i < r; ++i) {
            if (LFU_remove_one(self) != 0) return NULL;
        }
    }
    self->capacity = cap;
    Py_RETURN_NONE;
}

PyObject*
LFU_del_should_removed(LFU* self) {
    if (LFU_length(self) == 0) Py_RETURN_NONE;
    if (LFU_remove_one(self) !=0) return NULL;
    Py_RETURN_NONE;
}

/* tp_methods */
static PyMethodDef LFU_methods[] = {
        {"del_should_removed",     (PyNoArgsFunction) LFU_del_should_removed,     METH_NOARGS, NULL},
        {"set_capacity",   (PyCFunction) LFU_set_capacity,        METH_O,      NULL},
        {"hints",          (PyNoArgsFunction) LFU_get_hint,       METH_NOARGS, NULL},
        {"should_removed", (PyNoArgsFunction) LFU_should_removed, METH_NOARGS, NULL},
        {"lfu_of",         (PyCFunction) LFU_lfu_of,              METH_O,      NULL},
        {"get",            (PyCFunction) LFU_get,                 METH_VARARGS | METH_KEYWORDS, NULL},
        {"setdefault",     (PyCFunction) LFU_setdefault,          METH_VARARGS | METH_KEYWORDS, NULL},
        {"pop",            (PyCFunction) LFU_pop,                 METH_VARARGS | METH_KEYWORDS, NULL},
        {"keys",           (PyNoArgsFunction) LFU_keys,           METH_NOARGS, NULL},
        {"values",         (PyNoArgsFunction) LFU_values,         METH_NOARGS, NULL},
        {"items",          (PyNoArgsFunction) LFU_items,          METH_NOARGS, NULL},
        {"update",         (PyCFunctionWithKeywords) LFU_update,  METH_VARARGS | METH_KEYWORDS, NULL},
        {"clear",          (PyNoArgsFunction) LFU_clear,          METH_NOARGS, NULL},
        {NULL, NULL} /* sentinel */
};

PyDoc_STRVAR(LFU__doc__, "LFU()");

static PyTypeObject LFUType = {
        PyVarObject_HEAD_INIT(NULL, 0) "ctools.LFU", /* tp_name */
        sizeof(LFU), /* tp_basicsize */
        0, /* tp_itemsize */
        (destructor) LFU_dealloc, /* tp_dealloc */
        0, /* tp_print */
        0, /* tp_getattr */
        0, /* tp_setattr */
        0, /* tp_compare */
        (reprfunc) LFU_repr, /* tp_repr */
        0, /* tp_as_number */
        0, /* tp_as_sequence */
        &LFU_as_mapping, /* tp_as_mapping */
        0, /* tp_hash */
        0, /* tp_call */
        0, /* tp_str */
        0, /* tp_getattro */
        0, /* tp_setattro */
        0, /* tp_as_buffer */
        Py_TPFLAGS_DEFAULT, /* tp_flags */
        LFU__doc__, /* tp_doc */
        0, /* tp_traverse */
        0, /* tp_clear */
        0, /* tp_richcompare */
        0, /* tp_weaklistoffset */
        0, /* tp_iter */
        0, /* tp_iternext */
        LFU_methods, /* tp_methods */
        0, /* tp_members */
        0, /* tp_getset */
        0, /* tp_base */
        0, /* tp_dict */
        0, /* tp_descr_get */
        0, /* tp_descr_set */
        0, /* tp_dictoffset */
        (initproc) LFU_init, /* tp_init */
        0, /* tp_alloc */
        0, /* tp_new */
};

#endif // _CTOOLS_LFU_H

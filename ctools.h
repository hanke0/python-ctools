#ifndef _CTOOLS_H
#define _CTOOLS_H
#include <Python.h>
#include <datetime.h>

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

static PyObject*
Ctools__jump_hash(PyObject *Py_UNUSED(ignored), PyObject* args)
{
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

PyDoc_STRVAR(strhash__doc__,
    "strhash(s) -> int:\n\n\
    hash str with consistent value.\n\n\
    This function uses C bindings for speed.\n\n\
    :param s: The string to hash.\n\
    :type s: string\n\
    :return: hash number\n\
    :rtype: int\n");

static PyObject*
Ctools__strhash(PyObject *Py_UNUSED(ignored), PyObject* args)
{
    const char* s;
    if (!PyArg_ParseTuple(args, "s", &s))
        return NULL;
    unsigned int hash = 2166136261U;
    unsigned char c;
    while ((c = *s++)) {
        hash = hash ^ c;
        /* hash * (1 << 24 + 1 << 8 + 0x93) */
        hash += (hash << 1) + (hash << 4) + (hash << 7) + (hash << 8) + (hash << 24);
    }
    return Py_BuildValue("I", hash);
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

static PyObject*
Ctools__int8_to_datetime(PyObject *Py_UNUSED(ignored), PyObject* date_integer)
{
    register long date = PyLong_AsLong(date_integer);
    if (date > 99990101 || date < 101) {
        PyErr_SetString(PyExc_ValueError, "date integer should between 00000101 and 99991231");
        return NULL;
    }
    return PyDateTime_FromDate(date / 10000, date % 10000 / 100, date % 100);
}
#endif // _CTOOLS_H

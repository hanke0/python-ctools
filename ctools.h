#include <Python.h>
#include <datetime.h>

static PyObject*
Ctools__jump_hash(PyObject* self, PyObject* args)
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

static PyObject*
Ctools__strhash(PyObject* self, PyObject* args)
{
    const char* s;
    if (!PyArg_ParseTuple(args, "s", &s))
        return NULL;
    uint64_t hash = 5381;
    int c;

    while (c = *s++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return Py_BuildValue("i", hash);
}

#define PyDateTime_FromDate(year, month, day) \
    PyDateTime_FromDateAndTime(year, month, day, 0, 0, 0, 0)

static PyObject*
Ctools__int8_to_datetime(PyObject* self, PyObject* date_integer)
{
    register long date = PyLong_AsLong(date_integer);
    if (date > 99990101 || date < 101) {
        PyErr_SetString(PyExc_ValueError, "date integer should between 00000101 and 99991231");
        return NULL;
    }
    return PyDateTime_FromDate(date / 10000, date % 10000 / 100, date % 100);
}

#include <Python.h>
#include <datetime.h>

static PyObject *DateExt_IntegerToDatetime(PyObject *self, PyObject *date_integer)
{
    PyDateTime_IMPORT;
    if (!PyLong_Check(date_integer))
    {
        PyErr_SetString(PyExc_TypeError, "except integer!");
        PyErr_Occurred();
    }
    register long date = PyLong_AsLong(date_integer);
    return PyDateTime_FromDateAndTime(date / 10000, date % 10000 / 100, date % 100, 0, 0, 0, 0);
}

static char DateExt_IntegerToDatetime_docs[] =
    "usage: integer_to_datetime(date_integer: int) -> datetime.datetime\n";

static PyMethodDef module_methods[] = {
    {"integer_to_datetime", (PyCFunction)DateExt_IntegerToDatetime, METH_O, DateExt_IntegerToDatetime_docs},
    {NULL}};

static struct PyModuleDef DateExt =
    {
        PyModuleDef_HEAD_INIT,
        "date_ext", /* name of module */
        NULL,       /* module documentation, may be NULL */
        -1,         /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
        module_methods};

PyMODINIT_FUNC PyInit_date_ext(void)
{
    return PyModule_Create(&DateExt);
}
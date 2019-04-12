#include <Python.h>

#include "ctools.h"
#include "ctoolsdoc.h"

static PyMethodDef CtoolsMethods[] = {
    { "jump_consistent_hash", Ctools__jump_hash, METH_VARARGS, jump_consistent_hash__doc__ },
    { "strhash", Ctools__strhash, METH_VARARGS, strhash__doc__ },
    { "int8_to_datetime", Ctools__int8_to_datetime, METH_O, int8_to_datetime__doc__ },
    { NULL, NULL, 0, NULL },
};

static struct PyModuleDef ctools_module = {
    PyModuleDef_HEAD_INIT,
    "ctools",
    ctools__doc__, /* module documentation, may be NULL */
    -1, /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
    CtoolsMethods,
    NULL,
    NULL,
    NULL,
    NULL,
};

PyMODINIT_FUNC
PyInit_ctools(void)
{
    PyDateTime_IMPORT;
    return PyModule_Create(&ctools_module);
}

#include <Python.h>

#include "ctools.h"
#include "ctoolsdoc.h"
#include "lfu.h"

static PyMethodDef CtoolsMethods[] = {
    { "jump_consistent_hash", (PyCFunction)Ctools__jump_hash, METH_VARARGS, jump_consistent_hash__doc__ },
    { "strhash", (PyCFunction)Ctools__strhash, METH_VARARGS, strhash__doc__ },
    { "int8_to_datetime", (PyCFunction)Ctools__int8_to_datetime, METH_O, int8_to_datetime__doc__ },
    { NULL, NULL, 0, NULL },
};

static struct PyModuleDef ctools_module = {
    PyModuleDef_HEAD_INIT,
    "ctools", /* m_name */
    ctools__doc__, /* m_doc */
    -1, /* m_size */
    CtoolsMethods, /* m_methods */
    NULL, /* m_reload */
    NULL, /* m_traverse */
    NULL, /* m_clear */
    NULL, /* m_free */
};

PyMODINIT_FUNC
PyInit_ctools(void)
{
    PyDateTime_IMPORT;

    PyObject* m = PyModule_Create(&ctools_module);
    if (m == NULL)
        return NULL;

    LFUValueType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&LFUValueType) < 0)
        return NULL;

    LFUType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&LFUType) < 0)
        return NULL;

    Py_INCREF(&LFUValueType);
    Py_INCREF(&LFUType);
    PyModule_AddObject(m, "LFU", (PyObject*)&LFUType);
    return m;
}

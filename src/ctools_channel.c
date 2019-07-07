/* Copyright 2019 ko-han. All Rights Reserved.

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

#include <Python.h>
#include <time.h>

#include "ctools_config.h"
#include "ctools_macros.h"

typedef struct
{
  /* clang-format off */
  PyObject_VAR_HEAD
  PyObject** ob_item;
  /* clang-format on */
  /* cursor between [0, 2 * size), if cursor >= size, set flag to 1
   * if flag == -1; channel is closed
   * when wcursor == rcursor; rflag == wflag -> channel is empty
   *                          rflag != wflag -> channel is full
   *
   */
  Py_ssize_t wcursor;
  Py_ssize_t rcursor;
  int wflag;
  int rflag;
} Channel;

static PyTypeObject Channel_Type;

Channel*
Channel_New(Py_ssize_t size)
{
  Channel* op;
  Py_ssize_t i;
  assert(size > 0);
  /* check for overflow. */
  if (size > (PY_SSIZE_T_MAX - sizeof(Channel)) / sizeof(PyObject*)) {
    PyErr_NoMemory();
    return NULL;
  }

  op = PyObject_GC_New(Channel, &Channel_Type);
  RETURN_IF_NULL(op, NULL);

  op->ob_item = (PyObject**)PyMem_Calloc(size, sizeof(PyObject*));
  if (op->ob_item == NULL) {
    Py_DECREF(op);
    PyErr_NoMemory();
    return NULL;
  }

  for (i = 0; i < size; i++)
    op->ob_item[i] = NULL;

  op->wcursor = 0;
  op->rcursor = 0;
  op->wflag = 0;
  op->rflag = 0;
  Py_SIZE(op) = size;
  PyObject_GC_Track(op);
  return op;
}

static void
Channel_tp_dealloc(Channel* ob)
{
  Py_ssize_t i;
  Py_ssize_t len = Py_SIZE(ob);
  PyObject_GC_UnTrack(ob);
  /* clang-format off */
  Py_TRASHCAN_SAFE_BEGIN(ob)
  if (len > 0)
  /* clang-format on */
  {
    i = len;
    while (--i >= 0)
      Py_XDECREF(ob->ob_item[i]);
  }
  PyMem_FREE(ob->ob_item);
  PyObject_GC_Del(ob);
  /* clang-format off */
  Py_TRASHCAN_SAFE_END(ob)
  /* clang-format on */
}

static int
Channel_tp_traverse(Channel* o, visitproc visit, void* arg)
{
  Py_ssize_t i;

  for (i = Py_SIZE(o); --i >= 0;)
    Py_VISIT(o->ob_item[i]);
  return 0;
}

static int
Channel_tp_clear(Channel* op)
{
  Py_ssize_t i;
  PyObject** item = op->ob_item;
  if (item != NULL) {
    /* Because XDECREF can recursively invoke operations on
       this list, we make it empty first. */
    i = Py_SIZE(op);
    Py_SIZE(op) = 0;
    op->ob_item = NULL;
    op->wcursor = 0;
    op->rcursor = 0;
    op->wflag = 0;
    op->rflag = 0;
    while (--i >= 0) {
      Py_XDECREF(item[i]);
    }
    PyMem_FREE(item);
  }
  /* Never fails; the return value can be ignored.
     Note that there is no guarantee that the list is actually empty
     at this point, because XDECREF may have populated it again! */
  return 0;
}

static PyObject*
Channel_tp_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
  Py_ssize_t size;
  static char* kwlist[] = { "size", NULL };
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "n", kwlist, &size))
    return NULL;
  if (size <= 0) {
    PyErr_SetString(PyExc_ValueError, "size should be positive.");
    return NULL;
  }
  DEBUG_PRINTF("size is %ld", size);
  return (PyObject*)Channel_New(size);
}

static inline int
Channel_IsEmpty(Channel* self)
{
  if (Py_SIZE(self) == 1) {
    return self->ob_item[0] == NULL;
  }
  if (self->wcursor != self->rcursor) {
    return 0;
  }
  /* channel is empty */
  if (self->rflag == self->wflag) {
    return 1;
  }
  return 0;
}

static inline int
Channel_IsFull(Channel* self)
{
  if (Py_SIZE(self) == 1) {
    return self->ob_item[0] != NULL;
  }
  if (self->wcursor != self->rcursor) {
  }
  if (self->wcursor > self->rcursor) {
    return 0;
  }
  /* flag not equal means that channel is full */
  if (self->rflag != self->wflag) {
    return 1;
  }
  return 0;
}

static PyObject*
Channel_clear(Channel* self, PyObject* unused)
{
  PyObject** buffer = self->ob_item;
  Py_ssize_t size;
  size = Py_SIZE(self);
  for (Py_ssize_t i = 0; i < size; ++i) {
    if (buffer[i] != NULL) {
      Py_DECREF(buffer[i]);
      buffer[i] = NULL;
    }
  };
  Py_RETURN_NONE;
}

static PyObject*
Channel_recv(PyObject* self, PyObject* unused)
{
  PyObject* item;
  PyObject* rv;
  Py_ssize_t idx;
  Channel* ch = (Channel*)self;
  if (ch->rflag == -1) {
    PyErr_SetString(PyExc_RuntimeError, "channel is closed for receiving.");
    return NULL;
  }
  if ((rv = PyTuple_New(2)) == NULL) {
    return NULL;
  }

  if (Channel_IsEmpty(ch)) {
    Py_INCREF(Py_None);
    Py_INCREF(Py_False);
    PyTuple_SET_ITEM(rv, 0, Py_None);
    PyTuple_SET_ITEM(rv, 1, Py_False);
    return rv;
  }
  item = ch->ob_item[ch->rcursor];
  assert(item);
  ch->ob_item[ch->rcursor] = NULL;

  idx = ch->rcursor + 1;
  if (idx >= 2 * Py_SIZE(ch)) {
    idx -= 2 * Py_SIZE(ch);
    ch->rflag = 1;
  }
  ch->rcursor = idx;

  Py_INCREF(Py_True);
  PyTuple_SET_ITEM(rv, 0, item);
  PyTuple_SET_ITEM(rv, 1, Py_True);
  return rv;
}

static PyObject*
Channel_send(PyObject* self, PyObject* obj)
{
  Channel* ch = (Channel*)self;
  PyObject* item;
  Py_ssize_t idx;

  if (ch->wflag == -1) {
    PyErr_SetString(PyExc_RuntimeError, "channel is closed for sending.");
    return NULL;
  }

  if (Channel_IsFull(ch)) {
    Py_RETURN_FALSE;
  }
  item = ch->ob_item[ch->wcursor];
  Py_XDECREF(item);
  Py_INCREF(obj);
  ch->ob_item[ch->wcursor] = obj;
  idx = ch->wcursor + 1;
  if (idx >= 2 * Py_SIZE(ch)) {
    idx -= 2 * Py_SIZE(ch);
    ch->wflag = 1;
  }
  ch->wcursor = idx;
  Py_RETURN_TRUE;
}

static PyObject*
Channel_close(PyObject* self, PyObject* args, PyObject* kwds)
{
  Channel* ch;
  int write, read;
  write = 1;
  read = 1;
  static char* kwlist[] = { "send", "recv", NULL };
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "|pp", kwlist, &write, &read))
    return NULL;

  ch = (Channel*)self;
  if (write) {
    ch->wflag = -1;
  }
  if (read) {
    ch->rflag = -1;
  }
  Py_RETURN_NONE;
}

static PyObject*
Channel_safe_consume(PyObject* self, PyObject* callback)
{
  PyObject* item;
  PyObject* callback_rv;
  Channel* ch;
  Py_ssize_t idx;

  ch = (Channel*)self;

  if (!(PyCallable_Check(callback))) {
    PyErr_SetString(PyExc_TypeError, "object is not callable");
    return NULL;
  }

  if (ch->rflag == -1) {
    PyErr_SetString(PyExc_RuntimeError, "channel is closed for receiving.");
    return NULL;
  }

  if (Channel_IsEmpty(ch)) {
    Py_RETURN_FALSE;
  }

  item = ch->ob_item[ch->rcursor];
  assert(item);
  callback_rv = PyObject_CallFunction(callback, "O", item);
  RETURN_IF_NULL(callback_rv, NULL);
  Py_XDECREF(callback_rv);

  ch->ob_item[ch->rcursor] = NULL;
  idx = ch->rcursor + 1;
  if (idx >= 2 * Py_SIZE(ch)) {
    idx -= 2 * Py_SIZE(ch);
    ch->rflag = 1;
  }
  ch->rcursor = idx;
  return item;
}

static PyObject*
Channel_sendable(PyObject* self, PyObject* unused)
{
  Channel* ch = (Channel*)self;

  if (ch->wflag == -1) {
    Py_RETURN_FALSE;
  }

  if (Channel_IsFull(ch)) {
    Py_RETURN_FALSE;
  }
  Py_RETURN_TRUE;
}

static PyObject*
Channel_recvable(PyObject* self, PyObject* unused)
{
  Channel* ch = (Channel*)self;
  if (ch->rflag == -1) {
    Py_RETURN_FALSE;
  }

  if (Channel_IsEmpty(ch)) {
    Py_RETURN_FALSE;
  }
  Py_RETURN_TRUE;
}

static PyObject*
Channel_size(PyObject* self, PyObject* unused)
{
  return PyLong_FromLong(Py_SIZE(self));
}

static PyMethodDef Channel_methods[] = {
  { "send", (PyCFunction)Channel_send, METH_O, NULL },
  { "recv", (PyCFunction)Channel_recv, METH_NOARGS, NULL },
  { "clear", (PyCFunction)Channel_clear, METH_NOARGS, NULL },
  { "close", (PyCFunction)Channel_close, METH_VARARGS | METH_KEYWORDS, NULL },
  { "safe_consume", (PyCFunction)Channel_safe_consume, METH_O, NULL },
  { "sendable", (PyCFunction)Channel_sendable, METH_NOARGS, NULL },
  { "recvable", (PyCFunction)Channel_recvable, METH_NOARGS, NULL },
  { "size", (PyCFunction)Channel_size, METH_NOARGS, NULL },
  { NULL, NULL, 0, NULL } /* Sentinel */
};

static PyTypeObject Channel_Type = {
  /* clang-format off */
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  /* clang-format on */
  "Channel",                               /* tp_name */
  sizeof(Channel),                         /* tp_basicsize */
  0,                                       /* tp_itemsize */
  (destructor)Channel_tp_dealloc,          /* tp_dealloc */
  0,                                       /* tp_print */
  0,                                       /* tp_getattr */
  0,                                       /* tp_setattr */
  0,                                       /* tp_compare */
  0,                                       /* tp_repr */
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
  (traverseproc)Channel_tp_traverse,       /* tp_traverse */
  (inquiry)Channel_tp_clear,               /* tp_clear */
  0,                                       /* tp_richcompare */
  0,                                       /* tp_weaklistoffset */
  0,                                       /* tp_iter */
  0,                                       /* tp_iternext */
  Channel_methods,                         /* tp_methods */
  0,                                       /* tp_members */
  0,                                       /* tp_getset */
  0,                                       /* tp_base */
  0,                                       /* tp_dict */
  0,                                       /* tp_descr_get */
  0,                                       /* tp_descr_set */
  0,                                       /* tp_dictoffset */
  0,                                       /* tp_init */
  0,                                       /* tp_alloc */
  (newfunc)Channel_tp_new,                 /* tp_new */
  PyObject_GC_Del                          /* tp_free */
};

static struct PyModuleDef _ctools_channel_module = {
  PyModuleDef_HEAD_INIT,
  "_ctools_channel", /* m_name */
  NULL,              /* m_doc */
  -1,                /* m_size */
  NULL,              /* m_methods */
  NULL,              /* m_reload */
  NULL,              /* m_traverse */
  NULL,              /* m_clear */
  NULL,              /* m_free */
};

PyMODINIT_FUNC
PyInit__ctools_channel(void)
{
  if (PyType_Ready(&Channel_Type) < 0)
    return NULL;

  PyObject* m = PyModule_Create(&_ctools_channel_module);
  if (m == NULL)
    return NULL;

  Py_INCREF(&Channel_Type);

  if (PyModule_AddObject(m, "Channel", (PyObject*)&Channel_Type) == -1)
    return NULL;

  return m;
}
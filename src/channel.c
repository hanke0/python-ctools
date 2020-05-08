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

#include <Python.h>
#include <time.h>

typedef struct {
  /* clang-format off */
  PyObject_VAR_HEAD
  PyObject **ob_item;
  /* clang-format on */
  /* cursor between [0, 2 * size), if cursor >= size, set flag to 1
   * if flag <= 0; channel is closed
   * when sendx == recvx; rflag == sflag -> channel is empty
   *                      rflag != sflag -> channel is full
   *
   */
  int sendx;
  int recvx;
  char sflag;
  char rflag;
} CtsChannel;

static PyTypeObject Channel_Type;

static CtsChannel *Channel_New(int size) {
  CtsChannel *op;
  int i;
  assert(size > 0);
  /* check for overflow. */
  if ((Py_ssize_t)size > (Py_ssize_t)((PY_SSIZE_T_MAX - sizeof(CtsChannel)) /
                                      sizeof(PyObject *))) {
    PyErr_NoMemory();
    return NULL;
  }

  op = PyObject_GC_New(CtsChannel, &Channel_Type);
  ReturnIfNULL(op, NULL);

  op->ob_item = (PyObject **)PyMem_Calloc(size, sizeof(PyObject *));
  if (op->ob_item == NULL) {
    Py_DECREF(op);
    PyErr_NoMemory();
    return NULL;
  }

  for (i = 0; i < size; i++)
    op->ob_item[i] = NULL;

  op->sendx = 0;
  op->recvx = 0;
  if (IsPowerOf2(size)) {
    op->sflag = 3;
    op->rflag = 3;
  } else {
    op->sflag = 1;
    op->rflag = 1;
  }

  Py_SIZE(op) = size;
  PyObject_GC_Track(op);
  return op;
}

static void Channel_tp_dealloc(CtsChannel *ob) {
  int i;
  int len = Py_SIZE(ob);
  PyObject_GC_UnTrack(ob);
  /* clang-format off */
  Py_TRASHCAN_SAFE_BEGIN(ob)
      if (len > 0) {
        i = len;
        while (--i >= 0)
          Py_XDECREF(ob->ob_item[i]);
      }
      PyMem_FREE(ob->ob_item);
      PyObject_GC_Del(ob);
  Py_TRASHCAN_SAFE_END(ob)
  /* clang-format on */
}

static int Channel_tp_traverse(CtsChannel *o, visitproc visit, void *arg) {
  int i;
  for (i = Py_SIZE(o); --i >= 0;) {
    Py_VISIT(o->ob_item[i]);
  }
  return 0;
}

static int Channel_tp_clear(CtsChannel *op) {
  int i;
  PyObject **item = op->ob_item;
  if (item != NULL) {
    /* Because XDECREF can recursively invoke operations on
       this list, we make it empty first. */
    i = Py_SIZE(op);
    Py_SIZE(op) = 0;
    op->ob_item = NULL;
    op->sendx = 0;
    op->recvx = 0;
    op->sflag = 0;
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

static PyObject *Channel_tp_new(PyTypeObject *Py_UNUSED(type), PyObject *args,
                                PyObject *Py_UNUSED(kwds)) {
  int size = INT32_MAX;
  if (!PyArg_ParseTuple(args, "|i", &size)) {
    return NULL;
  }
  if (size <= 0) {
    PyErr_SetString(PyExc_ValueError, "size should be positive.");
    return NULL;
  }
  return (PyObject *)Channel_New(size);
}

static PyObject *Channel_clear(CtsChannel *self, PyObject *Py_UNUSED(u)) {
  PyObject **buffer = self->ob_item;
  Py_ssize_t size;
  size = Py_SIZE(self);
  for (Py_ssize_t i = 0; i < size; ++i) {
    if (buffer[i] != NULL) {
      Py_DECREF(buffer[i]);
      buffer[i] = NULL;
    }
  }
  Py_RETURN_NONE;
}

static void Channel_incr_sendx(CtsChannel *self) {
  assert(self->sflag > 0);
  if (self->sflag == 3) {
    self->sendx = (self->sendx + 1) & (2 * Py_SIZE(self) - 1);
    return;
  }
  int sendx;
  sendx = self->sendx + 1;
  if (sendx >= 2 * Py_SIZE(self)) {
    sendx %= 2 * Py_SIZE(self);
  }
  if (sendx >= Py_SIZE(self)) {
    self->sflag = 2;
  } else {
    self->sflag = 1;
  }
  self->sendx = sendx;
}

static void Channel_incr_recvx(CtsChannel *self) {
  assert(self->rflag > 0);
  if (self->rflag == 3) {
    self->recvx = (self->recvx + 1) & (2 * Py_SIZE(self) - 1);
    return;
  }
  int recvx;
  recvx = self->recvx + 1;
  if (recvx >= 2 * Py_SIZE(self)) {
    recvx %= 2 * Py_SIZE(self);
  }
  if (recvx >= Py_SIZE(self)) {
    self->rflag = 2;
  } else {
    self->rflag = 1;
  }
  self->recvx = recvx;
}

static int Channel_send_idx(CtsChannel *self) {
  if (self->sflag < 0) {
    return -2;
  }

  if (Py_SIZE(self) == 1) {
    if (self->ob_item[0] == NULL) {
      return 0;
    } else {
      return -1;
    }
  }

  /* size is 2**n */
  if (self->sflag == 3) {
    /* Is it full? */
    if (self->sendx == (self->recvx ^ Py_SIZE(self))) {
      return -1;
    }
    return self->sendx & (Py_SIZE(self) - 1);
  }

  if ((self->sendx % Py_SIZE(self)) != (self->recvx % Py_SIZE(self))) {
    return self->sendx % Py_SIZE(self);
  }

  /* flag not equal means that channel is full */
  if (abs(self->rflag) != self->sflag) {
    return -1;
  }
  return self->sendx % Py_SIZE(self);
}

static int Channel_recv_idx(CtsChannel *self) {
  if (self->rflag < 0) {
    return -2;
  }

  if (Py_SIZE(self) == 1) {
    if (self->ob_item[0] == NULL) {
      return -1;
    } else {
      return 0;
    }
  }

  /* size is 2**n */
  if (self->rflag == 3) {
    /* Is it empty? */
    if (self->sendx == self->recvx) {
      return -1;
    }
    return self->recvx & (Py_SIZE(self) - 1);
  }

  if ((self->sendx % Py_SIZE(self)) != (self->recvx % Py_SIZE(self))) {
    return self->recvx % Py_SIZE(self);
  }

  /* flag equal means that channel is empty */
  if (abs(self->sflag) == self->rflag) {
    return -1;
  }
  return self->recvx % Py_SIZE(self);
}

static PyObject *Channel_recv(PyObject *self, PyObject *Py_UNUSED(u)) {
  PyObject *item;
  PyObject *rv;
  int recvx;
  CtsChannel *ch = (CtsChannel *)self;

  recvx = Channel_recv_idx(ch);
  if (recvx == -2) {
    PyErr_SetString(PyExc_IndexError, "channel is closed for receiving.");
    return NULL;
  }

  if ((rv = PyTuple_New(2)) == NULL) {
    return NULL;
  }

  if (recvx == -1) {
    Py_INCREF(Py_None);
    Py_INCREF(Py_False);
    PyTuple_SET_ITEM(rv, 0, Py_None);
    PyTuple_SET_ITEM(rv, 1, Py_False);
    return rv;
  }
  item = ch->ob_item[recvx];
  assert(item);
  ch->ob_item[recvx] = NULL;
  Channel_incr_recvx(ch);

  Py_INCREF(Py_True);
  PyTuple_SET_ITEM(rv, 0, item);
  PyTuple_SET_ITEM(rv, 1, Py_True);
  return rv;
}

static PyObject *Channel_send(PyObject *self, PyObject *obj) {
  CtsChannel *ch = (CtsChannel *)self;
  int sendx;

  sendx = Channel_send_idx(ch);
  if (sendx == -2) {
    PyErr_SetString(PyExc_IndexError, "channel is closed for sending.");
    return NULL;
  }

  if (sendx == -1) {
    Py_RETURN_FALSE;
  }
  assert(ch->ob_item[sendx] == NULL);
  Py_INCREF(obj);
  ch->ob_item[sendx] = obj;
  Channel_incr_sendx(ch);
  Py_RETURN_TRUE;
}

static PyObject *Channel_close(PyObject *self, PyObject *args, PyObject *kwds) {
  CtsChannel *ch;
  int write, read;
  write = 1;
  read = 1;
  static char *kwlist[] = {"send", "recv", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "|pp", kwlist, &write, &read)) {
    return NULL;
  }

  ch = (CtsChannel *)self;
  if (write) {
    ch->sflag *= -1;
  }
  if (read) {
    ch->rflag *= -1;
  }
  Py_RETURN_NONE;
}

static PyObject *Channel_safe_consume(PyObject *self, PyObject *callback) {
  PyObject *item;
  PyObject *callback_rv;
  CtsChannel *ch = (CtsChannel *)self;
  int recvx;

  if (!(PyCallable_Check(callback))) {
    PyErr_SetString(PyExc_TypeError, "object is not callable");
    return NULL;
  }

  recvx = Channel_recv_idx(ch);

  if (recvx == -2) {
    PyErr_SetString(PyExc_RuntimeError, "channel is closed for receiving.");
    return NULL;
  }

  if (recvx == -1) {
    Py_RETURN_FALSE;
  }

  item = ch->ob_item[recvx];
  assert(item);
  Py_INCREF(item);
  callback_rv = PyObject_CallFunctionObjArgs(callback, item, NULL);
  if (callback_rv == NULL) {
    Py_DECREF(item);
    return NULL;
  }
  if (callback_rv == Py_False) {
    Py_DECREF(item);
    return callback_rv;
  }

  Py_DECREF(item);
  Py_DECREF(item);
  ch->ob_item[recvx] = NULL;
  Channel_incr_recvx(ch);
  return callback_rv;
}

static PyObject *Channel_sendable(PyObject *self, PyObject *Py_UNUSED(unused)) {
  CtsChannel *ch = (CtsChannel *)self;
  int sendx;
  sendx = Channel_send_idx(ch);

  if (sendx == -2 || sendx == -1) {
    Py_RETURN_FALSE;
  }
  Py_RETURN_TRUE;
}

static PyObject *Channel_recvable(PyObject *self, PyObject *Py_UNUSED(unused)) {
  CtsChannel *ch = (CtsChannel *)self;
  int recvx;
  recvx = Channel_recv_idx(ch);

  if (recvx == -2 || recvx == -1) {
    Py_RETURN_FALSE;
  }
  Py_RETURN_TRUE;
}

static PyObject *Channel_size(PyObject *self, PyObject *Py_UNUSED(unused)) {
  return PyLong_FromLong(Py_SIZE(self));
}

PyDoc_STRVAR(Channel_send__doc__, "send(obj, /)\n--\n\n"
                                  "Send an object to channel.\n"
                                  "\n"
                                  "Returns\n"
                                  "-------\n"
                                  "bool\n"
                                  "  Return True if send success else False\n"
                                  "\n"
                                  "Raises\n"
                                  "------\n"
                                  "IndexError\n"
                                  "  If the channel is closing for sending.\n");

PyDoc_STRVAR(Channel_recv__doc__,
             "recv()\n--\n\n"
             "Receive an object from channel.\n"
             "\n"
             "Returns\n"
             "-------\n"
             "o\n"
             "  Object that received. Return None if no items in channel.\n"
             "ok : bool\n"
             "  Return False if no items in channel else True.\n"
             "\n"
             "Raises\n"
             "------\n"
             "IndexError\n"
             "  If the channel is closing for receive.\n");

PyDoc_STRVAR(Channel_safe_consume__doc__,
             "safe_consume(fn, /)\n--\n\n"
             "Safe consume with a callable.\n\n"
             "Parameters\n"
             "----------\n"
             "fn : typing.Callable[[typing.Any], bool]\n"
             "  The `fn` receive an item as only one argument and must return "
             "True on success, False on fail.\n"
             "\n"
             "Returns\n"
             "-------\n"
             "bool\n"
             "  Return True if consume success, and False on fail.Return False "
             "if no item in the channel.\n");

static PyMethodDef Channel_methods[] = {
    {"send", (PyCFunction)Channel_send, METH_O, Channel_send__doc__},
    {"recv", (PyCFunction)Channel_recv, METH_NOARGS, Channel_recv__doc__},
    {
        "clear",
        (PyCFunction)Channel_clear,
        METH_NOARGS,
        "clear()\n--\n\nClear channel.",
    },
    {"close", (PyCFunction)Channel_close, METH_VARARGS | METH_KEYWORDS,
     "close(send=True, recv=True)\n--\n\nClose channel."},
    {
        "safe_consume",
        (PyCFunction)Channel_safe_consume,
        METH_O,
        Channel_safe_consume__doc__,
    },
    {"sendable", (PyCFunction)Channel_sendable, METH_NOARGS,
     "sendable()\n--\n\nReturn channel is available to send."},
    {"recvable", (PyCFunction)Channel_recvable, METH_NOARGS,
     "recvable()\n--\n\nReturn channel is available to receive."},
    {"size", (PyCFunction)Channel_size, METH_NOARGS,
     "size()\n--\n\nReturn the size of channel."},
    {NULL, NULL, 0, NULL} /* Sentinel */
};

PyDoc_STRVAR(Channel_Doc,
             "Channel(size=None, /)\n--\n\n"
             "A channel support sending and safe consuming.\n"
             "\n"
             "Parameters\n"
             "----------\n"
             "size : int, optional\n"
             "  The max size of channel, default to C ``MAX_INT32``.\n"
             "\n"
             "Examples\n"
             "--------\n"
             ">>> import ctools\n"
             ">>> ch = ctools.Channel(1)\n"
             ">>> ch.send('foo')\n"
             "True\n"
             ">>> ch.send('bar')\n"
             "False\n"
             ">>> ch.recv()\n"
             "('foo', True)\n"
             ">>> ch.recv()\n"
             "(None, False)\n");

static PyTypeObject Channel_Type = {
    /* clang-format off */
    PyVarObject_HEAD_INIT(NULL, 0)
    /* clang-format on */
    "ctools.Channel",                        /* tp_name */
    sizeof(CtsChannel),                      /* tp_basicsize */
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
    Channel_Doc,                             /* tp_doc */
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

EXTERN_C_START
int ctools_init_channel(PyObject *module) {
  if (PyType_Ready(&Channel_Type) < 0) {
    return -1;
  }

  Py_INCREF(&Channel_Type);
  if (PyModule_AddObject(module, "Channel", (PyObject *)&Channel_Type)) {
    Py_DECREF(&Channel_Type);
    return -1;
  }

  return 0;
}
EXTERN_C_END

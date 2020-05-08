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
#include "pydoc.h"

#include <Python.h>

#define RBTree_RED 1
#define RBTree_BLACK 0
#define RBTreeNode_CAST(node) ((CtsRBTreeNode *)(node))
#define RBTreeNode_Color(node) (RBTreeNode_CAST(node)->color)
#define RBTreeNode_SetRed(node) (RBTreeNode_Color(node) = RBTree_RED)
#define RBTreeNode_SetBlack(node) (RBTreeNode_Color(node) = RBTree_BLACK)
#define RBTreeNode_IsRed(node) (RBTreeNode_Color(node) == RBTree_RED)
#define RBTreeNode_IsBlack(node) (!RBTreeNode_IsRed(node))

/* clang-format off */
struct cts_rbtree_node {
  PyObject_HEAD
  PyObject *key;
  PyObject *value;
  struct cts_rbtree_node *left;
  struct cts_rbtree_node *right;
  struct cts_rbtree_node *parent;
  char color;
};
/* clang-format on */

typedef struct cts_rbtree_node CtsRBTreeNode;

/* clang-format off */
typedef struct {
  PyObject_HEAD
  CtsRBTreeNode *root;
  CtsRBTreeNode *sentinel;
  PyObject *cmpfunc;
  Py_ssize_t length;
} CtsRBTree;
/* clang-format on */

static PyTypeObject RBTreeNode_Type;
static PyTypeObject RBTree_Type;
static CtsRBTreeNode RBTree_SentinelNode;
#define RBTree_Sentinel (&RBTree_SentinelNode)

static CtsRBTreeNode *RBTreeNode_New(PyObject *key, PyObject *value) {
  CtsRBTreeNode *node;
  node = PyObject_GC_New(CtsRBTreeNode, &RBTreeNode_Type);
  ReturnIfNULL(node, NULL);
  Py_XINCREF(key);
  Py_XINCREF(value);
  node->key = key;
  node->value = value;
  node->parent = NULL;
  node->right = NULL;
  node->left = NULL;
  RBTreeNode_SetRed(node);
  PyObject_GC_Track(node);
  return node;
}

static int RBTreeNode_tp_traverse(CtsRBTreeNode *self, visitproc visit,
                                  void *arg) {
  Py_VISIT(self->key);
  Py_VISIT(self->value);
  Py_VISIT(self->left);
  Py_VISIT(self->right);
  /* children node doesn't need to visit parent node */
  return 0;
}

static int RBTreeNode_tp_clear(CtsRBTreeNode *self) {
  Py_CLEAR(self->key);
  Py_CLEAR(self->value);
  Py_CLEAR(self->left);
  Py_CLEAR(self->right);
  /* don't need to clear parent node */
  self->parent = NULL;
  return 0;
}

static void RBTreeNode_tp_dealloc(CtsRBTreeNode *self) {
  PyObject_GC_UnTrack(self);
  RBTreeNode_tp_clear(self);
  PyObject_GC_Del(self);
}

static PyObject *RBTreeNode_tp_new(PyTypeObject *Py_UNUSED(type),
                                   PyObject *args, PyObject *kwds) {
  PyObject *key;
  PyObject *value;
  static char *kwlist[] = {"key", "value", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist, &key, &value)) {
    return NULL;
  }
  return PyObjectCast(RBTreeNode_New(key, value));
}

static PyTypeObject RBTreeNode_Type = {
    /* clang-format off */
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "ctools.SortedMapNode",
    /* clang-format on */
    .tp_basicsize = sizeof(CtsRBTreeNode),
    .tp_dealloc = (destructor)RBTreeNode_tp_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = (traverseproc)RBTreeNode_tp_traverse,
    .tp_clear = (inquiry)RBTreeNode_tp_clear,
    .tp_new = (newfunc)RBTreeNode_tp_new,
};

static void RBTreeSentinel_dealloc(PyObject *Py_UNUSED(ignore)) {
  /* This should never get called, but we also don't want to SEGV if
   * we accidentally decref None out of existence.
   */
  Py_FatalError("dealloc SortedMapSentinel");
}

static PyObject *RBTreeSentinel_tp_repr(PyObject *self) {
  return PyUnicode_FromFormat("<SortedMapSentinel at %p>", self);
}

static PyObject *RBTreeSentinel_tp_new(PyObject *Py_UNUSED(ignore),
                                       PyObject *Py_UNUSED(unused)) {
  Py_INCREF(RBTree_Sentinel);
  return PyObjectCast(RBTree_Sentinel);
}

static PyTypeObject RBTreeSentinel_Type = {
    /* clang-format off */
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "ctools.SortedMapSentinel",
    /* clang-format on */
    .tp_basicsize = sizeof(CtsRBTreeNode),
    .tp_dealloc = (destructor)RBTreeSentinel_dealloc,
    .tp_repr = (reprfunc)RBTreeSentinel_tp_repr,
    .tp_str = (reprfunc)RBTreeSentinel_tp_repr,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = (newfunc)RBTreeSentinel_tp_new,
};

/* clang-format off */
static CtsRBTreeNode RBTree_SentinelNode = {
    PyObject_HEAD_INIT(&RBTreeSentinel_Type)
    .color = RBTree_BLACK, /* Sentinel should alway be black */
};
/* clang-format on */

#define RBTree_SetSentinel(tree, node)                                         \
  do {                                                                         \
    Py_INCREF(((CtsRBTree *)(tree))->sentinel);                                \
    (node) = ((CtsRBTree *)(tree))->sentinel;                                  \
  } while (0)

#define RBTree_EQ 0
#define RBTree_LT 1
#define RBTree_GT 2

static int rbtree_key_compare(CtsRBTree *tree, PyObject *key1, PyObject *key2) {
  int flag;
  long long cmp;
  PyObject *cmp_o = NULL;

  if (tree->cmpfunc == NULL) {
    flag = PyObject_RichCompareBool(key1, key2, Py_LT);
    if (flag < 0) {
      return -1;
    } else if (flag > 0) {
      return RBTree_LT;
    } else {
      flag = PyObject_RichCompareBool(key1, key2, Py_GT);
      if (flag < 0) {
        return -1;
      } else if (flag > 0) {
        return RBTree_GT;
      } else {
        return RBTree_EQ;
      }
    }
  }

  cmp_o = PyObject_CallFunctionObjArgs(tree->cmpfunc, key1, key2, NULL);
  if (!cmp_o) {
    flag = -1;
    goto finish;
  }
  cmp = PyLong_AsLongLong(cmp_o);
  if (cmp == -1 && PyErr_Occurred()) {
    PyErr_Format(
        PyExc_TypeError,
        "SortedMap cmp function return value expecting a integer but got %S",
        cmp_o);
    flag = -1;
    goto finish;
  }
  if (cmp > 0) {
    flag = RBTree_GT;
  } else if (cmp < 0) {
    flag = RBTree_LT;
  } else {
    flag = RBTree_EQ;
  }

finish:
  Py_XDECREF(cmp_o);
  return flag;
}

static int rbtree_node_compare(CtsRBTree *tree, CtsRBTreeNode *a,
                               CtsRBTreeNode *b) {
  assert(tree->sentinel != a);
  assert(tree->sentinel != b);
  return rbtree_key_compare(tree, a->key, b->key);
}
/*
 *          root            root              root            root
 *          / \             /  \              /  \            /  \
 *         x   d    ->     x    d   ->    x  y   d    ->     y    d
 *        / \             /\\            /\ / \             / \
 *       a   y           a b-y          a  b   c           x   c
 *          / \               \                           /\
 *         b   c               c                         a b
 *
 *  Note: [a, b, c d] means any sub tree.
 */
static void rbtree_left_rotate(CtsRBTree *tree, CtsRBTreeNode *x) {
  CtsRBTreeNode *y = x->right;
  CtsRBTreeNode *sentinel = tree->sentinel;
  /* First step */
  x->right = y->left;
  if (y->left != sentinel) {
    y->left->parent = x;
  }

  /* Second step */
  y->parent = x->parent;
  if (x->parent == sentinel) {
    tree->root = y;
  } else if (x == x->parent->left) {
    x->parent->left = y;
  } else {
    x->parent->right = y;
  }
  /* Third step */
  y->left = x;
  x->parent = y;
}

static void rbtree_right_rotate(CtsRBTree *tree, CtsRBTreeNode *x) {
  CtsRBTreeNode *y = x->left;
  CtsRBTreeNode *sentinel = tree->sentinel;
  assert(x != sentinel);

  x->left = y->right;
  if (y->right != sentinel) {
    y->right->parent = x;
  }
  y->parent = x->parent;
  if (x->parent == sentinel) {
    tree->root = y;
  } else if (x == x->parent->right) {
    x->parent->right = y;
  } else {
    x->parent->left = y;
  }
  y->right = x;
  x->parent = y;
}

static void rbtree_insert_fix(CtsRBTree *tree, CtsRBTreeNode *z) {
  CtsRBTreeNode *y;
  /* sentinel is always black */
  while (RBTreeNode_IsRed(z->parent)) {
    if (z->parent == z->parent->parent->left) {
      /* z is grandfather's left branch */
      y = z->parent->parent->right;
      if (RBTreeNode_IsRed(y)) {
        RBTreeNode_SetBlack(z->parent);       /* case 1 */
        RBTreeNode_SetBlack(y);               /* case 1 */
        RBTreeNode_SetRed(z->parent->parent); /* case 1 */
        z = z->parent->parent;
      } else {
        if (z == z->parent->right) {
          z = z->parent;
          rbtree_left_rotate(tree, z);
        }
        z->parent->color = RBTree_BLACK;
        z->parent->parent->color = RBTree_RED;
        rbtree_right_rotate(tree, z->parent->parent);
      }
    } else {
      /* z is grandfather's right branch */
      y = z->parent->parent->left;
      if (RBTreeNode_IsRed(y)) {
        RBTreeNode_SetBlack(z->parent);       /* case 1 */
        RBTreeNode_SetBlack(y);               /* case 1 */
        RBTreeNode_SetRed(z->parent->parent); /* case 1 */
        z = z->parent->parent;
      } else {
        if (z == z->parent->left) {
          z = z->parent;
          rbtree_right_rotate(tree, z);
        }
        RBTreeNode_SetBlack(z->parent);
        RBTreeNode_SetRed(z->parent->parent);
        rbtree_left_rotate(tree, z->parent->parent);
      }
    }
  }
  RBTreeNode_SetBlack(tree->root);
}

/* steal the reference of z */
static int RBTree_PutNode(CtsRBTree *tree, CtsRBTreeNode *z) {
  CtsRBTreeNode *sentinel = tree->sentinel;
  CtsRBTreeNode *x = tree->root;
  CtsRBTreeNode *y = sentinel;
  int flag;

  while (x != sentinel) {
    y = x;
    flag = rbtree_node_compare(tree, z, x);
    if (flag < 0) {
      goto fail;
    }
    if (flag == RBTree_LT) {
      x = x->left;
    } else if (flag == RBTree_GT) {
      x = x->right;
    } else { /* already has key, replace value */
      Py_INCREF(z->value);
      Py_SETREF(x->value, z->value);
      Py_DECREF(z);
      return 0;
    }
  }
  z->parent = y;
  if (y == sentinel) { /* tree is empty */
    Py_SETREF(tree->root, z);
    goto success;
  }

  /* insert in to sentinel node. */
  if (flag == RBTree_LT) {
    Py_SETREF(y->left, z);
  } else if (flag == RBTree_GT) {
    Py_SETREF(y->right, z);
  } else {
    abort();
  }

success:
  RBTree_SetSentinel(tree, z->left);
  RBTree_SetSentinel(tree, z->right);
  RBTreeNode_SetRed(z);
  tree->length++;
  rbtree_insert_fix(tree, z);
  return 0;
fail:
  Py_DECREF(z);
  return -1;
}

/* Don't steal references of key and value */
static int RBTree_Put(CtsRBTree *tree, PyObject *key, PyObject *value) {
  CtsRBTreeNode *node;
  node = RBTreeNode_New(key, value);
  ReturnIfNULL(node, -1);
  return RBTree_PutNode(tree, node);
}

/* borrowed reference */
static int rbtree_find(CtsRBTree *tree, PyObject *key, CtsRBTreeNode **node) {
  CtsRBTreeNode *x = tree->root;
  CtsRBTreeNode *sentinel = tree->sentinel;
  int flag;

  while (x != sentinel) {
    flag = rbtree_key_compare(tree, key, x->key);
    if (flag < 0) {
      return -1;
    }
    if (flag == RBTree_LT) {
      x = x->left;
    } else if (flag == RBTree_GT) {
      x = x->right;
    } else {
      *node = x;
      return 1;
    }
  }
  *node = NULL;
  return 0;
}

/* Return new reference */
static int RBTree_Get(CtsRBTree *tree, PyObject *key, PyObject **value) {
  CtsRBTreeNode *node;
  int flag;

  flag = rbtree_find(tree, key, &node);
  if (flag < 0) {
    return -1;
  } else if (flag > 0) {
    Py_INCREF(node->value);
    *value = node->value;
    return 1;
  }
  return 0;
}

/* must make sure node != sentinel before call this */
static CtsRBTreeNode *rbtree_min(CtsRBTreeNode *node, CtsRBTreeNode *sentinel) {
  while (node->left != sentinel) {
    node = node->left;
  }
  return node;
}

static CtsRBTreeNode *rbtree_next(CtsRBTree *tree, CtsRBTreeNode *node) {
  CtsRBTreeNode *root, *sentinel, *parent;

  sentinel = tree->sentinel;
  if (node->right != sentinel) {
    return rbtree_min(node->right, sentinel);
  }

  root = tree->root;

  for (;;) {
    parent = node->parent;

    if (node == root) {
      return NULL;
    }

    if (node == parent->left) {
      return parent;
    }

    node = parent;
  }
}

static void rbtree_transplant(CtsRBTree *tree, CtsRBTreeNode *u,
                              CtsRBTreeNode *v) {
  CtsRBTreeNode *sentinel = tree->sentinel;
  if (u->parent == sentinel) {
    tree->root = v;
  } else if (u == u->parent->left) {
    u->parent->left = v;
  } else {
    u->parent->right = v;
  }
  v->parent = u->parent;
}

static void rbtree_delete_fixup(CtsRBTree *tree, CtsRBTreeNode *x) {
  CtsRBTreeNode *root = tree->root;
  CtsRBTreeNode *w;
  while (x != root && RBTreeNode_IsBlack(x)) {
    if (x == x->parent->left) {
      w = x->parent->right;
      if (RBTreeNode_IsRed(w)) {
        RBTreeNode_SetBlack(w);
        RBTreeNode_SetRed(x->parent);
        rbtree_left_rotate(tree, x->parent);
        w = x->parent->right;
      }
      if (RBTreeNode_IsBlack(w->left) && RBTreeNode_IsBlack(w->right)) {
        RBTreeNode_SetRed(w);
        x = x->parent;
      } else {
        if (RBTreeNode_IsBlack(w->right)) {
          RBTreeNode_SetBlack(w->left);
          RBTreeNode_SetRed(w);
          rbtree_right_rotate(tree, w);
          w = x->parent->right;
        }
        w->color = x->parent->color;
        RBTreeNode_SetBlack(x->parent);
        RBTreeNode_SetBlack(w->right);
        rbtree_left_rotate(tree, x->parent);
        x = root;
      }
    } else {
      w = x->parent->left;
      if (RBTreeNode_IsRed(w)) {
        RBTreeNode_SetBlack(w);
        RBTreeNode_SetRed(x->parent);
        rbtree_right_rotate(tree, x->parent);
        w = x->parent->left;
      }
      if (RBTreeNode_IsBlack(w->left) && RBTreeNode_IsBlack(w->right)) {
        RBTreeNode_SetRed(w);
        x = x->parent;
      } else {
        if (RBTreeNode_IsBlack(w->left)) {
          RBTreeNode_SetBlack(w->right);
          RBTreeNode_SetRed(w);
          rbtree_left_rotate(tree, w);
          w = x->parent->left;
        }
        w->color = x->parent->color;
        RBTreeNode_SetBlack(x->parent);
        RBTreeNode_SetBlack(w->left);
        rbtree_right_rotate(tree, x->parent);
        x = root;
      }
    }
  }
  RBTreeNode_SetBlack(x);
}

static void rbtree_delete(CtsRBTree *tree, CtsRBTreeNode *z) {
  CtsRBTreeNode *y, *x, *sentinel;
  char y_origin_color;

  sentinel = tree->sentinel;
  y = z;
  y_origin_color = y->color;

  if (z->left == sentinel) {
    x = z->right;
    rbtree_transplant(tree, z, z->right);
  } else if (z->right == sentinel) {
    x = z->left;
    rbtree_transplant(tree, z, z->left);
  } else {
    y = rbtree_min(z->right, sentinel);
    y_origin_color = y->color;
    x = y->right;
    if (y->parent == z) {
      x->parent = y;
    } else {
      rbtree_transplant(tree, y, y->right);
      y->right = z->right;
      y->right->parent = y;
    }
    rbtree_transplant(tree, z, y);
    y->left = z->left;
    y->left->parent = y;
    y->color = z->color;
  }
  if (y_origin_color == RBTree_BLACK) {
    rbtree_delete_fixup(tree, x);
  }
}

static void RBTree_RemoveNode(CtsRBTree *tree, CtsRBTreeNode *node) {
  CtsRBTreeNode *root, *sentinel;

  root = tree->root;
  sentinel = tree->sentinel;
  assert(root != sentinel);
  assert(node != sentinel);
  if (root == node && tree->length == 1) {
    tree->root = sentinel;
  } else {
    rbtree_delete(tree, node);
  }

  Py_DECREF(sentinel);
  Py_DECREF(node->key);
  Py_DECREF(node->value);
  node->left = NULL;
  node->right = NULL;
  node->key = NULL;
  node->value = NULL;
  node->parent = NULL;
  Py_DECREF(node);
  tree->length--;
}

/* return new reference of value
 * if not in tree, do nothing, set value to NULL and return 0
 * value could be NULL, if value is NULL, delete silently */
static int RBTree_Remove(CtsRBTree *tree, PyObject *key, PyObject **value) {
  CtsRBTreeNode *node;
  int flag;
  flag = rbtree_find(tree, key, &node);
  if (flag < 0) {
    if (value) {
      *value = NULL;
    }
    return -1;
  } else if (flag == 0) {
    if (value) {
      *value = NULL;
    }
    return 0;
  } else {
    if (value) {
      Py_INCREF(node->value);
      *value = node->value;
    }
    RBTree_RemoveNode(tree, node);
    return 0;
  }
}

static CtsRBTree *RBTree_New(PyObject *cmp) {
  CtsRBTree *tree;
  if (cmp && !PyCallable_Check(cmp)) {
    PyErr_SetString(PyExc_TypeError, "cmp must be a callable object");
    return NULL;
  }

  tree = PyObject_GC_New(CtsRBTree, &RBTree_Type);
  ReturnIfNULL(tree, NULL);
  Py_XINCREF(cmp);
  Py_INCREF(RBTree_Sentinel);
  tree->sentinel = RBTree_Sentinel;
  Py_INCREF(tree->sentinel);
  tree->root = tree->sentinel;
  tree->cmpfunc = cmp;
  tree->length = 0;
  PyObject_GC_Track(tree);
  return tree;
}

static PyObject *RBTree_tp_new(PyTypeObject *Py_UNUSED(type), PyObject *args,
                               PyObject *Py_UNUSED(kwds)) {
  PyObject *o;
  PyObject *cmp = NULL;
  if (!PyArg_ParseTuple(args, "|O", &cmp)) {
    return NULL;
  }
  if (cmp == Py_None) {
    Py_DECREF(cmp);
    cmp = NULL;
  }
  o = PyObjectCast(RBTree_New(cmp));
  if (cmp) {
    Py_DECREF(cmp);
  }
  return o;
}

static int RBTree_tp_traverse(CtsRBTree *self, visitproc visit, void *arg) {
  Py_VISIT(self->root);
  Py_VISIT(self->sentinel);
  Py_VISIT(self->cmpfunc);
  return 0;
}

static int RBTree_tp_clear(CtsRBTree *self) {
  Py_CLEAR(self->root);
  Py_CLEAR(self->sentinel);
  Py_CLEAR(self->cmpfunc);
  return 0;
}

static void RBTree_tp_dealloc(CtsRBTree *self) {
  PyObject_GC_UnTrack(self);
  RBTree_tp_clear(self);
  PyObject_GC_Del(self);
}

static Py_ssize_t RBTree_size(CtsRBTree *tree) { return tree->length; }

/* __getitem__ */
static PyObject *RBTree_mp_subscript(CtsRBTree *tree, PyObject *key) {
  PyObject *value;
  int find;

  find = RBTree_Get(tree, key, &value);
  if (find < 0) {
    return NULL;
  } else if (find == 0) {
    return PyErr_Format(PyExc_KeyError, "%S", key);
  } else {
    return value;
  }
}

/* __setitem__, __delitem__ */
static int RBTree_mp_ass_sub(CtsRBTree *tree, PyObject *key, PyObject *value) {
  PyObject *removed;

  if (!value) {
    if (RBTree_Remove(tree, key, &removed)) {
      return -1;
    }
    if (!removed) {
      PyErr_Format(PyExc_KeyError, "%S", key);
      return -1;
    }
    Py_DECREF(removed);
    return 0;
  }

  return RBTree_Put(tree, key, value);
}

static PyMappingMethods RBTree_as_mapping = {
    (lenfunc)RBTree_size,             /*mp_length*/
    (binaryfunc)RBTree_mp_subscript,  /*mp_subscript*/
    (objobjargproc)RBTree_mp_ass_sub, /*mp_ass_subscript*/
};

static int RBTree_Contains(CtsRBTree *tree, PyObject *key) {
  PyObject *value;
  int find;
  find = RBTree_Get(tree, key, &value);
  if (find < 0) {
    return -1;
  } else if (find == 0) {
    return 0;
  } else {
    Py_DECREF(value);
    return 1;
  }
}

static PySequenceMethods RBTree_as_sequence = {
    0,                           /* sq_length */
    0,                           /* sq_concat */
    0,                           /* sq_repeat */
    0,                           /* sq_item */
    0,                           /* sq_slice */
    0,                           /* sq_ass_item */
    0,                           /* sq_ass_slice */
    (objobjproc)RBTree_Contains, /* sq_contains */
    0,                           /* sq_inplace_concat */
    0,                           /* sq_inplace_repeat */
};

#define RBTreeKeys 1
#define RBTreeValues 2
#define RBTreeItems 3

static PyObject *RBtree_iter(CtsRBTree *tree, int type) {
  PyObject *list;
  PyObject *tuple;
  CtsRBTreeNode *node;
  CtsRBTreeNode *sentinel;
  CtsRBTreeNode *root;
  Py_ssize_t top;

  list = PyList_New(tree->length);
  ReturnIfNULL(list, NULL);
  if (tree->length == 0) {
    return list;
  }

  root = tree->root;
  sentinel = tree->sentinel;
  top = -1;
  node = rbtree_min(root, sentinel);
  for (; node; node = rbtree_next(tree, node)) {
    switch (type) {
    case RBTreeKeys:
      Py_INCREF(node->key);
      if (PyList_SetItem(list, ++top, node->key)) {
        Py_DECREF(node->key);
        Py_DECREF(list);
        return NULL;
      }
      break;
    case RBTreeValues:
      Py_INCREF(node->value);
      if (PyList_SetItem(list, ++top, node->value)) {
        Py_DECREF(node->value);
        Py_DECREF(list);
        return NULL;
      }
      break;
    case RBTreeItems:
      tuple = PyTuple_New(2);
      if (!tuple) {
        Py_DECREF(list);
        return NULL;
      }
      Py_INCREF(node->key);
      if (PyTuple_SetItem(tuple, 0, node->key)) {
        Py_DECREF(node->key);
        Py_DECREF(list);
        Py_DECREF(tuple);
        return NULL;
      }
      Py_INCREF(node->value);
      if (PyTuple_SetItem(tuple, 1, node->value)) {
        Py_DECREF(node->value);
        Py_DECREF(list);
        Py_DECREF(tuple);
        return NULL;
      }
      if (PyList_SetItem(list, ++top, tuple)) {
        Py_DECREF(list);
        Py_DECREF(tuple);
        return NULL;
      }
      break;
    default:
      abort();
    }
  }
  return list;
}

static PyObject *RBTree_keys(CtsRBTree *tree, PyObject *Py_UNUSED(ignore)) {
  return RBtree_iter(tree, RBTreeKeys);
}

static PyObject *RBTree_values(CtsRBTree *tree, PyObject *Py_UNUSED(ignore)) {
  return RBtree_iter(tree, RBTreeValues);
}

static PyObject *RBTree_items(CtsRBTree *tree, PyObject *Py_UNUSED(ignore)) {
  return RBtree_iter(tree, RBTreeItems);
}

static PyObject *RBTree_get(CtsRBTree *tree, PyObject *args, PyObject *kwds) {
  PyObject *key = NULL;
  PyObject *_default = NULL;
  PyObject *value = NULL;
  int find;

  static char *kwlist[] = {"key", "default", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O", kwlist, &key,
                                   &_default)) {
    return NULL;
  }
  find = RBTree_Get(tree, key, &value);
  if (find < 0) {
    return NULL;
  }
  if (!find) {
    if (!_default) {
      Py_RETURN_NONE;
    }
    Py_INCREF(_default);
    return _default;
  }
  return value;
}

static PyObject *RBTree_setdefault(CtsRBTree *tree, PyObject *args,
                                   PyObject *kwds) {
  PyObject *key = NULL;
  PyObject *_default = NULL;
  PyObject *value = NULL;
  int find;

  static char *kwlist[] = {"key", "default", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O", kwlist, &key,
                                   &_default)) {
    return NULL;
  }
  find = RBTree_Get(tree, key, &value);
  if (find < 0) {
    return NULL;
  }
  if (!find) {
    if (!_default) {
      _default = Py_None;
    }
    Py_INCREF(_default);
    if (RBTree_Put(tree, key, _default)) {
      Py_DECREF(_default);
      return NULL;
    }
    value = _default;
  }
  return value;
}

static PyObject *RBTree_setnx(CtsRBTree *tree, PyObject *args, PyObject *kwds) {
  PyObject *key = NULL;
  PyObject *_default = NULL;
  PyObject *value = NULL;
  int find;

  static char *kwlist[] = {"key", "fn", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O", kwlist, &key,
                                   &_default)) {
    return NULL;
  }
  find = RBTree_Get(tree, key, &value);
  if (find < 0) {
    return NULL;
  }
  if (!find) {
    if (!_default) {
      _default = Py_None;
      Py_INCREF(_default);
    } else {
      _default = PyObject_CallFunctionObjArgs(_default, key, NULL);
      ReturnIfNULL(_default, NULL);
    }
    if (RBTree_Put(tree, key, _default)) {
      Py_DECREF(_default);
      return NULL;
    }
    value = _default;
  }
  return value;
}

static PyObject *RBTree_update(CtsRBTree *tree, PyObject *args,
                               PyObject *kwargs) {
  PyObject *key, *value;
  PyObject *arg = NULL;
  Py_ssize_t pos = 0;

  if ((PyArg_ParseTuple(args, "|O", &arg))) {
    if (arg && PyDict_Check(arg)) {
      while (PyDict_Next(arg, &pos, &key, &value))
        if (RBTree_Put(tree, key, value)) {
          return NULL;
        }
    }
  }

  if (kwargs != NULL && PyArg_ValidateKeywordArguments(kwargs)) {
    while (PyDict_Next(kwargs, &pos, &key, &value))
      if (RBTree_Put(tree, key, value)) {
        return NULL;
      }
  }

  Py_RETURN_NONE;
}

static PyObject *RBTree_clear(CtsRBTree *tree, PyObject *Py_UNUSED(ignore)) {
  CtsRBTreeNode *root;
  if (tree->length == 0) {
    Py_RETURN_NONE;
  }
  root = tree->root;
  Py_INCREF(tree->sentinel);
  tree->root = tree->sentinel;
  Py_DECREF(root);
  tree->length = 0;
  Py_RETURN_NONE;
}

static PyObject *RBTree_pop(CtsRBTree *tree, PyObject *args, PyObject *kwds) {
  PyObject *key, *value;
  PyObject *default_ = NULL;

  static char *kwlist[] = {"key", "default", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O", kwlist, &key,
                                   &default_)) {
    return NULL;
  }
  if (RBTree_Remove(tree, key, &value)) {
    return NULL;
  }

  if (!value) {
    if (!default_) {
      Py_RETURN_NONE;
    }
    Py_INCREF(default_);
    return default_;
  } else {
    return value;
  }
}

static PyObject *RBTree_popitem(CtsRBTree *tree, PyObject *Py_UNUSED(ignore)) {
  PyObject *key, *value, *tuple;
  CtsRBTreeNode *node;

  if (tree->root == tree->sentinel) {
    PyErr_SetString(PyExc_KeyError, "popitem(): mapping is empty");
    return NULL;
  }
  node = rbtree_min(tree->root, tree->sentinel);
  key = node->key;
  value = node->value;
  tuple = PyTuple_New(2);
  ReturnIfNULL(tuple, NULL);
  Py_INCREF(key);
  if (PyTuple_SetItem(tuple, 0, key)) {
    Py_DECREF(key);
    Py_DECREF(tuple);
    return NULL;
  }
  Py_INCREF(value);
  if (PyTuple_SetItem(tuple, 1, value)) {
    Py_DECREF(value);
    Py_DECREF(tuple);
    return NULL;
  }
  RBTree_RemoveNode(tree, node);
  return tuple;
}

static void rbtree_print_help(CtsRBTreeNode *node, CtsRBTreeNode *sentinel,
                              PyObject *list, int depth, int left) {
  PyObject *level;
  PyObject *tuple;

  if (node == sentinel) {
    return;
  }

  if (PyList_Size(list) == depth) {
    level = PyList_New(0);
    PyList_Append(list, level);
    Py_DECREF(level);
  } else {
    level = PyList_GetItem(list, depth);
  }
  tuple = PyTuple_New(3);
  PyTuple_SetItem(tuple, 0, PyUnicode_FromFormat("%S", node->parent->key));
  PyTuple_SetItem(tuple, 1, PyUnicode_FromFormat("%S", node->key));

  if (RBTreeNode_IsBlack(node)) {
    if (left) {
      PyTuple_SetItem(tuple, 2, PyUnicode_FromString("+*"));
    } else {
      PyTuple_SetItem(tuple, 2, PyUnicode_FromString("-*"));
    }
  } else {
    if (left) {
      PyTuple_SetItem(tuple, 2, PyUnicode_FromString("+o"));
    } else {
      PyTuple_SetItem(tuple, 2, PyUnicode_FromString("-o"));
    }
  }
  PyList_Append(level, tuple);
  Py_DECREF(tuple);
  rbtree_print_help(node->left, sentinel, list, ++depth, 1);
  rbtree_print_help(node->right, sentinel, list, depth, 0);
}

static PyObject *RBTree__print(CtsRBTree *tree, PyObject *Py_UNUSED(ignore)) {
  PyObject *list, *level;
  Py_ssize_t size;

  list = PyList_New(0);
  rbtree_print_help(tree->root, tree->sentinel, list, 0, 1);
  size = PyList_Size(list);
  for (int i = 0; i < size; i++) {
    level = PyList_GetItem(list, i);
    fprintf(stderr, "%d. ", i);
    PyObject_Print(level, stderr, Py_PRINT_RAW);
    fprintf(stderr, "\n");
  }
  fflush(stderr);
  Py_DECREF(list);
  Py_RETURN_NONE;
}

static PyObject *rbtree_build_tuple(CtsRBTreeNode *node) {
  PyObject *tuple;
  tuple = PyTuple_New(2);
  if (!tuple) {
    return NULL;
  }
  Py_INCREF(node->key);
  if (PyTuple_SetItem(tuple, 0, node->key)) {
    Py_DECREF(node->key);
    Py_DECREF(tuple);
    return NULL;
  }
  Py_INCREF(node->value);
  if (PyTuple_SetItem(tuple, 1, node->value)) {
    Py_DECREF(node->value);
    Py_DECREF(tuple);
    return NULL;
  }
  return tuple;
}

static PyObject *RBTree_max(CtsRBTree *tree, PyObject *Py_UNUSED(ignore)) {
  CtsRBTreeNode *node, *sentinel;

  sentinel = tree->sentinel;
  if (tree->root == sentinel) {
    PyErr_SetString(PyExc_KeyError, "max(): mapping is empty");
    return NULL;
  }
  node = tree->root;
  while (node->right != sentinel) {
    node = node->right;
  }
  return rbtree_build_tuple(node);
}

static PyObject *RBTree_min(CtsRBTree *tree, PyObject *Py_UNUSED(ignore)) {
  CtsRBTreeNode *node, *sentinel;

  sentinel = tree->sentinel;
  if (tree->root == sentinel) {
    PyErr_SetString(PyExc_KeyError, "max(): mapping is empty");
    return NULL;
  }
  node = tree->root;
  while (node->left != sentinel) {
    node = node->left;
  }
  return rbtree_build_tuple(node);
}

PyMethodDef RBTree_methods[] = {
    {"_print", (PyCFunction)RBTree__print, METH_NOARGS,
     "Print tree, for debug."},
    {
        "keys",
        (PyCFunction)RBTree_keys,
        METH_NOARGS,
        "keys()\n--\n\nIterate sorted keys.",
    },
    {
        "values",
        (PyCFunction)RBTree_values,
        METH_NOARGS,
        "values()\n--\n\nIterate over values in order of keys.",
    },
    {
        "items",
        (PyCFunction)RBTree_items,
        METH_NOARGS,
        "items()\n--\n\nIterate items in order of keys.",
    },
    {
        "get",
        (PyCFunction)RBTree_get,
        METH_VARARGS | METH_KEYWORDS,
        "get(key, default=None)\n--\n\nReturn value if find else default.",
    },
    {
        "setdefault",
        (PyCFunction)RBTree_setdefault,
        METH_VARARGS | METH_KEYWORDS,
        "setdefault(key, default=None)\n--\n\nReturn value if find else "
        "default and put default to mapping.",
    },
    {
        "setnx",
        (PyCFunction)RBTree_setnx,
        METH_VARARGS | METH_KEYWORDS,
        USUAL_SETNX_METHOD_DOC,
    },
    {
        "update",
        (PyCFunction)RBTree_update,
        METH_VARARGS | METH_KEYWORDS,
        "update(mp, **kwargs)\n--\n\nLike dict.update, but only accept dict.",
    },
    {
        "clear",
        (PyCFunction)RBTree_clear,
        METH_NOARGS,
        "clear()\n--\n\nClear mapping.",
    },
    {
        "pop",
        (PyCFunction)RBTree_pop,
        METH_VARARGS | METH_KEYWORDS,
        "pop(key, default=None)\n--\n\nPop an item, if key not exists, return "
        "default.",
    },
    {
        "popitem",
        (PyCFunction)RBTree_popitem,
        METH_NOARGS,
        "popitem()\n--\n\nRemove and return some (key, value) pair"
        "as a 2-tuple; but raise KeyError if mapping is empty. Ensure key is "
        "the smallest in the mapping.",
    },
    {
        "max",
        (PyCFunction)RBTree_max,
        METH_NOARGS,
        "max()\n--\n\nReturn maximum (key, value) pair"
        "as a 2-tuple; but raise KeyError if mapping is empty.",
    },
    {
        "min",
        (PyCFunction)RBTree_min,
        METH_NOARGS,
        "min()\n--\n\nReturn minimum (key, value) pair"
        "as a 2-tuple; but raise KeyError if mapping is empty.",
    },
    {NULL, NULL, 0, NULL},
};

static PyObject *RBTree_tp_richcompare(PyObject *self, PyObject *other,
                                       int opid) {
  switch (opid) {
  case Py_EQ:
    if (self == other) {
      Py_RETURN_TRUE;
    } else {
      Py_RETURN_FALSE;
    }
  default:
    Py_RETURN_FALSE;
  }
}

static PyObject *RBTree_tp_iter(CtsRBTree *tree) {
  PyObject *list;
  PyObject *it;

  list = RBTree_keys(tree, NULL);
  ReturnIfNULL(list, NULL);
  it = PySeqIter_New(list);
  Py_DECREF(list);
  return it;
}

PyDoc_STRVAR(RBTree__doc__,
             "SortedMap(cmp=None, /)\n--\n\n"
             "A sorted map base on red-black tree.\n\n"
             ".. versionadded:: 0.2.0\n"
             "\n"
             "Parameters\n"
             "----------\n"
             "cmp : typing.Callable[[typing.Any], int], optional\n"
             "  A optional callable receive two keys, that\n\n"
             "  return negative integer if `k1 < k2`, \n\n"
             "  return positive integer if `k1 > k2`, \n\n"
             "  return 0 if `k1 == k2`.\n\n"
             "  It's every similar to standard C library qsort comparator.\n"
             "\n"
             "Examples\n"
             "--------\n"
             ">>> import ctools\n"
             ">>> foo = ctools.SortedMap()\n"
             ">>> foo[1] = 1\n"
             ">>> foo[2] = 2\n"
             ">>> foo[1]\n"
             "1\n"
             ">>> foo.max()\n"
             "(2, 2)\n"
             ">>> foo.min()\n"
             "(1, 1)\n"
             ">>> foo.keys()\n"
             "[1, 2]\n"
             ">>> foo.popitem()\n"
             "(1, 1)\n");

static PyTypeObject RBTree_Type = {
    /* clang-format off */
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "ctools.SortedMap",
    /* clang-format on */
    .tp_basicsize = sizeof(CtsRBTree),
    .tp_dealloc = (destructor)RBTree_tp_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = (traverseproc)RBTree_tp_traverse,
    .tp_clear = (inquiry)RBTree_tp_clear,
    .tp_new = (newfunc)RBTree_tp_new,
    .tp_as_mapping = &RBTree_as_mapping,
    .tp_as_sequence = &RBTree_as_sequence,
    .tp_methods = RBTree_methods,
    .tp_richcompare = (richcmpfunc)RBTree_tp_richcompare,
    .tp_iter = (getiterfunc)RBTree_tp_iter,
    .tp_doc = RBTree__doc__,
};

EXTERN_C_START
int ctools_init_rbtree(PyObject *module) {
  if (PyType_Ready(&RBTreeNode_Type) < 0) {
    return -1;
  }
  if (PyType_Ready(&RBTreeSentinel_Type)) {
    return -1;
  }
  if (PyType_Ready(&RBTree_Type) < 0) {
    return -1;
  }

  Py_INCREF(&RBTreeNode_Type);
  if (PyModule_AddObject(module, "SortedMapNode",
                         (PyObject *)&RBTreeNode_Type) < 0) {
    Py_DECREF(&RBTreeNode_Type);
    return -1;
  }
  Py_INCREF(RBTree_Sentinel);
  if (PyModule_AddObject(module, "SortedMapSentinel",
                         PyObjectCast(RBTree_Sentinel))) {
    Py_DECREF(RBTree_Sentinel);
    Py_DECREF(&RBTreeNode_Type);
    return -1;
  }

  Py_INCREF(&RBTree_Type);
  if (PyModule_AddObject(module, "SortedMap", (PyObject *)&RBTree_Type) < 0) {
    Py_DECREF(RBTree_Sentinel);
    Py_DECREF(&RBTreeNode_Type);
    Py_DECREF(&RBTree_Type);
    return -1;
  }
  return 0;
}
EXTERN_C_END

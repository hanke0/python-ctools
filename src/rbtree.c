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

#define RBRED 1
#define RBBLACK 0

#define RBTreeNode_CAST(x) ((RBTreeNode *)(x))
#define RBTreeNode_Color(node) (RBTreeNode_CAST(node)->color)
#define RBTreeNode_Key(node) (RBTreeNode_CAST(node)->key)
#define RBTreeNode_Value(node) (RBTreeNode_CAST(node)->value)
#define RBTreeNode_Parent(node) (RBTreeNode_CAST(node)->parent)
#define RBTreeNode_Left(node) (RBTreeNode_CAST(node)->left)
#define RBTreeNode_Right(node) (RBTreeNode_CAST(node)->right)

#define RBTreeNode_SetColor(node, v) (RBTreeNode_CAST(node)->color = (v))
#define RBTreeNode_SetKey(node, v)                                             \
  (RBTreeNode_CAST(node)->key = PyObjectCast(v))
#define RBTreeNode_SetValue(node, v)                                           \
  (RBTreeNode_CAST(node)->value = PyObjectCast(v))
#define RBTreeNode_SetParent(node, v)                                          \
  (RBTreeNode_CAST(node)->parent = PyObjectCast(v))
#define RBTreeNode_SetLeft(node, v)                                            \
  (RBTreeNode_CAST(node)->left = PyObjectCast(v))
#define RBTreeNode_SetRight(node, v)                                           \
  (RBTreeNode_CAST(node)->right = PyObjectCast(v))

#define RBTreeNode_SetRed(node) (RBTreeNode_Color(node) = RBRED)
#define RBTreeNode_SetBlack(node) (RBTreeNode_Color(node) = RBBLACK)
#define RBTreeNode_IsRed(node) (RBTreeNode_Color(node) == RBRED)
#define RBTreeNode_IsBlack(node) (RBTreeNode_Color(node) != RBRED)

struct _rbtree_node {
  /* clang-format off */
  PyObject_HEAD
  PyObject *key;
  /* clang-format on */
  PyObject *value;
  struct _rbtree_node *left;
  struct _rbtree_node *right;
  struct _rbtree_node *parent;
  char color;
};

typedef struct _rbtree_node RBTreeNode;

typedef struct _rbtree {
  /* clang-format off */
  PyObject_HEAD
  struct _rbtree_node *root;
  struct _rbtree_node *sentinel;
  PyObject *cmpfunc;
  /* clang-format on */
} RBTree;

static PyTypeObject RBTreeNode_Type;
static PyTypeObject RBTree_Type;
static RBTreeNode *RBSentinel;

static RBTreeNode *RBTreeNode_New(PyObject *key, PyObject *value) {
  RBTreeNode *node;
  node = PyObject_GC_New(RBTreeNode, &RBTreeNode_Type);
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

static int RBTreeNode_tp_traverse(RBTreeNode *self, visitproc visit,
                                  void *arg) {
  Py_VISIT(self->key);
  Py_VISIT(self->value);
  Py_VISIT(self->left);
  Py_VISIT(self->right);
  Py_VISIT(self->parent);
  return 0;
}

static int RBTreeNode_tp_clear(RBTreeNode *self) {
  Py_CLEAR(self->key);
  Py_CLEAR(self->value);
  Py_CLEAR(self->left);
  Py_CLEAR(self->right);
  Py_CLEAR(self->parent);
  return 0;
}

static void RBTreeNode_tp_dealloc(RBTreeNode *self) {
  PyObject_GC_UnTrack(self);
  RBTreeNode_tp_clear(self);
  PyObject_GC_Del(self);
}

static PyObject *RBTreeNode_tp_new(PyTypeObject *type, PyObject *args,
                                   PyObject *kwds) {
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
    .tp_basicsize = sizeof(RBTreeNode),
    .tp_dealloc = (destructor)RBTreeNode_tp_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = (traverseproc)RBTreeNode_tp_traverse,
    .tp_clear = (inquiry)RBTreeNode_tp_clear,
    .tp_new = (newfunc)RBTreeNode_tp_new,
};

static void rbsentinel_dealloc(PyObject *ignore) {
  /* This should never get called, but we also don't want to SEGV if
   * we accidentally decref None out of existence.
   */
  Py_FatalError("deallocating RBSentinel");
}

#define RBSentinel_SET(v)                                                      \
  do {                                                                         \
    Py_INCREF(RBSentinel);                                                     \
    assert(Py_REFCNT(RBSentinel) > 0);                                         \
    (v) = RBSentinel;                                                          \
  } while (0)
#define RBSentinel_DEC(sentinel)                                               \
  do {                                                                         \
    Py_DECREF(sentinel);                                                       \
    (sentinel) = NULL;                                                         \
  assert(Py_REFCNT(RBSentinel) > 0) while (0)

/*
 *          root          root           root              root
 *          / \           /  \         /  /  \             / \
 *       node  d    ->  node  d  ->  node tmp d   ->     tmp  d
 *        / \           / \ \         /\   \            /  \
 *       a  tmp        a  b tmp      a  b   c         node  c
 *          /\               \                         /\
 *         b  c               c                       a b
 *
 *  Note: [a, b, c d] means any sub tree.
 */
static void left_rotate(RBTree *tree, RBTreeNode *x) {
  RBTreeNode *y = x->right;
  RBTreeNode *sentinel = tree->sentinel;
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

static void right_rotate(RBTree *tree, RBTreeNode *y) {
  RBTreeNode *tmp = y->left;
  RBTreeNode *sentinel = tree->sentinel;

  y->left = tmp->right;
  if (tmp->right != sentinel) {
    tmp->right->parent = y;
  }

  tmp->parent = y->parent;
  if (tmp->right == sentinel) {
    tree->root = tmp;
  } else if (y == y->parent->left) {
    y->parent->left = tmp;
  } else {
    y->parent->right = tmp;
  }
  tmp->right = y;
  y->parent = tmp;
}

static int rbtree_insert_fix(RBTree *tree, RBTreeNode *z) {
  RBTreeNode *y;

  /* sentinel is always black */
  while (RBTreeNode_IsRed(z->parent)) {
    if (z->parent == z->parent->parent->left) {
      y = z->parent->parent->right;
      if (RBTreeNode_IsRed(y)) {
        RBTreeNode_SetBlack(z->parent);       /* case 1 */
        RBTreeNode_SetBlack(y);               /* case 1 */
        RBTreeNode_SetRed(z->parent->parent); /* case 1 */
        z = z->parent->parent;
      } else if (z == z->parent->right) {
        z = z->parent;
        left_rotate(tree, z);
      }
      z->parent->color = RBBLACK;
      z->parent->parent->color = RBRED;
      right_rotate(tree, z->parent->parent);
    } else {
      if (z->parent == z->parent->parent->right) {
        y = z->parent->parent->left;
        if (RBTreeNode_IsRed(y)) {
          RBTreeNode_SetBlack(z->parent);       /* case 1 */
          RBTreeNode_SetBlack(y);               /* case 1 */
          RBTreeNode_SetRed(z->parent->parent); /* case 1 */
          z = z->parent->parent;
        } else if (z == z->parent->right) {
          z = z->parent;
          right_rotate(tree, z);
        }
        z->parent->color = RBBLACK;
        z->parent->parent->color = RBRED;
        left_rotate(tree, z->parent->parent);
      }
    }
  }
  tree->root->color = RBBLACK;
  return 0;
}

#define RBTree_EQ 0
#define RBTree_LT 1
#define RBTree_GT 2

static int rbtree_node_compare(RBTree *tree, RBTreeNode *a, RBTreeNode *b) {
  int flag;
  long long cmp;
  PyObject *cmp_o = NULL;
  RBTreeNode *sentinel = tree->sentinel;
  assert(sentinel != a);
  assert(sentinel != b);
  if (tree->cmpfunc == NULL) {
    flag = PyObject_RichCompareBool(a->key, b->key, Py_LT);
    if (flag < 0) {
      return -1;
    } else if (flag > 0) {
      return RBTree_LT;
    } else {
      flag = PyObject_RichCompareBool(a->key, b->key, Py_GT);
      if (flag < 0) {
        return -1;
      } else if (flag > 0) {
        return RBTree_GT;
      } else {
        return RBTree_EQ;
      }
    }
  }

  cmp_o = PyObject_CallFunctionObjArgs(tree->cmpfunc, a->key, b->key, a->value,
                                       b->value, NULL);
  if (!cmp_o) {
    flag = -1;
    goto finish;
  }
  cmp = PyLong_AsLongLong(cmp_o);
  if (cmp == -1 && PyErr_Occurred()) {
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

/* steal the reference of z */
static int rbtree_insert(RBTree *tree, RBTreeNode *z) {
  RBTreeNode **root = &tree->root;
  RBTreeNode *sentinel = tree->sentinel;
  RBTreeNode *x = *root;
  RBTreeNode *y = sentinel;
  int flag;

  if (*root == sentinel) {
    Py_XINCREF(sentinel);
    Py_XINCREF(sentinel);
    z->parent = sentinel;
    z->left = sentinel;
    z->right = sentinel;
    RBTreeNode_SetBlack(z);
    *root = z;
    return 0;
  }

  while (x != sentinel) {
    y = x;
    flag = rbtree_node_compare(tree, z, x);
    if (flag < 0) {
      goto fail;
    }
    if (flag == RBTree_LT) {
      x = RBTreeNode_Left(x);
    } else {
      x = RBTreeNode_Right(x);
    }
  }
  Py_INCREF(y);
  z->parent = y;
  if (y == sentinel) {
    Py_DECREF(tree->root);
    tree->root = z;
    goto success;
  }
  flag = rbtree_node_compare(tree, z, y);
  if (flag < 0) {
    Py_DECREF(y);
    goto fail;
  } else if (flag == RBTree_LT) {
    Py_DECREF(y->left);
    y->left = z;
  } else {
    Py_DECREF(y->right);
    y->right = z;
  }
success:
  RBSentinel_SET(z->right);
  RBSentinel_SET(z->left);
  RBTreeNode_SetRed(z);
  Py_INCREF(z->parent);
  return rbtree_insert_fix(tree, z);
fail:
  Py_DECREF(z);
  return -1;
}

static RBTree *RBTree_New(PyObject *cmp) {
  RBTree *tree;
  if (!PyCallable_Check(cmp)) {
    PyErr_SetString(PyExc_TypeError, "cmp must be a callable object");
    return NULL;
  }

  tree = PyObject_GC_New(RBTree, &RBTree_Type);
  ReturnIfNULL(tree, NULL);
  Py_XINCREF(cmp);
  tree->root = RBSentinel;
  tree->sentinel = RBSentinel;
  tree->cmpfunc = cmp;
  PyObject_GC_Track(tree);
  return tree;
}

static PyObject *RBTree_tp_new(PyTypeObject *type, PyObject *args,
                               PyObject *kwds) {
  PyObject *cmp;
  static char *kwlist[] = {"cmp", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &cmp)) {
    return NULL;
  }
  return PyObjectCast(RBTree_New(cmp));
}

static int RBTree_tp_traverse(RBTree *self, visitproc visit, void *arg) {
  Py_VISIT(self->root);
  /* No need to visit sentinel  */
  return 0;
}

static int RBTree_tp_clear(RBTree *self) {
  Py_CLEAR(self->root);
  return 0;
}

static void RBTree_tp_dealloc(RBTree *self) {
  PyObject_GC_UnTrack(self);
  RBTree_tp_clear(self);
  PyObject_GC_Del(self);
}

static PyTypeObject RBTree_Type = {
    /* clang-format off */
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "ctools.SortedMap",
    /* clang-format on */
    .tp_basicsize = sizeof(RBTree),
    .tp_dealloc = (destructor)RBTree_tp_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = (traverseproc)RBTree_tp_traverse,
    .tp_clear = (inquiry)RBTree_tp_clear,
    .tp_new = (newfunc)RBTree_New,
};

int ctools_init_rbtree(PyObject *module) {
  if (PyType_Ready(&RBTreeNode_Type) < 0) {
    return -1;
  }
  RBSentinel = RBTreeNode_New(NULL, NULL);
  if (RBSentinel == NULL) {
    return -1;
  }
  RBTreeNode_SetBlack(RBSentinel); /* sentinel's color is always black */
  RBSentinel->ob_base.ob_type->tp_dealloc = rbsentinel_dealloc;

  Py_INCREF(&RBTreeNode_Type);
  if (PyModule_AddObject(module, "SortedMapNode",
                         (PyObject *)&RBTreeNode_Type) < 0) {
    Py_DECREF(&RBTreeNode_Type);
    return -1;
  }

  if (PyType_Ready(&RBTree_Type) < 0) {
    return -1;
  }
  Py_INCREF(&RBTree_Type);
  if (PyModule_AddObject(module, "SortedMap", (PyObject *)&RBTree_Type) < 0) {
    Py_DECREF(&RBTree_Type);
    return -1;
  }
  return 0;
}
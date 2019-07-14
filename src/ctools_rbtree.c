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

#include "ctools_config.h"
#include "ctools_macros.h"

#define RBRED 1
#define RBBLACK 0

#define SET_RBNODE_RED(node) (node)->color = RBRED
#define SET_RBNODE_BLACK(node) (node)->color = RBBLACK
#define IS_RBNODE_BLACK(node) ((node)->color == RBBLACK)
#define IS_RBNODE_RED(node) (!(node)->color == RBBLACK)
#define RBTREE_NODE_CAST(x) ((RBTreeNode*)(x))

typedef struct _rbtree_node
{
  /* clang-format off */
  PyObject_HEAD
  PyObject* key;
  /* clang-format on */
  PyObject* value;
  PyObject* left;
  PyObject* right;
  PyObject* parent;
  char color;
} RBTreeNode;

typedef struct _rbtree
{
  /* clang-format off */
  PyObject_HEAD
  PyObject* root;
  /* clang-format on */
} RBTree;

static PyTypeObject RBTreeNode_Type;
static PyTypeObject RBTree_Type;

static RBTreeNode*
RBTreeNode_New(PyObject* key, PyObject* value)
{
  RBTreeNode* node;
  node = PyObject_GC_New(RBTreeNode, &RBTree_Type);
  RETURN_IF_NULL(node, NULL);
  Py_INCREF(key);
  Py_INCREF(value);
  node->key = key;
  node->value = value;
  node->parent = NULL;
  node->right = NULL;
  node->left = NULL;
  SET_RBNODE_RED(node);
  PyObject_GC_Track(node);
  return node;
}

static int
RBTreeNode_tp_traverse(RBTreeNode* self, visitproc visit, void* arg)
{
  Py_VISIT(self->key);
  Py_VISIT(self->value);
  Py_VISIT(self->left);
  Py_VISIT(self->right);
  Py_VISIT(self->parent);
  return 0;
}

static int
RBTreeNode_tp_clear(RBTreeNode* self)
{
  Py_CLEAR(self->key);
  Py_CLEAR(self->value);
  Py_CLEAR(self->left);
  Py_CLEAR(self->right);
  Py_CLEAR(self->parent);
  return 0;
}

static void
RBTreeNode_tp_dealloc(RBTreeNode* self)
{
  PyObject_GC_UnTrack(self);
  RBTreeNode_tp_clear(self);
  PyObject_GC_Del(self);
}

static PyObject*
RBTreeNode_tp_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
  PyObject* key;
  PyObject* value;
  static char* kwlist[] = { "key", "value", NULL };
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist, &key, &value))
    return NULL;
  return PYOBJECT_CAST(RBTreeNode_New(key, value));
}

static PyTypeObject RBTreeNode_Type = {
  /* clang-format off */
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  .tp_name = "RBTreeNone",
  /* clang-format on */
  .tp_basicsize = sizeof(RBTreeNode),
  .tp_dealloc = (destructor)RBTreeNode_tp_dealloc,
  .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
  .tp_traverse = (traverseproc)RBTreeNode_tp_traverse,
  .tp_clear = (inquiry)RBTreeNode_tp_clear,
  .tp_new = (newfunc)RBTreeNode_tp_new,
};

static void
rbsentinel_dealloc(PyObject* ignore)
{
  /* This should never get called, but we also don't want to SEGV if
   * we accidentally decref None out of existence.
   */
  Py_FatalError("deallocating RBSentinel");
}

static RBTreeNode _RBSentinelStruct;

#define RBSentinel (&_RBSentinelStruct)

#define RBSentinel_SET(v)                                                      \
  Py_INCREF(RBSentinel);                                                       \
  (v) = RBSentinel
#define RBSentinel_DEC(sentinel)                                               \
  Py_DECREF(sentinel);                                                         \
  (sentinel) = NULL

static PyObject*
rbsentinel_new(PyTypeObject* type, PyObject* args, PyObject* kwargs)
{
  if (PyTuple_GET_SIZE(args) || (kwargs && PyDict_GET_SIZE(kwargs))) {
    PyErr_SetString(PyExc_TypeError, "RBSentinel takes no arguments");
    return NULL;
  }
  Py_INCREF(RBSentinel);
  return PYOBJECT_CAST(RBSentinel);
}

static PyTypeObject _RBSentinel_Type = {
  /* clang-format off */
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  .tp_name = "RBSentinel",
  /* clang-format on */
  .tp_basicsize = sizeof(RBTreeNode),
  .tp_dealloc = (destructor)rbsentinel_dealloc,
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_new = (newfunc)rbsentinel_new,
};

static RBTreeNode _RBSentinelStruct = {
  1,                 /* ref count */
  &_RBSentinel_Type, /*  ob type */
  NULL,              /* key*/
  NULL,              /* value */
  NULL,              /* left */
  NULL,              /* right */
  NULL,              /* parent */
  RBBLACK,           /* color */
};

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
static void
left_rotate(PyObject** root, RBTreeNode* node)
{
  RBTreeNode* tmp = RBTREE_NODE_CAST(node->right);
  RBTreeNode* sentinel = RBSentinel;
  /* First step */
  node->right = tmp->left;
  if (tmp->left != sentinel)
    RBTREE_NODE_CAST(tmp->left)->parent = PYOBJECT_CAST(node);

  /* Second step */
  tmp->parent = node->parent;
  if (node->parent == sentinel)
    *root = PYOBJECT_CAST(tmp);
  else if (PYOBJECT_CAST(node) == (RBTREE_NODE_CAST(node->parent)->left))
    RBTREE_NODE_CAST(node->parent)->left = PYOBJECT_CAST(tmp);
  else
    RBTREE_NODE_CAST(node->parent)->right = PYOBJECT_CAST(tmp);

  /* Third step */
  tmp->left = PYOBJECT_CAST(node);
  node->parent = PYOBJECT_CAST(tmp);
}

static void
right_rotate(PyObject** root, RBTreeNode* node, PyObject* sentinel)
{
  RBTreeNode* tmp = RBTREE_NODE_CAST(node->left);

  node->left = tmp->right;
  if (tmp->right != sentinel)
    RBTREE_NODE_CAST(tmp->right)->parent = PYOBJECT_CAST(node);

  tmp->parent = node->parent;
  if (tmp->right == sentinel)
    *root = PYOBJECT_CAST(tmp);
  else if (PYOBJECT_CAST(node) == RBTREE_NODE_CAST(node->parent)->left)
    RBTREE_NODE_CAST(node->parent)->left = PYOBJECT_CAST(tmp);
  else
    RBTREE_NODE_CAST(node->parent)->right = PYOBJECT_CAST(tmp);

  tmp->right = PYOBJECT_CAST(node);
  node->parent = PYOBJECT_CAST(tmp);
}

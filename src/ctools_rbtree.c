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
  unsigned char color;
} RBTreeNode;

typedef struct _rbtree
{
  PyObject* root;
  PyObject* sentinel;
} RBTree;

#define RBRED 1
#define RBBLACK 0

#define SET_RBNODE_RED(node) (node)->color = RBRED
#define SET_RBNODE_BLACK(node) (node)->color = RBBLACK
#define IS_RBNODE_BLACK(node) ((node)->color == RBBLACK)
#define IS_RBNODE_RED(node) (!(node)->color == RBBLACK)
#define RBTREE_NODE_CVT(x) ((RBTreeNode*)(x))

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
left_rotate(PyObject** root, RBTreeNode* node, PyObject* sentinel)
{
  RBTreeNode* tmp = RBTREE_NODE_CVT(node->right);

  /* First step */
  node->right = tmp->left;
  if (tmp->left != sentinel)
    RBTREE_NODE_CVT(tmp->left)->parent = PYOBJECT_CVT(node);

  /* Second step */
  tmp->parent = node->parent;
  if (node->parent == sentinel)
    *root = PYOBJECT_CVT(tmp);
  else if (PYOBJECT_CVT(node) == (RBTREE_NODE_CVT(node->parent)->left))
    RBTREE_NODE_CVT(node->parent)->left = PYOBJECT_CVT(tmp);
  else
    RBTREE_NODE_CVT(node->parent)->right = PYOBJECT_CVT(tmp);

  /* Third step */
  tmp->left = PYOBJECT_CVT(node);
  node->parent = PYOBJECT_CVT(tmp);
}

static void
right_rotate(PyObject** root, RBTreeNode* node, PyObject* sentinel)
{
  RBTreeNode* tmp = RBTREE_NODE_CVT(node->left);

  node->left = tmp->right;
  if (tmp->right != sentinel)
    RBTREE_NODE_CVT(tmp->right)->parent = PYOBJECT_CVT(node);

  tmp->parent = node->parent;
  if (tmp->right == sentinel)
    *root = PYOBJECT_CVT(tmp);
  else if (PYOBJECT_CVT(node) == RBTREE_NODE_CVT(node->parent)->left)
    RBTREE_NODE_CVT(node->parent)->left = PYOBJECT_CVT(tmp);
  else
    RBTREE_NODE_CVT(node->parent)->right = PYOBJECT_CVT(tmp);

  tmp->right = PYOBJECT_CVT(node);
  node->parent = PYOBJECT_CVT(tmp);
}

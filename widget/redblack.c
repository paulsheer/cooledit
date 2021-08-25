/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */

#include "inspect.h"
#include "redblack.h"

static void __redblack_rotate_left (struct redblack_node *node, struct redblack_root *root)
{E_
    struct redblack_node *right = node->right;
    struct redblack_node *parent = node->parent;

    if ((node->right = right->left))
        right->left->parent = node;
    right->left = node;

    right->parent = parent;

    if (parent) {
        if (node == parent->left)
            parent->left = right;
        else
            parent->right = right;
    } else
        root->redblack_node = right;
    node->parent = right;
}

static void __redblack_rotate_right (struct redblack_node *node, struct redblack_root *root)
{E_
    struct redblack_node *left = node->left;
    struct redblack_node *parent = node->parent;

    if ((node->left = left->right))
        left->right->parent = node;
    left->right = node;

    left->parent = parent;

    if (parent) {
        if (node == parent->right)
            parent->right = left;
        else
            parent->left = left;
    } else
        root->redblack_node = left;
    node->parent = left;
}

void redblack_insert_color (struct redblack_node *node, struct redblack_root *root)
{E_
    struct redblack_node *parent, *gparent;

/*    assert (node->color); */

    while ((parent = node->parent) && parent->color == REDBLACK_RED) {
        gparent = parent->parent;

        if (parent == gparent->left) {
            {
                register struct redblack_node *uncle = gparent->right;
                if (uncle && uncle->color == REDBLACK_RED) {
                    uncle->color = REDBLACK_BLACK;
                    parent->color = REDBLACK_BLACK;
                    gparent->color = REDBLACK_RED;
                    node = gparent;
                    continue;
                }
            }

            if (parent->right == node) {
                register struct redblack_node *tmp;
                __redblack_rotate_left (parent, root);
                tmp = parent;
                parent = node;
                node = tmp;
            }

            parent->color = REDBLACK_BLACK;
            gparent->color = REDBLACK_RED;
            __redblack_rotate_right (gparent, root);
        } else {
            {
                register struct redblack_node *uncle = gparent->left;
                if (uncle && uncle->color == REDBLACK_RED) {
                    uncle->color = REDBLACK_BLACK;
                    parent->color = REDBLACK_BLACK;
                    gparent->color = REDBLACK_RED;
                    node = gparent;
                    continue;
                }
            }

            if (parent->left == node) {
                register struct redblack_node *tmp;
                __redblack_rotate_right (parent, root);
                tmp = parent;
                parent = node;
                node = tmp;
            }

            parent->color = REDBLACK_BLACK;
            gparent->color = REDBLACK_RED;
            __redblack_rotate_left (gparent, root);
        }
    }

    root->redblack_node->color = REDBLACK_BLACK;
}


static void __redblack_erase_color (struct redblack_node *node, struct redblack_node *parent, struct redblack_root *root)
{E_
    struct redblack_node *other;

    while ((!node || node->color == REDBLACK_BLACK) && node != root->redblack_node) {
        if (parent->left == node) {
            other = parent->right;
            if (other->color == REDBLACK_RED) {
                other->color = REDBLACK_BLACK;
                parent->color = REDBLACK_RED;
                __redblack_rotate_left (parent, root);
                other = parent->right;
            }
            if ((!other->left || other->left->color == REDBLACK_BLACK)
                && (!other->right || other->right->color == REDBLACK_BLACK)) {
                other->color = REDBLACK_RED;
                node = parent;
                parent = node->parent;
            } else {
                if (!other->right || other->right->color == REDBLACK_BLACK) {
                    struct redblack_node *o_left;
                    if ((o_left = other->left))
                        o_left->color = REDBLACK_BLACK;
                    other->color = REDBLACK_RED;
                    __redblack_rotate_right (other, root);
                    other = parent->right;
                }
                other->color = parent->color;
                parent->color = REDBLACK_BLACK;
                if (other->right)
                    other->right->color = REDBLACK_BLACK;
                __redblack_rotate_left (parent, root);
                node = root->redblack_node;
                break;
            }
        } else {
            other = parent->left;
            if (other->color == REDBLACK_RED) {
                other->color = REDBLACK_BLACK;
                parent->color = REDBLACK_RED;
                __redblack_rotate_right (parent, root);
                other = parent->left;
            }
            if ((!other->left || other->left->color == REDBLACK_BLACK)
                && (!other->right || other->right->color == REDBLACK_BLACK)) {
                other->color = REDBLACK_RED;
                node = parent;
                parent = node->parent;
            } else {
                if (!other->left || other->left->color == REDBLACK_BLACK) {
                    register struct redblack_node *o_right;
                    if ((o_right = other->right))
                        o_right->color = REDBLACK_BLACK;
                    other->color = REDBLACK_RED;
                    __redblack_rotate_left (other, root);
                    other = parent->left;
                }
                other->color = parent->color;
                parent->color = REDBLACK_BLACK;
                if (other->left)
                    other->left->color = REDBLACK_BLACK;
                __redblack_rotate_right (parent, root);
                node = root->redblack_node;
                break;
            }
        }
    }
    if (node)
        node->color = REDBLACK_BLACK;
}

void redblack_erase (struct redblack_node *node, struct redblack_root *root)
{E_
    struct redblack_node *child, *parent;
    int color;

/*    assert (node->color); */

    if (!node->left)
        child = node->right;
    else if (!node->right)
        child = node->left;
    else {
        struct redblack_node *old = node, *left;

        node = node->right;
        while ((left = node->left))
            node = left;
        child = node->right;
        parent = node->parent;
        color = node->color;

        if (child)
            child->parent = parent;
        if (parent == old) {
            parent->right = child;
            parent = node;
        } else
            parent->left = child;

        node->parent = old->parent;
        node->color = old->color;
        node->right = old->right;
        node->left = old->left;

        if (old->parent) {
            if (old->parent->left == old)
                old->parent->left = node;
            else
                old->parent->right = node;
        } else
            root->redblack_node = node;

        old->left->parent = node;
        if (old->right)
            old->right->parent = node;
        goto color;
    }

    parent = node->parent;
    color = node->color;

    if (child)
        child->parent = parent;
    if (parent) {
        if (parent->left == node)
            parent->left = child;
        else
            parent->right = child;
    } else
        root->redblack_node = child;

  color:
    if (color == REDBLACK_BLACK)
        __redblack_erase_color (child, parent, root);
}

struct redblack_node *redblack_first (struct redblack_root *root)
{E_
    struct redblack_node *n;

    n = root->redblack_node;
    if (!n)
        return (struct redblack_node *) 0UL;
    while (n->left)
        n = n->left;
    return n;
}


struct redblack_node *redblack_last (struct redblack_root *root)
{E_
    struct redblack_node *n;

    n = root->redblack_node;
    if (!n)
        return (struct redblack_node *) 0UL;
    while (n->right)
        n = n->right;
    return n;
}


struct redblack_node *redblack_next (struct redblack_node *node)
{E_
    struct redblack_node *parent;

    if (node->right) {
        node = node->right;
        while (node->left)
            node = node->left;
        return node;
    }

    while ((parent = node->parent) && node == parent->right)
        node = parent;

    return parent;
}


struct redblack_node *redblack_prev (struct redblack_node *node)
{E_
    struct redblack_node *parent;

    if (node->left) {
        node = node->left;
        while (node->right)
            node = node->right;
        return node;
    }

    while ((parent = node->parent) && node == parent->left)
        node = parent;

    return parent;
}

void redblack_replace_node (struct redblack_node *victim, struct redblack_node *new_node, struct redblack_root *root)
{E_
    struct redblack_node *parent = victim->parent;

    if (parent) {
        if (victim == parent->left)
            parent->left = new_node;
        else
            parent->right = new_node;
    } else {
        root->redblack_node = new_node;
    }
    if (victim->left)
        victim->left->parent = new_node;
    if (victim->right)
        victim->right->parent = new_node;

    *new_node = *victim;
}

void redblack_link_node (struct redblack_node *node, struct redblack_node *parent, struct redblack_node **redblack_link)
{E_
    node->color = REDBLACK_RED;
    node->parent = parent;
    node->left = node->right = (struct redblack_node *) 0UL;
    *redblack_link = node;
}


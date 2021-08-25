/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */


#define	REDBLACK_RED		0x1
#define	REDBLACK_BLACK	        0x2

struct redblack_node {
    struct redblack_node *right;
    struct redblack_node *left;
    struct redblack_node *parent;
    int color;
};

struct redblack_root {
    struct redblack_node *redblack_node;
};

void redblack_insert_color (struct redblack_node *, struct redblack_root *);
void redblack_erase (struct redblack_node *, struct redblack_root *);

struct redblack_node *redblack_next (struct redblack_node *);
struct redblack_node *redblack_prev (struct redblack_node *);
struct redblack_node *redblack_first (struct redblack_root *);
struct redblack_node *redblack_last (struct redblack_root *);

void redblack_replace_node (struct redblack_node *victim, struct redblack_node *noo, struct redblack_root *root);
void redblack_link_node (struct redblack_node *node, struct redblack_node *parent, struct redblack_node **redblack_link);

#define DECL_FUNCTION_REDBLACK_ADD(name, cmpfn, offset)                 \
    void name (struct redblack_root *root, void *record)                \
    {                                                                   \
        struct redblack_node **p = &root->redblack_node;                \
        struct redblack_node *parent = 0, *node;                        \
        void *i;                                                        \
                                                                        \
        while (*p) {                                                    \
	    int cmp;                                                    \
                                                                        \
	    parent = *p;                                                \
	    i = (void *) (((char *) parent) - offset);                  \
                                                                        \
	    cmp = cmpfn (i, record);                                    \
	    if (cmp > 0) {                                              \
	        p = &(*p)->left;                                        \
	    } else {                                                    \
	        p = &(*p)->right;                                       \
	    }                                                           \
        }                                                               \
                                                                        \
        node = (struct redblack_node *) (((char *) record) + offset);   \
        redblack_link_node (node, parent, p);                           \
        redblack_insert_color (node, root);                             \
    }

/* returns 1 on error */
#define DECL_FUNCTION_REDBLACK_ADD_UNIQUE(name, cmpfn, offset)          \
    int name (struct redblack_root *root, void *record)                 \
    {                                                                   \
        struct redblack_node **p = &root->redblack_node;                \
        struct redblack_node *parent = 0, *node;                        \
        void *i;                                                        \
                                                                        \
        while (*p) {                                                    \
	    int cmp;                                                    \
                                                                        \
	    parent = *p;                                                \
	    i = (void *) (((char *) parent) - offset);                  \
                                                                        \
	    cmp = cmpfn (i, record);                                    \
	    if (cmp > 0) {                                              \
	        p = &(*p)->left;                                        \
	    } else if (cmp < 0) {                                       \
	        p = &(*p)->right;                                       \
	    } else {                                                    \
	        return 1;                                               \
	    }                                                           \
        }                                                               \
                                                                        \
        node = (struct redblack_node *) (((char *) record) + offset);   \
        redblack_link_node (node, parent, p);                           \
        redblack_insert_color (node, root);                             \
                                                                        \
        return 0;                                                       \
    }




#include "avltree.h"
#include "avltree_imp.h"
#include "fsalloc.h"
#include "avltree_version.h"

const void *avl_elem_get_key(avl_elem_t *element)
{
    return element->key;
}

const void *avl_elem_get_value(avl_elem_t *element)
{
    return element->value;
}

avl_tree_t *make_avl_tree_2(int (*cmp)(const void *, const void *, void *),
                            void *obj)
{
    avl_tree_t *tree = fsalloc(sizeof *tree);
    tree->cmp = cmp;
    tree->obj = obj;
    tree->root = NULL;
    tree->size = 0;
    return tree;
}

avl_tree_t *make_avl_tree(int (*cmp)(const void *, const void *))
{
    return make_avl_tree_2((void *) cmp, NULL);
}

static void destroy_subtree(avl_elem_t *element)
{
    if (element != NULL) {
        destroy_subtree(element->left);
        destroy_subtree(element->right);
        fsfree(element);
    }
}

void destroy_avl_tree(avl_tree_t *tree)
{
    destroy_subtree(tree->root);
    fsfree(tree);
}

static avl_elem_t *make_element(const void *key, const void *value)
{
    avl_elem_t *element = fsalloc(sizeof *element);
    element->key = key;
    element->value = value;
    element->parent = element->left = element->right = NULL;
    element->balance = 0;
    return element;
}

void destroy_avl_element(avl_elem_t *element)
{
    fsfree(element);
}

size_t avl_tree_size(avl_tree_t *tree)
{
    return tree->size;
}

avl_elem_t *avl_tree_get(avl_tree_t *tree, const void *key)
{
    avl_elem_t *element = tree->root;
    void *obj = tree->obj;
    while (element) {
        int cmp = tree->cmp(key, element->key, obj);
        if (cmp == 0)
            return element;
        if (cmp > 0)
            element = element->right;
        else element = element->left;
    }
    return NULL;
}

static avl_elem_t *previous_descendant(avl_elem_t *element)
{
    avl_elem_t *adj = element->left;
    if (adj == NULL)
        return NULL;
    while (adj->right != NULL)
        adj = adj->right;
    return adj;
}

avl_elem_t *avl_tree_previous(avl_elem_t *element)
{
    avl_elem_t *adj = previous_descendant(element);
    if (adj != NULL)
        return adj;
    while (element->parent != NULL && element->parent->left == element)
        element = element->parent;
    return element->parent;
}

avl_elem_t *avl_tree_get_before(avl_tree_t *tree, const void *key)
{
    avl_elem_t *element = tree->root;
    avl_elem_t *candidate = NULL;
    void *obj = tree->obj;
    while (element) {
        int cmp = tree->cmp(key, element->key, obj);
        if (cmp <= 0)
            element = element->left;
        else {
            candidate = element;
            element = element->right;
        }
    }
    return candidate;
}

avl_elem_t *avl_tree_get_on_or_before(avl_tree_t *tree, const void *key)
{
    avl_elem_t *element = tree->root;
    avl_elem_t *candidate = NULL;
    void *obj = tree->obj;
    while (element) {
        int cmp = tree->cmp(key, element->key, obj);
        if (cmp < 0)
            element = element->left;
        else {
            candidate = element;
            element = element->right;
        }
    }
    return candidate;
}

static avl_elem_t *next_descendant(avl_elem_t *element)
{
    avl_elem_t *adj = element->right;
    if (adj == NULL)
        return NULL;
    while (adj->left != NULL)
        adj = adj->left;
    return adj;
}

avl_elem_t *avl_tree_next(avl_elem_t *element)
{
    avl_elem_t *adj = next_descendant(element);
    if (adj != NULL)
        return adj;
    while (element->parent != NULL && element->parent->right == element)
        element = element->parent;
    return element->parent;
}

avl_elem_t *avl_tree_get_after(avl_tree_t *tree, const void *key)
{
    avl_elem_t *element = tree->root;
    avl_elem_t *candidate = NULL;
    void *obj;
    while (element) {
        int cmp = tree->cmp(key, element->key, obj);
        if (cmp >= 0)
            element = element->right;
        else {
            candidate = element;
            element = element->left;
        }
    }
    return candidate;
}

avl_elem_t *avl_tree_get_on_or_after(avl_tree_t *tree, const void *key)
{
    avl_elem_t *element = tree->root;
    avl_elem_t *candidate = NULL;
    void *obj;
    while (element) {
        int cmp = tree->cmp(key, element->key, obj);
        if (cmp > 0)
            element = element->right;
        else {
            candidate = element;
            element = element->left;
        }
    }
    return candidate;
}

static avl_elem_t *rotate_right(avl_elem_t *element)
{
    avl_elem_t *new_top;
    if (element->left->balance <= 0) {
        new_top = element->left;
        element->left = new_top->right;
        new_top->right = element;
        element->balance = -++new_top->balance;
    } else {
        new_top = element->left->right;
        element->left->right = new_top->left;
        new_top->left = element->left;
        element->left = new_top->right;
        new_top->right = element;
        new_top->left->parent = new_top;
        if (new_top->left->right != NULL)
            new_top->left->right->parent = new_top->left;
        new_top->left->balance = -((1 + new_top->balance) / 2);
        new_top->right->balance = (1 - new_top->balance) / 2;
        new_top->balance = 0;
    }
    new_top->parent = element->parent;
    element->parent = new_top;
    if (element->left != NULL)
        element->left->parent = element;
    return new_top;
}

static int put(avl_tree_t *tree, avl_elem_t **ploc, avl_elem_t *element,
               avl_elem_t **premoved_element);

static int put_left(avl_tree_t *tree, avl_elem_t **ploc, avl_elem_t *element,
                    avl_elem_t **premoved_element)
{
    avl_elem_t *loc = *ploc;
    if (loc->left == NULL) {
        element->parent = loc;
        loc->left = element;
        tree->size++;
        loc->balance--;
        return loc->right == NULL;
    }
    if (!put(tree, &loc->left, element, premoved_element))
        return 0;
    switch (--loc->balance) {
        case -2:
            *ploc = rotate_right(loc);
            return 0;
        case -1:
            return 1;
        default:
            return 0;
    }
}

static avl_elem_t *rotate_left(avl_elem_t *element)
{
    avl_elem_t *new_top;
    if (element->right->balance >= 0) {
        new_top = element->right;
        element->right = new_top->left;
        new_top->left = element;
        element->balance = - --new_top->balance;
    } else {
        new_top = element->right->left;
        element->right->left = new_top->right;
        new_top->right = element->right;
        element->right = new_top->left;
        new_top->left = element;
        new_top->right->parent = new_top;
        if (new_top->right->left != NULL)
            new_top->right->left->parent = new_top->right;
        new_top->right->balance = (1 - new_top->balance) / 2;
        new_top->left->balance = -((1 + new_top->balance) / 2);
        new_top->balance = 0;
    }
    new_top->parent = element->parent;
    element->parent = new_top;
    if (element->right != NULL)
        element->right->parent = element;
    return new_top;
}

static int put_right(avl_tree_t *tree, avl_elem_t **ploc, avl_elem_t *element,
                     avl_elem_t **premoved_element)
{
    avl_elem_t *loc = *ploc;
    if (loc->right == NULL) {
        element->parent = loc;
        loc->right = element;
        tree->size++;
        loc->balance++;
        return loc->left == NULL;
    }
    if (!put(tree, &loc->right, element, premoved_element))
        return 0;
    switch (++loc->balance) {
        case 2:
            *ploc = rotate_left(loc);
            return 0;
        case 1:
            return 1;
        default:
            return 0;
    }
}

static void substitute(avl_tree_t *tree, avl_elem_t *new_child,
                       avl_elem_t *old_child)
{
    if (old_child == tree->root)
        tree->root = new_child;
    else {
        avl_elem_t *parent = old_child->parent;
        if (parent->left == old_child)
            parent->left = new_child;
        else parent->right = new_child;
    }
}

static int put(avl_tree_t *tree, avl_elem_t **ploc, avl_elem_t *element,
               avl_elem_t **premoved_element)
{
    avl_elem_t *loc = *ploc;
    int cmp = tree->cmp(element->key, loc->key, tree->obj);
    if (cmp < 0)
        return put_left(tree, ploc, element, premoved_element);
    if (cmp > 0)
        return put_right(tree, ploc, element, premoved_element);
    if (loc == element)
        return 0;
    element->parent = loc->parent;
    element->left = loc->left;
    element->right = loc->right;
    element->balance = loc->balance;
    substitute(tree, element, loc);
    if (element->left)
        element->left->parent = element;
    if (element->right)
        element->right->parent = element;
    *premoved_element  = loc;
    return 0;
}

avl_elem_t *avl_tree_put(avl_tree_t *tree, const void *key, const void *value)
{
    avl_elem_t *element = make_element(key, value);
    if (tree->root == NULL) {
        tree->root = element;
        tree->size = 1;
        return NULL;
    }
    avl_elem_t *removed_element = NULL;
    put(tree, &tree->root, element, &removed_element);
    return removed_element;
}

static int leaf_element(avl_elem_t *element)
{
    return element->right == NULL && element->left == NULL;
}

static avl_elem_t *adjacent_descendant(avl_elem_t *element)
{
    avl_elem_t *adj = previous_descendant(element);
    if (adj != NULL)
        return adj;
    return next_descendant(element);
}

static void lighten_right(avl_tree_t *tree, avl_elem_t *element);
static void lighten_left(avl_tree_t *tree, avl_elem_t *element);

static void lighten_parent(avl_tree_t *tree, avl_elem_t *element)
{
    avl_elem_t *parent = element->parent;
    if (parent->right == element)
        lighten_right(tree, parent);
    else lighten_left(tree, parent);
}

static void lighten_right(avl_tree_t *tree, avl_elem_t *element)
{
    avl_elem_t *new_top;
    switch (--element->balance) {
        case 0:
            if (element != tree->root)
                lighten_parent(tree, element);
            break;
        case -2:
            new_top = rotate_right(element);
            if (element == tree->root) {
                tree->root = new_top;
                break;
            }
            if (new_top->parent->right == element)
                new_top->parent->right = new_top;
            else new_top->parent->left = new_top;
            if (new_top->balance != 1)
                lighten_parent(tree, new_top);
            break;
        default:
            ;
    }
}

static void lighten_left(avl_tree_t *tree, avl_elem_t *element)
{
    avl_elem_t *new_top;
    switch (++element->balance) {
        case 0:
            if (element != tree->root)
                lighten_parent(tree, element);
            break;
        case 2:
            new_top = rotate_left(element);
            if (element == tree->root) {
                tree->root = new_top;
                break;
            }
            if (new_top->parent->right == element)
                new_top->parent->right = new_top;
            else new_top->parent->left = new_top;
            if (new_top->balance != -1)
                lighten_parent(tree, new_top);
            break;
        default:
            ;
    }
}

static void exchange(avl_tree_t *tree, avl_elem_t *e1, avl_elem_t *e2)
{
    substitute(tree, e1, e2);
    substitute(tree, e2, e1);
    avl_elem_t *l = e1->left; e1->left = e2->left; e2->left = l;
    avl_elem_t *r = e1->right; e1->right = e2->right; e2->right = r;
    avl_elem_t *p = e1->parent; e1->parent = e2->parent; e2->parent = p;
    int b = e1->balance; e1->balance = e2->balance; e2->balance = b;
    if (e1->left != NULL)
        e1->left->parent = e1;
    if (e1->right != NULL)
        e1->right->parent = e1;
    if (e2->left != NULL)
        e2->left->parent = e2;
    if (e2->right != NULL)
        e2->right->parent = e2;
}

void avl_tree_detach(avl_tree_t *tree, avl_elem_t *element)
{
    while (!leaf_element(element))
        exchange(tree, element, adjacent_descendant(element));
    if (element == tree->root)
        tree->root = NULL;
    else if (element->parent->left == element) {
        element->parent->left = NULL;
        lighten_left(tree, element->parent);
    } else {
        element->parent->right = NULL;
        lighten_right(tree, element->parent);
    }
    tree->size--;
}

void avl_tree_remove(avl_tree_t *tree, avl_elem_t *element)
{
    avl_tree_detach(tree, element);
    destroy_avl_element(element);
}

avl_elem_t *avl_tree_pop(avl_tree_t *tree, const void *key)
{
    avl_elem_t *element = avl_tree_get(tree, key);
    if (element != NULL)
        avl_tree_detach(tree, element);
    return element;
}

int avl_tree_empty(avl_tree_t *tree)
{
    return tree->root == NULL;
}

avl_elem_t *avl_tree_get_first(avl_tree_t *tree)
{
    if (avl_tree_empty(tree))
        return NULL;
    avl_elem_t *element;
    for (element = tree->root;
         element->left != NULL;
         element = element->left)
        ;
    return element;
}

avl_elem_t *avl_tree_get_last(avl_tree_t *tree)
{
    if (avl_tree_empty(tree))
        return NULL;
    avl_elem_t *element;
    for (element = tree->root;
         element->right != NULL;
         element = element->right)
        ;
    return element;
}

avl_elem_t *avl_tree_pop_first(avl_tree_t *tree)
{
    avl_elem_t *element = avl_tree_get_first(tree);
    if (element != NULL)
        avl_tree_detach(tree, element);
    return element;
}

avl_elem_t *avl_tree_pop_last(avl_tree_t *tree)
{
    avl_elem_t *element = avl_tree_get_last(tree);
    if (element != NULL)
        avl_tree_detach(tree, element);
    return element;
}

static avl_elem_t *copy_tree(avl_elem_t *element)
{
    avl_elem_t *copy = make_element(element->key, element->value);
    copy->balance = element->balance;
    if (element->left) {
        copy->left = copy_tree(element->left);
        copy->left->parent = copy;
    }
    if (element->right) {
        copy->right = copy_tree(element->right);
        copy->right->parent = copy;
    }
    return copy;
}

avl_tree_t *avl_tree_copy(avl_tree_t *tree)
{
    avl_tree_t *copy = make_avl_tree_2(tree->cmp, tree->obj);
    if (tree->root) {
        copy->root = copy_tree(tree->root);
        copy->size = tree->size;
    }
    return copy;
}

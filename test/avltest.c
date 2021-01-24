#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include "avltree.h"
#include "avltree_imp.h"

enum {
    K = 20,
    N = 1000000
};

typedef struct {
    uint8_t key[K];
} element_t;

static element_t elements[N];

static avl_tree_t *tree;
static avl_tree_t *tree_ordered;
static avl_tree_t *tree_reverse;

static void prepare_data(void)
{
    int i;
    for (i = 0; i < N; i++) {
        int j;
        for (j = 0; j < K; j++)
            elements[i].key[j] = random() % 256;
    }
}

static int keycmp(const void *key1, const void *key2)
{
    return memcmp(key1, key2, K);
}

static void enter_element(int i)
{
    avl_elem_t *old = avl_tree_put(tree, elements[i].key, &elements[i]);
    assert(old == NULL);
}

static void enter_data(void)
{
    tree = make_avl_tree(keycmp);
    int i;
    for (i = 0; i < N; i++)
        enter_element(i);
}

static void indent(int depth)
{
    int i;
    for (i = 0; i < depth; i++)
        printf(". ");
}

static void print_node(avl_elem_t *node)
{
    int i;
    printf("%p: ", node);
    const uint8_t *key = avl_elem_get_key(node);
    for (i = 0; i < K; i++)
        printf("%02x", key[i]);
    printf("\n");
}

static void emit_node(avl_elem_t *node, int depth)
{
    if (node == NULL) {
        indent(depth);
        printf("#\n");
    }
    else {
        emit_node(node->left, depth + 1);
        indent(depth);
        print_node(node);
        emit_node(node->right, depth + 1);
    }
}

__attribute__((unused))
static void print_tree(avl_tree_t *t)
{
    printf("===============================================================\n");
    emit_node(t->root, 0);
}

static void verify_node(avl_elem_t *node)
{
    if (node->left != NULL) {
        assert(node->left->parent == node);
        verify_node(node->left);
    }
    if (node->right != NULL) {
        assert(node->right->parent == node);
        verify_node(node->right);
    }
}

static void verify_structure(avl_tree_t *t)
{
    if (t->root == NULL)
        return;
    assert(t->root->parent == NULL);
    verify_node(t->root);
}

static void test_tree_size(avl_tree_t *t, int size)
{
    assert(t->size == size);
    int n = 0;
    avl_elem_t *node;
    for (node = avl_tree_get_first(t);
         node != NULL;
         node = avl_tree_next(node)) {
        //print_node(node);
        assert(n < size);
        n++;
    }
    assert(n == size);
    n = 0;
    for (node = avl_tree_get_last(t);
         node != NULL;
         node = avl_tree_previous(node)) {
        //print_node(node);
        assert(n < size);
        n++;
    }
    assert(n == size);
}

static void check_height(avl_elem_t *node, int height, double limit)
{
    if (node == NULL)
        return;
    assert(height < limit);
    check_height(node->left, height + 1, limit);
    check_height(node->right, height + 1, limit);
}

static void verify_height(avl_tree_t *t, int size)
{
    double limit = 1.45 * log2(size + 2);
    check_height(t->root, 1, limit);
}

static void verify_order(avl_tree_t *t)
{
    avl_elem_t *node1 = avl_tree_get_first(t);
    if (node1 == NULL)
        return;
    for (;;) {
        avl_elem_t *node2 = avl_tree_next(node1);
        if (node2 == NULL)
            break;
        assert(keycmp(node1->key, node2->key) < 0);
        node1 = node2;
    }
}

static void verify_copy(avl_tree_t *t)
{
    avl_tree_t *copy = avl_tree_copy(t);
    avl_elem_t *node;
    for (node = avl_tree_get_first(t); node; node = avl_tree_next(node)) {
        avl_elem_t *copy_node = avl_tree_pop(copy, avl_elem_get_key(node));
        assert(copy_node);
        assert(avl_elem_get_value(node) == avl_elem_get_value(copy_node));
        destroy_avl_element(copy_node);
    }
    assert(avl_tree_empty(copy));
    destroy_avl_tree(copy);
}

static void remove_elements(avl_tree_t *t, int begin, int end)
{
    int i;
    for (i = begin; i < end; i++) {
        avl_elem_t *node = avl_tree_pop(t, elements[i].key);
        assert(node != NULL);
        destroy_avl_element(node);
    }
}

static void verify_tree(avl_tree_t *t, int size, const char *label)
{
    //printf("print_tree\n");
    //print_tree();
    printf("verify_structure (%s:%d)\n", label, size);
    verify_structure(t);
    printf("test_tree_size (%s:%d)\n", label, size);
    test_tree_size(t, size);
    printf("verify_height (%s:%d)\n", label, size);
    verify_height(t, size);
    printf("verify_order (%s:%d)\n", label, size);
    verify_order(t);
    printf("verify_copy (%s:%d)\n", label, size);
    verify_copy(t);
}

static void do_tree(avl_tree_t *t, int size, const char *label)
{
    verify_tree(t, size, label);
    int half = size / 2;
    printf("remove elements (%s:0:%d)\n", label, half);
    remove_elements(t, 0, half);
    verify_tree(t, size - half, label);
    printf("remove elements (%s:%d:%d)\n", label, half, size);
    remove_elements(t, half, size);
    verify_tree(t, 0, label);
}

static void enter_ordered_data(void)
{
    tree_ordered = make_avl_tree(keycmp);
    avl_elem_t *node;
    for (node = avl_tree_get_first(tree);
         node != NULL;
         node = avl_tree_next(node)) {
        const element_t *element = avl_elem_get_value(node);
        avl_elem_t *old = avl_tree_put(tree_ordered, element->key, element);
        assert(old == NULL);
    }
}

static void enter_reverse_data(void)
{
    tree_reverse = make_avl_tree(keycmp);
    avl_elem_t *node;
    for (node = avl_tree_get_last(tree);
         node != NULL;
         node = avl_tree_previous(node)) {
        const element_t *element = avl_elem_get_value(node);
        avl_elem_t *old = avl_tree_put(tree_reverse, element->key, element);
        assert(old == NULL);
    }
}

int main(void)
{
    printf("prepare_data\n");
    prepare_data();
    printf("enter_data\n");
    enter_data();
    enter_ordered_data();
    enter_reverse_data();
    do_tree(tree, N, "random");
    do_tree(tree_ordered, N, "ordered");
    do_tree(tree_reverse, N, "reverse");
    return 0;
}

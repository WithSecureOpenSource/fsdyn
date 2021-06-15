#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <fsdyn/avltree.h>
#include <fsdyn/priority_queue.h>

enum {
    K = 20,
    N = 1000000,
};

typedef struct {
    uint8_t key[K];
    void *loc;
} element_t;

static element_t elements[N];

static void prepare_data(void)
{
    int i;
    for (i = 0; i < N; i++) {
        int j;
        for (j = 0; j < K; j++)
            elements[i].key[j] = random() % 256;
    }
}

static int cmp(const void *value1, const void *value2)
{
    return memcmp(((element_t *) value1)->key, ((element_t *) value2)->key, K);
}

static void reloc(const void *value, void *loc)
{
    ((element_t *) value)->loc = loc;
}

static avl_tree_t *enter_avl_data()
{
    avl_tree_t *tree = make_avl_tree(cmp);
    int i;
    for (i = 0; i < N; i++)
        avl_tree_put(tree, &elements[i], &elements[i]);
    return tree;
}

static priorq_t *enter_pr_data()
{
    priorq_t *prq = make_priority_queue(cmp, reloc);
    int i;
    for (i = 0; i < N; i++)
        priorq_enqueue(prq, &elements[i]);
    return prq;
}

static bool test_correctness()
{
    fprintf(stderr, "enter_data\n");
    avl_tree_t *tree = enter_avl_data();
    avl_elem_t *ae = avl_tree_get_first(tree);
    priorq_t *prq = enter_pr_data();
    while (!priorq_empty(prq)) {
        element_t *e = (element_t *) priorq_dequeue(prq);
        if (e != avl_elem_get_value(ae)) {
            fprintf(stderr, "Mismatch!\n");
            return false;
        }
        ae = avl_tree_next(ae);
    }
    destroy_priority_queue(prq);
    destroy_avl_tree(tree);
    return true;
}

int main()
{
    fprintf(stderr, "prepare_data\n");
    prepare_data();
    if (!test_correctness())
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

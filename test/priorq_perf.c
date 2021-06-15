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

uint64_t now_ns()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return (uint64_t) t.tv_sec * 1000000000 + t.tv_usec * 1000;
}

static void test_performance()
{
    fprintf(stderr, "measure AVL tree\n");
    uint64_t start = now_ns();
    avl_tree_t *tree = enter_avl_data();
    avl_elem_t *ae = avl_tree_get_first(tree);
    while (ae)
        ae = avl_tree_next(ae);
    destroy_avl_tree(tree);
    uint64_t finish = now_ns();
    fprintf(stderr, "  %g s\n", (finish - start) * 1e-9);
    fprintf(stderr, "measure priority queue dequeue\n");
    start = now_ns();
    priorq_t *prq = enter_pr_data();
    while (!priorq_empty(prq))
        priorq_dequeue(prq);
    destroy_priority_queue(prq);
    finish = now_ns();
    fprintf(stderr, "  %g s\n", (finish - start) * 1e-9);
    fprintf(stderr, "measure priority queue remove\n");
    start = now_ns();
    prq = enter_pr_data();
    int i;
    for (i = 0; i < N; i++)
        priorq_remove(prq, elements[i].loc);
    destroy_priority_queue(prq);
    finish = now_ns();
    fprintf(stderr, "  %g s\n", (finish - start) * 1e-9);
}

int main()
{
    fprintf(stderr, "prepare_data\n");
    prepare_data();
    test_performance();
    return EXIT_SUCCESS;
}

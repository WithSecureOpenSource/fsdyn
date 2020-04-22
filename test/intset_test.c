#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fsdyn/intset.h>

enum {
    N = 128
};

static bool elements[256];

static void read_data(void)
{
    int i;
    for (i = 0; i < N; i++) {
        uint8_t elem = random() % 256;
        elements[elem] = true;
    }
}

int main()
{
    read_data();
    intset_t *s = make_intset(256);
    assert(intset_empty(s));
    unsigned i;
    for (i = 0; i < 256; i++)
        if (elements[i])
            intset_add(s, i);
    for (i = 0; i < 256; i++) {
        bool has = intset_has(s, i);
        assert(has == elements[i]);
    }
    int elem = 0;
    for (;;) {
        elem = intset_find_next_hit(s, elem);
        if (elem < 0)
            break;
        assert(elements[elem]);
        elem++;
    }
    elem = 0;
    for (;;) {
        elem = intset_find_next_miss(s, elem);
        if (elem < 0)
            break;
        assert(!elements[elem]);
        elem++;
    }
    for (i = 0; i < 256; i++)
        intset_remove(s, i);
    assert(intset_empty(s));
    destroy_intset(s);
}

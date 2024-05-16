#include "utils.h"
#include <stdint.h>
#include <stdio.h>

typedef struct Numbers {
    int *items;
    uint32_t count;
    uint32_t capacity;
} Numbers;

int main(void)
{
    Arena a;
    Numbers nums;
    ut_memset(&a, 0, sizeof(a));
    ut_memset(&nums, 0, sizeof(nums));

    arena_da_append(&a, &nums, 69);

    int nums2[] = { 129,2,1,23,133123,3123,124 };
    arena_da_append_many(&a, &nums, nums2, UT_ARRAY_LEN(nums2));

    for(uint32_t i = 0; i < nums.count; ++i) {
        fprintf(stderr, "%u] %d\n", i, nums.items[i]);
    }
}


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#define VERA_IMPLEMENTATION
#include "vera.h"

vera_allocator allocator;

void test_interning(void) {
    vera_ctx ctx;
    vera_init_ctx(&ctx, allocator, "");
    const char *str1 = "abc def", *str2 = "abc    def", *str3 = "abc de", *str4 = "efqhqi 12rq";
    assert(vera_intern(&ctx, str1, strlen(str1)) == 0);
    assert(vera_intern(&ctx, str2, strlen(str2)) == 1);
    assert(vera_intern(&ctx, str3, strlen(str3)) == 2);
    assert(vera_intern(&ctx, str4, strlen(str4)) == 3);
    assert(vera_intern(&ctx, str1, strlen(str1)) == 0);
    assert(vera_intern(&ctx, str2, strlen(str2)) == 1);
    assert(vera_intern(&ctx, str3, strlen(str3)) == 2);
    assert(vera_intern(&ctx, str4, strlen(str4)) == 3);
}

int main(void) {
    allocator.alloc = malloc;
    allocator.free = free;
    test_interning();
    printf("OK\n");
    return 0;
}
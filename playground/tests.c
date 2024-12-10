#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#define VERA_IMPLEMENTATION
#include "vera.h"

vera_allocator allocator;

#define INIT_VERA_STRINGS(s1, s2) \
    vstr1.string = examples[s1]; \
    vstr1.len = strlen(examples[s1]); \
    vstr2.string = examples[s2]; \
    vstr2.len = strlen(examples[s2]);

#define ASSERT_STR_EQUAL(s1, s2) \
    INIT_VERA_STRINGS(s1, s2) \
    assert(vera_scmp(&vstr1, &vstr2));

#define ASSERT_STR_NOT_EQUAL(s1, s2) \
    INIT_VERA_STRINGS(s1, s2) \
    assert(!vera_scmp(&vstr1, &vstr2));


void test_scmp(void) {
    const char *examples[] = {
    /* 0 */    "abc def", 
    /* 1 */    "abc    def", 
    /* 2 */    "abc de", 
    /* 3 */    "efqhqi 12rq", 
    /* 4 */    " abc def ", 
    /* 5 */    "   "
    };
    vera_string vstr1, vstr2;

    ASSERT_STR_EQUAL(0, 1);
    ASSERT_STR_NOT_EQUAL(0, 2);
    ASSERT_STR_NOT_EQUAL(1, 2);
    ASSERT_STR_NOT_EQUAL(0, 3);
    ASSERT_STR_NOT_EQUAL(1, 3);
    ASSERT_STR_NOT_EQUAL(2, 3);
    ASSERT_STR_EQUAL(0, 4);
    ASSERT_STR_EQUAL(1, 4);
    ASSERT_STR_NOT_EQUAL(2, 4);
    for(int i = 0; i <= 4; i++) {
        ASSERT_STR_NOT_EQUAL(i, 5);
        ASSERT_STR_NOT_EQUAL(5, i);
    }
}

int main(void) {
    allocator.alloc = malloc;
    allocator.free = free;
    test_scmp();
    printf("OK\n");
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#define VERA_IMPLEMENTATION
#include "vera.h"

int main(void) {
    char *src = 
    "|| sugar\n"
    "||  oranges\n"
    "|| apples  ,   apples\n"
    "||  cherries\n"
    "||flour\n"
    "\n"
    "|      flour,      sugar,    apples|  apple cake\n"
    "|     apples,    oranges,  cherries   |   fruit    salad\n"
    "|fruit   salad,   apple  cake             |  fruit  cake   ";
    vera_allocator allocator = {
        .alloc = malloc,
        .free = free
    };
    vera_ctx ctx;
    vera_init_ctx(&ctx, allocator, src);
    vera_compile(&ctx);
    return 0;
}
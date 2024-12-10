#include <stdio.h>
#define VERA_IMPLEMENTATION
#include "vera.h"
#include <stdlib.h>

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
    vera_ctx ctx;
    vera_init_ctx(&ctx, src, NULL, 0);
    size_t pool_size = vera_parse(&ctx);
    printf("%zu objects parsed\n", pool_size);

    size_t bytes = sizeof(vera_obj) * pool_size;
    printf("allocating %zu bytes\n", bytes);
    vera_obj *pool = (vera_obj*)malloc(bytes);
    vera_init_ctx(&ctx, src, pool, pool_size);
    vera_parse(&ctx);
    for(int i = 0; i < ctx.obj_count; i++)
        printf("%d\t type=%d\n", i, ctx.pool[i].type);
    free(pool);
    return 0;
}
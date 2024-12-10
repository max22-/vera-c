#include <stdio.h>
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
    vera_ctx ctx;
    const size_t pool_size = 1024;
    vera_obj pool[pool_size];
    vera_init_ctx(&ctx, src, pool, pool_size);
    int objects_parsed = vera_parse(&ctx);
    printf("%d objects parsed\n", objects_parsed);
    for(int i = 0; i < ctx.obj_count; i++)
        printf("%d\t type=%d\n", i, ctx.pool[i].type);
    sizeof(vera_obj);
    return 0;
}
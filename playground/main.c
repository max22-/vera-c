#include <stdio.h>
#define VERA_IMPLEMENTATION
#include "vera.h"
#include <stdlib.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

int main(void) {
    const char *src = 
    "|| sugar\n"
    "||  oranges\n"
    "|| apples  ,   apples\n"
    "||  cherries\n"
    "||flour\n"
    "\n"
    "|      flour,      sugar,    apples|  apple cake\n"
    "|     apples,    oranges,  cherries   |   fruit    salad\n"
    "|fruit   salad,   apple  cake             |  fruit  cake   ";

    const char *ports[] = {
        "@port1", "@port2", "@port3"
    };

    vera_ctx ctx;
    vera_init_ctx(&ctx, src, NULL, 0);
    vera_add_ports(&ctx, ports, ARRAY_SIZE(ports));
    size_t pool_size = vera_parse(&ctx);
    printf("%zu objects parsed\n", pool_size);

    size_t bytes = sizeof(vera_obj) * pool_size;
    printf("allocating %zu bytes\n", bytes);
    vera_obj *pool = (vera_obj*)malloc(bytes);
    vera_init_ctx(&ctx, src, pool, pool_size);
    vera_add_ports(&ctx, ports, ARRAY_SIZE(ports));
    vera_parse(&ctx);
    for(int i = 0; i < ctx.obj_count; i++)
        printf("%d\t type=%d\n", i, ctx.pool[i].type);
    free(pool);
    return 0;
}
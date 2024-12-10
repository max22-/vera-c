#ifndef VERA_H
#define VERA_H

#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#include <stdlib.h> /* for exit() */

typedef struct vera_string {
    const char *string;
    size_t len;
} vera_string;

enum vera_obj_type {
    VERA_FACT,
    VERA_LHS,
    VERA_RHS,
};

typedef struct vera_object {
    enum vera_obj_type type;
    struct {
        vera_string vstr;
        int intern;
        union {
            int keep;
            unsigned int count;
        } attr;
    } fact;
} vera_obj;

typedef struct {
    const char *src;
    int pos;
    char delimiter;
    vera_obj *pool;
    size_t pool_size;
    unsigned int obj_count;
} vera_ctx;

void vera_init_ctx(vera_ctx *ctx, const char *src, vera_obj *pool, size_t pool_size);
void vera_compile(vera_ctx *ctx);

#ifdef VERA_IMPLEMENTATION

#define ERROR(...) \
    do { \
        fprintf(stderr, "%s:%d: ", __FILE__, __LINE__); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
        exit(1); \
    } while(0)

void vera_init_ctx(vera_ctx *ctx, const char *src, vera_obj *pool, size_t pool_size) {
    ctx->src = src;
    ctx->pos = 0;
    ctx->delimiter = 0;
    ctx->pool = pool;
    ctx->pool_size = pool_size;
    ctx->obj_count = 0;
}

static int vera_scmp(vera_string *s1, vera_string *s2) {
    size_t len1 = s1->len, len2 = s2->len;
    int i1 = 0, i2 = 0;
    /* trim left */
    while(i1 < len1 && isspace(s1->string[i1]))
        i1++;
    while(i2 < len2 && isspace(s2->string[i2]))
        i2++;
    /* trim right */
    while(len1 > i1 && isspace(s1->string[len1 - 1]))
        len1--;
    while(len2 > i2 && isspace(s2->string[len2 - 1]))
        len2--;
    for(;;) {
        if(i1 >= len1 && i2 >= len2)
            return 1;
        else if(i1 >= len1 || i2 >= len2)
            return 0;
        if(isspace(s1->string[i1]) && isspace(s2->string[i2])) {
            while(i1 < len1 && isspace(s1->string[i1]))
                i1++;
            while(i2 < len2 && isspace(s2->string[i2]))
                i2++;
        } else if(s1->string[i1] == s2->string[i2]) {
            i1++;
            i2++;
        } else {
            return 0;
        }
    }
    /* we will never get here */
    return 1;
}

static void vera_add_side(vera_ctx *ctx, enum vera_obj_type type) {
    const int obj_id = ctx->obj_count;
    if(obj_id >= ctx->pool_size)
        ERROR("out of memory");
    ctx->pool[obj_id].type = type;
    ctx->obj_count++;
}

static void vera_add_fact(vera_ctx *ctx, vera_string vstr, enum vera_obj_type side, int keep, unsigned int count) {
    if(ctx->obj_count >= ctx->pool_size)
        ERROR("out of memory");
    vera_obj *obj = &ctx->pool[ctx->obj_count];
    obj->type = VERA_FACT;
    obj->fact.vstr = vstr;
    obj->fact.intern = -1;
    if(side == VERA_LHS) {
        obj->fact.attr.keep = keep;
    } else if(side == VERA_RHS) {
        obj->fact.attr.count = count;
    } else {
        ERROR("unreachable");
    }
    ctx->obj_count++;
}

#define CURSOR (ctx->src[ctx->pos])
#define DELIM (ctx->delimiter)

static void vera_advance(vera_ctx *ctx) {
    if(CURSOR)
        ctx->pos++;
    else
        ERROR("unexpected end of file");
}

static void vera_skipspace(vera_ctx *ctx) {
    while(isspace(CURSOR))
        vera_advance(ctx);
}

static void vera_match(vera_ctx *ctx, char c) {
    if(CURSOR != c)
        ERROR("expected `%c`", c);
    if(c != '\0')
        vera_advance(ctx);
}

static int vera_is_in(char c, const char *set) {
    while(*set) {
        if(c == *(set++))
            return 1;
    }
    return 0;
}

int vera_int(vera_ctx *ctx) {
    int start = ctx->pos;
    if(!isdigit(CURSOR)) ERROR("expected digit");
    while(isdigit(CURSOR))
        vera_advance(ctx);
    int end = ctx->pos;
    int n = 0;
    for(int i = start; i < end; i++)
        n = 10 * n + ctx->src[i] - '0';
    return n;
}

void vera_fact(vera_ctx *ctx, enum vera_obj_type side) {
    if(ctx->obj_count >= ctx->pool_size) ERROR("out of memory");
    int start = ctx->pos;
    if(vera_is_in(CURSOR, " ?:,") || CURSOR == DELIM)
        ERROR("unexpected `%c`", CURSOR);
    while(CURSOR && !vera_is_in(CURSOR, "?:,") && CURSOR != DELIM)
        vera_advance(ctx);
    int end = ctx->pos;
    if(end <= start) ERROR("empty string");
    vera_string vstr;
    vstr.string = &ctx->src[start];
    vstr.len = end - start - 1;
    if(side == VERA_LHS) {
        int keep = 0;
        if(CURSOR == '?') {
            vera_advance(ctx);
            keep = 1;
        }
        vera_add_fact(ctx, vstr, VERA_LHS, keep, 0);
    } else if(side == VERA_RHS) {
        int count = 1;
        if(CURSOR == ':') {
            vera_advance(ctx);
            vera_skipspace(ctx);
            count = vera_int(ctx);
        }
        vera_add_fact(ctx, vstr, VERA_RHS, 0, count);
    } else {
        ERROR("unreachable");
    }
}

void vera_side(vera_ctx *ctx, enum vera_obj_type side) {
    if(side == VERA_FACT) ERROR("unreachable");
    vera_add_side(ctx, side);
    if(CURSOR == DELIM)
        return;
    vera_fact(ctx, side);
    vera_skipspace(ctx);
    while(CURSOR == ',') {
        vera_advance(ctx);
        vera_skipspace(ctx);
        vera_fact(ctx, side);
        vera_skipspace(ctx);
    }
}

void vera_rule(vera_ctx *ctx) {
    vera_match(ctx, DELIM);
    vera_skipspace(ctx);
    vera_side(ctx, VERA_LHS);
    vera_skipspace(ctx);
    vera_match(ctx, DELIM);
    vera_skipspace(ctx);
    vera_side(ctx, VERA_RHS);
}

size_t vera_parse(vera_ctx *ctx) {
    vera_skipspace(ctx);
    if(CURSOR)
        DELIM = CURSOR;
    else
        ERROR("empty source");
    while(CURSOR == DELIM) {
        vera_rule(ctx);
        vera_skipspace(ctx);
    }
    return ctx->obj_count;
}

#undef CURSOR
#undef ERROR

/* ************************************************************************** */

void vera_compile(vera_ctx *ctx) {

}

#endif
#endif
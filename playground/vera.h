#ifndef VERA_H
#define VERA_H

#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

typedef struct vera_string {
    char *string;
    size_t size;
    struct vera_string *next;
} vera_string;

typedef struct {
    void *(*alloc)(size_t);
    void (*free)(void*);
} vera_allocator;

enum vera_side {
    VERA_NO_SIDE,
    VERA_LEFT,
    VERA_RIGHT
};

enum vera_obj_type {
    VERA_FACT,
    VERA_LHS,
    VERA_RHS,
    VERA_RULE
};

typedef struct vera_object {
    enum vera_obj_type type;
    struct {
        int str;
        union {
            int keep;
            unsigned int count;
        } attr;
    } fact;
    struct vera_object *next;
} vera_obj;



typedef struct {
    const char *src;
    vera_allocator allocator;
    vera_string *strings;
    int pos;
    char delimiter;
    vera_obj *objs, *last_obj;
} vera_ctx;

void vera_init_ctx(vera_ctx *ctx, vera_allocator allocator, const char *src);
void vera_compile(vera_ctx *ctx);

#ifdef VERA_IMPLEMENTATION

#define ERROR(...) \
    do { \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
        exit(1); \
    } while(0)

void vera_init_ctx(vera_ctx *ctx, vera_allocator allocator, const char *src) {
    ctx->src = src;
    ctx->allocator = allocator;
    ctx->strings = NULL;
    ctx->pos = 0;
    ctx->delimiter = 0;
    ctx->objs = NULL;
    ctx->last_obj = NULL;
}

static vera_string *vera_new_string(vera_ctx *ctx, const char *str, size_t size) {
    vera_string *res = ctx->allocator.alloc(sizeof(vera_string));
    if(!res) ERROR("failed to allocate memory");
    res->string = ctx->allocator.alloc(size);
    if(!res->string) {
        ctx->allocator.free(res);
        ERROR("failed to allocate memory");
    }
    strncpy(res->string, str, size);
    res->size = size;
    res->next = NULL;
    return res;
}

static void vera_shift(vera_string *str, size_t pos) {
    printf("shift at %zu:\n", pos);
    for(int i = 0; i < str->size; i++)
        printf("%c", str->string[i]);
    printf("\n");
    assert(pos < str->size);
    str->size -= 1;
    for(size_t i = pos; i < str->size; i++)
        str->string[i] = str->string[i+1];
}

/* we suppose the string is already trimmed on the left and on the right */
static vera_string *vera_clean_spaces(vera_ctx *ctx, const char *str, size_t size) {
    vera_string *res = vera_new_string(ctx, str, size);
    for(size_t i = 0; i < res->size - 1 && str[i]; i++) {
        if(res->string[i] == ' ' && res->string[i + 1] == ' ')
            vera_shift(res, i);
    }
    return res;
}

static int vera_scmp(vera_string *s1, vera_string *s2) {
    if(s1->size != s2->size) return 0;
    for(size_t i = 0; i < s1->size; i++)
        if(s1->string[i] != s2->string[i])
            return 0;
    return 1;
}

static int vera_intern(vera_ctx *ctx, const char *str, size_t size) {
    vera_string *temp = vera_clean_spaces(ctx, str, size);
    if(ctx->strings == NULL) {
        ctx->strings = temp;
        return 0;
    }
    int i = 0;
    vera_string *prev = ctx->strings;
    for(vera_string *s = ctx->strings; s != NULL; prev = s, s = s->next, i++) {
        if(vera_scmp(s, temp)) {
            free(temp);
            return i;
        }
    }
    prev->next = temp;
    return i;
}

/* Constructors ************************************************************* */

static void vera_add_obj(vera_ctx *ctx, enum vera_obj_type type, int str, enum vera_side side, int keep, unsigned int count) {
    vera_obj *obj = ctx->allocator.alloc(sizeof(vera_obj));
    if(!obj) ERROR("failed to allocate memory");
    obj->type = type;
    obj->next = NULL;
    if(type == VERA_FACT) {
        obj->fact.str = str;
        switch(side) {
        case VERA_LEFT:
            obj->fact.attr.keep = keep;
            break;
        case VERA_RIGHT:
            obj->fact.attr.count = count;
            break;
        default:
            ERROR("unreachable");
        }
    }
    if(ctx->objs == NULL) {
        ctx->objs = obj;
        ctx->last_obj = obj;
    } else {
        ctx->last_obj->next = obj;
        ctx->last_obj = obj;
    }
}

#define vera_add_rule(ctx) vera_add_obj(ctx, VERA_RULE, 0, VERA_NO_SIDE, 0, 0)
#define vera_add_lhs(ctx) vera_add_obj(ctx, VERA_LHS, 0, VERA_NO_SIDE, 0, 0)
#define vera_add_rhs(ctx) vera_add_obj(ctx, VERA_RHS, 0, VERA_NO_SIDE, 0, 0)
#define vera_add_lhs_fact(ctx, str, keep) vera_add_obj(ctx, VERA_FACT, str, VERA_LEFT, keep, 0)
#define vera_add_rhs_fact(ctx, str, count) vera_add_obj(ctx, VERA_FACT, str, VERA_RIGHT, 0, count)

/* Parser ******************************************************************* */

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
    char *temp = (char*)ctx->allocator.alloc(end - start);
    strncpy(temp, &ctx->src[start], end - start - 1);
    int res = atoi(temp);
    ctx->allocator.free(temp);
    return res;
}

void vera_fact(vera_ctx *ctx, enum vera_side side) {
    int start = ctx->pos;
    if(vera_is_in(CURSOR, " ?:,") || CURSOR == DELIM)
        ERROR("unexpected `%c`", CURSOR);
    while(CURSOR && !vera_is_in(CURSOR, "?:,") && CURSOR != DELIM)
        vera_advance(ctx);
    int end = ctx->pos;
    if(end <= start) ERROR("empty string");
    int str = vera_intern(ctx, &ctx->src[start], end - start - 1);
    if(side == VERA_LEFT) {
        int keep = 0;
        if(CURSOR == '?') {
            vera_advance(ctx);
            keep = 1;
        }
        vera_add_lhs_fact(ctx, str, keep);
    } else if(side == VERA_RIGHT) {
        int count = 1;
        if(CURSOR == ':') {
            vera_advance(ctx);
            vera_skipspace(ctx);
            count = vera_int(ctx);
        }
        vera_add_rhs_fact(ctx, str, count);
    } else {
        ERROR("unreachable");
    }
}

void vera_side(vera_ctx *ctx, enum vera_side side) {
    if(side == VERA_LEFT)
        vera_add_lhs(ctx);
    else if(side == VERA_RIGHT)
        vera_add_rhs(ctx);
    else
        ERROR("unreachable");
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
    vera_add_rule(ctx);
    vera_match(ctx, DELIM);
    vera_skipspace(ctx);
    vera_side(ctx, VERA_LEFT);
    vera_skipspace(ctx);
    vera_match(ctx, DELIM);
    vera_skipspace(ctx);
    vera_side(ctx, VERA_RIGHT);
}

static void vera_parse(vera_ctx *ctx) {
    vera_skipspace(ctx);
    if(CURSOR)
        DELIM = CURSOR;
    else
        ERROR("empty source");
    while(CURSOR == DELIM) {
        vera_rule(ctx);
        vera_skipspace(ctx);
    }
    
}

#undef CURSOR
#undef ERROR

/* ************************************************************************** */

void vera_compile(vera_ctx *ctx) {
    vera_parse(ctx);
    int i = 0;
    for(vera_obj *obj = ctx->objs; obj != NULL; obj = obj->next, i++);
    printf("%d objects parsed\n", i);
}

#endif
#endif
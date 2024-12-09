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

typedef struct {
    const char *src;
    vera_allocator allocator;
    vera_string *strings;
} vera_ctx;

void vera_init_ctx(vera_ctx *ctx, vera_allocator allocator, const char *src);
void vera_compile(vera_ctx *ctx);

#ifdef VERA_IMPLEMENTATION

#define ERROR(...) \
    do { \
        fprintf(stderr, __VA_ARGS__); \
        exit(1); \
    } while(0)

void vera_init_ctx(vera_ctx *ctx, vera_allocator allocator, const char *src) {
    ctx->src = src;
    ctx->allocator = allocator;
    ctx->strings = NULL;
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
    assert(pos < str->size);
    str->size -= 1;
    for(size_t i = pos; i < str->size; i++)
        str->string[i] = str->string[i+1];
}

/* we suppose the string is already trimmed on the left and on the right */
static vera_string *vera_clean_spaces(vera_ctx *ctx, char *str, size_t size) {
    vera_string *res = vera_new_string(ctx, str, size);
    for(size_t i = 0; i < size - 1 && str[i]; i++) {
        if(res->string[i] == ' ' && res->string[i + 1] == ' ')
            vera_shift(res, i+1);
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

void vera_compile(vera_ctx *ctx) {
    printf("%d\n", vera_intern(ctx, "abcd  efgh", 10));
    printf("%d\n", vera_intern(ctx, "abcd efgh", 9));
    printf("%d\n", vera_intern(ctx, "abcd efh", 8));
}

#endif
#endif
#ifndef VERA_H
#define VERA_H

#include <stdint.h>
#include <ctype.h>
#include <assert.h>

#include <stdlib.h> /* for exit() */

/* The parser in inspired by https://zserge.com/jsmn/ */

typedef struct vera_string {
    const char *string;
    size_t len;
} vera_string;

enum vera_obj_type {
    VERA_LHS,
    VERA_RHS,
    VERA_FACT,
    VERA_PORT,
};

typedef struct vera_object {
    enum vera_obj_type type;
    union {
        struct {
            vera_string vstr;
            int intern;
            union {
                int keep;
                unsigned int count;
            } attr;
        } fact;
        struct {
            vera_string vstr;
            int intern;
        } port;
    } as;
} vera_obj;

typedef struct {
    const char *src;
    int pos;
    char delimiter;
    vera_obj *pool;
    size_t pool_size;
    unsigned int obj_count;
    unsigned int register_count;
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

size_t slen(const char *str) {
    size_t res = 0;
    while(*(str++))
        res++;
    return res;
}

static void vera_add_port(vera_ctx *ctx, const char *name) {
    if(ctx->pool == NULL) { /* if we do a first pass to calculate the needed pool size */
        ctx->obj_count++;
        return;
    }
    if(ctx->obj_count >= ctx->pool_size)
        ERROR("out of memory");
    vera_obj *obj = &ctx->pool[ctx->obj_count];
    obj->type = VERA_PORT;
    obj->as.port.vstr.string = name;
    obj->as.port.vstr.len = slen(name);
    obj->as.port.intern = -1;
    ctx->obj_count++;
}

/* Make sure that `name` stays valid during the whole compilation */
void vera_add_ports(vera_ctx *ctx, const char **ports, size_t port_count) {
    for(size_t i = 0; i < port_count; i++)
        vera_add_port(ctx, ports[i]);
}

static void vera_add_side(vera_ctx *ctx, enum vera_obj_type type) {
    if(ctx->pool == NULL) { /* if we do a first pass to calculate the needed pool size */
        ctx->obj_count++;
        return;
    }
    const int obj_id = ctx->obj_count;
    if(obj_id >= ctx->pool_size)
        ERROR("out of memory");
    ctx->pool[obj_id].type = type;
    ctx->obj_count++;
}

static void vera_add_fact(vera_ctx *ctx, vera_string vstr, enum vera_obj_type side, int keep, unsigned int count) {
    if(ctx->pool == NULL) { /* if we do a first pass to calculate the needed pool size */
        ctx->obj_count++;
        return;
    }
    if(ctx->obj_count >= ctx->pool_size)
        ERROR("out of memory");
    vera_obj *obj = &ctx->pool[ctx->obj_count];
    obj->type = VERA_FACT;
    obj->as.fact.vstr = vstr;
    obj->as.fact.intern = -1;
    if(side == VERA_LHS) {
        obj->as.fact.attr.keep = keep;
    } else if(side == VERA_RHS) {
        obj->as.fact.attr.count = count;
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

static int vera_int(vera_ctx *ctx) {
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

static void vera_fact(vera_ctx *ctx, enum vera_obj_type side) {
    int start = ctx->pos;
    if(vera_is_in(CURSOR, " ?:,") || CURSOR == DELIM)
        ERROR("unexpected `%c`", CURSOR);
    while(CURSOR && !vera_is_in(CURSOR, "?:,") && CURSOR != DELIM)
        vera_advance(ctx);
    int end = ctx->pos;
    if(end <= start) ERROR("empty string");
    vera_string vstr;
    vstr.string = &ctx->src[start];
    vstr.len = end - start;
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

static void vera_side(vera_ctx *ctx, enum vera_obj_type side) {
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

static void vera_rule(vera_ctx *ctx) {
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
#undef DELIM


/* helper function for vera_intern_strings */
static int vera_find_string(vera_ctx *ctx, vera_string *vstr) {
    for(size_t i = 0; i < ctx->obj_count; i++) {
        vera_obj *obj = &ctx->pool[i];
        if(obj->type == VERA_FACT) {
            if(obj->as.fact.intern < 0)
                return -1;
            if(vera_scmp(&obj->as.fact.vstr, vstr))
                return obj->as.fact.intern;
        } else if(obj->type == VERA_PORT) {
            if(obj->as.port.intern < 0)
                return -1;
            if(vera_scmp(&obj->as.port.vstr, vstr))
                return obj->as.port.intern;
        }
    }
    return -1;
}

void vera_intern_strings(vera_ctx *ctx) {
    int n = 0;
    for(size_t i = 0; i < ctx->obj_count; i++) {
        vera_obj *obj = &ctx->pool[i];
        if(obj->type == VERA_FACT) {
            int intern = vera_find_string(ctx, &obj->as.fact.vstr);
            if(intern == -1)
                obj->as.fact.intern = n++;
            else
                obj->as.fact.intern = intern;
        } else if(obj->type == VERA_PORT) {
            int intern = vera_find_string(ctx, &obj->as.port.vstr);
            if(intern == -1)
                obj->as.port.intern = n++;
            else
                obj->as.port.intern = intern;
        }
    }
    ctx->register_count = n;
}

#ifdef VERA_RISCV32


#define emit(instr) \
    do { \
        *(uint32_t*)&output[pc] = instr; \
        pc += 4; \
    } while(0)
#define I_type(opcode, funct3, rd, rs, imm) emit((opcode) | (funct3) << 12 | ((rd) & 0x1f) << 7 | ((rs) & 0x1f) << 15 | ((imm) & 0xfff) << 20)
#define rv_addi(rd, rs, imm) I_type(0x13, 0, rd, rs, imm)
#define rv_jal(reg, imm) emit(0x6f | (reg) << 7 | ((imm) & 0xff000) | ((imm) & (1 << 11)) << 9 | ((imm) & 0x7fe) << (21 - 1) | ((imm) & (1 << 20)) << 10)
#define rv_jalr(rd, rs, imm) I_type(0x67, 0, rd, rs, imm)
#define U_type(opcode, rd, imm) emit((opcode) | ((rd) & 0x1f) << 7 | ((imm) & 0xfffff) << 12)
#define rv_lui(rd, imm) U_type(0x37, (rd), (imm))
#define rv_auipc(rd, imm) U_type(0x17, (rd), (imm))
#define rv_lw(rd, rs, imm) I_type(0x3, 0x2, rd, rs, imm)
#define S_type(opcode, funct3, rs1, rs2, imm) emit((opcode) | ((imm) & 0x1f) << 7 | (funct3) << 12 | ((rs1) & 0x1f) << 15 | ((rs2) & 0x1f) << 20 | (((imm) & 0xfe0) << 20))
#define rv_sw(rs1, rs2, imm) S_type(0x23, 0x2, rs1, rs2, imm)
#define B_type(opcode, funct3, rs1, rs2, imm) emit((opcode) | (funct3) << 12 | (((imm) >> 11) & 0x1) << 7 | (((imm) >> 1) & 0xf) << 8 | ((rs1) & 0x1f) << 15 | ((rs2) & 0x1f) << 20 | (((imm) >> 5) & 0x1f) << 25 | (((imm) >> 12) & 0x1f) << 31)
#define rv_bgeu(rs1, rs2, imm) B_type(0x63, 0x7, rs1, rs2, imm)
#define rv_beq(rs1, rs2, imm) B_type(0x63, 0x0, rs1, rs2, imm)
#define R_type(opcode, funct3, funct7, rd, rs1, rs2) emit((opcode) | ((rd) & 0x1f) << 7 | (funct3) << 12 | ((rs1) & 0x1f) << 15 | ((rs2) & 0x1f) << 20 | funct7 << 25)
#define rv_add(rd, rs1, rs2) R_type(0x33, 0, 0, rd, rs1, rs2)
#define rv_mul(rd, rs1, rs2) R_type(0x33, 0, 0x1, rd, rs1, rs2)
#define rv_break() I_type(0x73, 0x0, 0, 0, 1)
/* pseudo instructions */
#define rv_b(addr) rv_jal(0, addr - pc)
#define rv_ret() rv_jalr(zero, ra, 0)
#define rv_li(rd, imm) rv_addi(rd, zero, imm)
/* my own pseudo instructions */
#define rv_load(rd, addr) \
    do { \
        int32_t offset = addr - pc; \
        int32_t upper = (offset / (1 << 12)), lower = offset - upper; \
        assert(lower < 1 << 12); \
        printf("load addr = %d, pc = %u, offset = %d, upper = 0x%x, lower=0x%x\n", addr, pc, offset, upper, lower); \
        rv_auipc(rd, upper); \
        rv_lw(rd, rd, lower); \
    } while(0)
#define rv_store(data_reg, temp_reg, addr) \
    do { \
        int32_t offset = addr - pc; \
        int32_t upper = (offset / (1 << 12)), lower = offset - upper; \
        assert(lower < 1 << 12); \
        printf("store addr = %d, pc = %u, offset = %d, upper = 0x%x, lower=0x%x\n", addr, pc, offset, upper, lower); \
        rv_auipc(temp_reg, upper); \
        rv_sw(temp_reg, data_reg, lower); \
    } while(0)
#define rv_load_i32_imm(rd, imm) \
    do { \
        int32_t upper = (imm) / (1 << 12), lower = (imm) - upper; \
        assert(lower < 1 << 12); \
        if(upper != 0) \
            rv_lui((rd), upper); \
        rv_li((rd), lower); \
    } while(0)


#define SKIP_PORTS() \
    do { \
        while(i < ctx->obj_count && ctx->pool[i].type == VERA_PORT) \
            i++; \
    } while(0)

#define SKIP_RULE() \
    do { \
        i++; \
        while(i < ctx->obj_count && ctx->pool[i].type != VERA_LHS) \
            i++; \
    } while(0)

#define SKIP_RULES_WITH_EMPTY_LHS() \
    do { \
        for(;;) { \
            if(i >= ctx->obj_count - 1) \
                break; \
            vera_obj *obj1 = &ctx->pool[i], *obj2 = &ctx->pool[i+1]; \
            if(obj1->type == VERA_LHS && obj2->type == VERA_RHS) \
                SKIP_RULE(); \
            else \
                break; \
        } \
    } while(0)

static void vera_riscv32_fill_registers(vera_ctx* ctx, uint32_t *registers) {
    size_t i = 0;
    SKIP_PORTS();
    while(i < ctx->obj_count) {
        assert(ctx->pool[i].type == VERA_LHS);
        i++; /* we skip the lhs delimiter */
        if(i < ctx->obj_count && ctx->pool[i].type == VERA_RHS) {
            /* we have an empty lhs*/
            i++; /* we skip the rhs delimiter */
            while(i < ctx->obj_count && ctx->pool[i].type == VERA_FACT) {
                vera_obj *obj = &ctx->pool[i];
                registers[obj->as.fact.intern] += obj->as.fact.attr.count;
                i++;
            }
        } else {
            while(i < ctx->obj_count && ctx->pool[i].type != VERA_LHS)
                i++;
        }
    }
}

#define LABELS_LIST_SIZE 256
#define DECLARE_LABELS_LIST(x) \
    static uint32_t x##_labels[LABELS_LIST_SIZE]; \
    unsigned int x##_labels_counter = 0;

#define MAKE_LABEL(x) \
    do { \
        x##_labels[x##_labels_counter++] = pc; \
    } while(0)

/* Assembler inspired by https://zserge.com/posts/post-apocalyptic-programming/ */

static size_t vera_riscv32_assemble(vera_ctx *ctx, uint8_t *output, size_t max_size) {
    uint32_t pc = 0;
    static uint32_t start_label = 0, end_label = 0;
    DECLARE_LABELS_LIST(registers);
    DECLARE_LABELS_LIST(rules);
    static uint32_t skip_labels[256];
    unsigned int skip_labels_counter = 0;
    /* flags used to check removed duplicates in lhs */
    int register_processed[ctx->register_count]; /* boolean */
    /* used to memorize the lhs (then we add the lhs values, and we generate the code if diff != 0) */
    int32_t register_diff[ctx->register_count];
    /* risc-v registers */
    const uint8_t zero = 0, ra = 1, t0 = 5, t1 = 6, t2 = 7, a0 = 10;
    /* **************** */
    rv_b(start_label);
    for(int i = 0; i < ctx->register_count; i++) {
        MAKE_LABEL(registers);
        emit(0);
    }
    /* the registers start at output + 4, because the first word is a jump instruction */
    vera_riscv32_fill_registers(ctx, (uint32_t*)(output + 4));

    start_label = pc;
    rv_li(a0, 0);
    int i = 0;
    SKIP_PORTS();
    while(i < ctx->obj_count) {
        for(unsigned int i = 0; i < ctx->register_count; i++) {
            register_diff[i] = 0;
            register_processed[i] = 0;
        }
        SKIP_RULES_WITH_EMPTY_LHS();
        if(i >= ctx->obj_count) break;
        assert(ctx->pool[i].type == VERA_LHS);
        i++; /* skip lhs delimiter */
        MAKE_LABEL(rules);
        printf("new rule\n");
        /* we will use t1 to compute the min of the lhs */
        rv_li(t1, 0xffffffff);
        while(ctx->pool[i].type == VERA_FACT) {
            vera_obj *obj = &ctx->pool[i];
            if(register_processed[obj->as.fact.intern]) {
                i++; 
                continue;
            }
            if(obj->as.fact.attr.keep)
                register_diff[obj->as.fact.intern] = 0;
            else
                register_diff[obj->as.fact.intern] = -1;
            rv_load(t0, registers_labels[obj->as.fact.intern]);
            rv_li(t2, 0);
            rv_beq(t0, t2, rules_labels[rules_labels_counter] - pc); /* we skip to next rule if one of the registers is zero */
            rv_bgeu(t0, t1, skip_labels[skip_labels_counter] - pc);
            rv_add(t1, zero, t0);
            MAKE_LABEL(skip);
            i++;
        }
        assert(ctx->pool[i].type == VERA_RHS);
        i++; /* skip rhs delimiter */
        while(i < ctx->obj_count && ctx->pool[i].type == VERA_FACT) {
            const vera_obj *obj = &ctx->pool[i];
            const int interned = obj->as.fact.intern;
            register_diff[interned] += obj->as.fact.attr.count;
            i++; 
        }
        for(unsigned int j = 0; j < ctx->register_count; j++) {
            int32_t diff = register_diff[j];
            if(diff != 0) {
                rv_load(t0, registers_labels[j]);
                rv_load_i32_imm(t2, diff);
                rv_mul(t2, t2, t1);
                rv_add(t0, t0, t2);
                rv_store(t0, t2, registers_labels[j]);
            }
        }
        rv_addi(a0, a0, 1);
        rv_b(end_label);
    }
    MAKE_LABEL(rules); /* make a new (empty) rule label so that the last rule can make a jump here */
    for(unsigned int i = 0; i < rules_labels_counter; i++) {
        printf("rules_labels[%u] = %d\n", i, rules_labels[i]);
    }
    end_label = pc;
    rv_break();
    rv_ret();
    return pc;
}

#undef SKIP_PORTS

size_t vera_riscv32_codegen(vera_ctx *ctx, uint8_t *output, size_t max_size) {
    printf("pass 1\n");
    vera_riscv32_assemble(ctx, output, max_size); /* first pass to calculate the labels */
    printf("pass 2\n");
    return vera_riscv32_assemble(ctx, output, max_size);
}

#endif

void vera_compile(vera_ctx *ctx) {

}

#undef ERROR

#endif
#endif
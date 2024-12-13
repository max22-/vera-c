#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#define VERA_IMPLEMENTATION
#define VERA_RISCV32
#include "vera.h"
#define LITTLE_ENDIAN_HOST
#define RV32_IMPLEMENTATION
#define TRACE
#include "rv32.h"

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

rv32_mmio_result_t mmio_load8(uint32_t addr, uint8_t *ret) { return RV32_MMIO_ERR; }
rv32_mmio_result_t mmio_load16(uint32_t addr, uint16_t *ret) { return RV32_MMIO_ERR; }
rv32_mmio_result_t mmio_load32(uint32_t addr, uint32_t *ret) { return RV32_MMIO_ERR; }
rv32_mmio_result_t mmio_store8(uint32_t addr, uint8_t val) { return RV32_MMIO_ERR; }
rv32_mmio_result_t mmio_store16(uint32_t addr, uint16_t val) { return RV32_MMIO_ERR; }
rv32_mmio_result_t mmio_store32(uint32_t addr, uint32_t val) { return RV32_MMIO_ERR; }
void ecall(RV32 *rv32) { }


int main(void) {
    test_scmp();
    RV32 *rv32;
    uint8_t *memory = NULL;
    const size_t ram_size = 0x10000;
    memory = (uint8_t*)malloc(RV32_NEEDED_MEMORY(ram_size));
    if(!memory) {
        fprintf(stderr, "Failed to allocate memory.\n");
        return 1;
    }
    rv32 = rv32_new(memory, ram_size);


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
    vera_ctx ctx;
    vera_init_ctx(&ctx, src, NULL, 0);
    size_t pool_size = vera_parse(&ctx);
    printf("%zu objects parsed\n", pool_size);

    size_t bytes = sizeof(vera_obj) * pool_size;
    printf("allocating %zu bytes\n", bytes);
    vera_obj *pool = (vera_obj*)malloc(bytes);
    vera_init_ctx(&ctx, src, pool, pool_size);
    vera_parse(&ctx);
    vera_intern_strings(&ctx);

    size_t binary_size = vera_riscv32_codegen(&ctx, rv32->mem, 1024);

    do {
        while (rv32->status == RV32_RUNNING) {
            rv32_cycle(rv32);
            switch(rv32->status) {
            case RV32_RUNNING:
                break;
            case RV32_EBREAK:
                fprintf(stderr, "ebreak at pc=%08x\n", rv32->pc);
                if(rv32->r[REG_A0]) {
                    rv32->pc = 0;
                    rv32->status = RV32_RUNNING;
                }
                break;
            default:
                fprintf(stderr, "Error %d at pc=%08x\n", rv32->status, rv32->pc);
                fprintf(stderr, "instr = %08x\n", *(uint32_t *)&rv32->mem[rv32->pc]);
            }
        }
    } while(rv32->r[REG_A0] == 1);
    for(unsigned int i = 0; i < ctx.register_count; i++) {
        printf("%u:\t%u\n", i, ((uint32_t*)rv32->mem)[1 + i]);
    }
    free(memory);

    printf("OK\n");
    return 0;
}
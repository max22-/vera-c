CFLAGS = -std=c99 -Wall -pedantic -g
all: vera

vera: main.c vera.h
	$(CC) $(CFLAGS) $< -o $@

tests: tests.c vera.h
	$(CC) $(CFLAGS) $< -o $@

.PHONY: run clean test

run: vera
	./vera
	riscv64-unknown-elf-objdump -D -b binary -m riscv:rv32 --adjust-vma=0x80000000 out.bin

test: tests
	./tests

clean:
	rm -f vera tests
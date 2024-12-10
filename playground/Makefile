CFLAGS = -std=c99 -Wall -pedantic -g
all: vera

vera: main.c vera.h
	$(CC) $(CFLAGS) $< -o $@

tests: tests.c vera.h
	$(CC) $(CFLAGS) $< -o $@

.PHONY: run clean test

run: vera
	./vera

test: tests
	./tests

clean:
	rm -f vera tests
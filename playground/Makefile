CFLAGS = -std=c99 -Wall -pedantic -g
all: vera

vera: main.c vera.h
	$(CC) $(CFLAGS) $< -o $@

.PHONY: run clean

run: vera
	./vera

clean:
	rm -f vera
CC=clang
CFLAGS=-g -W -Wall -Wextra -shared -fPIC

malloc.so: malloc.c
	$(CC) $< -o $@ $(CFLAGS)

.PHONY: clean
clean:
	rm -f malloc.so

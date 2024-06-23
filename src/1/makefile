CC=gcc
CFLAGS=-Wall -Wextra -Wno-implicit-fallthrough -std=gnu17 -fPIC -O2
LDFLAGS=-shared -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc -Wl,--wrap=reallocarray -Wl,--wrap=free -Wl,--wrap=strdup -Wl,--wrap=strndup
 
SOURCES=nand.c memory_tests.c
OBJ=$(SOURCES:.c=.o)
HEADERS=nand.h memory_tests.h

LIB=libnand.so
 
.PHONY: all clean
 
all: $(LIB)
 
$(LIB): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^
 
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<
 
clean:
	rm -f $(OBJ) $(LIB)
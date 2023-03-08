
CC=gcc
LCC=gcc

CFLAGS=-g
LDFLAGS=-lcurl -lm

SRC=$(wildcard src/*.c) $(wildcard src/libs/*.c) $(wildcard src/build/*.c)
OBJECTS=$(patsubst %.c, debug/build/%.o, $(SRC))
TARGET=ki

ki: $(OBJECTS)
	$(LCC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJECTS): debug/build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean

clean:
	rm -f ki $(OBJECTS) core

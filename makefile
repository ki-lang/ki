
CC=gcc
CFLAGS = -g -Wall -O0 -std=gnu99 -fcommon
LDFLAGS += -lm -fstack-protector -lpthread -pthread

ifeq ($(OS),Windows_NT)
	CFLAGS += -I./misc/curl/include
    LDFLAGS += -L./misc/curl/ -lcurl
else
    LDFLAGS += -lcurl
endif

SRC=$(wildcard src/*.c) $(wildcard src/libs/*.c) $(wildcard src/helpers/*.c) $(wildcard src/build/*.c) $(wildcard src/cfg/*.c) $(wildcard src/pkg/*.c) $(wildcard src/cache/*.c)
OBJECTS=$(patsubst %.c, build/%.o, $(SRC))
TARGET=ki

$(TARGET) : $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJECTS): build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean

clean:
	rm -f $(TARGET) $(OBJECTS) core

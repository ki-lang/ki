
CC=gcc
LCC=gcc

CFLAGS=-g -pthread
LLVMCF=`llvm-config --cflags`
LDFLAGS=-lcurl -lm `llvm-config --ldflags --system-libs --libs all-targets analysis bitreader bitwriter core codegen executionengine instrumentation interpreter ipo irreader linker mc mcjit objcarcopts option profiledata scalaropts support target object transformutils debuginfodwarf` -lm -lz -lstdc++

SRC=$(wildcard src/*.c) $(wildcard src/libs/*.c) $(wildcard src/build/*.c) $(wildcard src/build/LLVM/*.c)
SRC := $(filter-out src/build/stage-8-link.c, $(SRC))
OBJECTS=$(patsubst %.c, debug/build/%.o, $(SRC))
TARGET=ki

os_linux=./lib/libs/linux-x64/libki_os.a
os_macos=./lib/libs/macos-x64/libki_os.a

ki: $(OBJECTS) $(os_linux) debug/build/link.o
	$(LCC) $(CFLAGS) -o $@ $(OBJECTS) debug/build/link.o $(LDFLAGS)

debug/build/link.o: src/build/stage-8-link.c
	$(CC) $(CFLAGS) $(LLVMCF) -o debug/build/link.o -c src/build/stage-8-link.c

$(OBJECTS): debug/build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ -c $<

$(os_linux): ./src/os/linux.c
	mkdir -p ./lib/libs/linux-x64/
	gcc -g -O3 -c "./src/os/linux.c" -o /tmp/libki_os.o
	ar rcs $(os_linux) /tmp/libki_os.o

$(os_macos): ./src/os/macos.c
	mkdir -p ./lib/libs/macos-x64/
	gcc -g -O3 -c "./src/os/macos.c" -o /tmp/libki_os.o
	ar rcs $(os_macos) /tmp/libki_os.o

.PHONY: clean linux macos

clean:
	rm -f ki $(OBJECTS)

linux: ki $(os_linux)
macos: ki $(os_macos)


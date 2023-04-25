
CC=gcc
LCC=gcc

CFLAGS=-g -pthread -O2
LLVMCF=`llvm-config --cflags`
LDFLAGS=`llvm-config --ldflags --system-libs --libs all-targets analysis bitreader bitwriter core codegen executionengine instrumentation interpreter ipo irreader linker mc mcjit objcarcopts option profiledata scalaropts support target object transformutils debuginfodwarf` -lcurl -lm -lz -lstdc++
LDFLAGS_DIST=`llvm-config --ldflags --system-libs --libs --link-static all-targets analysis bitreader bitwriter core codegen executionengine instrumentation interpreter ipo irreader linker mc mcjit objcarcopts option profiledata scalaropts support target object transformutils debuginfodwarf` `curl-config --static-libs` -lm -lz -lstdc++ -ldl -lpthread

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

clean:
	rm -f ki $(OBJECTS)

linux: ki $(os_linux)
macos: ki $(os_macos)

# STATIC DIST BULDS

dist_dir: mkdir -p dist

linux_dist: $(OBJECTS) $(os_linux) debug/build/link.o
	mkdir -p dist/linux-x64/lib
	rm -rf dist/linux-x64/lib
	cp -r lib dist/linux-x64/
	$(LCC) $(CFLAGS_DIST) -o dist/linux-x64/ki $(OBJECTS) debug/build/link.o -L/usr/lib/llvm-14/lib/ $(LDFLAGS_DIST)

#

.PHONY: clean linux macos linux_dist
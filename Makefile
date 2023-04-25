
CC=clang
LCC=clang

VERSION=dev
UNAME=$(shell uname)

CFLAGS=-g -O2 -std=gnu99 -pthread
LLVMCF=`llvm-config --cflags`

LDFLAGS=`llvm-config --ldflags --system-libs --libs all-targets analysis bitreader bitwriter core codegen executionengine instrumentation interpreter ipo irreader linker mc mcjit objcarcopts option profiledata scalaropts support target object transformutils debuginfodwarf` -lcurl -lm -lz -lstdc++

LDFLAGS_DIST=`llvm-config --ldflags --system-libs --libs --link-static all-targets analysis bitreader bitwriter core codegen executionengine instrumentation interpreter ipo irreader linker mc mcjit objcarcopts option profiledata scalaropts support target object transformutils debuginfodwarf` `curl-config --static-libs` -lm -lz -lstdc++ -ldl -lpthread

LDFLAGS_OSX_DIST=-lLLVMOption -lLLVMObjCARCOpts -lLLVMMCJIT -lLLVMInterpreter -lLLVMExecutionEngine -lLLVMRuntimeDyld -lLLVMXCoreDisassembler -lLLVMXCoreCodeGen -lLLVMXCoreDesc -lLLVMXCoreInfo -lLLVMX86Disassembler -lLLVMX86AsmParser -lLLVMX86CodeGen -lLLVMX86Desc -lLLVMX86Info -lLLVMWebAssemblyDisassembler -lLLVMWebAssemblyCodeGen -lLLVMWebAssemblyDesc -lLLVMWebAssemblyAsmParser -lLLVMWebAssemblyInfo -lLLVMWebAssemblyUtils -lLLVMSystemZDisassembler -lLLVMSystemZCodeGen -lLLVMSystemZAsmParser -lLLVMSystemZDesc -lLLVMSystemZInfo -lLLVMSparcDisassembler -lLLVMSparcCodeGen -lLLVMSparcAsmParser -lLLVMSparcDesc -lLLVMSparcInfo -lLLVMRISCVDisassembler -lLLVMRISCVCodeGen -lLLVMRISCVAsmParser -lLLVMRISCVDesc -lLLVMRISCVInfo -lLLVMPowerPCDisassembler -lLLVMPowerPCCodeGen -lLLVMPowerPCAsmParser -lLLVMPowerPCDesc -lLLVMPowerPCInfo -lLLVMNVPTXCodeGen -lLLVMNVPTXDesc -lLLVMNVPTXInfo -lLLVMMSP430Disassembler -lLLVMMSP430CodeGen -lLLVMMSP430AsmParser -lLLVMMSP430Desc -lLLVMMSP430Info -lLLVMMipsDisassembler -lLLVMMipsCodeGen -lLLVMMipsAsmParser -lLLVMMipsDesc -lLLVMMipsInfo -lLLVMLanaiDisassembler -lLLVMLanaiCodeGen -lLLVMLanaiAsmParser -lLLVMLanaiDesc -lLLVMLanaiInfo -lLLVMHexagonDisassembler -lLLVMHexagonCodeGen -lLLVMHexagonAsmParser -lLLVMHexagonDesc -lLLVMHexagonInfo -lLLVMBPFDisassembler -lLLVMBPFCodeGen -lLLVMBPFAsmParser -lLLVMBPFDesc -lLLVMBPFInfo -lLLVMAVRDisassembler -lLLVMAVRCodeGen -lLLVMAVRAsmParser -lLLVMAVRDesc -lLLVMAVRInfo -lLLVMARMDisassembler -lLLVMARMCodeGen -lLLVMARMAsmParser -lLLVMARMDesc -lLLVMARMUtils -lLLVMARMInfo -lLLVMAMDGPUDisassembler -lLLVMAMDGPUCodeGen -lLLVMMIRParser -lLLVMipo -lLLVMInstrumentation -lLLVMVectorize -lLLVMLinker -lLLVMIRReader -lLLVMAsmParser -lLLVMFrontendOpenMP -lLLVMAMDGPUAsmParser -lLLVMAMDGPUDesc -lLLVMAMDGPUUtils -lLLVMAMDGPUInfo -lLLVMAArch64Disassembler -lLLVMMCDisassembler -lLLVMAArch64CodeGen -lLLVMCFGuard -lLLVMGlobalISel -lLLVMSelectionDAG -lLLVMAsmPrinter -lLLVMDebugInfoDWARF -lLLVMCodeGen -lLLVMTarget -lLLVMScalarOpts -lLLVMInstCombine -lLLVMAggressiveInstCombine -lLLVMTransformUtils -lLLVMBitWriter -lLLVMAnalysis -lLLVMProfileData -lLLVMObject -lLLVMTextAPI -lLLVMBitReader -lLLVMCore -lLLVMRemarks -lLLVMBitstreamReader -lLLVMAArch64AsmParser -lLLVMMCParser -lLLVMAArch64Desc -lLLVMMC -lLLVMDebugInfoCodeView -lLLVMDebugInfoMSF -lLLVMBinaryFormat -lLLVMAArch64Utils -lLLVMAArch64Info -lLLVMSupport -lLLVMDemangle -lLLVMPasses -lLLVMCoroutines -lLLVMVECodeGen -lLLVMVEAsmParser -lLLVMVEDesc -lLLVMVEDisassembler -lLLVMVEInfo

CROSS_OSX_FROM_LINUX=--sysroot=$(CURDIR)/dist/macos-sdk-11-3 -fuse-ld=lld -I$(CURDIR)/dist/macos-llvm-14/include -L$(CURDIR)/dist/macos-llvm-14/lib

SRC=$(wildcard src/*.c) $(wildcard src/libs/*.c) $(wildcard src/build/*.c) $(wildcard src/build/LLVM/*.c)
SRC := $(filter-out src/build/stage-8-link.c, $(SRC))
OBJECTS=$(patsubst %.c, debug/build/%.o, $(SRC))
TARGET=ki

os_linux=./lib/libs/linux-x64/libki_os.a
os_macos=./lib/libs/macos-x64/libki_os.a

ki: $(OBJECTS) debug/build/link.o
	$(LCC) $(CFLAGS) -o $@ $(OBJECTS) debug/build/link.o $(LDFLAGS)

debug/build/link.o: src/build/stage-8-link.c
	$(CC) $(CFLAGS) $(LLVMCF) -o debug/build/link.o -c src/build/stage-8-link.c

$(OBJECTS): debug/build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ -c $<

$(os_linux): ./src/os/linux.c
	mkdir -p ./lib/libs/linux-x64/
	gcc -g -O2 -c "./src/os/linux.c" -o /tmp/libki_os.o
	ar rcs $(os_linux) /tmp/libki_os.o

$(os_macos): ./src/os/macos.c
ifeq ($(UNAME), Linux)
	mkdir -p ./lib/libs/macos-x64/
	clang -g -O2 -c "./src/os/macos.c" -o /tmp/libki_os.o $(CROSS_OSX_FROM_LINUX)
	ar rcs $(os_macos) /tmp/libki_os.o
else
	mkdir -p ./lib/libs/macos-x64/
	gcc -g -O2 -c "./src/os/macos.c" -o /tmp/libki_os.o
	ar rcs $(os_macos) /tmp/libki_os.o
endif

clean:
	rm -f ki $(OBJECTS)

linux: ki $(os_linux)
macos: ki $(os_macos)

# STATIC DIST BULDS

linux_dist_from_linux: $(OBJECTS) $(os_linux) debug/build/link.o
	mkdir -p dist/linux-x64/lib
	rm -rf dist/linux-x64/lib
	cp -r lib dist/linux-x64/
	cp -r src/scripts/install.sh dist/linux-x64/
	sed -i 's/__VERSION__/$(VERSION)/' dist/linux-x64/install.sh
	$(LCC) -o dist/linux-x64/ki $(OBJECTS) debug/build/link.o $(LDFLAGS_DIST)
	tar -czf dist/ki-linux-$(VERSION)-x64.tar.gz dist/linux-x64/*

macos_dist_from_linux: $(os_macos) debug/build/link.o
	mkdir -p dist/macos-x64/lib
	rm -rf dist/macos-x64/lib
	cp -r lib dist/macos-x64/
	cp -r src/scripts/install.sh dist/macos-x64/
	sed -i 's/__VERSION__/$(VERSION)/' dist/macos-x64/install.sh
	export SDKROOT=$(CURDIR)/dist/macos-sdk-11-3 && \
	export MACOSX_DEPLOYMENT_TARGET=11.6 && \
	$(LCC) $(CFLAGS) --target=x86_64-apple-darwin-macho $(CROSS_OSX_FROM_LINUX) -o dist/macos-x64/ki $(SRC) src/build/stage-8-link.c -lm -lz -lcurses -lcurl -lpthread -pthread -lc++ -Wl,-platform_version,macos,11.6.0,11.3 $(LDFLAGS_OSX_DIST)
	tar -czf dist/ki-macos-$(VERSION)-x64.tar.gz dist/macos-x64/*

#

.PHONY: clean linux macos linux_dist


CC=clang-15
LCC=clang-15

VERSION=dev
UNAME=$(shell uname)

CFLAGS=-g -O2 -pthread 

LDFLAGS=

ifeq ($(UNAME), Darwin)
# From macos
CC=clang
LCC=clang
CFLAGS:=$(CFLAGS) `llvm-config --cflags`
LDFLAGS:=$(LDFLAGS) `llvm-config --ldflags --system-libs --libs all-targets analysis bitreader bitwriter core codegen executionengine instrumentation interpreter ipo irreader linker mc mcjit objcarcopts option profiledata scalaropts support target object transformutils debuginfodwarf`
else
# From linux
CFLAGS:=$(CFLAGS) `llvm-config-15 --cflags`
LDFLAGS:=$(LDFLAGS) `llvm-config-15 --ldflags --system-libs --libs all-targets analysis bitreader bitwriter core codegen executionengine instrumentation interpreter ipo irreader linker mc mcjit objcarcopts option profiledata scalaropts support target object transformutils debuginfodwarf`
endif

LDFLAGS:=$(LDFLAGS) -lcurl -lm -lz -lstdc++

LDFLAGS_DIST=`llvm-config --ldflags --system-libs --libs --link-static all-targets analysis bitreader bitwriter core codegen executionengine instrumentation interpreter ipo irreader linker mc mcjit objcarcopts option profiledata scalaropts support target object transformutils debuginfodwarf` `curl-config --static-libs` -lm -lz -lstdc++ -ldl -lpthread

LDFLAGS_OSX_DIST=-lLLVMOption -lLLVMObjCARCOpts -lLLVMMCJIT -lLLVMInterpreter -lLLVMExecutionEngine -lLLVMRuntimeDyld -lLLVMXCoreDisassembler -lLLVMXCoreCodeGen -lLLVMXCoreDesc -lLLVMXCoreInfo -lLLVMX86Disassembler -lLLVMX86AsmParser -lLLVMX86CodeGen -lLLVMX86Desc -lLLVMX86Info -lLLVMWebAssemblyDisassembler -lLLVMWebAssemblyCodeGen -lLLVMWebAssemblyDesc -lLLVMWebAssemblyAsmParser -lLLVMWebAssemblyInfo -lLLVMWebAssemblyUtils -lLLVMSystemZDisassembler -lLLVMSystemZCodeGen -lLLVMSystemZAsmParser -lLLVMSystemZDesc -lLLVMSystemZInfo -lLLVMSparcDisassembler -lLLVMSparcCodeGen -lLLVMSparcAsmParser -lLLVMSparcDesc -lLLVMSparcInfo -lLLVMRISCVDisassembler -lLLVMRISCVCodeGen -lLLVMRISCVAsmParser -lLLVMRISCVDesc -lLLVMRISCVInfo -lLLVMPowerPCDisassembler -lLLVMPowerPCCodeGen -lLLVMPowerPCAsmParser -lLLVMPowerPCDesc -lLLVMPowerPCInfo -lLLVMNVPTXCodeGen -lLLVMNVPTXDesc -lLLVMNVPTXInfo -lLLVMMSP430Disassembler -lLLVMMSP430CodeGen -lLLVMMSP430AsmParser -lLLVMMSP430Desc -lLLVMMSP430Info -lLLVMMipsDisassembler -lLLVMMipsCodeGen -lLLVMMipsAsmParser -lLLVMMipsDesc -lLLVMMipsInfo -lLLVMLanaiDisassembler -lLLVMLanaiCodeGen -lLLVMLanaiAsmParser -lLLVMLanaiDesc -lLLVMLanaiInfo -lLLVMHexagonDisassembler -lLLVMHexagonCodeGen -lLLVMHexagonAsmParser -lLLVMHexagonDesc -lLLVMHexagonInfo -lLLVMBPFDisassembler -lLLVMBPFCodeGen -lLLVMBPFAsmParser -lLLVMBPFDesc -lLLVMBPFInfo -lLLVMAVRDisassembler -lLLVMAVRCodeGen -lLLVMAVRAsmParser -lLLVMAVRDesc -lLLVMAVRInfo -lLLVMARMDisassembler -lLLVMARMCodeGen -lLLVMARMAsmParser -lLLVMARMDesc -lLLVMARMUtils -lLLVMARMInfo -lLLVMAMDGPUDisassembler -lLLVMAMDGPUCodeGen -lLLVMMIRParser -lLLVMipo -lLLVMInstrumentation -lLLVMVectorize -lLLVMLinker -lLLVMIRReader -lLLVMAsmParser -lLLVMFrontendOpenMP -lLLVMAMDGPUAsmParser -lLLVMAMDGPUDesc -lLLVMAMDGPUUtils -lLLVMAMDGPUInfo -lLLVMAArch64Disassembler -lLLVMMCDisassembler -lLLVMAArch64CodeGen -lLLVMCFGuard -lLLVMGlobalISel -lLLVMSelectionDAG -lLLVMAsmPrinter -lLLVMDebugInfoDWARF -lLLVMCodeGen -lLLVMTarget -lLLVMScalarOpts -lLLVMInstCombine -lLLVMAggressiveInstCombine -lLLVMTransformUtils -lLLVMBitWriter -lLLVMAnalysis -lLLVMProfileData -lLLVMObject -lLLVMTextAPI -lLLVMBitReader -lLLVMCore -lLLVMRemarks -lLLVMBitstreamReader -lLLVMAArch64AsmParser -lLLVMMCParser -lLLVMAArch64Desc -lLLVMMC -lLLVMDebugInfoCodeView -lLLVMDebugInfoMSF -lLLVMBinaryFormat -lLLVMAArch64Utils -lLLVMAArch64Info -lLLVMSupport -lLLVMDemangle -lLLVMPasses -lLLVMCoroutines -lLLVMVECodeGen -lLLVMVEAsmParser -lLLVMVEDesc -lLLVMVEDisassembler -lLLVMVEInfo

SRC=$(wildcard src/*.c) $(wildcard src/libs/*.c) $(wildcard src/build/*.c) $(wildcard src/build/LLVM/*.c)
OBJECTS=$(patsubst %.c, debug/build/%.o, $(SRC))
TARGET=ki

ki: $(OBJECTS)
	$(LCC) $(CFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

$(OBJECTS): debug/build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f ki $(OBJECTS)

# STATIC DIST BULDS

linux_dist_from_linux: $(OBJECTS)
	mkdir -p dist/linux-x64/lib
	rm -rf dist/linux-x64/lib
	cp -r lib dist/linux-x64/
	cp -r src/scripts/install.sh dist/linux-x64/
	sed -i 's/__VERSION__/$(VERSION)/' dist/linux-x64/install.sh
	$(LCC) -o dist/linux-x64/ki $(OBJECTS) $(LDFLAGS_DIST)
	cd dist/linux-x64 && tar -czf ../ki-$(VERSION)-linux-x64.tar.gz ki lib install.sh

macos_dist_from_linux:
	mkdir -p dist/macos-x64/lib
	rm -rf dist/macos-x64/lib
	cp -r lib dist/macos-x64/
	cp -r src/scripts/install.sh dist/macos-x64/
	sed -i 's/__VERSION__/$(VERSION)/' dist/macos-x64/install.sh
	export SDKROOT=$(CURDIR)/dist/macos-sdk-11-3 && \
	export MACOSX_DEPLOYMENT_TARGET=11.6 && \
	$(LCC) -g -O2 -pthread -arch=x86_64 --target=x86_64-apple-darwin-macho \
	--sysroot=$(CURDIR)/dist/macos-sdk-11-3 -fuse-ld=lld -I$(CURDIR)/dist/macos-llvm-15-x64/include -L$(CURDIR)/dist/macos-llvm-15-x64/lib \
	-o dist/macos-x64/ki $(SRC) -lm -lz -lcurses -lcurl -lpthread -pthread -lc++ -Wl,-platform_version,macos,11.6.0,11.3 $(LDFLAGS_OSX_DIST)
	cd dist/macos-x64 && tar -czf ../ki-$(VERSION)-macos-x64.tar.gz ki lib install.sh

macos_arm_dist_from_linux: 
	mkdir -p dist/macos-arm64/lib
	rm -rf dist/macos-arm64/lib
	cp -r lib dist/macos-arm64/
	cp -r src/scripts/install.sh dist/macos-arm64/
	sed -i 's/__VERSION__/$(VERSION)/' dist/macos-arm64/install.sh
	export SDKROOT=$(CURDIR)/dist/macos-sdk-11-3 && \
	export MACOSX_DEPLOYMENT_TARGET=11.6 && \
	$(LCC) -g -O2 -pthread -arch arm64 --target=arm64-apple-darwin-macho \
	--sysroot=$(CURDIR)/dist/macos-sdk-11-3 -fuse-ld=lld -I$(CURDIR)/dist/macos-llvm-15-arm64/include -L$(CURDIR)/dist/macos-llvm-15-arm64/lib \
	-o dist/macos-arm64/ki $(SRC) -lm -lz -lcurses -lcurl -lpthread -pthread -lc++ -Wl,-platform_version,macos,11.6.0,11.3 $(LDFLAGS_OSX_DIST)
	cd dist/macos-arm64 && tar -czf ../ki-$(VERSION)-macos-arm64.tar.gz ki lib install.sh

dists_from_linux: linux_dist_from_linux macos_dist_from_linux macos_arm_dist_from_linux
#

.PHONY: clean linux macos linux_dist
